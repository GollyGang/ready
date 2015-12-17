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
# 
# Please Note: this might not work, the main supported build is through the regular ready cmake with the boolean PLUGIN_BUILD set to true!
#
#

READYROOT = ../../../
DSONAME = readyHoudini.so 
SOURCES = readyHoudini.C
INCDIRS	= -I$(READYROOT)/src/readybase -I/usr/include/vtk-6.1 -I/opt/cuda/include/CL
LDFLAGS = -Wl,-Bstatic $(READYROOT)/libreadybase.a -Wl,-Bdynamic -L/home/dan/dev/vtk/vtk.git/lib 


##### LIBDIRS = -Wl,-Bdynamic -Wl,-Bstatic $(READYROOT)/libreadybase.a -Wl,-Bdynamic
#-lvtkalglib -lvtkChartsCorePython27D -lvtkChartsCorePython27D -lvtkChartsCore -lvtkChartsCore -lvtkCommonColorPython27D -lvtkCommonColorPython27D -lvtkCommonColor -lvtkCommonColor -lvtkCommonComputationalGeometryPython27D -lvtkCommonComputationalGeometryPython27D -lvtkCommonComputationalGeometry -lvtkCommonComputationalGeometry -lvtkCommonCorePython27D -lvtkCommonCorePython27D -lvtkCommonCore -lvtkCommonCore -lvtkCommonDataModelPython27D -lvtkCommonDataModelPython27D -lvtkCommonDataModel -lvtkCommonDataModel -lvtkCommonExecutionModelPython27D -lvtkCommonExecutionModelPython27D -lvtkCommonExecutionModel -lvtkCommonExecutionModel -lvtkCommonMathPython27D -lvtkCommonMathPython27D -lvtkCommonMath -lvtkCommonMath -lvtkCommonMiscPython27D -lvtkCommonMiscPython27D -lvtkCommonMisc -lvtkCommonMisc -lvtkCommonSystemPython27D -lvtkCommonSystemPython27D -lvtkCommonSystem -lvtkCommonSystem -lvtkCommonTransformsPython27D -lvtkCommonTransformsPython27D -lvtkCommonTransforms -lvtkCommonTransforms -lvtkDICOMParser -lvtkDICOMParser -lvtkDomainsChemistryPython27D -lvtkDomainsChemistryPython27D -lvtkDomainsChemistry -lvtkDomainsChemistry -lvtkexoIIc -lvtkexoIIc -lvtkFiltersAMRPython27D -lvtkFiltersAMRPython27D -lvtkFiltersAMR -lvtkFiltersAMR -lvtkFiltersCorePython27D -lvtkFiltersCorePython27D -lvtkFiltersCore -lvtkFiltersCore -lvtkFiltersExtractionPython27D -lvtkFiltersExtractionPython27D -lvtkFiltersExtraction -lvtkFiltersExtraction -lvtkFiltersFlowPathsPython27D -lvtkFiltersFlowPathsPython27D -lvtkFiltersFlowPaths -lvtkFiltersFlowPaths -lvtkFiltersGeneralPython27D -lvtkFiltersGeneralPython27D -lvtkFiltersGeneral -lvtkFiltersGeneral -lvtkFiltersGenericPython27D -lvtkFiltersGenericPython27D -lvtkFiltersGeneric -lvtkFiltersGeneric -lvtkFiltersGeometryPython27D -lvtkFiltersGeometryPython27D -lvtkFiltersGeometry -lvtkFiltersGeometry -lvtkFiltersHybridPython27D -lvtkFiltersHybridPython27D -lvtkFiltersHybrid -lvtkFiltersHybrid -lvtkFiltersHyperTreePython27D -lvtkFiltersHyperTreePython27D -lvtkFiltersHyperTree -lvtkFiltersHyperTree -lvtkFiltersImagingPython27D -lvtkFiltersImagingPython27D -lvtkFiltersImaging -lvtkFiltersImaging -lvtkFiltersModelingPython27D -lvtkFiltersModelingPython27D -lvtkFiltersModeling -lvtkFiltersModeling -lvtkFiltersParallelImagingPython27D -lvtkFiltersParallelImagingPython27D -lvtkFiltersParallelImaging -lvtkFiltersParallelImaging -lvtkFiltersParallelPython27D -lvtkFiltersParallelPython27D -lvtkFiltersParallel -lvtkFiltersParallel -lvtkFiltersProgrammablePython27D -lvtkFiltersProgrammablePython27D -lvtkFiltersProgrammable -lvtkFiltersProgrammable -lvtkFiltersSelectionPython27D -lvtkFiltersSelectionPython27D -lvtkFiltersSelection -lvtkFiltersSelection -lvtkFiltersSMPPython27D -lvtkFiltersSMPPython27D -lvtkFiltersSourcesPython27D -lvtkFiltersSourcesPython27D -lvtkFiltersSources -lvtkFiltersSources -lvtkFiltersStatisticsPython27D -lvtkFiltersStatisticsPython27D -lvtkFiltersStatistics -lvtkFiltersStatistics -lvtkFiltersTexturePython27D -lvtkFiltersTexturePython27D -lvtkFiltersTexture -lvtkFiltersTexture -lvtkFiltersVerdictPython27D -lvtkFiltersVerdictPython27D -lvtkFiltersVerdict -lvtkFiltersVerdict -lvtkftgl -lvtkftgl -lvtkGUISupportQtOpenGL -lvtkGUISupportQtOpenGL -lvtkGUISupportQt -lvtkGUISupportQt -lvtkGUISupportQtSQL -lvtkGUISupportQtSQL -lvtkGUISupportQtWebkit -lvtkGUISupportQtWebkit -lvtkImagingColorPython27D -lvtkImagingColorPython27D -lvtkImagingColor -lvtkImagingColor -lvtkImagingCorePython27D -lvtkImagingCorePython27D -lvtkImagingCore -lvtkImagingCore -lvtkImagingFourierPython27D -lvtkImagingFourierPython27D -lvtkImagingFourier -lvtkImagingFourier -lvtkImagingGeneralPython27D -lvtkImagingGeneralPython27D -lvtkImagingGeneral -lvtkImagingGeneral -lvtkImagingHybridPython27D -lvtkImagingHybridPython27D -lvtkImagingHybrid -lvtkImagingHybrid -lvtkImagingMathPython27D -lvtkImagingMathPython27D -lvtkImagingMath -lvtkImagingMath -lvtkImagingMorphologicalPython27D -lvtkImagingMorphologicalPython27D -lvtkImagingMorphological -lvtkImagingMorphological -lvtkImagingSourcesPython27D -lvtkImagingSourcesPython27D -lvtkImagingSources -lvtkImagingSources -lvtkImagingStatisticsPython27D -lvtkImagingStatisticsPython27D -lvtkImagingStatistics -lvtkImagingStatistics -lvtkImagingStencilPython27D -lvtkImagingStencilPython27D -lvtkImagingStencil -lvtkImagingStencil -lvtkInfovisCorePython27D -lvtkInfovisCorePython27D -lvtkInfovisCore -lvtkInfovisCore -lvtkInfovisLayoutPython27D -lvtkInfovisLayoutPython27D -lvtkInfovisLayout -lvtkInfovisLayout -lvtkInteractionStylePython27D -lvtkInteractionStylePython27D -lvtkInteractionStyle -lvtkInteractionStyle -lvtkInteractionWidgetsPython27D -lvtkInteractionWidgetsPython27D -lvtkInteractionWidgets -lvtkInteractionWidgets -lvtkIOAMRPython27D -lvtkIOAMRPython27D -lvtkIOAMR -lvtkIOAMR -lvtkIOCorePython27D -lvtkIOCorePython27D -lvtkIOCore -lvtkIOCore -lvtkIOEnSightPython27D -lvtkIOEnSightPython27D -lvtkIOEnSight -lvtkIOEnSight -lvtkIOExodusPython27D -lvtkIOExodusPython27D -lvtkIOExodus -lvtkIOExodus -lvtkIOExportPython27D -lvtkIOExportPython27D -lvtkIOExport -lvtkIOExport -lvtkIOGeometryPython27D -lvtkIOGeometryPython27D -lvtkIOGeometry -lvtkIOGeometry -lvtkIOImagePython27D -lvtkIOImagePython27D -lvtkIOImage -lvtkIOImage -lvtkIOImportPython27D -lvtkIOImportPython27D -lvtkIOImport -lvtkIOImport -lvtkIOInfovisPython27D -lvtkIOInfovisPython27D -lvtkIOInfovis -lvtkIOInfovis -lvtkIOLegacyPython27D -lvtkIOLegacyPython27D -lvtkIOLegacy -lvtkIOLegacy -lvtkIOLSDynaPython27D -lvtkIOLSDynaPython27D -lvtkIOLSDyna -lvtkIOLSDyna -lvtkIOMINCPython27D -lvtkIOMINCPython27D -lvtkIOMINC -lvtkIOMINC -lvtkIOMoviePython27D -lvtkIOMoviePython27D -lvtkIOMovie -lvtkIOMovie -lvtkIONetCDFPython27D -lvtkIONetCDFPython27D -lvtkIONetCDF -lvtkIONetCDF -lvtkIOParallelPython27D -lvtkIOParallelPython27D -lvtkIOParallel -lvtkIOParallel -lvtkIOPLYPython27D -lvtkIOPLYPython27D -lvtkIOPLY -lvtkIOPLY -lvtkIOSQLPython27D -lvtkIOSQLPython27D -lvtkIOSQL -lvtkIOSQL -lvtkIOVideoPython27D -lvtkIOVideoPython27D -lvtkIOVideo -lvtkIOVideo -lvtkIOXMLParserPython27D -lvtkIOXMLParserPython27D -lvtkIOXMLParser -lvtkIOXMLParser -lvtkIOXMLPython27D -lvtkIOXMLPython27D -lvtkIOXML -lvtkIOXML -lvtkmetaio -lvtkmetaio -lvtkParallelCorePython27D -lvtkParallelCorePython27D -lvtkParallelCore -lvtkParallelCore -lvtkRenderingAnnotationPython27D -lvtkRenderingAnnotationPython27D -lvtkRenderingAnnotation -lvtkRenderingAnnotation -lvtkRenderingContext2DPython27D -lvtkRenderingContext2DPython27D -lvtkRenderingContext2D -lvtkRenderingContext2D -lvtkRenderingCorePython27D -lvtkRenderingCorePython27D -lvtkRenderingCore -lvtkRenderingCore -lvtkRenderingFreeTypePython27D -lvtkRenderingFreeTypePython27D -lvtkRenderingFreeType -lvtkRenderingFreeType -lvtkRenderingGL2PSPython27D -lvtkRenderingGL2PSPython27D -lvtkRenderingGL2PS -lvtkRenderingGL2PS -lvtkRenderingLabelPython27D -lvtkRenderingLabelPython27D -lvtkRenderingLabel -lvtkRenderingLabel -lvtkRenderingOpenGLPython27D -lvtkRenderingOpenGLPython27D -lvtkRenderingOpenGL -lvtkRenderingOpenGL -lvtkRenderingQtPython27D -lvtkRenderingQtPython27D -lvtkRenderingQt -lvtkRenderingQt -lvtkRenderingVolumePython27D -lvtkRenderingVolumePython27D -lvtkRenderingVolume -lvtkRenderingVolume -lvtksqlite -lvtksqlite -lvtksys -lvtksys -lvtkverdict -lvtkverdict -lvtkViewsCorePython27D -lvtkViewsCorePython27D -lvtkViewsCore -lvtkViewsCore -lvtkViewsInfovisPython27D -lvtkViewsInfovisPython27D -lvtkViewsInfovis -lvtkViewsInfovis -lvtkViewsQt -lvtkViewsQt -lvtkvtkFiltersSMP -lvtkvtkFiltersSMP -lvtkWrappingPython27Core -lvtkWrappingPython27Core -lvtkWrappingTools
# Adding all these resolved a bunch of unresolved links:
# -lvtkalglib -lvtkalglib -lvtkChartsCorePython27D -lvtkChartsCorePython27D -lvtkChartsCore -lvtkChartsCore -lvtkCommonColorPython27D -lvtkCommonColorPython27D -lvtkCommonColor -lvtkCommonColor -lvtkCommonComputationalGeometryPython27D -lvtkCommonComputationalGeometryPython27D -lvtkCommonComputationalGeometry -lvtkCommonComputationalGeometry -lvtkCommonCorePython27D -lvtkCommonCorePython27D -lvtkCommonCore -lvtkCommonCore -lvtkCommonDataModelPython27D -lvtkCommonDataModelPython27D -lvtkCommonDataModel -lvtkCommonDataModel -lvtkCommonExecutionModelPython27D -lvtkCommonExecutionModelPython27D -lvtkCommonExecutionModel -lvtkCommonExecutionModel -lvtkCommonMathPython27D -lvtkCommonMathPython27D -lvtkCommonMath -lvtkCommonMath -lvtkCommonMiscPython27D -lvtkCommonMiscPython27D -lvtkCommonMisc -lvtkCommonMisc -lvtkCommonSystemPython27D -lvtkCommonSystemPython27D -lvtkCommonSystem -lvtkCommonSystem -lvtkCommonTransformsPython27D -lvtkCommonTransformsPython27D -lvtkCommonTransforms -lvtkCommonTransforms -lvtkDICOMParser -lvtkDICOMParser -lvtkDomainsChemistryPython27D -lvtkDomainsChemistryPython27D -lvtkDomainsChemistry -lvtkDomainsChemistry -lvtkexoIIc -lvtkexoIIc -lvtkFiltersAMRPython27D -lvtkFiltersAMRPython27D -lvtkFiltersAMR -lvtkFiltersAMR -lvtkFiltersCorePython27D -lvtkFiltersCorePython27D -lvtkFiltersCore -lvtkFiltersCore -lvtkFiltersExtractionPython27D -lvtkFiltersExtractionPython27D -lvtkFiltersExtraction -lvtkFiltersExtraction -lvtkFiltersFlowPathsPython27D -lvtkFiltersFlowPathsPython27D -lvtkFiltersFlowPaths -lvtkFiltersFlowPaths -lvtkFiltersGeneralPython27D -lvtkFiltersGeneralPython27D -lvtkFiltersGeneral -lvtkFiltersGeneral -lvtkFiltersGenericPython27D -lvtkFiltersGenericPython27D -lvtkFiltersGeneric -lvtkFiltersGeneric -lvtkFiltersGeometryPython27D -lvtkFiltersGeometryPython27D -lvtkFiltersGeometry -lvtkFiltersGeometry -lvtkFiltersHybridPython27D -lvtkFiltersHybridPython27D -lvtkFiltersHybrid -lvtkFiltersHybrid -lvtkFiltersHyperTreePython27D -lvtkFiltersHyperTreePython27D -lvtkFiltersHyperTree -lvtkFiltersHyperTree -lvtkFiltersImagingPython27D -lvtkFiltersImagingPython27D -lvtkFiltersImaging -lvtkFiltersImaging -lvtkFiltersModelingPython27D -lvtkFiltersModelingPython27D -lvtkFiltersModeling -lvtkFiltersModeling -lvtkFiltersParallelImagingPython27D -lvtkFiltersParallelImagingPython27D -lvtkFiltersParallelImaging -lvtkFiltersParallelImaging -lvtkFiltersParallelPython27D -lvtkFiltersParallelPython27D -lvtkFiltersParallel -lvtkFiltersParallel -lvtkFiltersProgrammablePython27D -lvtkFiltersProgrammablePython27D -lvtkFiltersProgrammable -lvtkFiltersProgrammable -lvtkFiltersSelectionPython27D -lvtkFiltersSelectionPython27D -lvtkFiltersSelection -lvtkFiltersSelection -lvtkFiltersSMPPython27D -lvtkFiltersSMPPython27D -lvtkFiltersSourcesPython27D -lvtkFiltersSourcesPython27D -lvtkFiltersSources -lvtkFiltersSources -lvtkFiltersStatisticsPython27D -lvtkFiltersStatisticsPython27D -lvtkFiltersStatistics -lvtkFiltersStatistics -lvtkFiltersTexturePython27D -lvtkFiltersTexturePython27D -lvtkFiltersTexture -lvtkFiltersTexture -lvtkFiltersVerdictPython27D -lvtkFiltersVerdictPython27D -lvtkFiltersVerdict -lvtkFiltersVerdict -lvtkftgl -lvtkftgl -lvtkGUISupportQtOpenGL -lvtkGUISupportQtOpenGL -lvtkGUISupportQt -lvtkGUISupportQt -lvtkGUISupportQtSQL -lvtkGUISupportQtSQL -lvtkGUISupportQtWebkit -lvtkGUISupportQtWebkit -lvtkImagingColorPython27D -lvtkImagingColorPython27D -lvtkImagingColor -lvtkImagingColor -lvtkImagingCorePython27D -lvtkImagingCorePython27D -lvtkImagingCore -lvtkImagingCore -lvtkImagingFourierPython27D -lvtkImagingFourierPython27D -lvtkImagingFourier -lvtkImagingFourier -lvtkImagingGeneralPython27D -lvtkImagingGeneralPython27D -lvtkImagingGeneral -lvtkImagingGeneral -lvtkImagingHybridPython27D -lvtkImagingHybridPython27D -lvtkImagingHybrid -lvtkImagingHybrid -lvtkImagingMathPython27D -lvtkImagingMathPython27D -lvtkImagingMath -lvtkImagingMath -lvtkImagingMorphologicalPython27D -lvtkImagingMorphologicalPython27D -lvtkImagingMorphological -lvtkImagingMorphological -lvtkImagingSourcesPython27D -lvtkImagingSourcesPython27D -lvtkImagingSources -lvtkImagingSources -lvtkImagingStatisticsPython27D -lvtkImagingStatisticsPython27D -lvtkImagingStatistics -lvtkImagingStatistics -lvtkImagingStencilPython27D -lvtkImagingStencilPython27D -lvtkImagingStencil -lvtkImagingStencil -lvtkInfovisCorePython27D -lvtkInfovisCorePython27D -lvtkInfovisCore -lvtkInfovisCore -lvtkInfovisLayoutPython27D -lvtkInfovisLayoutPython27D -lvtkInfovisLayout -lvtkInfovisLayout -lvtkInteractionStylePython27D -lvtkInteractionStylePython27D -lvtkInteractionStyle -lvtkInteractionStyle -lvtkInteractionWidgetsPython27D -lvtkInteractionWidgetsPython27D -lvtkInteractionWidgets -lvtkInteractionWidgets -lvtkIOAMRPython27D -lvtkIOAMRPython27D -lvtkIOAMR -lvtkIOAMR -lvtkIOCorePython27D -lvtkIOCorePython27D -lvtkIOCore -lvtkIOCore -lvtkIOEnSightPython27D -lvtkIOEnSightPython27D -lvtkIOEnSight -lvtkIOEnSight -lvtkIOExodusPython27D -lvtkIOExodusPython27D -lvtkIOExodus -lvtkIOExodus -lvtkIOExportPython27D -lvtkIOExportPython27D -lvtkIOExport -lvtkIOExport -lvtkIOGeometryPython27D -lvtkIOGeometryPython27D -lvtkIOGeometry -lvtkIOGeometry -lvtkIOImagePython27D -lvtkIOImagePython27D -lvtkIOImage -lvtkIOImage -lvtkIOImportPython27D -lvtkIOImportPython27D -lvtkIOImport -lvtkIOImport -lvtkIOInfovisPython27D -lvtkIOInfovisPython27D -lvtkIOInfovis -lvtkIOInfovis -lvtkIOLegacyPython27D -lvtkIOLegacyPython27D -lvtkIOLegacy -lvtkIOLegacy -lvtkIOLSDynaPython27D -lvtkIOLSDynaPython27D -lvtkIOLSDyna -lvtkIOLSDyna -lvtkIOMINCPython27D -lvtkIOMINCPython27D -lvtkIOMINC -lvtkIOMINC -lvtkIOMoviePython27D -lvtkIOMoviePython27D -lvtkIOMovie -lvtkIOMovie -lvtkIONetCDFPython27D -lvtkIONetCDFPython27D -lvtkIONetCDF -lvtkIONetCDF -lvtkIOParallelPython27D -lvtkIOParallelPython27D -lvtkIOParallel -lvtkIOParallel -lvtkIOPLYPython27D -lvtkIOPLYPython27D -lvtkIOPLY -lvtkIOPLY -lvtkIOSQLPython27D -lvtkIOSQLPython27D -lvtkIOSQL -lvtkIOSQL -lvtkIOVideoPython27D -lvtkIOVideoPython27D -lvtkIOVideo -lvtkIOVideo -lvtkIOXMLParserPython27D -lvtkIOXMLParserPython27D -lvtkIOXMLParser -lvtkIOXMLParser -lvtkIOXMLPython27D -lvtkIOXMLPython27D -lvtkIOXML -lvtkIOXML -lvtkmetaio -lvtkmetaio -lvtkParallelCorePython27D -lvtkParallelCorePython27D -lvtkParallelCore -lvtkParallelCore -lvtkRenderingAnnotationPython27D -lvtkRenderingAnnotationPython27D -lvtkRenderingAnnotation -lvtkRenderingAnnotation -lvtkRenderingContext2DPython27D -lvtkRenderingContext2DPython27D -lvtkRenderingContext2D -lvtkRenderingContext2D -lvtkRenderingCorePython27D -lvtkRenderingCorePython27D -lvtkRenderingCore -lvtkRenderingCore -lvtkRenderingFreeTypePython27D -lvtkRenderingFreeTypePython27D -lvtkRenderingFreeType -lvtkRenderingFreeType -lvtkRenderingGL2PSPython27D -lvtkRenderingGL2PSPython27D -lvtkRenderingGL2PS -lvtkRenderingGL2PS -lvtkRenderingLabelPython27D -lvtkRenderingLabelPython27D -lvtkRenderingLabel -lvtkRenderingLabel -lvtkRenderingOpenGLPython27D -lvtkRenderingOpenGLPython27D -lvtkRenderingOpenGL -lvtkRenderingOpenGL -lvtkRenderingQtPython27D -lvtkRenderingQtPython27D -lvtkRenderingQt -lvtkRenderingQt -lvtkRenderingVolumePython27D -lvtkRenderingVolumePython27D -lvtkRenderingVolume -lvtkRenderingVolume -lvtksqlite -lvtksqlite -lvtksys -lvtksys -lvtkverdict -lvtkverdict -lvtkViewsCorePython27D -lvtkViewsCorePython27D -lvtkViewsCore -lvtkViewsCore -lvtkViewsInfovisPython27D -lvtkViewsInfovisPython27D -lvtkViewsInfovis -lvtkViewsInfovis -lvtkViewsQt -lvtkViewsQt -lvtkvtkFiltersSMP -lvtkvtkFiltersSMP -lvtkWrappingPython27Core -lvtkWrappingPython27Core -lvtkWrappingTools

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
