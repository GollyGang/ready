project(FitzHughNagumo3D)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

INCLUDE_DIRECTORIES( "../Display" )

add_executable(FitzHughNagumo3D
  fitz_hugh_nagumo_3d.cpp 
  ../Display/display.cpp
  ../Display/display.h
)

TARGET_LINK_LIBRARIES(FitzHughNagumo3D ${OpenCV_LIBS} )
