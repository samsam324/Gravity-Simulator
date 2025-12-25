cmake_minimum_required(VERSION 3.16)
project(gravity_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(gravity_sim
  src/main.cpp
  src/app_config.cpp
  src/system_info.cpp
  src/particles.cpp
  src/barnes_hut.cpp
  src/thread_pool.cpp
  src/gravity_simulation.cpp
  src/renderer.cpp
)

target_include_directories(gravity_sim PRIVATE src)
target_link_libraries(gravity_sim PRIVATE sfml-graphics sfml-window sfml-system opengl32)

if (MSVC)
  target_compile_options(gravity_sim PRIVATE /O2)
else()
  target_compile_options(gravity_sim PRIVATE -O3)
endif()
