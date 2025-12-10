cmake_minimum_required(VERSION 3.18.4)
project(libnecrolog LANGUAGES CXX)

set(WITH_EXAMPLES OFF CACHE BOOL "Enable build of examples")
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries instead of static")

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
include(GenerateExportHeader)

set(CMAKE_SHARED_LIBRARY_PREFIX "") # we don't want CMake to prepend "lib" to our libraries, we prefer adding that ourselves

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(NECROLOG_WARNING_FLAGS "-Wall -Wextra -pedantic -Wshadow -Wcast-align -Wunused -Wpedantic -Wconversion -Wsign-conversion -Wmisleading-indentation -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual")

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(NECROLOG_WARNING_FLAGS "-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast ${NECROLOG_WARNING_FLAGS}")
	endif()

	set(CMAKE_CXX_FLAGS "${NECROLOG_WARNING_FLAGS} ${CMAKE_CXX_FLAGS}")
endif()

add_library(libnecrolog)
generate_export_header(libnecrolog EXPORT_FILE_NAME necrolog/libnecrolog_export.h)

target_sources(
	libnecrolog
	PRIVATE libnecrolog/necrolog.cpp
	PUBLIC
		FILE_SET HEADERS
			BASE_DIRS include
			FILES
				include/necrolog.h
				include/necrolog/necrologlevel.h
		FILE_SET generated_headers
			TYPE HEADERS
			FILES
			BASE_DIRS $<TARGET_PROPERTY:libnecrolog,BINARY_DIR>
			FILES "${CMAKE_CURRENT_BINARY_DIR}/necrolog/libnecrolog_export.h"

)

add_library(libnecrolog::libnecrolog ALIAS libnecrolog)

if(WITH_EXAMPLES)
	add_executable(log_example examples/log_example.cpp)
	target_include_directories(log_example PRIVATE include)
	target_link_libraries(log_example PRIVATE libnecrolog)
endif()

if(BUILD_TESTING)
	find_package(doctest QUIET)
	if(NOT doctest_FOUND)
		message(STATUS "doctest library NOT found, disabling tests")
		set(BUILD_TESTING OFF)
	endif()
endif()

if(BUILD_TESTING)
	add_executable(test_necrolog tests/test_necrolog.cpp)
	add_test(NAME test_necrolog COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:test_necrolog>)
	target_link_libraries(test_necrolog PRIVATE libnecrolog doctest::doctest)
endif()

include(GNUInstallDirs)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/necrolog.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/necrolog.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/necrolog.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

install(TARGETS libnecrolog EXPORT necrologConfig FILE_SET HEADERS FILE_SET generated_headers)
export(TARGETS libnecrolog NAMESPACE libnecrolog:: FILE "${CMAKE_CURRENT_BINARY_DIR}/necrologConfig.cmake")
install(EXPORT necrologConfig DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/necrolog/cmake" NAMESPACE libnecrolog::)

install(DIRECTORY libnecrolog/ TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
