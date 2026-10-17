cmake_minimum_required(VERSION 3.15)
project(PhysicsEngineConsumer LANGUAGES CXX)

find_package(PhysicsEngine CONFIG REQUIRED)

add_executable(physics_engine_consumer main.cpp)
target_compile_features(physics_engine_consumer PRIVATE cxx_std_17)
target_link_libraries(physics_engine_consumer PRIVATE PhysicsEngine::PhysicsEngine)