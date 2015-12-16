#==========
#
# Copyright (c) 2010, Dan Bethell.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
#     * Neither the name of Dan Bethell nor the names of any
#       other contributors to this software may be used to endorse or
#       promote products derived from this software without specific prior
#       written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========
#
# Variables defined by this module:
#   Houdini_FOUND    
#   Houdini_INCLUDE_DIR
#   Houdini_COMPILE_FLAGS
#   Houdini_LINK_FLAGS
#   Houdini_LIBRARIES
#   Houdini_LIBRARY_DIR
#
# Usage: 
#   FIND_PACKAGE( Houdini )
#   FIND_PACKAGE( Houdini REQUIRED )
#
# Note:
# You can tell the module where Houdini is installed by setting
# the Houdini_INSTALL_PATH (or setting the HFS environment
# variable before calling FIND_PACKAGE.
# 
# E.g. 
#   SET( Houdini_INSTALL_PATH "/opt/hfs10.0.430" )
#   FIND_PACKAGE( Houdini REQUIRED )
#
#==========

# our includes
FIND_PATH( Houdini_INCLUDE_DIR UT/UT_DSOVersion.h
  $ENV{HFS}/toolkit/include
  ${Houdini_INSTALL_PATH}/toolkit/include
  /Library/Frameworks/Houdini.framework/Resources/toolkit/include/
  )

# our libraries
SET( __Houdini_LIBS
  HoudiniUI
  HoudiniOPZ 
  HoudiniOP3 
  HoudiniOP2 
  HoudiniOP1 
  HoudiniSIM 
  HoudiniGEO 
  HoudiniPRM 
  HoudiniUT
  GLU 
  GL 
  X11 
  Xext 
  Xi 
  dl 
  pthread
  )

# Some libs only required on OSX
IF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  SET( __Houdini_LIBS 
    ${__Houdini_LIBS} 
    GR 
    )
ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )

# here we loop over the libraries try to find them
SET( __Houdini_LIBPATHS "" )
FOREACH( __Houdini_LIB ${__Houdini_LIBS} )
  SET( LIBVAR_NAME "H_LIB_${__Houdini_LIB}" )
  FIND_LIBRARY( ${LIBVAR_NAME} ${__Houdini_LIB}
    $ENV{HFS}/dsolib
    ${Houdini_INSTALL_PATH}/dsolib
    /Library/Frameworks/Houdini.framework/Libraries
    )
  IF( ${${LIBVAR_NAME}} MATCHES "${LIBVAR_NAME}-NOTFOUND" )
    MESSAGE( WARNING "Could not find Houdini library: ${__Houdini_LIB}" )
    UNSET( __Houdini_LIBPATHS )
    BREAK()  
  ENDIF( ${${LIBVAR_NAME}} MATCHES "${LIBVAR_NAME}-NOTFOUND" )
  SET( __Houdini_LIBPATHS ${__Houdini_LIBPATHS} ${${LIBVAR_NAME}} )
ENDFOREACH( __Houdini_LIB "${__Houdini_LIBS}" )
SET( Houdini_LIBRARIES ${__Houdini_LIBPATHS} ) 
UNSET( __Houdini_LIBPATHS )
UNSET( __Houdini_LIBS )

# find out where our Houdini libraries are
LIST( GET Houdini_LIBRARIES 0 H_SRC_LIB )
GET_FILENAME_COMPONENT( Houdini_LIBRARY_DIR ${H_SRC_LIB} PATH )
UNSET( H_SRC_LIB )

# mac osx
IF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  SET( Houdini_COMPILE_FLAGS "-DSIZEOF_VOID_P=8 -DMBSD -DMBSD_COCOA -D__APPLE__ -arch x86_64" )
  SET( Houdini_LINK_FLAGS "-Wl,-rpath,${Houdini_LIBRARY_DIR} -arch x86_64" )
ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )

# linux
IF( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
  IF( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
    SET( Houdini_COMPILE_FLAGS "-DSIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P} -m64 -fPIC -DAMD64 -DLINUX -D__LINUX__" )
  ELSE()
    SET( Houdini_COMPILE_FLAGS "-DSIZEOF_VOID_P=${CMAKE_SIZEOF_VOID_P} -march=i686 -march=pentium3 -msse -mmmx -Di386 -D__LINUX__" )
  ENDIF( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64" )
ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )

# compilation flags
SET( Houdini_COMPILE_FLAGS "${Houdini_COMPILE_FLAGS} -O2 -DDLLEXPORT=\"\" -DMAKING_DSO -D_GNU_SOURCE -DSESI_LITTLE_ENDIAN -DENABLE_THREADS -DUSE_PTHREADS -DENABLE_UI_THREADS -DGCC3 -DGCC4" )

# linker flags
SET( Houdini_LINK_FLAGS "${Houdini_LINK_FLAGS} -Wall -W -Wno-parentheses -Wno-sign-compare -Wno-reorder -Wno-uninitialized -Wunused -Wno-unused-parameter -Wno-deprecated" )

# did we find everything?
INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "Houdini" DEFAULT_MSG
  Houdini_INCLUDE_DIR
  Houdini_COMPILE_FLAGS
  Houdini_LINK_FLAGS
  Houdini_LIBRARIES
  )
