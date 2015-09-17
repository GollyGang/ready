#  A Blender script to import a sequence of OBJ meshes into an animation.
#
# ------------------------------------------------------------------------
#  Copyright 2015 The Ready Bunch
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
# ------------------------------------------------------------------------

# Usage:
# In Blender, change one of the panels into a Text Editor (using the icon on the left with the up/down arrow)
# Hit 'New', paste in the script, and hit 'Run Script'
# It should load all the OBJ files it found and make them visible in sequence.
# (If the script gives errors, use Window > Toggle System Console to see them)
# Hit the 'Play animation' button in the Timeline panel and your mesh should start animating.
# Set up the camera and lighting and use Render > Render animation.

# Many thanks to: https://www.blender.org/forum/viewtopic.php?t=23355

import bpy
import glob

path = 'c:/Tim/Ready_frames/*.obj'      # <------- Change this to the folder with your OBJ files

obj_files = glob.glob( path )

for iObj,fn in enumerate(obj_files):

    bpy.ops.import_scene.obj( filepath=fn )
    ob = bpy.context.selected_editable_objects[0]
   
    iKeyFrame = iObj + 1
   
    # show the object in one frame
    ob.hide_render = False
    ob.hide = False
    ob.keyframe_insert(data_path="hide_render", frame=iKeyFrame)
    ob.keyframe_insert(data_path="hide", frame=iKeyFrame)
   
    # hide the object in all the other frames
    ob.hide_render = True
    ob.hide = True
    ob.keyframe_insert(data_path="hide_render", frame=iKeyFrame-1)
    ob.keyframe_insert(data_path="hide", frame=iKeyFrame-1)
    ob.keyframe_insert(data_path="hide_render", frame=iKeyFrame+1)
    ob.keyframe_insert(data_path="hide", frame=iKeyFrame+1)
