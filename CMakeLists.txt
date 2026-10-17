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

include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Submodule root paths ─────────────────────────────────────────────────────
set(PHYSICSCORE_REPOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH
    "Directory containing the PhysicsCore sibling repositories")
get_filename_component(PHYSICSCORE_REPOS_DIR "${PHYSICSCORE_REPOS_DIR}" ABSOLUTE)

set(PHYSICSCORE_REQUIRED_REPOS
    QuantumCore
    ClassicalMechanicsCore
    ElectrodynamicsCore
    FluidDynamicsCore
    NuclearCore
    OpticsCore
    SolidStateCore
    ThermodynamicsCore
)

foreach(REPO_NAME IN LISTS PHYSICSCORE_REQUIRED_REPOS)
    if(NOT IS_DIRECTORY "${PHYSICSCORE_REPOS_DIR}/${REPO_NAME}")
        message(FATAL_ERROR
            "Required sibling repository '${REPO_NAME}' was not found at "
            "'${PHYSICSCORE_REPOS_DIR}/${REPO_NAME}'. Set PHYSICSCORE_REPOS_DIR "
            "to the directory containing all eight PhysicsCore repositories.")
    endif()
endforeach()

set(QUANTUM_DIR         "${PHYSICSCORE_REPOS_DIR}/QuantumCore")
set(CLASSICAL_DIR       "${PHYSICSCORE_REPOS_DIR}/ClassicalMechanicsCore")
set(EM_DIR              "${PHYSICSCORE_REPOS_DIR}/ElectrodynamicsCore")
set(FLUID_DIR           "${PHYSICSCORE_REPOS_DIR}/FluidDynamicsCore")
set(NUCLEAR_DIR         "${PHYSICSCORE_REPOS_DIR}/NuclearCore")
set(OPTICS_DIR          "${PHYSICSCORE_REPOS_DIR}/OpticsCore")
set(SOLIDSTATE_DIR      "${PHYSICSCORE_REPOS_DIR}/SolidStateCore")
set(THERMAL_DIR         "${PHYSICSCORE_REPOS_DIR}/ThermodynamicsCore")

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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${QUANTUM_DIR}>
    $<BUILD_INTERFACE:${CLASSICAL_DIR}>
    $<BUILD_INTERFACE:${EM_DIR}>
    $<BUILD_INTERFACE:${FLUID_DIR}>
    $<BUILD_INTERFACE:${NUCLEAR_DIR}>
    $<BUILD_INTERFACE:${OPTICS_DIR}>
    $<BUILD_INTERFACE:${SOLIDSTATE_DIR}>
    $<BUILD_INTERFACE:${THERMAL_DIR}>
    $<BUILD_INTERFACE:${EIGEN_DIR}>
    $<BUILD_INTERFACE:${SOLIDSTATE_DIR}/Eigen>
    $<INSTALL_INTERFACE:include>
)

set(PHYSICSENGINE_PUBLIC_HEADERS
    PhysicsEngine.h
    PhysicsEngine_export.h
    PhysicsAsync.h
    PhysicsCoupling.h
    ${QUANTUM_DIR}/quantum_c_api.h
    ${QUANTUM_DIR}/QuantumExport.h
    ${CLASSICAL_DIR}/classical_c_api.h
    ${CLASSICAL_DIR}/ClassicalExport.h
    ${EM_DIR}/em_c_api.h
    ${EM_DIR}/EMExport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fluid_c_api.h
    ${FLUID_DIR}/FluidExport.h
    ${NUCLEAR_DIR}/nuclear_c_api.h
    ${NUCLEAR_DIR}/NuclearExport.h
    ${OPTICS_DIR}/optics_c_api.h
    ${OPTICS_DIR}/OpticsExport.h
    ${SOLIDSTATE_DIR}/solidstate_c_api.h
    ${THERMAL_DIR}/thermo_c_api.h
    ${THERMAL_DIR}/ThermoExport.h
)

