#  Copyright 2011-2015 The Ready Bunch
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

set( READY_VERSION 0.7 )   # check matches Help/about.html
add_definitions( -D READY_VERSION=${READY_VERSION} )

if( APPLE OR WIN32 )
  # app names are usually capitalized on Mac OS X and Windows
  set( APP_NAME Ready )
else()
  # Linux binaries are usually all lowercase
  set( APP_NAME ready )
endif()
set( CMD_NAME rdy ) # command-line version

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
  src/readybase/SystemFactory.hpp             src/readybase/SystemFactory.cpp
  src/readybase/scene_items.hpp               src/readybase/scene_items.cpp
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
  src/gui/InteractorStylePainter.hpp       src/gui/InteractorStylePainter.cpp
  src/gui/wxVTKRenderWindowInteractor.h    src/gui/wxVTKRenderWindowInteractor.cxx
  src/gui/RecordingDialog.hpp              src/gui/RecordingDialog.cpp
)
include_directories( src/gui )

set( CMD_SOURCES      # code used for the command-line version
  src/cmd/main.cpp
)

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
)
include_directories( resources )

set( PATTERN_FILES
  Patterns/Meinhardt1982/stripes.vti       Patterns/Meinhardt1982/zebra.vtu
  Patterns/advection.vti
  Patterns/bilaplacian_interpolation.vti
  Patterns/Schlogl.vti 
  Patterns/heat_equation.vti 
  Patterns/Turing1952/spots.vti            Patterns/Turing1952/spots_noisy.vti
  Patterns/kernel_test.vti 
  Patterns/parameter_modulation_demo.vti
  Patterns/parameter_modulation_demo2.vti
  Patterns/bunny.vtu
  Patterns/lion.vtu
  Patterns/heat_equation_interpolation.vti
  Patterns/Ginzburg-Landau/complex_Ginzburg-Landau.vti
  Patterns/Ginzburg-Landau/complex_Ginzburg-Landau_magnitude.vti
  Patterns/wave_equation.vti
  Patterns/oregonator.vti
  Patterns/Brusselator.vti
  Patterns/SmoothLife2011/smoothglider.vti     Patterns/SmoothLife2011/smoothlifeL.vti     Patterns/SmoothLife2011/glider_3D.vti
  Patterns/SmoothLife2011/gaussian-smoothlife.vti
  Patterns/Purwins1999/glider.vti          Patterns/Purwins1999/glider_3D.vti      Patterns/Purwins1999/multiGlider.vti
  Patterns/CPU-only/grayscott_1D.vti 
  Patterns/CPU-only/grayscott_2D.vti 
  Patterns/CPU-only/grayscott_3D.vti
  Patterns/FitzHugh-Nagumo/tip-splitting.vti 
  Patterns/FitzHugh-Nagumo/spiral_turbulence.vti
  Patterns/FitzHugh-Nagumo/pulsate.vti 
  Patterns/FitzHugh-Nagumo/squid_axon.vti
  Patterns/FitzHugh-Nagumo/Ising_regime.vti
  Patterns/Gray-Scott/Lesmes_noisy.vti
  Patterns/Gray-Scott/noisy_solitons_mitosis.vti
  Patterns/Gray-Scott/parameter-map.vti
  Patterns/Gray-Scott/Pearson1993.vti
  Patterns/Gray-Scott/self-replicating_spots.vti 
  Patterns/Gray-Scott/U-Skate/Hutton-and-helix-gliders.vti
  Patterns/Gray-Scott/U-Skate/Munafo_glider.vti
  Patterns/Gray-Scott/U-Skate/o-ring_2D.vti
  Patterns/CellularAutomata/Bays_3D.vti
  Patterns/CellularAutomata/Conway_life.vti 
  Patterns/CellularAutomata/life_torus.vtu
  Patterns/CellularAutomata/larger-than-life.vti
  Patterns/CellularAutomata/Buss_hex.vtu
  Patterns/CellularAutomata/tri_life.vtu
  Patterns/CellularAutomata/hex_B2oS2m34_gliders.vtu
  Patterns/CellularAutomata/PenroseTilings/life.vtu
  Patterns/CellularAutomata/PenroseTilings/life_oscillators.vtu
  Patterns/CellularAutomata/PenroseTilings/Goucher_glider.vtu
  Patterns/CellularAutomata/PenroseTilings/Imai_glider_B2SC4.vtu
  Patterns/CellularAutomata/PenroseTilings/Goucher_loops.vtu
  Patterns/CellularAutomata/Salt/salt2D_demo.vti
  Patterns/CellularAutomata/Salt/salt3D_circular330.vti
  Patterns/Yang2002/Yang_1.vti    Patterns/Yang2002/Yang_2b.vti  Patterns/Yang2002/Yang_2c.vti
  Patterns/Yang2002/Yang_2d.vti   Patterns/Yang2002/Yang_3a.vti  Patterns/Yang2002/Yang_3b.vti
  Patterns/Yang2002/Yang_3c.vti   Patterns/Yang2002/Yang_3d.vti  Patterns/Yang2002/Yang_4.vti
  Patterns/Yang2003/Fig2.vti
  Patterns/Yang2003/Fig3a.vti     Patterns/Yang2003/Fig3b.vti    Patterns/Yang2003/Fig3c.vti
  Patterns/McCabe2010/McCabe.vti               Patterns/McCabe2010/McCabe_simple.vti
  Patterns/McCabe2010/McCabe_additive2b.vti    Patterns/McCabe2010/McCabe_additive2a.vti
  Patterns/McCabe2010/fast-mccabe.vti
  Patterns/Kytta2007/Fig5.7a.vti  Patterns/Kytta2007/Fig5.7c.vti
  Patterns/Kytta2007/Fig5.8c.vti  Patterns/Kytta2007/Fig5.8d.vti Patterns/Kytta2007/Fig5.8e.vti
  Patterns/Kytta2007/Fig5.8f.vti  Patterns/Kytta2007/Fig5.8g.vti
  Patterns/Yang2006/jumping.vti   Patterns/Yang2006/jumping_cGL.vti
  Patterns/Schrodinger1926/packet.vti             Patterns/Schrodinger1926/packet_reflect.vti
  Patterns/Schrodinger1926/packet_pass.vti        Patterns/Schrodinger1926/quantum_tunnelling.vti
  Patterns/Schrodinger1926/packet_reflect2D.vti
  Patterns/Froese2014/Froese2014_Fig3.vti
  Patterns/Pennybacker2013/phyllotaxis_fibonacci.vti Patterns/Pennybacker2013/phyllotaxis_hexagons.vti
  Patterns/Pennybacker2013/spots.vti Patterns/Pennybacker2013/stripes.vti
  Patterns/Pennybacker2013/parameter_map.vti
  Patterns/Experiments/TimHutton/mutually-catalytic_spots.vti
  Patterns/Experiments/DanWills/cglrd_ramps_example.vti  
  Patterns/Experiments/DanWills/grayscott-historyWave_fuseWorms.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_moreLifelike.vti
  Patterns/Experiments/DanWills/grayscott-historyWaveDC_solitonsAndWorms_init.vti
  Patterns/Experiments/DanWills/orbits_explodey_init.vti
  Patterns/Experiments/DanWills/orbits_sharpWaves-init.vti
  Patterns/Experiments/DanWills/grayscott-historyWave_coralGrow.vti
  Patterns/Experiments/SimonGladman/vermiformSolitons.vti
  Patterns/Experiments/TimHutton/LifeBlur.vti
  Patterns/Experiments/CornusAmmonis/MandelbrotWorms.vti
  Patterns/Experiments/CornusAmmonis/mutually-catalytic-pinwheels.vti
  Patterns/Experiments/CornusAmmonis/mutually-catalytic-spots-second-order-sobel.vti
  Patterns/Experiments/CornusAmmonis/pearson-chained-edge-detectors.vti
  Patterns/Experiments/CornusAmmonis/surfing-solitons.vti
  Patterns/Experiments/CornusAmmonis/tip-splitting-web.vti
  Patterns/Experiments/CornusAmmonis/worms2.vti
  Patterns/Experiments/CornusAmmonis/bz-warpsharp.vti
  Patterns/Experiments/CornusAmmonis/smoke.vti
  Patterns/Experiments/CornusAmmonis/splats.vti
  Patterns/Agmon2014/cells.vti      Patterns/Agmon2014/cells_double.vti
  Patterns/Agmon2014/oil-water.vti
  Patterns/NumericalMethods/advection_forwardEuler.vti
  Patterns/NumericalMethods/advection_2stepAdamsBashforth.vti
  Patterns/NumericalMethods/advection_3stepAdamsBashforth.vti
  Patterns/NumericalMethods/advection_modifiedEuler.vti
  Patterns/NumericalMethods/advection_midpointMethod.vti
  Patterns/NumericalMethods/advection_HeunsMethod.vti
  Patterns/NumericalMethods/advection_LaxFriedrichs.vti
  Patterns/NumericalMethods/advection_LaxWendroff.vti
  Patterns/NumericalMethods/advection_RungeKutta4.vti
  Patterns/NumericalMethods/advection_staggeredLeapfrog.vti
  Patterns/NumericalMethods/advection_forwardEuler_double.vti
  Patterns/NumericalMethods/advection_2stepAdamsBashforth_double.vti
  Patterns/NumericalMethods/advection_3stepAdamsBashforth_double.vti
  Patterns/NumericalMethods/advection_modifiedEuler_double.vti
  Patterns/NumericalMethods/advection_midpointMethod_double.vti
  Patterns/NumericalMethods/advection_HeunsMethod_double.vti
  Patterns/NumericalMethods/advection_LaxFriedrichs_double.vti
  Patterns/NumericalMethods/advection_LaxWendroff_double.vti
  Patterns/NumericalMethods/advection_RungeKutta4_double.vti
  Patterns/NumericalMethods/advection_staggeredLeapfrog_double.vti
  Patterns/Guo2014/guo.vti
)

