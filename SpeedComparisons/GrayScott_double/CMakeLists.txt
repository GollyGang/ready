project(GrayScott_double)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(GrayScott_double 
  gray_scott_double.cpp 
  ../Display/defs.h
)

TARGET_LINK_LIBRARIES(GrayScott_double ${OpenCV_LIBS} )
