# Rdy Houdini

## About

'Rdy Houdini' is a set of Houdini Digital Assets ('.hdanc' files, the non-commercial variant) that import some Ready VTI files into Houdini as a 2D Volume/voxel-based Dop object, and into the kernel source of Gas OpenCL microsolver Dops. It mostly imports VTI's that use Formula mode, and a couple that use Full Kernel mode have worked as well (so far, more supported Ready formula types TBA!).

This is a first demonstration of a prototype of an idea and should be regarded as unstable alpha/beta level tools for the moment. There are quite a few difficulties one can run into, and it is slow to initialise at present. All of this will be resolved eventually. Part of the reason for sharing it so early in its existence is to show that the idea is in-theory possible, provide a scaffolding to do future work within and hopefully make it easier/possible for others to play with it and contribute. If pattern import does not work for you, make sure to check out the instructions about 'Use Text File' below.

The hdanc files will ship with Ready 0.9, and to import new patterns they rely on some new revisions to the 'rdy' command binary which are also shipping in 0.9. These are beta tools, and it is expected that there will be some teething issues with them working flawlessly on all platforms. The platform that has been tested the most is Linux. The version of Houdini they were made and tested with is 17.5 and they are untested in 18.0 as of yet.

### Pre-Release

Before things are released you can still clone the github repo and try to build rdy yourself. This should hopefully be no way near as hard (or have so many dependencies) as building the old readyHoudini plugin used to be. After that there is a 'Ready Install Path' parameter on the 'RD Object' node in Houdini that you can point to the root of the Ready/rdy build to find the rdy binary.

## Operation

These HDAs call the rdy command (via the python subprocess module) to extract the properties of a ready simulation-scene (VTI) file. They then use the output of the command to import the simulation as a native Houdini setup that does the RD stages using Gas OpenCL Dop nodes. After being imported sucessfully, the VTI setup should still function whether the VTI is present or not (ie it should be directly shareable with others, as long as the HDAs are embedded (see the Config Tab on Asset Manager in Houdini for embedding options)).

There is a 'Force Reload' button that can help with bringing states in, an 'Update Downstream Solvers' button that does as its name suggests (it usually doesn't need to be pressed after reload as it is done as part of reload as long as 'Update Downstream on Load' is ticked). A 'Get Maxs and Mins' button that will re-compute the current reagent max and min values for better viewing when looking through the 'RD Post Process' Object, which is the recommended way to view and adjust the simulation unless debugging.

### Use Text File

A workaround is provided for issues regarding the calling of the rdy command in a subshell under Houdini. In some environments the call might crash because the rdy binary can't find the VTK shared objects or any other linkage issue might break it. In this case a manual-workaround is available (assuming the rdy command is working on your system(s) elsewhere):

To convert a VTI to text file, first you need the command to convert it. To get the command: point the 'vti_file' parameter of an 'RD Object' Dop at a VTI file by pasting in a path or pressing 'Pick VTI'. Swap to the 'Info' tab and copy the command that should be in the 'Rdy Command' parameter as the result of a python expression. The value should look something like this:

/home/dan/dev/ready/readyGit/readyAgain/ready/rdy --vti-in "/home/dan/bin/ready-0.6/Patterns/grayscott-withWave/grayscott-historyWave_fuseWorms_halfK-started.vti" -r -k -p -u -f -n 0

You can adjust this command to point to other vti files if desired and needn't necessarily copy it from the parameter, this is just a good place to get it from in the setup.

Running the command in a terminal on your system should produce a large lump of text (especially if initial states are enabled '-m', which is enabled by default). To use the 'Use Text File' feature, you need to get that text into a file, and point the 'Text File' parameter at it. On Linux (in a shell) one would usually just redirect the output of the command to a file using the '>' pipe-to-file redirection character. For instance to put the output of the above into a file in your home directory called vtiExample01.txt, you would append " > ~/vtiExample.txt" to the end of the command. If all else fails I guess you could try copy-paste too but you might need a long scrollback history and lots of scrolling to get that done with the initial states in there!

In at least one case where it was being temperamental about loading the initial states, unlocking the rd_object HDA helped. Still working out the details here. Pressing 'Force Reload' seems to be frequently necessary, and sometimes the 'Clear Userdata' button too. You also might presently need to specify the VTI file at the same time as the text file for it to work. Again, it's beta and eventually working purely from different text files will become possible.

### Storage of Parameter Data

The data is presently stored in the scene as spare-parameter values on a Null inside the 'RD Object' that are extracted from the rdy command output and cached in the UserDataDict of a Null node inside. This is not a high-performance solution but only a working prototype, so some patience is required to use this version. This will be improved in future versions.

A near-term plan is to soon swap to using purely the node.UserDataDict on the 'RD Object' node, and not parameters so much, particularly for inital states. We're hoping that this should improve some of the UI slowdown, shorten load-times and improve initialisation performance. Additionally, the plan is to set the initial voxel values from python directly using hou.Volume.setAllVoxelValues( list of float ), and potentially manage some Sop-locking in that part of the setup so that next time it executes/loads it is instantaneous to read the data back.

The Houdini parameter UI can be very sluggish if you try to view the initial-state parms (on the Null rd_object/GET_vti_info down the end). That parameter can have some *very* long string values, which makes things very slow in the Parameter Pane. This is particularly an issue for large simulations (1024+ square voxels or more with many reagents). Don't try to view the parameter interface of that null unless you want to see just how slow it is!!

### Limitations

Some components of the setup make it currently only compatible with up to 5 reagents: a,b,c,d,e. This is inflexible and will eventually be remade into something more procedural, that supports any number of reagents.

Only 2D simulations are supported so far, but it shouldn't be hard to extend it to 3D eventually. Potentially the runs-on-a-polymesh type of Ready-scene may be able to be supported as well (and we can still stay using OpenCL). Please contact Dan Wills (email below) to co-ordinate if you want to contribute to any of these efforts.

Because the data from the rdy command called in this setup purposefully includes the reagent initial-state image data - and it's going via a text format - the initial import: On VTI-file change or by pressing the 'Force Reload' button, can take quite a long time. This is particularly true for large resolution simulations (such as 1024x1024 voxels or more). This inefficient storage can also make hipfiles (and hdanc's) containing 'RD Object' larger than they really need to be.

