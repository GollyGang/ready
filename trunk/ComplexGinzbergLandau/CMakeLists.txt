project(ComplexGinzbergLandau)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(ComplexGinzbergLandau 
  complex_ginzberg_landau.cpp 
  ../Display/display.cpp
  ../Display/display.h
)

TARGET_LINK_LIBRARIES(ComplexGinzbergLandau ${OpenCV_LIBS} )
