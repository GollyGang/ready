project(TuringSpots)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(TuringSpots 
  turing_spots.cpp 
  ../Display/display.cpp
  ../Display/display.h
)

TARGET_LINK_LIBRARIES(TuringSpots ${OpenCV_LIBS} )
