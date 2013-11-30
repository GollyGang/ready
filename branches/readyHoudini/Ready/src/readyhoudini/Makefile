# Makefile helper for GNU make utility.
#
# To use, create your own file named 'Makefile' with something like this:
#	DSONAME = SOP_MySOP.so 
#		 (or SOP_MySOP.dylib for Mac OSX)
#		 (or SOP_MySOP.dll for Windows)
#	SOURCES = SOP_MySOP.C
#	include $(HFS)/toolkit/makefiles/Makefile.gnu
# Then you just need to invoke make from the same directory.
#
# Complete list of variables used by this file:
#   OPTIMIZER	Override the optimization level (optional, defaults to -O2)
#   INCDIRS	Specify any additional include directories.
#   LIBDIRS	Specify any addition library include directories
#   SOURCES	List all .C files required to build the DSO or App
#   DSONAME	Name of the desired output DSO file (if applicable)
#   APPNAME	Name of the desires output application (if applicable)
#   INSTDIR	Directory to be installed. If not specified, this will
#		default to the the HOME houdini directory.
#   ICONS	Name of the icon files to install (optionial)

# To make it link I had to add -fPIC to Ready's CMakeLists.txt, which I put in the first add_definitions() call in the build block (not a good spot, should be moved down.. but need to discuss this with the team in any case).

READYROOT = ../../
DSONAME = readyHoudini.so 
SOURCES = readyHoudini.C
INCDIRS	= -I/home/dan/dev/ready/readyCommit/reaction-diffusion/src/readybase -I/usr/include/vtk-5.10 -I/opt/cuda/include/CL
LIBDIRS = -Wl,-Bdynamic -lvtkIO -Wl,-Bstatic $(READYROOT)/libreadybase.a -Wl,-Bdynamic
#LDFLAGS = -Wl,-Bstatic $(READYROOT)/libreadybase.a -Wl,-Bdynamic -lvtkIO
# -Wl,-Bstatic -L$(READYROOT) -lreadybase -Wl,-Bdynamic
# -lvtkGraphics -lvtkRendering -lvtkHybrid -lvtkCommon -Wl,-rpath,/usr/X11R6/lib64
#-lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lvtkCommon -lvtkGraphics -lvtkIO -lvtkRendering -lvtkHybrid -Wl,-rpath,/usr/X11R6/lib64
#LDFLAGS = -Wno-deprecated -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 $(READYROOT)libreadybase.a -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lvtkCommon -lvtkGraphics -lvtkIO -lvtkRendering -lvtkHybrid -Wl,-rpath,/usr/X11R6/lib64 
OS_NAME := $(shell uname -s)

ifeq ($(OS),Windows_NT)
    ifndef OPTIMIZER
	OPTIMIZER = -Ox
    endif

    AMD64 = 0 
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
	AMD64 = 1
    endif
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
	AMD64 = 1
    endif
    ifeq ($(AMD64),1)
	AMD64_SUFFIX = /amd64
	ARCHDEFS = -DAMD64 -DSIZEOF_VOID_P=8
    else
	ARCHDEFS = -DSIZEOF_VOID_P=4
    endif

    include $(HFS)/toolkit/makefiles/Makefile.win
else
ifeq ($(OS_NAME),Darwin)
    include $(HFS)/toolkit/makefiles/Makefile.osx
else
    include $(HFS)/toolkit/makefiles/Makefile.linux
endif
endif

HVERSION = 13.0
ifndef INSTDIR
    ifeq ($(OS_NAME),Darwin)
	INSTDIR = $(HOME)/Library/Preferences/houdini/$(HVERSION)
    else
	INSTDIR = $(HOME)/houdini$(HVERSION)
    endif
endif


OBJECTS = $(SOURCES:.C=.o)
OBJECTS := $(OBJECTS:.cpp=.o)

ifdef DSONAME
TAGINFO = $(shell (echo -n "Compiled on:" `date`"\n         by:" `whoami`@`hostname`"\n$(SESI_TAGINFO)") | sesitag -m)

%.o:		%.C
	$(CC) $(CXXFLAGS) $(OBJFLAGS) -DMAKING_DSO $(TAGINFO) \
	    $< $(OBJOUTPUT) $@

%.o:		%.cpp
	$(CC) $(CXXFLAGS) $(OBJFLAGS) -DMAKING_DSO $(TAGINFO) \
	    $< $(OBJOUTPUT) $@

$(DSONAME):	$(OBJECTS)
	$(LINK) $(LDFLAGS) $(SHAREDFLAG) $(OBJECTS) $(DSOFLAGS) \
	    $(DSOOUTPUT) $@
else

%.o:		%.C
	$(CC) $(CXXFLAGS) $(OBJFLAGS) $< $(OBJOUTPUT) $@

%.o:		%.cpp
	$(CC) $(CXXFLAGS) $(OBJFLAGS) $< $(OBJOUTPUT) $@

$(APPNAME):	$(OBJECTS) $(MBSD_GC_OBJ)
	$(LINK) $(LDFLAGS) $(OBJECTS) $(MBSD_GC_OBJ) $(SAFLAGS) $(SAOUTPUT) $@

ifeq ($(OS_NAME),Darwin)
# For OSX standalone applications, we need to build 
# the garbage-colection module into the standalone application.
$(MBSD_GC_OBJ): $(MBSD_GC_SRC_DIR)/$(MBSD_GC_SRC)
	$(CC) $(CXXFLAGS) $(OBJCFLAGS) $< $(OBJOUTPUT) $@
endif
endif

default:	$(DSONAME) $(APPNAME)

ifdef ICONS
icons:		$(ICONS)
	@mkdir -p $(INSTDIR)/config/Icons
	@cp $(ICONS) $(INSTDIR)/config/Icons
else
icons:
endif

ifdef DSONAME
install:	default	icons
	@mkdir -p $(INSTDIR)/dso
	@cp $(DSONAME) $(INSTDIR)/dso
else
install:	default icons
endif


clean:
	rm -f $(OBJECTS) $(APPNAME) $(DSONAME) $(MBSD_GC_OBJ)
