#  Copyright 2011, 2012 The Ready Bunch
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
    
cmake_minimum_required( VERSION 2.6 )
cmake_policy( SET CMP0003 NEW )

project( Ready )

set( READY_VERSION 0.3 )   # check matches Help/about.html
add_definitions( -D READY_VERSION=${READY_VERSION} )

if( APPLE OR WIN32 )
  # app names are usually capitalized on Mac OS X and Windows
  set( APP_NAME Ready )
else()
  # Linux binaries are usually all lowercase
  set( APP_NAME ready )
endif()

#-------------------------------------------source files----------------------------------------------

set( BASE_SOURCES      # low-level code used in all executables
  src/readybase/AbstractRD.hpp                src/readybase/AbstractRD.cpp
  src/readybase/ImageRD.hpp                   src/readybase/ImageRD.cpp
  src/readybase/GrayScottImageRD.hpp          src/readybase/GrayScottImageRD.cpp
  src/readybase/OpenCLImageRD.hpp             src/readybase/OpenCLImageRD.cpp
  src/readybase/FormulaOpenCLImageRD.hpp      src/readybase/FormulaOpenCLImageRD.cpp
  src/readybase/FullKernelOpenCLImageRD.hpp   src/readybase/FullKernelOpenCLImageRD.cpp
  src/readybase/MeshRD.hpp                    src/readybase/MeshRD.cpp
  src/readybase/GrayScottMeshRD.hpp           src/readybase/GrayScottMeshRD.cpp
  src/readybase/OpenCLMeshRD.hpp              src/readybase/OpenCLMeshRD.cpp
  src/readybase/FormulaOpenCLMeshRD.hpp       src/readybase/FormulaOpenCLMeshRD.cpp
  src/readybase/FullKernelOpenCLMeshRD.hpp    src/readybase/FullKernelOpenCLMeshRD.cpp
  src/readybase/OpenCL_MixIn.hpp              src/readybase/OpenCL_MixIn.cpp
  src/readybase/OpenCL_utils.hpp              src/readybase/OpenCL_utils.cpp
  src/readybase/IO_XML.hpp                    src/readybase/IO_XML.cpp
  src/readybase/overlays.hpp                  src/readybase/overlays.cpp
  src/readybase/Properties.hpp                src/readybase/Properties.cpp
  src/readybase/utils.hpp                     src/readybase/utils.cpp
  src/readybase/OpenCL_Dyn_Load.h             src/readybase/OpenCL_Dyn_Load.c 
  src/readybase/MeshGenerators.hpp            src/readybase/MeshGenerators.cpp
)
include_directories( src/readybase )

set( GUI_SOURCES      # high-level GUI code used only in Ready
  src/gui/IDs.hpp
  src/gui/wxutils.hpp                      src/gui/wxutils.cpp
  src/gui/dialogs.hpp                      src/gui/dialogs.cpp
  src/gui/prefs.hpp                        src/gui/prefs.cpp
  src/gui/app.hpp                          src/gui/app.cpp 
  src/gui/frame.hpp                        src/gui/frame.cpp 
  src/gui/HelpPanel.hpp                    src/gui/HelpPanel.cpp
  src/gui/InfoPanel.hpp                    src/gui/InfoPanel.cpp
  src/gui/PatternsPanel.hpp                src/gui/PatternsPanel.cpp
  src/gui/vtk_pipeline.hpp                 src/gui/vtk_pipeline.cpp
  src/gui/wxVTKRenderWindowInteractor.h    src/gui/wxVTKRenderWindowInteractor.cxx
  src/gui/RecordingDialog.hpp              src/gui/RecordingDialog.cpp
)
include_directories( src/gui )

set( RESOURCES
  resources/ready.rc
  resources/appicon.ico 
  resources/appicon16.ico 
  resources/appicon32.ico 
  resources/appicon48.ico 
  resources/appicon.xpm
  resources/Info.plist.in
  resources/app.icns 
  resources/file.icns
  resources/logo.png
)
include_directories( resources )

