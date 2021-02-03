#  A Blender script to import a sequence of OBJ meshes into an animation.
#
# ------------------------------------------------------------------------
#  Copyright 2011-2021 The Ready Bunch
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
# Hit 'New' and paste in the script.
# Edit the 'path' string to be the folder where your OBJ files live.
# Hit 'Run Script'
# It should load all the OBJ files it found and add keyframes for them.
# (If the script gives errors, use Window > Toggle System Console to see them.)
# Hit the 'Play animation' button in the Timeline panel and your mesh should start animating.
# Set up the camera and lighting and use Render > Render animation.

import bpy
import glob

path = 'c:/Ready_frames/*.obj'      # <------- Change this to the folder with your OBJ files

obj_files = glob.glob( path )

if len(obj_files)==0:
    print('ERROR: No files in path: ' + path)

if bpy.data.materials.get("Material") is not None:
    mat = bpy.data.materials["Material"]
else:
    mat = bpy.data.materials.new(name="Material")

# load the objects
objs = []
for iObj,fn in enumerate(obj_files):
    bpy.ops.import_scene.obj( filepath=fn )
    ob = bpy.context.selected_editable_objects[0]
    objs.append(ob)

    # assign the same material to all frames
    ob.data.materials.clear()
    ob.data.materials.append(mat)

bpy.context.scene.frame_start = 0
bpy.context.scene.frame_end = len(objs)-1

def frame_change(self):
    i_frame = int(self.frame_current)
    for i_ob, ob in enumerate(objs):
        if i_ob == i_frame:
            ob.hide_render = ob.hide_viewport = False
        else:
            ob.hide_render = ob.hide_viewport = True

bpy.app.handlers.frame_change_pre.append(frame_change)
