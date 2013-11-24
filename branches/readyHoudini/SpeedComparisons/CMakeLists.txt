cmake_minimum_required(VERSION 2.6)

project(SpeedComparisons)

if (NOT CMAKE_BUILD_TYPE)
 message(STATUS "No build type selected, default to Release")
 set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory(GrayScott)
add_subdirectory(GrayScott_HWIVector)
add_subdirectory(GrayScott_double)
add_subdirectory(GrayScott_OpenCV)
add_subdirectory(GrayScott_SSE)
add_subdirectory(GrayScott_SSE_OpenMP)
add_subdirectory(GrayScott_OpenMP)
add_subdirectory(GrayScott_OpenCL)
add_subdirectory(GrayScott_OpenCL_float2)
add_subdirectory(GrayScott_OpenCL_Local)
add_subdirectory(GrayScott_OpenCL_2x2)
add_subdirectory(GrayScott_OpenCL_Image)
add_subdirectory(GrayScott_OpenCL_Image_2x2)