set( PATTERN_FILES
  grayscott_demo.vti 
  meinhardt_stripes.vti 
  schlogl.vti 
  heat_equation.vti 
  turing.vti
  turing_noisy.vti
  kernel_test.vti 
  conway_life.vti 
  bays_3D.vti
  munafo-uskate.vti
  grayscott_parameter_map.vti
  grayscott_lesmes_noisy.vti
  grayscott_noisy_solitons_mitosis.vti
  mutually-catalytic_spots.vti
  parameter_modulation_demo.vti
  parameter_modulation_demo2.vti
  life_torus.vtu
  penrose.vtu
  bunny.vtu
  horse.vtu
  lion.vtu
  heat_equation_interpolation.vti
  complex_Ginsburg-Landau.vti
  wave_equation.vti
  CPU-only/grayscott_1D.vti 
  CPU-only/grayscott_2D.vti 
  CPU-only/grayscott_3D.vti
  FitzHugh-Nagumo/tip-splitting.vti 
  FitzHugh-Nagumo/spiral_turbulence.vti
  FitzHugh-Nagumo/pulsate.vti 
  FitzHugh-Nagumo/squid_axon.vti
  FitzHugh-Nagumo/Ising_regime.vti
)

set( HELP_FILES
  about.gif about.html
  action.html credits.html file.html help.html mouse.html quickstart.html tips.html
  changes.html edit.html formats.html index.html problems.html view.html
  introduction.html
)

set( OTHER_FILES
  ./README.txt ./COPYING.txt ./TODO.txt ./BUILD.txt 
  ./CMakeLists.txt 
  src/FindOpenCL.cmake src/Doxyfile.in
)

#-------------------------------------------VTK----------------------------------------------

find_package( VTK )
if( VTK_FOUND )
  include( ${VTK_USE_FILE} )
else()
  message(FATAL_ERROR "Cannot build the executable without VTK. Please set the VTK variables.")
endif()

#-------------------------------------------wxVTK----------------------------------------------

# The following allows you to access wxGLCanvas for GTK
IF(WIN32)
  SET(GUI_EXECUTABLE WIN32)
