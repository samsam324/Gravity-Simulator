#include "barnes_hut.h"
#include <algorithm>
#include <cmath>

bool BarnesHutNode::isLeaf() const {
    return childIndex0 < 0 && childIndex1 < 0 && childIndex2 < 0 && childIndex3 < 0;
}

const std::vector<BarnesHutNode>& BarnesHutTree::nodes() const {
    return treeNodes;
}

int BarnesHutTree::createNode(float cx, float cy, float halfSize) {
    BarnesHutNode node;
    node.centerX = cx;
    node.centerY = cy;
    node.halfSize = halfSize;
    treeNodes.push_back(node);
    return (int)treeNodes.size() - 1;
}

int BarnesHutTree::selectQuadrant(const BarnesHutNode& node, float x, float y) const {
    bool east = x >= node.centerX;
    bool south = y >= node.centerY;
    if (!east && !south) return 0;
    if ( east && !south) return 1;
    if (!east &&  south) return 2;
    return 3;
}

void BarnesHutTree::childBounds(const BarnesHutNode& node, int quadrant, float& outCenterX, float& outCenterY, float& outHalfSize) const {
    outHalfSize = node.halfSize * 0.5f;
    float dx = (quadrant == 1 || quadrant == 3) ? outHalfSize : -outHalfSize;
    float dy = (quadrant == 2 || quadrant == 3) ? outHalfSize : -outHalfSize;
    outCenterX = node.centerX + dx;
    outCenterY = node.centerY + dy;
}

void BarnesHutTree::accumulateIntoLeaf(int nodeIndex, const Particles& particles, int particleIndex) {
    BarnesHutNode& node = treeNodes[nodeIndex];

    float px = particles.positionX[particleIndex];
    float py = particles.positionY[particleIndex];
    float pm = particles.mass[particleIndex];

    if (node.particleIndex == -1) {
        node.particleIndex = particleIndex;
        return;
    }

    if (node.particleIndex >= 0) {
        int existing = node.particleIndex;
        float ex = particles.positionX[existing];
        float ey = particles.positionY[existing];
        float em = particles.mass[existing];

        node.totalMass = em;
        node.centerOfMassX = ex;
        node.centerOfMassY = ey;
        node.particleIndex = -2;
    }

    float oldMass = node.totalMass;
    float newMass = oldMass + pm;

    node.centerOfMassX = (node.centerOfMassX * oldMass + px * pm) / newMass;
    node.centerOfMassY = (node.centerOfMassY * oldMass + py * pm) / newMass;
    node.totalMass = newMass;
}

void BarnesHutTree::build(const Particles& particles) {
    treeNodes.clear();
    treeNodes.reserve(particles.count() * 3 + 64);
    if (particles.count() == 0) return;

    float minX = particles.positionX[0];
    float maxX = particles.positionX[0];
    float minY = particles.positionY[0];
    float maxY = particles.positionY[0];

    for (std::size_t i = 1; i < particles.count(); i++) {
        minX = std::min(minX, particles.positionX[i]);
        maxX = std::max(maxX, particles.positionX[i]);
        minY = std::min(minY, particles.positionY[i]);
        maxY = std::max(maxY, particles.positionY[i]);
    }

    float centerX = 0.5f * (minX + maxX);
    float centerY = 0.5f * (minY + maxY);
    float span = std::max(maxX - minX, maxY - minY);
    float halfSize = std::max(512.0f, 0.75f * span + 128.0f);

    int rootIndex = createNode(centerX, centerY, halfSize);

    for (int i = 0; i < (int)particles.count(); i++) {
        insertParticle(rootIndex, particles, i, 0);
    }

    computeMassProperties(rootIndex, particles);
}

