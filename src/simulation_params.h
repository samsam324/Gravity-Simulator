#pragma once

struct SimulationParams {
    float gravitationalConstant = 220.0f;
    float softeningLength = 8.0f;
    float fixedTimeStep = 1.0f / 60.0f;
    float barnesHutTheta = 2.00f;
    float velocityClamp = 2600.0f;
};
