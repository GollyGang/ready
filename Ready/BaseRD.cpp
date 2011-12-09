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
    iss->SetScale(1.0);
    iss->SetShift(0.0);
    iss->SetOutputScalarTypeToFloat();
    iss->ClampOverflowOn();
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

int BaseRD::GetDimensionality() 
{
    assert(this->buffer[0]);
    int dimensionality=0;
    for(int iDim=0;iDim<3;iDim++)
        if(this->buffer[0]->GetDimensions()[iDim]>1)
            dimensionality++;
    return dimensionality;
}

int BaseRD::GetNumberOfChemicals()
{
    assert(this->buffer[0]);
    return this->buffer[0]->GetNumberOfScalarComponents();
}

float BaseRD::GetTimestep() 
{ 
    return this->timestep; 
}

vtkImageData* BaseRD::GetImageToRender() 
{ 
    assert(this->buffer_switcher);
    this->buffer_switcher->Update();
    return this->buffer_switcher->GetOutput(); 
}

vtkImageData* BaseRD::GetNewImage() 
{ 
    assert(this->buffer[this->iCurrentBuffer]);
    return this->buffer[this->iCurrentBuffer]; 
}

vtkImageData* BaseRD::GetOldImage() 
{ 
    assert(this->buffer[1-this->iCurrentBuffer]);
    return this->buffer[1-this->iCurrentBuffer]; 
}

int BaseRD::GetTimestepsTaken()
{
    return this->timesteps_taken;
}

void BaseRD::SwitchBuffers()
{
    this->buffer_switcher->SetInput(this->buffer[this->iCurrentBuffer]);
    this->iCurrentBuffer = 1-this->iCurrentBuffer;
}

// ------------- utility functions feel a bit lost here ----------------------

float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

double hypot2(double x,double y) 
{ 
    return sqrt(x*x+y*y); 
}

double hypot3(double x,double y,double z) 
{ 
    return sqrt(x*x+y*y+z*z); 
}

