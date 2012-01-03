/*  Copyright 2011, The Ready Bunch

    This file is part of Ready.

    Ready is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ready is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ready. If not, see <http://www.gnu.org/licenses/>.         */

// local:
#include "BaseRD.hpp"

// stdlib:
#include <stdlib.h>
#include <math.h>

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkImageData.h>

BaseRD::BaseRD()
{
    this->timesteps_taken = 0;
    this->image = NULL;
    this->need_reload_program = true;
}

BaseRD::~BaseRD()
{
    if(this->image)
        this->image->Delete();
}

int BaseRD::GetDimensionality() const
{
    assert(this->image);
    int dimensionality=0;
    for(int iDim=0;iDim<3;iDim++)
        if(this->image->GetDimensions()[iDim]>1)
            dimensionality++;
    return dimensionality;
}

int BaseRD::GetNumberOfChemicals() const
{
    assert(this->image);
    return this->image->GetNumberOfScalarComponents();
}

float BaseRD::GetTimestep() const
{ 
    return this->timestep; 
}

vtkImageData* BaseRD::GetImage() const
{ 
    return this->image; 
}

int BaseRD::GetTimestepsTaken() const
{
    return this->timesteps_taken;
}

void BaseRD::AllocateImage(int x,int y,int z,int nc)
{
    assert(!this->image);
    this->image = AllocateVTKImage(x,y,z,nc);
}

vtkImageData* BaseRD::AllocateVTKImage(int x,int y,int z,int nc)
{
    vtkImageData *im = vtkImageData::New();
    im->SetNumberOfScalarComponents(nc);
    im->SetScalarTypeToFloat();
    im->SetDimensions(x,y,z);
    im->AllocateScalars();
    if(im->GetDimensions()[0]!=x || im->GetDimensions()[1]!=y || im->GetDimensions()[2]!=z)
        throw runtime_error("BaseRD::AllocateVTKImage : Failed to allocate image data - dimensions too big?");
    return im;
}

float* BaseRD::vtk_at(float* origin,int x,int y,int z,int iC,int X,int Y,int NC)
{
    // vtkImageData scalars stored as: component1,component2,...componentN,component1,.... for consecutive x, then y, then z
    return origin + iC + (x + (y + z*Y)*X)*NC;
}

void BaseRD::SetProgram(string s)
{
    if(s != this->program_string)
        this->need_reload_program = true;
    this->program_string = s;
}

string BaseRD::GetProgram() const
{
    return this->program_string;
}