set( HELP_FILES
  Help/about.gif              Help/about.html
  Help/action.html            Help/credits.html 
  Help/file.html              Help/help.html 
  Help/mouse.html             Help/quickstart.html 
  Help/tips.html              Help/changes.html 
  Help/edit.html              Help/formats.html 
  Help/index.html             Help/problems.html 
  Help/view.html              Help/introduction.html
)

set( OTHER_FILES
  ./README.txt 
  ./COPYING.txt 
  ./TODO.txt 
  ./BUILD.txt 
  ./CMakeLists.txt 
  src/FindOpenCL.cmake 
  src/Doxyfile.in
  resources/logo.png
  resources/Icons/22px/icon-pointer.png
  resources/Icons/22px/draw-freehand.png
  resources/Icons/22px/draw-brush.png
  resources/Icons/22px/color-picker.png
  resources/Icons/22px/document-new.png
  resources/Icons/22px/document-open.png
  resources/Icons/22px/document-save.png
  resources/Icons/22px/document-revert.png
  resources/Icons/22px/media-playback-start_green.png
  resources/Icons/22px/media-playback-pause_red.png
  resources/Icons/22px/media-seek-forward.png
  resources/Icons/22px/media-seek-backward.png
  resources/Icons/22px/media-skip-backward_modified.png
  resources/Icons/22px/media-record.png
  resources/Icons/22px/system-run.png
  resources/Icons/22px/list-add_gray.png
  resources/Icons/22px/camera-photo.png
  resources/Icons/32px/icon-pointer.png
  resources/Icons/32px/draw-freehand.png
  resources/Icons/32px/draw-brush.png
  resources/Icons/32px/color-picker.png
  resources/Icons/32px/document-new.png
  resources/Icons/32px/document-open.png
  resources/Icons/32px/document-save.png
  resources/Icons/32px/document-revert.png
  resources/Icons/32px/media-playback-start_green.png
  resources/Icons/32px/media-playback-pause_red.png
  resources/Icons/32px/media-seek-forward.png
  resources/Icons/32px/media-seek-backward.png
  resources/Icons/32px/media-skip-backward_modified.png
  resources/Icons/32px/media-record.png
  resources/Icons/32px/system-run.png
  resources/Icons/32px/list-add_gray.png
  resources/Icons/32px/camera-photo.png
  resources/Cursors/pencil-cursor.png
  resources/Cursors/brush-cursor.png
  resources/Cursors/picker-cursor.png
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
    # CMake 2.6:
    FIND_PACKAGE(PkgConfig)
    pkg_check_modules (GTK2 gtk+-2.0)
    INCLUDE_DIRECTORIES(${GTK2_INCLUDE_DIRS})
    LINK_LIBRARIES(${GTK2_LIBRARIES})
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

foreach( file ${PATTERN_FILES} ${HELP_FILES} ${RESOURCES} ${OTHER_FILES} )
  add_custom_command( 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${file}"
    COMMAND cmake -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
  )
  list( APPEND files_dest "${CMAKE_CURRENT_BINARY_DIR}/${file}" )
endforeach() 

add_custom_target( CopyFiles ALL DEPENDS ${files_dest} )

#-------------------------------- build ------------------------------------------------------

# ensure we link the C runtime statically (N.B. still appears as /MD in the CMake gui but ignore this)
# see: http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace
foreach( var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO )
  string( REGEX REPLACE "/MD" "/MT" ${var} "${${var}}" )
endforeach()

set( USE_SSE "YES" )
if( USE_SSE )
  # enable SSE to allow us to set flags to avoid denormals, and relax the floating-point accuracy for speed
  if( MSVC )
    if( CMAKE_CL_64 )
      set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fp:fast" )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast" )
    else() # avoid warning on x64, which always comes with SSE2
      set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2 /fp:fast" )
      set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2 /fp:fast" )
    endif()
  else()
    add_definitions( -msse2 -ffast-math )
  endif()
  add_definitions( -DUSE_SSE )
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
if( ${VTK_MAJOR_VERSION} GREATER 5 )
  target_link_libraries( 
    readybase 
    vtkCommonCore 
    vtkFiltersExtraction
    vtkFiltersGeometry
    vtkFiltersModeling
    vtkFiltersTexture
    vtkInteractionStyle
    vtkIOImage 
    vtkIOXML 
    vtkRenderingCore 
    vtkRenderingAnnotation 
    vtkRenderingFreeType 
    vtkRenderingFreeTypeOpenGL 
  )
