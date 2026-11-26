cmake_minimum_required(VERSION 3.15)
project(PhysicsEngineRelocatedConsumers LANGUAGES C CXX)

if(NOT DEFINED PACKAGE_PREFIX OR NOT IS_DIRECTORY "${PACKAGE_PREFIX}")
	message(FATAL_ERROR "PACKAGE_PREFIX must identify the relocated package.")
endif()
get_filename_component(package_root "${PACKAGE_PREFIX}" REALPATH)
find_package(PhysicsEngine CONFIG REQUIRED PATHS "${package_root}" NO_DEFAULT_PATH)

get_target_property(package_includes PhysicsEngine::PhysicsEngine INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(package_archive PhysicsEngine::PhysicsEngine IMPORTED_LOCATION_RELEASE)
get_filename_component(expected_include "${package_root}/include" REALPATH)
get_filename_component(expected_archive "${package_root}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}PhysicsEngine${CMAKE_STATIC_LIBRARY_SUFFIX}" REALPATH)
if(NOT expected_include IN_LIST package_includes OR NOT package_archive STREQUAL expected_archive)
	message(FATAL_ERROR "Imported target does not reference the relocated package: includes=[${package_includes}], archive=[${package_archive}]")
endif()

get_filename_component(project_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(relocated_c_consumer
	"${project_root}/examples/c_consumer/main.c"
	"${project_root}/examples/c_consumer/metadata_header.c")
set_target_properties(relocated_c_consumer PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
target_link_libraries(relocated_c_consumer PRIVATE PhysicsEngine::PhysicsEngine)

add_executable(relocated_cpp_consumer
	"${project_root}/examples/consumer/main.cpp"
	"${project_root}/examples/consumer/metadata_header.cpp")
set_target_properties(relocated_cpp_consumer PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(relocated_cpp_consumer PRIVATE PhysicsEngine::PhysicsEngine)

enable_testing()
add_test(NAME relocated_c_consumer COMMAND relocated_c_consumer)
add_test(NAME relocated_cpp_consumer COMMAND relocated_cpp_consumer)
