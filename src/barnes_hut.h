#pragma once
#include <vector>
#include "particles.h"

struct BarnesHutNode {
    float centerX = 0.0f;
    float centerY = 0.0f;
    float halfSize = 0.0f;

    float totalMass = 0.0f;
    float centerOfMassX = 0.0f;
    float centerOfMassY = 0.0f;

    int childIndex0 = -1;
    int childIndex1 = -1;
    int childIndex2 = -1;
    int childIndex3 = -1;

    int particleIndex = -1;

    bool isLeaf() const;
};

class BarnesHutTree {
public:
    void build(const Particles& particles);
    const std::vector<BarnesHutNode>& nodes() const;

private:
    static constexpr int kMaxDepth = 20;
    static constexpr float kMinHalfSize = 2.0f;

    std::vector<BarnesHutNode> treeNodes;

    int createNode(float cx, float cy, float halfSize);
    void insertParticle(int nodeIndex, const Particles& particles, int particleIndex, int depth);
    void computeMassProperties(int nodeIndex, const Particles& particles);

    int selectQuadrant(const BarnesHutNode& node, float x, float y) const;
    void childBounds(const BarnesHutNode& node, int quadrant, float& outCenterX, float& outCenterY, float& outHalfSize) const;

    void accumulateIntoLeaf(int nodeIndex, const Particles& particles, int particleIndex);
};
