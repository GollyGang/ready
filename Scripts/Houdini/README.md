Rdy Houdini
===============================================

==About==

'Rdy Houdini' is a set of Houdini Digital Assets (HDAs) that import Ready VTI files into Houdini, as a 2D Volume/voxel-based Dop object and Gas OpenCL-based Solver. It mostly imports VTI's that use Formula mode, and a couple that use Full Kernel mode have worked as well (so far, more supported RD types TBA).

The hdanc files will ship with Ready, and they rely on new revisions to the 'rdy' command binary, which will soon be available in an official Ready release.


==Pre-Release==

Before the new rdy revisions are released in a Ready release you can still clone the github repo and build rdy yourself, and this should hopefully be no way near as hard (or have so many dependencies) as building the old readyHoudini plugin used to be, thank goodness! After that there is a 'Ready Install Path' parameter on the 'RD Object' node in Houdini that you can point to the root of the Ready/rdy build to find the rdy binary.


==Operation==

These HDAs call the rdy command (via the python subprocess module) to extract the properties of a ready simulation-scene (VTI) file, and then use the output of the command to import the simulation as a native Houdini simulation setup that does the RD stages using Gas OpenCL Dop nodes. Once imported, the scene should still function whether the VTI is present or not (ie it should be directly shareable with others, as long as the HDAs are embedded (see the Config Tab on Asset Manager in Houdini for Embedding options)). 

The data is presently stored in the scene as spare-parameter values on a Null inside the 'RD Object' that are extracted from the rdy command output and cached in the UserDataDict of a Null node inside. 

A near-term plan is to soon swap to using purely the node.UserDataDict on the 'RD Object' node, and not any parameters. We're hoping that this should improve some UI slowdowns, shorten load-times and improve initialisation performance. 

The Houdini parameter UI will likely be very sluggish if you try to view the initial-state parms, as they have *very* long string values. This is particularly an issue for large simulations (1024+ square voxels or more), do don't try to view the parameter interface of the Null rd_object/GET_vti_info unless you just want to see how slow it is.

Only 2D simulations are supported so far, but it won't be hard to extend it to 3D eventually, and potentially the runs-on-a-polymesh type of Ready-scene too (and we can still stay using OpenCL). Please contact Dan Wills (email below) to co-ordinate if you want to contribute to any of these efforts.

Because the data from the rdy command called in this setup purposefully includes the reagent initial-state image data - and it's going via a text format - the initial import: On VTI-file change or by pressing the 'Force Reload' button, can take quite a long time. This is particularly true for large resolution simulations (such as 1024x1024 voxels or more). This fairly-inefficient storage makes hipfiles (and hdanc's) containing 'RD Object' larger than they really need to be.

Ready setups will load much quicker if you first save them with small, (for example 256x256-voxel) initial states. You can still run the sims at a higher resolution once they are imported to Houdini. 

Some settings are provided (on the 'RD Object' node) that you can use to perturb the initial states with Improved Perlin Noise. If you are increasing the resolution, the initial state will tile out (be repeating). The noise presently only affects the first 2 reagents, but since this is Houdini, the setup can easily be extended to initialise any number of reagents in any way that you can think up.

Extra fields can also be applied at each substep of the simulation if desired. For example you could 'collide' the (2D) sim with another Houdini object by building a 2D collision mask volume from it that has the same dimensions and resolution as the sim, then importing the field to Dops and tuning-in the application of it to the reagents. There are many interesting possibilities here!


==Getting Started==

To get started with Rdy Houdini, first make sure that you have a recent (at least 17.0) version of Houdini installed: The free/learning (Apprentice), Indie or Education editions (which you can get from www.sidefx.com) are a perfect host for the present state of these tools. Indeed the supplied digital assets are all hdanc format (Houdini Digital Asset Non-Commercial), so if you import them in a commercial Houdini session it will become non-commercial. 

If you want to use these tools on a commercial project please contact Dan Wills (gdanzo@gmail.com). 

