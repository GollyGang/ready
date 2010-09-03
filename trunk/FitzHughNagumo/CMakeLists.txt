project(FitzHughNagumo)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(FitzHughNagumo
  fitz_hugh_nagumo.cpp 
  ../Display/display.cpp
  ../Display/display.h
)

TARGET_LINK_LIBRARIES(FitzHughNagumo ${OpenCV_LIBS} )
