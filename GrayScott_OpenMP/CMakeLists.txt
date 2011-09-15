project(GrayScott_OpenMP)

find_package( OpenCV REQUIRED )
include_directories( ${OPENCV_INCLUDE_DIR} )
link_libraries( ${OpenCV_LIBS} )

include(FindOpenMP)
if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

INCLUDE_DIRECTORIES( "../Display" )

add_executable(GrayScott_OpenMP
  gray_scott_openmp.cpp 
  ../Display/display.cpp
  ../Display/display.h
  ../Display/defs.h
)

