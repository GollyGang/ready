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
#include "OpenCL2D_2Chemicals.hpp"

// STL:
#include <cassert>
using namespace std;

// VTK:
#include <vtkImageData.h>

OpenCL2D_2Chemicals::OpenCL2D_2Chemicals()
{
    this->timestep = 1.0f;
    this->program_string = "__kernel void rd_compute(\n\
    __global float *input,__global float *output)\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int NC = 2; // TODO: make a param\n\
    const int i = NC*(X*(Y*z + y) + x);\n\
\n\
    output[i] = input[i]/2.0f+0.1;\n\
    output[i+1] = input[i+1]/2.0f+0.1;\n\
}";
}

void OpenCL2D_2Chemicals::Allocate(int x,int y)
{
    this->AllocateBuffers(x,y,1,2);
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

void OpenCL2D_2Chemicals::InitWithBlobInCenter()
{
    vtkImageData *old_image = this->GetOldImage();
    vtkImageData *new_image = this->GetNewImage();
    assert(old_image);
    assert(new_image);

    const int X = old_image->GetDimensions()[0];
    const int Y = old_image->GetDimensions()[1];
    const int NC = old_image->GetNumberOfScalarComponents();

    float* old_data = static_cast<float*>(old_image->GetScalarPointer());
    float* new_data = static_cast<float*>(new_image->GetScalarPointer());

    for(int x=0;x<X;x++)
    {
        for(int y=0;y<Y;y++)
        {
            if(hypot2(x-X/2,(y-Y/2)/1.5)<=frand(2.0f,5.0f)) // start with a uniform field with an approximate circle in the middle
            {
                *vtk_at(new_data,x,y,0,0,X,Y,NC) = 0.0f;
                *vtk_at(new_data,x,y,0,1,X,Y,NC) = 1.0f;
            }
            else 
            {
                *vtk_at(new_data,x,y,0,0,X,Y,NC) = 1.0f;
                *vtk_at(new_data,x,y,0,1,X,Y,NC) = 0.0f;
            }
        }
    }
    this->SwitchBuffers(); // new_data -> old_data
    this->WriteToOpenCLBuffers(); // old_data -> buffer1 
}

void OpenCL2D_2Chemicals::Update(int n_steps)
{
    if(n_steps%2)
        throw runtime_error("OpenCL2D_2Chemicals::Update : n_steps must be divisible by 2");

    for(int it=0;it<n_steps/2;it++)
    {
        this->Update2Steps(); // take data from buffer1, leaves output in buffer1
    }

    this->timesteps_taken += n_steps;

    this->ReadFromOpenCLBuffers(); // buffer1 -> new_data = buffers[iCurrentBuffer]
    this->SwitchBuffers(); // buffers[iCurrentBuffer] gets connected to output for rendering
}
