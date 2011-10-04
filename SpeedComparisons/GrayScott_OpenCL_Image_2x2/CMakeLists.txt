project(GrayScott_OpenCL_Image_2x2)

set(CMAKE_MODULE_PATH ${GrayScott_OpenCL_Image_2x2_SOURCE_DIR})
# (we include our own FindOpenCL.cmake until the time that CMake comes with its own)

find_package(OpenCV REQUIRED)
include_directories( ${OPENCV_INCLUDE_DIR})
link_libraries( ${OpenCV_LIBS} )

# only build the OpenCL version if OpenCL was found
find_package ( OpenCL )
if(OPENCL_FOUND)
  include_directories ( ${OPENCL_INCLUDE_DIRS} )
  link_libraries ( ${OPENCL_LIBRARIES} )

  # tell the code where the .cl file will live
  add_definitions(-DCL_SOURCE_DIR="${GrayScott_OpenCL_Image_2x2_SOURCE_DIR}")
  
  include_directories(../Display)

  add_executable(GrayScott_OpenCL_Image_2x2
    gray_scott_opencl_image_2x2.cpp 
    grayscott_kernel_image_2x2.cl 
    ../Display/defs.h
  )
endif()