ELSE(WIN32)
  IF(APPLE)
    SET(GUI_EXECUTABLE MACOSX_BUNDLE)
    IF(VTK_USE_COCOA)
      SET_SOURCE_FILES_PROPERTIES(
        src/gui/wxVTKRenderWindowInteractor.cxx
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

if( APPLE )
  # on Mac OS X it's better to use locally installed wxWidgets headers and libs
  # (the pre-installed stuff tends to be out of date; eg. 10.6 has wxMac 2.8.8 and it's a 32-bit debug build)
  set( wxWidgets_CONFIG_EXECUTABLE /usr/local/bin/wx-config )
  set( wxWidgets_wxrc_EXECUTABLE /usr/local/bin/wxrc )         # not used, but no harm leaving it in
elseif(UNIX)
  # remove -rdynamic from link options on Linux to reduce size by about 1.2MB
  set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" )
endif()

# wxWidgets is required to build the project
FIND_PACKAGE( wxWidgets COMPONENTS html aui ${WXGLCANVASLIBS} core adv base )

IF( wxWidgets_FOUND )
  INCLUDE( ${wxWidgets_USE_FILE} )
ELSE()
  MESSAGE( FATAL_ERROR "Cannot build the executable without wxWidgets. Please set the wxWidgets variables." )
ENDIF()

if( WIN32 )
  # prevent link errors with wxMSW 2.9.x
  add_definitions( -DwxDEBUG_LEVEL=0 )
endif()

#-------------------------------------------OpenCL----------------------------------------------

set( CMAKE_MODULE_PATH ${Ready_SOURCE_DIR}/src )
# (we include our own FindOpenCL.cmake until the time that CMake comes with its own)

# we need to build against OpenCL
find_package( OpenCL REQUIRED )
include_directories( ${OPENCL_INCLUDE_DIRS} )
if( APPLE )
  link_libraries( ${OPENCL_LIBRARIES} ) # on MacOSX we assume that OpenCL is available (might need to rethink for versions before 10.6)
endif()
  
#---------------copy installation files to build folder (helps with testing)--------------------

foreach( pattern_file ${PATTERN_FILES} )
  add_custom_command( 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Patterns/${pattern_file}"
    COMMAND cmake -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Patterns/${pattern_file}" "${CMAKE_CURRENT_BINARY_DIR}/Patterns/${pattern_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Patterns/${pattern_file}"
  )
  list( APPEND files_dest "${CMAKE_CURRENT_BINARY_DIR}/Patterns/${pattern_file}" )
endforeach() 

foreach( help_file ${HELP_FILES} )
  add_custom_command( 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/Help/${help_file}"
    COMMAND cmake -E copy "${CMAKE_CURRENT_SOURCE_DIR}/Help/${help_file}" "${CMAKE_CURRENT_BINARY_DIR}/Help/${help_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Help/${help_file}"
  )
  list( APPEND files_dest "${CMAKE_CURRENT_BINARY_DIR}/Help/${help_file}" )
endforeach() 

foreach( other_file ${OTHER_FILES} )
  add_custom_command( 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${other_file}"
    COMMAND cmake -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${other_file}" "${CMAKE_CURRENT_BINARY_DIR}/${other_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${other_file}"
  )
  list( APPEND files_dest "${CMAKE_CURRENT_BINARY_DIR}/${other_file}" )
endforeach() 

add_custom_target( CopyFiles ALL DEPENDS ${files_dest} )

#-------------------------------- build ------------------------------------------------------

# enable SSE to allow us to set flags to avoid denormals, and relax the floating-point accuracy for speed
if(MSVC)
  add_definitions(/arch:SSE2 /fp:fast)
else()
  add_definitions(-msse2 -ffast-math)
endif()

if( APPLE )
  # support Mac OS 10.5 or later
  add_definitions( -mmacosx-version-min=10.5 )
endif()
if( APPLE OR UNIX )
  # use same settings as in makefiles
  add_definitions( -D_LARGE_FILES )
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
  vtkCommon vtkGraphics vtkIO vtkRendering vtkHybrid
)

if( APPLE )
  # create Info.plist (using Info.plist.in) and PkgInfo files inside .app bundle
  add_custom_target( app_bundle
    COMMAND sed -e "s/VERSION/${READY_VERSION}/" ${CMAKE_SOURCE_DIR}/resources/Info.plist.in >Ready.app/Contents/Info.plist
    COMMAND echo -n "APPLReDy" >Ready.app/Contents/PkgInfo
  )
  add_dependencies( ${APP_NAME} app_bundle )
   
  # copy *.icns files into Resources directory inside .app bundle
  set_source_files_properties( ${CMAKE_SOURCE_DIR}/resources/app.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
  set_source_files_properties( ${CMAKE_SOURCE_DIR}/resources/file.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
   
  # remove unreachable functions and data, and don't add debug info (reduces app size by about 12MB)
  target_link_libraries( ${APP_NAME} -Wl,-dead_strip -Wl,-S )
endif()

# (Visual Studio only) put the executable in the root binary folder, not in "Debug" or "Release"
# http://stackoverflow.com/questions/543203/cmake-runtime-output-directory-on-windows
if( MSVC_IDE )
  set_target_properties( ${APP_NAME} PROPERTIES PREFIX "../" )
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  list( APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE "-s" )  # strip release binary, for smaller file size
endif()

#----------------------------------------doxygen------------------------------------------------

find_package( Doxygen )
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()
# (if doxygen is found then 'make doc' should produce html documentation of the source code)

#----------------------------------------install------------------------------------------------

# put Ready in the root of the installation folder instead of in "bin"
install( TARGETS ${APP_NAME} DESTINATION "." )

# install our source files, resource files and text files
foreach( source_file ${BASE_SOURCES} ${GUI_SOURCES} ${RESOURCES} ${OTHER_FILES} )
  get_filename_component( path_name "${source_file}" PATH )
  install( FILES "${source_file}" DESTINATION ${path_name} )
endforeach()

# install our pattern files
foreach( pattern_file ${PATTERN_FILES} )
  get_filename_component( path_name "Patterns/${pattern_file}" PATH )
  install( FILES "Patterns/${pattern_file}" DESTINATION ${path_name} )
endforeach()

# install our help files
foreach( help_file ${HELP_FILES} )
  get_filename_component( path_name "Help/${help_file}" PATH )
  install( FILES "Help/${help_file}" DESTINATION ${path_name} )
endforeach()

#----------------------------------------package----------------------------------------------

if( APPLE )
  set( CPACK_SYSTEM_NAME "Mac" )  # nicer than "Darwin"
endif()
set( CPACK_GENERATOR "ZIP" )
set( CPACK_PACKAGE_VERSION "${READY_VERSION}" )
set( CPACK_SOURCE_GENERATOR "ZIP" )
include( CPack )
