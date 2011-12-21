#  Copyright 2011, The Ready Bunch
#
#  This file is part of Ready.
#
#  Ready is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Ready is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Ready. If not, see <http://www.gnu.org/licenses/>.
#
#--------------------------------------------------------------------------
    
cmake_minimum_required(VERSION 2.6)

if(COMMAND cmake_policy)
cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project( Ready )

set( READY_VERSION 0.0.1 )
add_definitions( -D READY_VERSION=${READY_VERSION} )

if(APPLE OR WIN32)
   # app names are usually capitalized on Mac OS X and Windows
   set( APP_NAME Ready )
else()
   # Linux binaries are usually all lowercase
   set( APP_NAME ready )
endif()

#-------------------------------------------source files----------------------------------------------

# low-level code used in all executables
set(BASE_SOURCES
   BaseRD.hpp BaseRD.cpp
   GrayScott_slow.hpp GrayScott_slow.cpp
   GrayScott_slow_3D.hpp GrayScott_slow_3D.cpp
   OpenCL_RD.hpp OpenCL_RD.cpp
   OpenCL2D_2Chemicals.hpp OpenCL2D_2Chemicals.cpp
   OpenCL3D_2Chemicals.hpp OpenCL3D_2Chemicals.cpp
   utils.hpp utils.cpp
)
if (NOT APPLE)
   # dynamic loading of OpenCL is only done on Windows and Linux
   set( BASE_SOURCES ${BASE_SOURCES} OpenCL_Dyn_Load.h OpenCL_Dyn_Load.c )
endif()

# high-level GUI code used only in Ready
set(GUI_SOURCES
   app.hpp app.cpp 
   frame.hpp frame.cpp 
   vtk_pipeline.hpp vtk_pipeline.cpp
   wxVTKRenderWindowInteractor.h wxVTKRenderWindowInteractor.cxx   # a local copy, since it gets released so infrequently
)

set(RESOURCES
   ready.rc
   appicon.ico appicon16.ico appicon32.ico appicon48.ico appicon.xpm
   Info.plist.in
   app.icns
)

#-------------------------------------------VTK----------------------------------------------

find_package( VTK )
IF(VTK_FOUND)
  INCLUDE(${VTK_USE_FILE})
ELSE(VTK_FOUND)
  MESSAGE(FATAL_ERROR "Cannot build the executable without VTK. Please set the VTK variables.")
ENDIF(VTK_FOUND)

#-------------------------------------------wxVTK----------------------------------------------

# The following allows you to access wxGLCanvas for GTK
IF(WIN32)
  SET(GUI_EXECUTABLE WIN32)
ELSE(WIN32)
  IF(APPLE)
    SET(GUI_EXECUTABLE MACOSX_BUNDLE)
    IF(VTK_USE_COCOA)
      SET_SOURCE_FILES_PROPERTIES(
        wxVTKRenderWindowInteractor.cxx
        PROPERTIES COMPILE_FLAGS "-ObjC++")
    ENDIF(VTK_USE_COCOA)
  ELSE(APPLE)
    # Ok X11 for sure, but just check:
    IF(NOT VTK_USE_X)
      MESSAGE(FATAL_ERROR "You need to have VTK_USE_X")
    ENDIF(NOT VTK_USE_X)
    # CMake 2.6:
    # technically those packages are not required since one can still use the Motif/X11 version and not the gtk one:
    FIND_PACKAGE(PkgConfig)
    pkg_check_modules (GTK2 gtk+-2.0)
    #MESSAGE("${GTK2_INCLUDE_DIRS}")
    INCLUDE_DIRECTORIES(${GTK2_INCLUDE_DIRS})
    LINK_LIBRARIES(${GTK2_LIBRARIES})
    # Can I require all my user to have the gl lib on linux, even if they do not really need it...
    SET(WXGLCANVASLIBS "gl")
  ENDIF(APPLE)
ENDIF(WIN32)

#-------------------------------------------wxWidgets----------------------------------------------

if(APPLE)
   # on Mac OS X it's better to use locally installed wxWidgets headers and libs
   # (the pre-installed stuff tends to be out of date; eg. 10.6 has wxMac 2.8.8 and it's a 32-bit debug build)
   set( wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config )
   set( wxWidgets_wxrc_EXECUTABLE /usr/local/bin/wxrc )         # not used, but no harm leaving it in
