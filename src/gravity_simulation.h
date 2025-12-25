#pragma once
#include <vector>
#include <cstdint>
#include "particles.h"
#include "barnes_hut.h"
#include "thread_pool.h"
#include "simulation_params.h"

class GravitySimulation {
public:
    GravitySimulation(unsigned int workerThreads, int particleCount, uint32_t seed);

    void reset();
    void stepFixed(double fixedDeltaSeconds);

    const Particles& particles() const;
    const SimulationParams& params() const;
    SimulationParams& params();

private:
    void initializeParticles();
    void computeAccelerationsBarnesHut();
    void integrateSymplecticEuler(float dtSeconds);

    unsigned int workers;
    int configuredParticleCount;
    uint32_t configuredSeed;

    ThreadPool pool;
    SimulationParams simulationParams;

    Particles particleData;
    BarnesHutTree quadtree;

    std::vector<float> accelerationX;
    std::vector<float> accelerationY;
};
