#!/usr/bin/env python

import vtk
import os

# create a rendering window and renderer
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(300,300)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read the data
sp = vtk.vtkStructuredPointsReader()
sp.SetFileName("../../build/FitzHughNagumo3D/vol_00875.vtk")
sp.Update()
dims = sp.GetOutput().GetDimensions()

# contour
cont = vtk.vtkContourFilter()
cont.SetInput(sp.GetOutput())
cont.SetValue(0,0.5)

# make an actor
mapper = vtk.vtkPolyDataMapper()
mapper.SetInput(cont.GetOutput())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetDiffuse(0.6)
actor.GetProperty().SetAmbient(0.1)
actor.GetProperty().SetSpecular(0.5)
actor.GetProperty().SetSpecularPower(100.0)

# assign our actor to the renderer
ren.AddActor(actor)

# add a cube outline actor
cube = vtk.vtkCubeSource()
cube.SetXLength(dims[0])
cube.SetYLength(dims[1])
cube.SetZLength(dims[2])
cube.SetCenter(dims[0]/2,dims[1]/2,dims[2]/2)
cube_mapper = vtk.vtkPolyDataMapper()
cube_mapper.SetInput(cube.GetOutput())
cube_actor = vtk.vtkActor()
cube_actor.SetMapper(cube_mapper)
cube_actor.GetProperty().SetRepresentationToWireframe()
cube_actor.GetProperty().SetAmbient(1)
ren.AddActor(cube_actor)

wif = vtk.vtkWindowToImageFilter()
wif.SetInput(renWin)
png = vtk.vtkPNGWriter()
png.SetInput(wif.GetOutput())

iren.Initialize()
renWin.Render()
iren.Start()

#pov = vtk.vtkPOVExporter()
#pov.SetRenderWindow(renWin)
#pov.SetFileName("out.pov")
#pov.Write()

#exit()

# after user has positioned the scene, run through the frames
renWin.OffScreenRenderingOn()
i=0
while True:
    filename = "../../build/FitzHughNagumo3D/vol_"+'0'*(5-len(str(i)))+str(i)+".vtk"
    if not os.path.exists(filename):
        break
    sp.SetFileName(filename)
    renWin.Render()
    wif.Modified()
    png.SetFileName("out_"+str(i)+".png")
    png.Write()
    i+=1