else()
  target_link_libraries( readybase vtkCommon vtkGraphics vtkIO vtkRendering vtkHybrid )
endif()

# create command-line utility
add_executable( ${CMD_NAME} ${CMD_SOURCES} )
target_link_libraries( ${CMD_NAME} readybase )

# create GUI application
add_executable( ${APP_NAME} ${GUI_EXECUTABLE} ${GUI_SOURCES} ${RESOURCES} )
target_link_libraries( ${APP_NAME} readybase ${wxWidgets_LIBRARIES} )

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

# avoid security warnings
if( MSVC )
  add_definitions( /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS )
endif()

if( CMAKE_COMPILER_IS_GNUCXX )
  list( APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE "-s" )  # strip release binary, for smaller file size
  set ( CMAKE_CXX_FLAGS "-std=c++11")
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
install( TARGETS ${APP_NAME} DESTINATION "." )      # (add ${CMD_NAME} if we want to distribute the command-line version too)

# install our source files, resource files, pattern files, help files and text files
foreach( source_file ${BASE_SOURCES} ${GUI_SOURCES} ${CMD_SOURCES} ${RESOURCES} ${PATTERN_FILES} ${HELP_FILES} ${OTHER_FILES} )
  get_filename_component( path_name "${source_file}" PATH )
  install( FILES "${source_file}" DESTINATION ${path_name} )
endforeach()

#----------------------------------------package----------------------------------------------

if( APPLE )
  set( CPACK_SYSTEM_NAME "Mac" )  # nicer than "Darwin"
elseif( UNIX )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( CPACK_SYSTEM_NAME "Linux-64bit" )  
  else()
    set( CPACK_SYSTEM_NAME "Linux-32bit" )  
  endif()
elseif( WIN32)
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( CPACK_SYSTEM_NAME "Windows-64bit" )  
  else()
    set( CPACK_SYSTEM_NAME "Windows-32bit" )  
  endif()
endif()
if( NOT USE_SSE )
  set( CPACK_SYSTEM_NAME "${CPACK_SYSTEM_NAME}-noSSE" )
endif()
set( CPACK_GENERATOR "ZIP" )
set( CPACK_PACKAGE_VERSION "${READY_VERSION}" )
set( CPACK_SOURCE_GENERATOR "ZIP" )
include( CPack )
