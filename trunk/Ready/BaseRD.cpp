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
#include <vtkImageShiftScale.h>
#include <vtkImageAlgorithm.h>

BaseRD::BaseRD() : 
    timesteps_taken(0),
    iCurrentBuffer(0)
{
    this->buffer[0] = NULL;
    this->buffer[1] = NULL;
    vtkImageShiftScale *iss = vtkImageShiftScale::New();
    iss->SetOutputScalarTypeToFloat();
    this->buffer_switcher = iss;
}

BaseRD::~BaseRD()
{
    if(this->buffer[0])
        this->buffer[0]->Delete();
    if(this->buffer[1])
        this->buffer[1]->Delete();
    if(this->buffer_switcher)
        this->buffer_switcher->Delete();
}

int BaseRD::GetDimensionality() const
{
    assert(this->buffer[0]);
    int dimensionality=0;
    for(int iDim=0;iDim<3;iDim++)
        if(this->buffer[0]->GetDimensions()[iDim]>1)
            dimensionality++;
    return dimensionality;
}

int BaseRD::GetNumberOfChemicals() const
{
    assert(this->buffer[0]);
    return this->buffer[0]->GetNumberOfScalarComponents();
}

float BaseRD::GetTimestep() const
{ 
    return this->timestep; 
}

vtkImageData* BaseRD::GetImageToRender() const
{ 
    assert(this->buffer_switcher);
    this->buffer_switcher->Update();
    return this->buffer_switcher->GetOutput(); 
}

vtkImageData* BaseRD::GetNewImage() const
{ 
    assert(this->buffer[this->iCurrentBuffer]);
    return this->buffer[this->iCurrentBuffer]; 
}

vtkImageData* BaseRD::GetOldImage() const
{ 
    assert(this->buffer[1-this->iCurrentBuffer]);
    return this->buffer[1-this->iCurrentBuffer]; 
}

int BaseRD::GetTimestepsTaken() const
{
    return this->timesteps_taken;
}

void BaseRD::SwitchBuffers()
{
    this->buffer_switcher->SetInput(this->buffer[this->iCurrentBuffer]);
    this->iCurrentBuffer = 1-this->iCurrentBuffer;
}

void BaseRD::AllocateBuffers(int x,int y,int z,int nc)
{
    for(int iB=0;iB<2;iB++)
    {
        assert(!this->buffer[iB]);
        this->buffer[iB] = vtkImageData::New();
        this->buffer[iB]->SetNumberOfScalarComponents(nc);
        this->buffer[iB]->SetScalarTypeToFloat();
        this->buffer[iB]->SetDimensions(x,y,z);
        this->buffer[iB]->AllocateScalars();
        if(this->buffer[iB]->GetDimensions()[0]!=x || this->buffer[iB]->GetDimensions()[1]!=y || this->buffer[iB]->GetDimensions()[2]!=z)
            throw runtime_error("BaseRD::AllocateBuffers : Failed to allocate image data - dimensions too big?");
    }
    this->buffer_switcher->SetInput(this->buffer[this->iCurrentBuffer]);
}

float* BaseRD::vtk_at(float* origin,int x,int y,int z,int iC,int X,int Y,int NC)
{
    // vtkImageData scalars stored as: component1,component2,...componentN,component1,.... for consecutive x, then y, then z
    return origin + NC*(X*(Y*z + y) + x) + iC;
}
