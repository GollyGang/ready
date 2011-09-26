project(GrayScott_SSE_OpenMP)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

if(MSVC)
  add_definitions(/arch:SSE2)
else()
  add_definitions(-msse)
endif()

# only build the OpenMP version if OpenMP was found
include(FindOpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

  add_executable(GrayScott_SSE_OpenMP
    gray_scott_SSE_OpenMP.cpp 
    ../Display/display.cpp
    ../Display/display.h
    ../Display/defs.h
  )

  TARGET_LINK_LIBRARIES(GrayScott_SSE_OpenMP ${OpenCV_LIBS} )
endif()
