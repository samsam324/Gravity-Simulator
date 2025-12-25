#include "gravity_simulation.h"
#include "deterministic_rng.h"
#include <cmath>
#include <algorithm>
#include <vector>

static inline void addGravity(float& ax, float& ay,
                              float px, float py,
                              float sx, float sy, float smass,
                              float gravitationalConstant, float softeningSquared) {
    float dx = sx - px;
    float dy = sy - py;
    float r2 = dx * dx + dy * dy + softeningSquared;
    float invR = 1.0f / std::sqrt(r2);
    float invR3 = invR * invR * invR;
    float scale = gravitationalConstant * smass * invR3;
    ax += dx * scale;
    ay += dy * scale;
}

GravitySimulation::GravitySimulation(unsigned int workerThreads, int particleCount, uint32_t seed)
    : workers(std::max(1u, workerThreads)),
      configuredParticleCount(std::max(1, particleCount)),
      configuredSeed(seed),
      pool(workers) {
    reset();
}

void GravitySimulation::reset() {
    initializeParticles();
}

const Particles& GravitySimulation::particles() const { return particleData; }
const SimulationParams& GravitySimulation::params() const { return simulationParams; }
SimulationParams& GravitySimulation::params() { return simulationParams; }

void GravitySimulation::initializeParticles() {
    particleData.clear();
    particleData.reserve((std::size_t)configuredParticleCount + 1);

    DeterministicRng rng(configuredSeed);

    float galaxyCenterX = 0.0f;
    float galaxyCenterY = 0.0f;

    for (int i = 0; i < configuredParticleCount; i++) {
        float radius = std::sqrt(rng.nextFloat01()) * 560.0f;
        float angle = rng.range(0.0f, 6.2831853f);

        float px = galaxyCenterX + radius * std::cos(angle);
        float py = galaxyCenterY + radius * std::sin(angle);

        float baseSpeed = std::sqrt(std::max(25.0f, radius)) * 5.0f;
        float tangentX = -std::sin(angle);
        float tangentY =  std::cos(angle);

        float speedJitter = rng.range(0.86f, 1.14f);

        float vx = tangentX * baseSpeed * speedJitter;
        float vy = tangentY * baseSpeed * speedJitter;

        float m = rng.range(0.65f, 1.55f);
        if ((rng.nextU32() & 2047u) == 0u) m *= 70.0f;

        particleData.add(px, py, vx, vy, m);
    }

    particleData.add(galaxyCenterX, galaxyCenterY, 0.0f, 0.0f, 24000.0f);

    accelerationX.resize(particleData.count());
    accelerationY.resize(particleData.count());
}

void GravitySimulation::stepFixed(double fixedDeltaSeconds) {
    float dt = (float)fixedDeltaSeconds;
    computeAccelerationsBarnesHut();
    integrateSymplecticEuler(dt);
}

void GravitySimulation::computeAccelerationsBarnesHut() {
    quadtree.build(particleData);

    std::fill(accelerationX.begin(), accelerationX.end(), 0.0f);
    std::fill(accelerationY.begin(), accelerationY.end(), 0.0f);

    const auto& nodes = quadtree.nodes();
    if (nodes.empty()) return;

    const float softeningSquared = simulationParams.softeningLength * simulationParams.softeningLength;
    const float gravitationalConstant = simulationParams.gravitationalConstant;
    const float theta = simulationParams.barnesHutTheta;
    const float thetaSquared = theta * theta;

    auto computeRange = [&](std::size_t begin, std::size_t end) {
        std::vector<int> traversalStack;
        traversalStack.reserve(4096);

        for (std::size_t i = begin; i < end; i++) {
            float ax = 0.0f;
            float ay = 0.0f;

            const float px = particleData.positionX[i];
            const float py = particleData.positionY[i];

            traversalStack.clear();
            traversalStack.push_back(0);

            while (!traversalStack.empty()) {
                int nodeIndex = traversalStack.back();
                traversalStack.pop_back();

                const BarnesHutNode& node = nodes[(std::size_t)nodeIndex];
                if (node.totalMass <= 0.0f) continue;

                if (node.isLeaf()) {
                    if (node.particleIndex >= 0 && node.particleIndex != (int)i) {
                        addGravity(ax, ay, px, py,
                                   node.centerOfMassX, node.centerOfMassY, node.totalMass,
                                   gravitationalConstant, softeningSquared);
                    }
                    continue;
                }

                const float dx = node.centerOfMassX - px;
                const float dy = node.centerOfMassY - py;
                const float d2 = dx * dx + dy * dy + softeningSquared;

                const float s = node.halfSize * 2.0f;

                // (s / d) < theta  <=>  s*s < theta^2 * d^2   (avoid sqrt)
                if ((s * s) < (thetaSquared * d2)) {
                    addGravity(ax, ay, px, py,
                               node.centerOfMassX, node.centerOfMassY, node.totalMass,
                               gravitationalConstant, softeningSquared);
                } else {
                    int c0 = node.childIndex0;
                    int c1 = node.childIndex1;
                    int c2 = node.childIndex2;
                    int c3 = node.childIndex3;

                    if (c3 >= 0) traversalStack.push_back(c3);
                    if (c2 >= 0) traversalStack.push_back(c2);
                    if (c1 >= 0) traversalStack.push_back(c1);
                    if (c0 >= 0) traversalStack.push_back(c0);
                }
            }

            accelerationX[i] = ax;
            accelerationY[i] = ay;
        }
    };

    pool.parallelFor(0, particleData.count(), 16384, computeRange);
}

void GravitySimulation::integrateSymplecticEuler(float dtSeconds) {
    float velocityClampSquared = simulationParams.velocityClamp * simulationParams.velocityClamp;

    for (std::size_t i = 0; i < particleData.count(); i++) {
        particleData.velocityX[i] += accelerationX[i] * dtSeconds;
        particleData.velocityY[i] += accelerationY[i] * dtSeconds;

        float v2 = particleData.velocityX[i] * particleData.velocityX[i] + particleData.velocityY[i] * particleData.velocityY[i];
        if (v2 > velocityClampSquared) {
            float scale = simulationParams.velocityClamp / std::sqrt(v2);
            particleData.velocityX[i] *= scale;
            particleData.velocityY[i] *= scale;
        }

        particleData.positionX[i] += particleData.velocityX[i] * dtSeconds;
        particleData.positionY[i] += particleData.velocityY[i] * dtSeconds;
    }
}