void BarnesHutTree::insertParticle(int nodeIndex, const Particles& particles, int particleIndex, int depth) {
    while (true) {
        BarnesHutNode& node = treeNodes[nodeIndex];

        if (depth >= kMaxDepth || node.halfSize <= kMinHalfSize) {
            accumulateIntoLeaf(nodeIndex, particles, particleIndex);
            return;
        }

        if (node.isLeaf() && node.particleIndex < 0) {
            node.particleIndex = particleIndex;
            return;
        }

        if (node.isLeaf() && node.particleIndex == -2) {
            accumulateIntoLeaf(nodeIndex, particles, particleIndex);
            return;
        }

        if (node.isLeaf() && node.particleIndex >= 0) {
            int existingParticle = node.particleIndex;
            node.particleIndex = -1;

            BarnesHutNode nodeCopy = node;

            float c0x, c0y, c0h;
            float c1x, c1y, c1h;
            float c2x, c2y, c2h;
            float c3x, c3y, c3h;

            childBounds(nodeCopy, 0, c0x, c0y, c0h);
            childBounds(nodeCopy, 1, c1x, c1y, c1h);
            childBounds(nodeCopy, 2, c2x, c2y, c2h);
            childBounds(nodeCopy, 3, c3x, c3y, c3h);

            int child0 = createNode(c0x, c0y, c0h);
            int child1 = createNode(c1x, c1y, c1h);
            int child2 = createNode(c2x, c2y, c2h);
            int child3 = createNode(c3x, c3y, c3h);

            treeNodes[nodeIndex].childIndex0 = child0;
            treeNodes[nodeIndex].childIndex1 = child1;
            treeNodes[nodeIndex].childIndex2 = child2;
            treeNodes[nodeIndex].childIndex3 = child3;

            int qExisting = selectQuadrant(treeNodes[nodeIndex], particles.positionX[existingParticle], particles.positionY[existingParticle]);
            int qNew = selectQuadrant(treeNodes[nodeIndex], particles.positionX[particleIndex], particles.positionY[particleIndex]);

            int existingChild =
                (qExisting == 0) ? treeNodes[nodeIndex].childIndex0 :
                (qExisting == 1) ? treeNodes[nodeIndex].childIndex1 :
                (qExisting == 2) ? treeNodes[nodeIndex].childIndex2 :
                                   treeNodes[nodeIndex].childIndex3;

            int newChild =
                (qNew == 0) ? treeNodes[nodeIndex].childIndex0 :
                (qNew == 1) ? treeNodes[nodeIndex].childIndex1 :
                (qNew == 2) ? treeNodes[nodeIndex].childIndex2 :
                              treeNodes[nodeIndex].childIndex3;

            insertParticle(existingChild, particles, existingParticle, depth + 1);
            nodeIndex = newChild;
            depth = depth + 1;
            continue;
        }

        int q = selectQuadrant(node, particles.positionX[particleIndex], particles.positionY[particleIndex]);
        int nextChild =
            (q == 0) ? node.childIndex0 :
            (q == 1) ? node.childIndex1 :
            (q == 2) ? node.childIndex2 :
                       node.childIndex3;

        nodeIndex = nextChild;
        depth = depth + 1;
    }
}

void BarnesHutTree::computeMassProperties(int nodeIndex, const Particles& particles) {
    BarnesHutNode& node = treeNodes[nodeIndex];

    if (node.isLeaf()) {
        if (node.particleIndex == -2) {
            return;
        }

        if (node.particleIndex >= 0) {
            int i = node.particleIndex;
            node.totalMass = particles.mass[i];
            node.centerOfMassX = particles.positionX[i];
            node.centerOfMassY = particles.positionY[i];
        } else {
            node.totalMass = 0.0f;
            node.centerOfMassX = node.centerX;
            node.centerOfMassY = node.centerY;
        }
        return;
    }

    float massSum = 0.0f;
    float weightedX = 0.0f;
    float weightedY = 0.0f;

    int children[4] = { node.childIndex0, node.childIndex1, node.childIndex2, node.childIndex3 };
    for (int c : children) {
        if (c < 0) continue;
        computeMassProperties(c, particles);
        float m = treeNodes[c].totalMass;
        massSum += m;
        weightedX += m * treeNodes[c].centerOfMassX;
        weightedY += m * treeNodes[c].centerOfMassY;
    }

    node.totalMass = massSum;
    if (massSum > 0.0f) {
        node.centerOfMassX = weightedX / massSum;
        node.centerOfMassY = weightedY / massSum;
    } else {
        node.centerOfMassX = node.centerX;
        node.centerOfMassY = node.centerY;
    }
}