Once Houdini is running (or perhaps do this before you run it, read ahead..), get the HDAs all installed to your Houdini session: This can be done by copying the .hdanc files from the Ready location ($READY_ROOT/scripts/Houdini/otls/*.hdanc) to your user's home directory 'otls' location such as $HOME/houdini17.0/otls, and restarting Houdini. You may be able to use the 'Refresh Asset Libraries' action after copying but this will only work if you already had a $HOME/houdini17.5/otls folder when the Houdini session was started.

Alternatively, you can also install the HDAs by using 'File->Import->Houdini Digital Asset' to import the hdanc files from the Ready path directly into the current scene, one file at a time. This is not the recommended method because it will only make the HDAs available in the current hipnc file (and its ancestors) - it will not make the HDAs available to you in any new hipnc files. If you do install them in this way, you might want to think about turning on the option to embed HDAs (Asset Manager->Config) so that the scene still functions in the case that the Ready HDA path is not present for any reason in the future, or if you want to be able to send the scene to someone.


==HDA Code==

The main code that drives the Rdy Houdini HDAs is contained in the 'HDA Module' sections of their type properties. To see or modify the code, check out the Scripts tab. (On the node's right-click menu, choose 'Type Properties..' theb go to the 'Scripts' Tab). If you find that you want to change anything, you can unlock the HDA definition, and edit the code, press apply, and then save (and ideally lock) the HDA. (To allow editing of an HDA, right-click on the node, and select 'Allow Editing of Contents'). You will be prompted about unlocking the definition if you try to apply changes from the 'Edit Type Properties' window if the definition is locked. You must make sure that the actual hdanc files are not read-only in either case!


==Using the Digital Assets==

Once you have the HDAs installed into Houdini, in /obj (or any Object context) you should be able to drop a new node of type 'RD Template'. Drop one of these and press the 'Extract' button on its parameters to turn it into an editable Subnet. 

The result should contain:


* A Dopnet Object - Containing the RD simulation.

* A Geometry Object - In which the sim is merged back into Sops and made ready for display and/or rendering or whatever else you want to cook up!

* A Light - So that the Mantra render is not black.

* A Camera - Orthographic, pointing directly at, and showing the result of the whole simulation (if square, which is the default).

* A Ropnet - Containing some examples of how to use the above nodes to produce geometry or image file output.

====Loading VTI Files====

If you want to load new VTI files you will find a 'VTI File' parameter on the 'RD Object' node inside dop subnet: 'dopnet_rd' in the template.

If you are having troubles loading new VTIs you should check that the path to the 'rdy' binary is correctly specified on the 'Ready Install Path' parameter on the rd_object Dop node.

To load a new VTI file, press the 'File Chooser' button to the right of the VTI File parameter on the 'RD Object' node, and select the VTI file that you wish to load. The import should happen (via callback) when the value of this parameter changes. Make sure that you are on the first frame of the sim when you do this!

Not all types of VTI will work yet, only ones that define 2D-voxel simulations that use 'Formula' mode, and a few (definitely not all!) that use 'Full Kernel' mode. We are planning to eventually support more Ready VTI types. It would also be nice to be able to write back to VTI from Houdini too, but currently it is exclusively a VTI importer, not an exporter.


====Load VTIs When on First Frame Of Sim====

At the moment it is important to load new VTIs when you are on the first frame of the simulation (in other words after rewinding!). If you forget to do that and things are not working, you may be able to first rewind and then use the 'Force Reload' button to hopefully help get things working again. There are still probably buggy edge-cases so no guarantees about what it will load or do.


==Node Types==

The node-types provided by the HDAs are:

=== RD Template Object===

The 'RD Template' Object can be dropped and the 'Unpack' button pressed to turn it into a regular unlocked subnet.


=== RD Object Dop ===

Defined by Dop_rd_object.hdanc

This object manages the importing of VTI file data and the creation of the initial Dop Object (that has type 'rd_object'). The rdy command is called when a new VTI is picked and its info is imported, but the output of it is cached otherwise (unless forced to reload). Several convenience functions are built in here as well, such as the ability to re-aquire the min/max range-fitting values from the reagent fields on a linked 'RD Post Process' Sop downstream (this can also be done on the post-process by pressing the 'Get Maxes and Mins' button.)


=== RD Solver Dop ===

Defined by Dop_rd_object.hdanc

This Dop node implements the retrieval of the initial-state on the first frame of the sim, as well as switching between formula mode and full-kernel mode depending on the loaded VTI.


=== RD Post Process Sop ===

Defined by Sop_rd_object_post_process.hdanc

This node is where the reagent values from the simulation can be remapped (to a 'normalised' 0-1 range) and colored (Prim Cd). 

It also makes a second version of the fields with increased resolution (by scale, default 4x using any of the Volume Resample filters) which you can get from the second output of the RD Post Process Sop.


== Help! ==

If you are motivated but got stuck somewhere, totally send me an email!: gdanzo@gmail.com
