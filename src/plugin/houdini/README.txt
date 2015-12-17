* Please see also the README.txt in the root of the Ready project.

This houdini plugin was written by Dan Wills - gdanzo@gmail.com

To build the Ready Houdini plugin you first need to get the Ready project building successfully itself. 

This currently may necessitate having a local (ie usually home-directory) build of VTK and you'll definitely need a local build of wxWidgets because a private header is used that doesn't get installed. The cmake build should point to your local vtk path and the wxrc and wx-config paths from your local wx build.

Most recent versions of these that have worked are VTK 6.3.0, and wxWidgets-3.0.2.

Once that is working:

* Source your Houdini environment, on my system these commands would achieve that:
	> cd /opt/hfs15.0.244.16/
	> source houdini_setup
* Use "ccmake ." in the project root and set the boolean in the cmake called PLUGIN_BUILD to ON.
* Configure and Generate.
* Make.
* If sucessful the .so file will be created.
* You can use 'make install' to copy it to the appropriate place in the houdini folder in your home dir.
* There's a demonstration hipfile in the folder:
     - src/houdini/plugins/hip 
  Which uses an otl from here:
     - src/houdini/plugins/otl
* The otl is there to simplify creating the right kind of volume primitives to retrieve Ready data using the "Ready RD" node.
