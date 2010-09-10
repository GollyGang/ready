project(Schnakenberg)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(Schnakenberg 
  schnakenberg.cpp 
  ../Display/display.cpp
  ../Display/display.h
)

TARGET_LINK_LIBRARIES(Schnakenberg ${OpenCV_LIBS} )
