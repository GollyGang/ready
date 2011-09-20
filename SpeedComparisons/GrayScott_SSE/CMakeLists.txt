project(GrayScott_SSE)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

if(MSVC)
  add_definitions(/arch:SSE2)
else()
  add_definitions(-msse3)
endif()

add_executable(GrayScott_SSE
  gray_scott_SSE.cpp 
  ../Display/display.cpp
  ../Display/display.h
  ../Display/defs.h
)

TARGET_LINK_LIBRARIES(GrayScott_SSE ${OpenCV_LIBS} )
