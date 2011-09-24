project(GrayScott_HWIVector)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(GrayScott_HWIVector 
  gray_scott_hwivector.cpp 
  hwi_vector.h
  ../Display/display.cpp
  ../Display/display.h
  ../Display/defs.h
)

TARGET_LINK_LIBRARIES(GrayScott_HWIVector ${OpenCV_LIBS} )