elseif(UNIX)
   # remove -rdynamic from link options on Linux to reduce size by about 1.2MB
   set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" )
endif()

# wxWidgets is required to build the project
FIND_PACKAGE(wxWidgets COMPONENTS aui ${WXGLCANVASLIBS} core adv base )

IF(wxWidgets_FOUND)
  INCLUDE( ${wxWidgets_USE_FILE} )
ELSE(wxWidgets_FOUND)
  MESSAGE(FATAL_ERROR
   "Cannot build the executable without wxWidgets. Please set the wxWidgets variables.")
ENDIF(wxWidgets_FOUND)

#-------------------------------------------zLib----------------------------------------------

if(WIN32)
   # use zlib library included in wxWidgets
   set( ZLIB_INCLUDE_DIR ${wxWidgets_ROOT_DIR}/src/zlib )
   set( ZLIB_LIBRARY ${wxWidgets_LIB_DIR}/wxzlib.lib )
endif()

find_package( ZLIB REQUIRED )
include_directories( 
   ${ZLIB_INCLUDE_DIR}
)
add_definitions( -DZLIB )

#-------------------------------------------OpenCL----------------------------------------------

set(CMAKE_MODULE_PATH ${Ready_SOURCE_DIR})
# (we include our own FindOpenCL.cmake until the time that CMake comes with its own)

# we need to build against OpenCL
find_package ( OpenCL REQUIRED )
include_directories ( ${OPENCL_INCLUDE_DIRS} )
if(APPLE)
  link_libraries ( ${OPENCL_LIBRARIES} ) # on MacOSX we assume that OpenCL is available (might need to rethink for versions before 10.6)
endif()
  
#-----------------------------------------------------------------------------------------------

if(APPLE)
   # support Mac OS 10.5 or later
   add_definitions(-mmacosx-version-min=10.5)
endif()
if(APPLE OR UNIX)
   # use same settings as in makefiles
   add_definitions(-D_LARGE_FILES)
endif()

# create base library used by all executables
add_library( readybase STATIC ${BASE_SOURCES} )

add_executable( ${APP_NAME}
   ${GUI_EXECUTABLE}
   ${GUI_SOURCES}
   ${RESOURCES} 
)

target_link_libraries( ${APP_NAME}
  readybase
  ${wxWidgets_LIBRARIES} 
  vtkCommon 
  vtkGraphics 
  vtkIO 
  vtkRendering
  vtkHybrid
  vtkVolumeRendering
)

if(APPLE OR WIN32)
   target_link_libraries( ${APP_NAME} ${ZLIB_LIBRARIES} )
else()
   # on Linux we statically link zlib because some systems (eg. Ubuntu) have a non-standard zlib that contains gzopen64
   add_library( static_zlib STATIC IMPORTED )
   set_property( TARGET static_zlib PROPERTY IMPORTED_LOCATION /usr/lib/libz.a )
   target_link_libraries( ${APP_NAME} static_zlib )
endif()

if(APPLE)
   # create Info.plist (using Info.plist.in) and PkgInfo files inside .app bundle
   add_custom_target( app_bundle
      COMMAND sed -e "s/VERSION/${APP_VERSION}/" ${CMAKE_SOURCE_DIR}/Info.plist.in >Ready.app/Contents/Info.plist
      COMMAND echo -n "APPLReDy" >Ready.app/Contents/PkgInfo
   )
   add_dependencies( ${APP_NAME} app_bundle )
   
   # copy *.icns files into Resources directory inside .app bundle
   set_source_files_properties( ${CMAKE_SOURCE_DIR}/app.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
   
   # remove unreachable functions and data, and don't add debug info (reduces app size by about 12MB)
   target_link_libraries( ${APP_NAME} -Wl,-dead_strip -Wl,-S )
endif()

# hack to get around the "Debug" and "Release" folders Visual Studio adds on Windows
# http://stackoverflow.com/questions/543203/cmake-runtime-output-directory-on-windows
if(MSVC_IDE)
   set_target_properties( ${APP_NAME} PROPERTIES PREFIX "../" )
endif()
