cmake_minimum_required(VERSION 3.15)
project(PhysicsEngineCConsumer LANGUAGES C CXX)

find_package(PhysicsEngine CONFIG REQUIRED)

add_executable(physics_engine_c_consumer main.c metadata_header.c)
set_target_properties(physics_engine_c_consumer PROPERTIES
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	LINKER_LANGUAGE CXX
)
target_link_libraries(physics_engine_c_consumer PRIVATE PhysicsEngine::PhysicsEngine)

enable_testing()
add_test(NAME physics_engine_c_consumer COMMAND physics_engine_c_consumer)