Ready setups will load much quicker if you first save them with small, (for example 256x256-voxel) initial states. You can still run the sims at a higher resolution once they are imported to Houdini by using the resolution override parameters. Settings are also provided (on the 'RD Object' node) that you can use to perturb the initial states with Noise. If you are increasing the resolution, the initial state will tile out (ie it will repeat) but can be made nonperiodic by adding some noise. The noise presently only affects the first 2 reagents, but since this is Houdini, the setup could easily be extended to initialise any number of reagents in any way that you please.

Extra fields can also be applied at each substep of the simulation if desired. For example you could 'collide' the (2D) sim with another Houdini object by building a 2D collision mask volume for it that has the same dimensions and resolution as the sim, then importing the field to Dops and tuning-in the application of it to the reagents. There are many interesting possibilities here!

## Getting Started

Short version: 

Get the hdanc files installed into your houdini session, Once the HDAs are installed, create an 'RD Template' Object. Press the 'Extract RD Template' button, and an example setup for the rdyHoudini nodes can be found inside.

Longer version:

To get started with Rdy Houdini, first make sure that you have a recent (at least 17.0, not yet tested in 18.0!) version of Houdini installed: The free/learning (Apprentice), Indie or Education editions (which you can get from www.sidefx.com) are a perfect host for the present state of these tools. Indeed the supplied digital assets are all hdanc format (Houdini Digital Asset Non-Commercial), so if you import them in a commercial Houdini session it will become non-commercial. These tools cannot currently be used for commercial work. 

If you want to use these tools on a commercial project please contact Dan Wills (gdanzo@gmail.com) and we can hopefully work something out.

