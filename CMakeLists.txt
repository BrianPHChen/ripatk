cmake_minimum_required (VERSION 3.5)
project (ripatk)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
	message(STATUS " ===> Setting default build type RELEASE")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "setting default build type RELEASE" FORCE)
endif()

set(GLOBAL_FLAGS "-Wall -fopenmp -fPIC -std=c++11")
set(CMAKE_CXX_FLAGS "${GLOBAL_FLAGS}")
message(STATUS "Global flags: ${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "" FORCE)
message(STATUS "RELEASE flags: ${CMAKE_CXX_FLAGS_RELEASE}")

set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/json)

add_executable(main src/main.cpp)
target_link_libraries(main nlohmann_json)

