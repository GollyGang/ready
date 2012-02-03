cmake_minimum_required(VERSION 2.6)

project( propgrid_sample )

find_package( wxWidgets REQUIRED COMPONENTS propgrid aui core adv base )
include( ${wxWidgets_USE_FILE} )

add_executable( propgrid_sample WIN32 propgrid.cpp propgrid.h )

target_link_libraries( propgrid_sample ${wxWidgets_LIBRARIES} )
