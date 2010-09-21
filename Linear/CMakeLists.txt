project(Linear)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(Linear 
  linear.cpp 
  ../Display/display.cpp
  ../Display/display.h
)

TARGET_LINK_LIBRARIES(Linear ${OpenCV_LIBS} )
