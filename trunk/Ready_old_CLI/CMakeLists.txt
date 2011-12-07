cmake_minimum_required(VERSION 2.6)

project(ReaDy)

if (NOT CMAKE_BUILD_TYPE)
 message(STATUS "No build type selected, default to Release")
 set(CMAKE_BUILD_TYPE "Release")
endif()

if(MSVC)
  add_definitions(/arch:SSE2)
else()
  add_definitions(-msse2)
endif()

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "calc_Brusselator" )
INCLUDE_DIRECTORIES( "calc_Gray_Scott" )
INCLUDE_DIRECTORIES( "ready_display" )

add_executable(ReaDy 
  main-ready.cpp
  util.h
  util.cpp
  calc_Brusselator/brusselator.cpp
  calc_Brusselator/brusselator.h
  calc_Gray_Scott/gray_scott_scalar.cpp
  calc_Gray_Scott/gray_scott_scalar.h
  calc_Gray_Scott/gray_scott_hwivector.cpp
  calc_Gray_Scott/gray_scott_hwivector.h
  calc_Gray_Scott/hwi_vector.h
  calc_Gray_Scott/dicek.h
  ready_display/ready_display.cpp
  ready_display/ready_display.h
)

TARGET_LINK_LIBRARIES(ReaDy ${OpenCV_LIBS} )
