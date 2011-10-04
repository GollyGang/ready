project(GrayScott_HWIVector)

FIND_PACKAGE(OpenCV REQUIRED)
INCLUDE_DIRECTORIES( ${OPENCV_INCLUDE_DIR})

if(MSVC)
  add_definitions(/arch:SSE2)
else()
  add_definitions(-msse2)
endif()

add_executable(GrayScott_HWIVector 
  gray_scott_hwivector.cpp 
  hwi_vector.h
  dicek.h
  display_hwiv.cpp
  display_hwiv.h
)

TARGET_LINK_LIBRARIES(GrayScott_HWIVector ${OpenCV_LIBS} )
