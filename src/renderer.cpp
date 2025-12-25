#include "renderer.h"
#include "shaders.h"
#include <cmath>
#include <algorithm>

static sf::Color speedToColor(float speed) {
    float t = std::clamp(speed / 1200.0f, 0.0f, 1.0f);

    auto lerp = [](float a, float b, float u) { return a + (b - a) * u; };

    sf::Color c1(0, 255, 255);
    sf::Color c2(255, 0, 255);
    sf::Color c3(255, 255, 255);

    sf::Color out;
    if (t < 0.7f) {
        float u = t / 0.7f;
        out.r = (sf::Uint8)lerp((float)c1.r, (float)c2.r, u);
        out.g = (sf::Uint8)lerp((float)c1.g, (float)c2.g, u);
        out.b = (sf::Uint8)lerp((float)c1.b, (float)c2.b, u);
    } else {
        float u = (t - 0.7f) / 0.3f;
        out.r = (sf::Uint8)lerp((float)c2.r, (float)c3.r, u);
        out.g = (sf::Uint8)lerp((float)c2.g, (float)c3.g, u);
        out.b = (sf::Uint8)lerp((float)c2.b, (float)c3.b, u);
    }
    out.a = 255;
    return out;
}

Renderer::Renderer(int windowWidth, int windowHeight)
    : particleQuads(sf::Quads) {
    glowShader.loadFromMemory(kGlowFragmentShader, sf::Shader::Fragment);
    blurShader.loadFromMemory(kBlurFragmentShader, sf::Shader::Fragment);
    setQualityPreset(2);
    ensureTargets(windowWidth, windowHeight);
}

void Renderer::resize(int windowWidth, int windowHeight) {
    ensureTargets(windowWidth, windowHeight);
}

void Renderer::setQualityPreset(int presetIndex) {
    if (presetIndex <= 1) {
        visualQuality.trailFadeAlpha = 30.0f;
        visualQuality.glowIntensity = 0.95f;
        visualQuality.bloomRadius = 6.0f;
        visualQuality.bloomSigma = 4.0f;
        visualQuality.bloomDownscale = 0.5f;
    } else if (presetIndex == 2) {
        visualQuality.trailFadeAlpha = 18.0f;
        visualQuality.glowIntensity = 1.15f;
        visualQuality.bloomRadius = 8.0f;
        visualQuality.bloomSigma = 5.0f;
        visualQuality.bloomDownscale = 0.5f;
    } else {
        visualQuality.trailFadeAlpha = 12.0f;
        visualQuality.glowIntensity = 1.35f;
        visualQuality.bloomRadius = 10.0f;
        visualQuality.bloomSigma = 6.0f;
        visualQuality.bloomDownscale = 0.4f;
    }

    fadeRectangle.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)std::clamp(visualQuality.trailFadeAlpha, 1.0f, 255.0f)));
}

void Renderer::ensureTargets(int width, int height) {
    if (width == currentWidth && height == currentHeight) return;

    currentWidth = width;
    currentHeight = height;

    trailTarget.create((unsigned)width, (unsigned)height);
    trailTarget.clear(sf::Color::Black);
    trailTarget.display();

    fadeRectangle.setSize(sf::Vector2f((float)width, (float)height));
    fadeRectangle.setPosition(0.0f, 0.0f);

    int bloomWidth = std::max(64, (int)std::round(width * visualQuality.bloomDownscale));
    int bloomHeight = std::max(64, (int)std::round(height * visualQuality.bloomDownscale));

    bloomTargetA.create((unsigned)bloomWidth, (unsigned)bloomHeight);
    bloomTargetB.create((unsigned)bloomWidth, (unsigned)bloomHeight);
    bloomTargetA.clear(sf::Color::Black);
    bloomTargetB.clear(sf::Color::Black);
    bloomTargetA.display();
    bloomTargetB.display();
}

