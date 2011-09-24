cmake_minimum_required(VERSION 2.6)

project(SpeedComparisons)

# this increases GrayScott and _double fps rate by 3 and _OpenMP by 2
if(UNIX)
   add_definitions(-O3)
endif()

add_subdirectory(GrayScott)
add_subdirectory(GrayScott_double)
add_subdirectory(GrayScott_SSE)
add_subdirectory(GrayScott_OpenMP)
add_subdirectory(GrayScott_OpenCL)

