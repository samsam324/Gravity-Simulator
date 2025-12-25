#pragma once
#include <SFML/Graphics.hpp>
#include "particles.h"

struct VisualQuality {
    float trailFadeAlpha = 18.0f;
    float glowIntensity = 1.15f;
    float bloomRadius = 8.0f;
    float bloomSigma = 5.0f;
    float bloomDownscale = 0.5f;
};

class Renderer {
public:
    Renderer(int windowWidth, int windowHeight);

    void resize(int windowWidth, int windowHeight);
    void setQualityPreset(int presetIndex);

    void render(sf::RenderWindow& window, const sf::View& worldView, const Particles& particles);

private:
    void ensureTargets(int width, int height);
    void updateParticleQuads(const Particles& particles, float worldUnitsPerPixel);

    sf::RenderTexture trailTarget;
    sf::RenderTexture bloomTargetA;
    sf::RenderTexture bloomTargetB;

    sf::Shader glowShader;
    sf::Shader blurShader;

    sf::VertexArray particleQuads;
    sf::RectangleShape fadeRectangle;

    VisualQuality visualQuality;
    int currentWidth = 0;
    int currentHeight = 0;
};
