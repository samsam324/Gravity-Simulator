#include <SFML/Graphics.hpp>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "system_info.h"
#include "app_config.h"
#include "gravity_simulation.h"
#include "renderer.h"

int main() {
    sf::ContextSettings contextSettings;
    contextSettings.antialiasingLevel = 8;

    sf::RenderWindow window(
        sf::VideoMode(1600, 1000),
        "Gravity Simulator",
        sf::Style::Default,
        contextSettings
    );

    window.setVerticalSyncEnabled(true);
    window.setActive(true);

    sf::View worldView(sf::FloatRect(0.0f, 0.0f, 1600.0f, 1000.0f));
    worldView.setCenter(0.0f, 0.0f);
    window.setView(worldView);

    SystemInfo systemInfo = detectSystemInfo();
    AppConfig config = buildAppConfig(systemInfo);

    GravitySimulation simulation(config.workerThreads, config.particleCount, config.deterministicSeed);
    Renderer renderer((int)window.getSize().x, (int)window.getSize().y);

    bool isPaused = false;

    bool isPanning = false;
    sf::Vector2i lastMousePixelPosition;

    double fixedStepAccumulatorSeconds = 0.0;
    sf::Clock frameClock;

    int frameCounter = 0;
    sf::Clock fpsClock;

    auto updateWindowTitle = [&](int fps) {
        std::ostringstream thetaStream;
        thetaStream << std::fixed << std::setprecision(2) << simulation.params().barnesHutTheta;

        std::string title =
            "Gravity Simulator | Threads=" + std::to_string(config.workerThreads) +
            " | N=" + std::to_string(simulation.particles().count()) +
            " | GPU=" + systemInfo.gpuRendererString +
            " | theta=" + thetaStream.str() +
            " | Barnes-Hut" +
            " | FPS~" + std::to_string(fps);

        window.setTitle(title);
    };

    updateWindowTitle(0);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::Resized) {
                renderer.resize((int)event.size.width, (int)event.size.height);
            }

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) window.close();
                if (event.key.code == sf::Keyboard::Space) isPaused = !isPaused;
                if (event.key.code == sf::Keyboard::R) simulation.reset();

                if (event.key.code == sf::Keyboard::Up) {
                    simulation.params().barnesHutTheta = std::min(1.20f, simulation.params().barnesHutTheta + 0.05f);
                }
                if (event.key.code == sf::Keyboard::Down) {
                    simulation.params().barnesHutTheta = std::max(0.25f, simulation.params().barnesHutTheta - 0.05f);
                }

                if (event.key.code == sf::Keyboard::Num1) renderer.setQualityPreset(1);
                if (event.key.code == sf::Keyboard::Num2) renderer.setQualityPreset(2);
                if (event.key.code == sf::Keyboard::Num3) renderer.setQualityPreset(3);
            }

            if (event.type == sf::Event::MouseWheelScrolled) {
                float zoomFactor = (event.mouseWheelScroll.delta > 0.0f) ? 0.9f : 1.111111f;
                worldView.zoom(zoomFactor);
                window.setView(worldView);
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Middle) {
                isPanning = true;
                lastMousePixelPosition = sf::Mouse::getPosition(window);
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Middle) {
                isPanning = false;
            }
        }

        if (isPanning) {
            sf::Vector2i currentMousePixelPosition = sf::Mouse::getPosition(window);
            sf::Vector2f previousWorld = window.mapPixelToCoords(lastMousePixelPosition);
            sf::Vector2f currentWorld = window.mapPixelToCoords(currentMousePixelPosition);
            sf::Vector2f worldDelta = previousWorld - currentWorld;

            worldView.move(worldDelta);
            window.setView(worldView);
            lastMousePixelPosition = currentMousePixelPosition;
        }

        double frameSeconds = frameClock.restart().asSeconds();
        fixedStepAccumulatorSeconds += frameSeconds;

        if (!isPaused) {
            const double fixedStepSeconds = simulation.params().fixedTimeStep;
            if (fixedStepAccumulatorSeconds > fixedStepSeconds) {
                fixedStepAccumulatorSeconds = fixedStepSeconds;
            }
            
            if (fixedStepAccumulatorSeconds >= fixedStepSeconds) {
                simulation.stepFixed(fixedStepSeconds);
                fixedStepAccumulatorSeconds -= fixedStepSeconds;
            }
        }

        renderer.render(window, worldView, simulation.particles());
        window.display();

        frameCounter++;
        if (fpsClock.getElapsedTime().asSeconds() >= 0.5f) {
            float seconds = fpsClock.getElapsedTime().asSeconds();
            int fps = (seconds > 0.0f) ? (int)(frameCounter / seconds) : 0;
            frameCounter = 0;
            fpsClock.restart();
            updateWindowTitle(fps);
        }
    }

    return 0;
}
