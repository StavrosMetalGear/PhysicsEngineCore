# ╔══════════════════════════════════════════════════════════════════════════════╗
#  PhysicsEngineCore — Unified physics engine aggregating all 8 submodules
#
#  Compiles all physics source files from every submodule directly into a
#  single static library (PhysicsEngine).  This avoids duplicate-target
#  conflicts from each submodule's imgui_lib / implot_lib / GUI executable.
#
#  Submodules:
#    QuantumCore, ClassicalMechanicsCore, ElectrodynamicsCore,
#    FluidDynamicsCore, NuclearCore, OpticsCore, SolidStateCore,
#    ThermodynamicsCore
#
#  Produces:
#    PhysicsEngine  — static library (all physics + async + coupling)
# ╚══════════════════════════════════════════════════════════════════════════════╝

cmake_minimum_required(VERSION 3.15)
project(PhysicsEngineCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Submodule root paths ─────────────────────────────────────────────────────
set(REPOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(QUANTUM_DIR         "${REPOS_DIR}/QuantumCore")
set(CLASSICAL_DIR       "${REPOS_DIR}/ClassicalMechanicsCore")
set(EM_DIR              "${REPOS_DIR}/ElectrodynamicsCore")
set(FLUID_DIR           "${REPOS_DIR}/FluidDynamicsCore")
set(NUCLEAR_DIR         "${REPOS_DIR}/NuclearCore")
set(OPTICS_DIR          "${REPOS_DIR}/OpticsCore")
set(SOLIDSTATE_DIR      "${REPOS_DIR}/SolidStateCore")
set(THERMAL_DIR         "${REPOS_DIR}/ThermodynamicsCore")

# ── Eigen (header-only, bundled in QuantumCore and SolidStateCore) ───────────
set(EIGEN_DIR "${QUANTUM_DIR}/Eigen")

# ── Physics source files from each submodule ─────────────────────────────────
# Each submodule is compiled as a separate OBJECT library to avoid header
# name collisions (e.g. both QuantumCore and ClassicalMechanicsCore define
# NumericalSolver.h with different classes).

# Helper macro: create an OBJECT library for a physics module
macro(add_physics_module NAME DIR)
    add_library(${NAME}_obj OBJECT ${ARGN})
    target_include_directories(${NAME}_obj PRIVATE ${DIR})
    target_include_directories(${NAME}_obj PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    if(MSVC)
        target_compile_definitions(${NAME}_obj PRIVATE _USE_MATH_DEFINES NOMINMAX)
        target_compile_options(${NAME}_obj PRIVATE /W3)
    else()
        target_compile_options(${NAME}_obj PRIVATE -Wall -Wextra -pedantic)
    endif()
    set_target_properties(${NAME}_obj PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
endmacro()

# 1. QuantumCore
add_physics_module(Quantum ${QUANTUM_DIR}
    ${QUANTUM_DIR}/QuantumParticle.cpp
    ${QUANTUM_DIR}/NumericalSolver.cpp
    ${QUANTUM_DIR}/quantum_c_api.cpp
)
target_include_directories(Quantum_obj PRIVATE ${EIGEN_DIR})

# 2. ClassicalMechanicsCore
add_physics_module(Classical ${CLASSICAL_DIR}
    ${CLASSICAL_DIR}/ClassicalParticle.cpp
    ${CLASSICAL_DIR}/NumericalSolver.cpp
    ${CLASSICAL_DIR}/classical_c_api.cpp
)

# 3. ElectrodynamicsCore
add_physics_module(EM ${EM_DIR}
    ${EM_DIR}/EMField.cpp
    ${EM_DIR}/NumericalSolverEM.cpp
    ${EM_DIR}/em_c_api.cpp
)

# 4. FluidDynamicsCore
add_physics_module(Fluid ${FLUID_DIR}
    ${FLUID_DIR}/FluidSystem.cpp
    ${FLUID_DIR}/NumericalSolverFD.cpp
    ${FLUID_DIR}/fluid_c_api.cpp
)

# 5. NuclearCore
add_physics_module(Nuclear ${NUCLEAR_DIR}
    ${NUCLEAR_DIR}/NuclearSystem.cpp
    ${NUCLEAR_DIR}/NumericalSolverNuc.cpp
    ${NUCLEAR_DIR}/nuclear_c_api.cpp
)

# 6. OpticsCore
add_physics_module(Optics ${OPTICS_DIR}
    ${OPTICS_DIR}/OpticalSystem.cpp
    ${OPTICS_DIR}/NumericalSolverOpt.cpp
    ${OPTICS_DIR}/optics_c_api.cpp
)

# 7. SolidStateCore
add_physics_module(SolidState ${SOLIDSTATE_DIR}
    ${SOLIDSTATE_DIR}/SolidStateSystem.cpp
    ${SOLIDSTATE_DIR}/NumericalSolverSS.cpp
    ${SOLIDSTATE_DIR}/solidstate_c_api.cpp
)
target_include_directories(SolidState_obj PRIVATE ${SOLIDSTATE_DIR}/Eigen)

# 8. ThermodynamicsCore
add_physics_module(Thermal ${THERMAL_DIR}
    ${THERMAL_DIR}/ThermoSystem.cpp
    ${THERMAL_DIR}/NumericalSolverTD.cpp
    ${THERMAL_DIR}/thermo_c_api.cpp
)

# ── Engine glue sources ──────────────────────────────────────────────────────
set(ENGINE_SOURCES
    PhysicsEngine.cpp
    PhysicsAsync.cpp
    PhysicsCoupling.cpp
)

set(ENGINE_HEADERS
    PhysicsEngine.h
    PhysicsEngine_export.h
    PhysicsAsync.h
    PhysicsCoupling.h
)

# ── PhysicsEngine unified static library ─────────────────────────────────────
add_library(PhysicsEngine STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    $<TARGET_OBJECTS:Quantum_obj>
    $<TARGET_OBJECTS:Classical_obj>
    $<TARGET_OBJECTS:EM_obj>
    $<TARGET_OBJECTS:Fluid_obj>
    $<TARGET_OBJECTS:Nuclear_obj>
    $<TARGET_OBJECTS:Optics_obj>
    $<TARGET_OBJECTS:SolidState_obj>
    $<TARGET_OBJECTS:Thermal_obj>
)

target_include_directories(PhysicsEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QUANTUM_DIR}
    ${CLASSICAL_DIR}
    ${EM_DIR}
    ${FLUID_DIR}
    ${NUCLEAR_DIR}
    ${OPTICS_DIR}
    ${SOLIDSTATE_DIR}
    ${THERMAL_DIR}
    ${EIGEN_DIR}
    ${SOLIDSTATE_DIR}/Eigen
)

# ── Automated tests ───────────────────────────────────────────────────────────
enable_testing()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_quantum.cpp")
    add_executable(test_quantum tests/test_quantum.cpp)
    target_link_libraries(test_quantum PRIVATE PhysicsEngine)
    target_include_directories(test_quantum PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QUANTUM_DIR}
        ${CLASSICAL_DIR}
        ${EM_DIR}
        ${FLUID_DIR}
        ${NUCLEAR_DIR}
        ${OPTICS_DIR}
        ${SOLIDSTATE_DIR}
        ${THERMAL_DIR}
    )
    add_test(NAME test_quantum COMMAND test_quantum)
endif()

if(MSVC)
    target_compile_definitions(PhysicsEngine PUBLIC _USE_MATH_DEFINES NOMINMAX)
    target_compile_options(PhysicsEngine PRIVATE /W3)
else()
    target_compile_options(PhysicsEngine PRIVATE -Wall -Wextra -pedantic)
endif()

set_target_properties(PhysicsEngine PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ── Smoke test executable ─────────────────────────────────────────────────────
add_executable(PhysicsEngineTest main.cpp)
target_link_libraries(PhysicsEngineTest PRIVATE PhysicsEngine)
target_include_directories(PhysicsEngineTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QUANTUM_DIR}
    ${CLASSICAL_DIR}
    ${EM_DIR}
    ${FLUID_DIR}
    ${NUCLEAR_DIR}
    ${OPTICS_DIR}
    ${SOLIDSTATE_DIR}
    ${THERMAL_DIR}
    ${EIGEN_DIR}
    ${SOLIDSTATE_DIR}/Eigen
)
# ── AABB collision test ─────────────────────────────────────────────────────
add_executable(test_aabb tests/test_aabb.cpp)
target_include_directories(test_aabb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_aabb COMMAND test_aabb)
