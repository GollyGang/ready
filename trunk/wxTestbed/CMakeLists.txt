cmake_minimum_required(VERSION 2.6)

project( wxTestbed )

find_package( wxWidgets REQUIRED COMPONENTS propgrid html aui core adv base )
include( ${wxWidgets_USE_FILE} )

add_executable( wxTestbed WIN32 main.cpp )

target_link_libraries( wxTestbed ${wxWidgets_LIBRARIES} )