void Renderer::updateParticleQuads(const Particles& particles, float worldUnitsPerPixel) {
    std::size_t n = particles.count();
    particleQuads.resize(n * 4);

    float baseSize = std::clamp(1.4f * worldUnitsPerPixel, 0.9f, 6.0f);

    for (std::size_t i = 0; i < n; i++) {
        float px = particles.positionX[i];
        float py = particles.positionY[i];
        float vx = particles.velocityX[i];
        float vy = particles.velocityY[i];

        float speed = std::sqrt(vx * vx + vy * vy);
        sf::Color color = speedToColor(speed);

        float size = (i == n - 1) ? (baseSize * 9.0f) : baseSize;
        if (i == n - 1) color = sf::Color(255, 255, 255);

        sf::Vertex* v = &particleQuads[i * 4];

        v[0].position = { px - size, py - size }; v[0].texCoords = { 0.f, 0.f }; v[0].color = color;
        v[1].position = { px + size, py - size }; v[1].texCoords = { 1.f, 0.f }; v[1].color = color;
        v[2].position = { px + size, py + size }; v[2].texCoords = { 1.f, 1.f }; v[2].color = color;
        v[3].position = { px - size, py + size }; v[3].texCoords = { 0.f, 1.f }; v[3].color = color;
    }
}

void Renderer::render(sf::RenderWindow& window, const sf::View& worldView, const Particles& particles) {
    ensureTargets((int)window.getSize().x, (int)window.getSize().y);

    float worldUnitsPerPixel = worldView.getSize().x / (float)window.getSize().x;
    updateParticleQuads(particles, worldUnitsPerPixel);

    glowShader.setUniform("uIntensity", visualQuality.glowIntensity);

    trailTarget.setView(trailTarget.getDefaultView());
    trailTarget.draw(fadeRectangle, sf::BlendAlpha);

    trailTarget.setView(worldView);

    sf::RenderStates glowStates;
    glowStates.shader = &glowShader;
    glowStates.blendMode = sf::BlendAdd;

    trailTarget.draw(particleQuads, glowStates);
    trailTarget.display();

    int bloomWidth = (int)bloomTargetA.getSize().x;
    int bloomHeight = (int)bloomTargetA.getSize().y;

    sf::Sprite downsample(trailTarget.getTexture());
    downsample.setScale((float)bloomWidth / (float)trailTarget.getSize().x,
                        (float)bloomHeight / (float)trailTarget.getSize().y);

    bloomTargetA.setView(bloomTargetA.getDefaultView());
    bloomTargetA.clear(sf::Color::Black);
    bloomTargetA.draw(downsample, sf::BlendAdd);
    bloomTargetA.display();

    blurShader.setUniform("uTexture", sf::Shader::CurrentTexture);
    blurShader.setUniform("uRadius", visualQuality.bloomRadius);
    blurShader.setUniform("uSigma", visualQuality.bloomSigma);

    blurShader.setUniform("uDirection", sf::Glsl::Vec2(1.0f / (float)bloomWidth, 0.0f));
    bloomTargetB.setView(bloomTargetB.getDefaultView());
    bloomTargetB.clear(sf::Color::Black);
    {
        sf::RenderStates st;
        st.shader = &blurShader;
        bloomTargetB.draw(sf::Sprite(bloomTargetA.getTexture()), st);
    }
    bloomTargetB.display();

    blurShader.setUniform("uDirection", sf::Glsl::Vec2(0.0f, 1.0f / (float)bloomHeight));
    bloomTargetA.setView(bloomTargetA.getDefaultView());
    bloomTargetA.clear(sf::Color::Black);
    {
        sf::RenderStates st;
        st.shader = &blurShader;
        bloomTargetA.draw(sf::Sprite(bloomTargetB.getTexture()), st);
    }
    bloomTargetA.display();

    window.setView(window.getDefaultView());
    window.clear(sf::Color::Black);

    sf::Sprite base(trailTarget.getTexture());
    window.draw(base);

    sf::Sprite bloom(bloomTargetA.getTexture());
    bloom.setScale((float)window.getSize().x / (float)bloomWidth,
                   (float)window.getSize().y / (float)bloomHeight);
    window.draw(bloom, sf::BlendAdd);
}