Once Houdini is running (or perhaps do this before you run it, read ahead..), get all of the the HDAs installed to your Houdini session: This can be done by copying the .hdanc files from the Ready location ($READY_ROOT/scripts/Houdini/otls/*.hdanc) to your user's home directory 'otls' location such as $HOME/houdini17.5/otls, and restarting Houdini. You may be able to do the copy and then use the 'Refresh Asset Libraries' action in an already-running Houdini session but this will only work if you already had a $HOME/houdini17.5/otls folder when the session was started (one reason to check it prior to launching if you don't know).

Alternatively, you can also install the HDAs by using 'File->Import->Houdini Digital Asset' to import the hdanc files from the Ready path directly into the current scene, one file at a time. This is not the recommended method because it's annoying to do them one-at-a-time and it will only make the HDAs available in the current hipnc file and its ancestors. This --will not-- make the HDAs available to you in any new hipnc files. If you do install the HDAs 'manually' in this way, you might want to think about turning on the option to embed HDAs (Asset Manager->Configuration) so that the scene still functions in the case that the Ready HDA path is not present in the future, or if you want to be able to send the scene to someone.

Once the HDAs are installed, first make sure you navigate to /obj or another Object context in the Network Editor, then create an instance of the 'RD Template Object' asset by selecting it from the tab menu. Once the node has been created, press the button on its parameter pane: 'Extract RD Template'. The result will contain an editable rdyRD example setup.

## HDA Code

The code that drives the Rdy Houdini HDAs is contained in the 'HDA Module' sections of the nodes' type properties. To see or modify the code, check out the Scripts tab. (On the node right-click menu, choose 'Type Properties..' then go to the 'Scripts' Tab). If you find that you want to change anything, you can unlock the HDA definition and edit the code, press apply, and then once you're finished save (and ideally lock: "Match Current Definition") the HDA. Note that this type of editing will work a lot better if you copied the HDAs (and made them editable) when installing them (see previous section).

To allow editing of an HDA, right-click on the node, and select 'Allow Editing of Contents'. Alternatively you will also be prompted about unlocking the definition if you try to apply changes from the 'Edit Type Properties' window if the definition is currently locked. In either case you must make sure that the hdanc files are not read-only (ie writable by your user) to be actually able to save changes.

## Using the Digital Assets

Once you have the HDAs installed into Houdini, in /obj (or in an Object context) drop a new node of type 'RD Template'. Once one is dropped, press the 'Extract RD Template' button on its parameter pane to extract its contents and turn it into an editable Subnet, so that the settings can be adjusted.

The result will contain the following *Object* nodes:

- A Dopnet called *dopnet_rd* - This contains the RD simulation.
- A Geometry called *geo_mergeSim* - In which the sim is merged back into Sops and made ready for display and/or rendering or whatever else you want to cook up with it!
- A Matnet called *matnet_basicShader* - That has a 'Constant Smoke' Material inside that is used for Mantra rendering (17.5-era).
- A Camera called *cam_ortho_unit* - To view the sim. It is set to orthographic view and fits and points directly at the sim.
- A Ropnet called *ropnet_outputs* - Containing some examples of how to use the above nodes to produce geometry or image file outputs.

The best view of the sim is not from inside the dopnet (where it generally looks very bright, grayscale and data-y), but the 'RD Post Process' object inside 'geo_mergesim' in the template. Inside the dopnet is where the central control is though (on the rd_object node), so before diving in, you may wish to **pin** the Scene Viewer with the post-processed sim geometry visible (for example in the root of the 'rd_template' Subnet), before diving into the dopnet.

To tune the look of the resulting simulation, you can take a look at the node called 'rd_object_post_process' inside 'geo_mergeSim' in the template, or build your own visualisation/generated result based on the reagent fields.

### Loading VTI Files

If you want to load new VTI files you will find a 'VTI File' parameter on the 'RD Object' node inside Dop Subnet: 'dopnet_rd' in the template.

If you are having troubles loading new VTIs you should check that the path to the directory containing the 'rdy' binary is correctly specified on the 'Ready Install Path' parameter on the rd_object Dop node. Please note that this is beta software and might not work for everyone yet. If it still doesn't work there is a workaround where you can call the command yourself, put the result in a text file and load that instead, please see the 'Use Text File' section above.

To load a new VTI file, press the 'File Chooser' button to the right of the VTI File parameter on the rd_object node, and select the VTI file that you wish to load. The import should happen (via callback) when the value of this parameter changes. Make sure that you are on the first frame of the sim when you do this!

Not all types of VTI will work yet, only ones that define 2D-voxel simulations that use 'Formula' mode, and a few (definitely not all!) that use 'Full Kernel' mode. We are planning to eventually support more Ready VTI types. It would also be nice to be able to write back to VTI from Houdini too, but currently this is exclusively a VTI importer, not an exporter.


### Load VTIs When on First Frame Of Sim

At the moment it is important to load new VTIs when you are on the first frame of the simulation, in other words after rewinding! (Ctrl-Up is the hotkey to rewind). If you forget to do that and things are not working, you may be able to first rewind and then use the 'Force Reload' button to hopefully help get things working again. There are still probably buggy edge-cases so no guarantees about what it will load or do.


## Node Types

The node-types provided by the HDAs are:

### RD Template Object

Defined by Object_rd_template_object.hdanc

The 'RD Template' Object can be dropped in an object context such as /obj and the 'Unpack' button may be pressed to turn it into a regular unlocked subnet containing an example usage of the rd_object and rd_solver Dops and the post-processing Sop. The idea being to provide an example setup that can be worked from like a shelf-tool.


### RD Object Dop

Defined by: Dop_rd_object.hdanc

This Dop node manages the importing of VTI file data and the creation of the initial Dop Object (it has type 'rd_object'). The rdy command is called when a new VTI is picked and its info is imported, but the output of it is cached otherwise (unless forced to reload). Several convenience functions are built in here as well, such as the ability to re-aquire the min/max range-fitting values from the reagent fields on a linked 'RD Post Process' Sop downstream (this can also be done on the post-process Sop by pressing the 'Get Maxes and Mins' button.)


### RD Solver Dop

Defined by: Dop_rd_object.hdanc

This Dop node implements the retrieval of the initial-state on the first frame of the sim, as well as switching between formula mode and full-kernel mode depending on the loaded VTI.


### RD Post Process Sop

Defined by: Sop_rd_object_post_process.hdanc

This node is where the reagent values from the simulation can be remapped (to a 'normalised' 0-1 range) shaped with a power function and multiplier, and colored by adding a Primitive Cd attribute to make things look nice and/or informative. 

The post-process node also makes a second version of the fields available that is set to render (ie it has the Render flag on it by default in the template Sops). The render fields have increased resolution (by scale, default 4x using a Volume Resample filter (gaussian in the template)) and are available on the second output of the RD Post Process Sop.

### Tooltips

Help has been added to nearly all of the parameters of the provided Digital Assets, you can hover over each parameter label to see it.

## Help!

If you are motivated to play with this but got stuck somewhere, totally send me an email!: gdanzo@gmail.com
