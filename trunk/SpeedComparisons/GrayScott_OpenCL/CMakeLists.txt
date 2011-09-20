project(GrayScott_OpenCL)

find_package(OpenCV REQUIRED)
include_directories( ${OPENCV_INCLUDE_DIR})
link_libraries( ${OpenCV_LIBS} )


# only build the OpenCL version if OpenCL was found
find_package ( OpenCL )
if(OPENCL_FOUND)
  include_directories ( ${OPENCL_INCLUDE_DIRS} )
  link_libraries ( ${OPENCL_LIBRARIES} )

  # tell the code where the .cl file will live
  add_definitions(-DCL_SOURCE_DIR="${GrayScott_OpenCL_SOURCE_DIR}")

  INCLUDE_DIRECTORIES( "../Display" )

  add_executable(GrayScott_OpenCL
    gray_scott_opencl.cpp 
    grayscott_kernel.cl 
    ../Display/display.cpp
    ../Display/display.h
    ../Display/defs.h
  )
endif()