install(TARGETS PhysicsEngine
    EXPORT PhysicsEngineTargets
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)
install(FILES ${PHYSICSENGINE_PUBLIC_HEADERS} DESTINATION include)
install(DIRECTORY include/physics DESTINATION include)

install(EXPORT PhysicsEngineTargets
    FILE PhysicsEngineTargets.cmake
    NAMESPACE PhysicsEngine::
    DESTINATION lib/cmake/PhysicsEngine
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PhysicsEngineConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PhysicsEngineConfig.cmake
    INSTALL_DESTINATION lib/cmake/PhysicsEngine
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/PhysicsEngineConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/PhysicsEngineConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/PhysicsEngineConfigVersion.cmake
    DESTINATION lib/cmake/PhysicsEngine
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

# ── Unified API integration test ─────────────────────────────────────────────
add_executable(test_engine_integration tests/test_engine_integration.cpp)
target_link_libraries(test_engine_integration PRIVATE PhysicsEngine)
target_include_directories(test_engine_integration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(NAME test_engine_integration COMMAND test_engine_integration)

add_executable(test_engine_metadata tests/test_engine_metadata.cpp)
target_link_libraries(test_engine_metadata PRIVATE PhysicsEngine)
target_include_directories(test_engine_metadata PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(NAME test_engine_metadata COMMAND test_engine_metadata)

add_executable(test_async_stress tests/test_async_stress.cpp)
target_link_libraries(test_async_stress PRIVATE PhysicsEngine)
target_include_directories(test_async_stress PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(NAME test_async_stress COMMAND test_async_stress)

add_executable(test_coupling_science tests/test_coupling_science.cpp)
target_link_libraries(test_coupling_science PRIVATE PhysicsEngine)
target_include_directories(test_coupling_science PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(NAME test_coupling_science COMMAND test_coupling_science)

# ── OBB collision test ───────────────────────────────────────────────────────
add_executable(test_obb tests/test_obb.cpp)
target_include_directories(test_obb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_obb COMMAND test_obb)

# ── Sphere collision test ────────────────────────────────────────────────────
add_executable(test_sphere tests/test_sphere.cpp)
target_include_directories(test_sphere PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_sphere COMMAND test_sphere)
# ── Narrowphase collision test ───────────────────────────────────────────────
add_executable(test_narrowphase tests/test_narrowphase.cpp)
target_include_directories(test_narrowphase PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_narrowphase COMMAND test_narrowphase)
# ── Broadphase collision test ────────────────────────────────────────────────
add_executable(test_broadphase tests/test_broadphase.cpp)
target_include_directories(test_broadphase PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_broadphase COMMAND test_broadphase)
# ── Triangle collision test ──────────────────────────────────────────────────
add_executable(test_triangle tests/test_triangle.cpp)
target_include_directories(test_triangle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_triangle COMMAND test_triangle)
# ── Plane collision test ─────────────────────────────────────────────────────
add_executable(test_plane tests/test_plane.cpp)
target_include_directories(test_plane PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_plane COMMAND test_plane)

# ── Ray intersection test ────────────────────────────────────────────────────
add_executable(test_ray tests/test_ray.cpp)
target_include_directories(test_ray PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_ray COMMAND test_ray)
# ── Capsule collision test ───────────────────────────────────────────────────
add_executable(test_capsule tests/test_capsule.cpp)
target_include_directories(test_capsule PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_capsule COMMAND test_capsule)
# ── AABB collision test ─────────────────────────────────────────────────────
add_executable(test_aabb tests/test_aabb.cpp)
target_include_directories(test_aabb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_aabb COMMAND test_aabb)
# ── Convex Polyhedron collision test ─────────────────────────────────────────
add_executable(test_convex_polyhedron tests/test_convex_polyhedron.cpp)
target_include_directories(test_convex_polyhedron PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_convex_polyhedron COMMAND test_convex_polyhedron)

# ── Collision edge-case test ─────────────────────────────────────────────────
add_executable(test_collision_edges tests/test_collision_edges.cpp)
target_include_directories(test_collision_edges PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_test(NAME test_collision_edges COMMAND test_collision_edges)
