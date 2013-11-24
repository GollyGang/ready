project(GrayScott_OpenCV)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(GrayScott_OpenCV 
  gray_scott_OpenCV.cpp 
  ../Display/defs.h
)

TARGET_LINK_LIBRARIES(GrayScott_OpenCV ${OpenCV_LIBS} )
