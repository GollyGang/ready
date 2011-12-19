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
#include "OpenCL3D_2Chemicals.hpp"
#include "utils.hpp"

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkImageData.h>

OpenCL3D_2Chemicals::OpenCL3D_2Chemicals()
{
    this->timestep = 1.0f;
    this->program_string = "__kernel void rd_compute(\n\
    __global float2 *input,__global float2 *output)\n\
{\n\
    const float D_u = 0.082f;\n\
    const float D_v = 0.041f;\n\
\n\
    const float k = 0.064f;\n\
    const float F = 0.035f;\n\
\n\
    const float delta_t = 1.0f;\n\
\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int Z = get_global_size(2);\n\
    const int i = X*(Y*z + y) + x;\n\
\n\
    const float2 uv = input[i];\n\
    const float u = uv.x;\n\
    const float v = uv.y;\n\
\n\
    // compute the Laplacians of u and v (assuming X and Y are powers of 2)\n\
    const int xm1 = ((x-1+X) & (X-1));\n\
    const int xp1 = ((x+1) & (X-1));\n\
    const int ym1 = ((y-1+Y) & (Y-1));\n\
    const int yp1 = ((y+1) & (Y-1));\n\
    const int zm1 = ((z-1+Z) & (Z-1));\n\
    const int zp1 = ((z+1) & (Z-1));\n\
    const int iLeft =  X*(Y*z + y) + xm1;\n\
    const int iRight = X*(Y*z + y) + xp1;\n\
    const int iUp =    X*(Y*z + ym1) + x;\n\
    const int iDown =  X*(Y*z + yp1) + x;\n\
    const int iFore =  X*(Y*zm1 + y) + x;\n\
    const int iBack =  X*(Y*zp1 + y) + x;\n\
\n\
    // Standard 7-point stencil\n\
    const float2 nabla_uv = input[iLeft] + input[iRight] + input[iUp] + input[iDown] + input[iFore] + input[iBack] - 6.0f*uv;\n\
    const float nabla_u = nabla_uv.x;\n\
    const float nabla_v = nabla_uv.y;\n\
\n\
    // compute the new rate of change\n\
    const float delta_u = D_u * nabla_u - u*v*v + F*(1.0f-u);\n\
    const float delta_v = D_v * nabla_v + u*v*v - (F+k)*v;\n\
\n\
    // apply the change (to the new buffer)\n\
    output[i] = uv + delta_t * (float2)(delta_u,delta_v);\n\
}";
    // TODO: parameterize the kernel code
}

void OpenCL3D_2Chemicals::Allocate(int x,int y,int z)
{
    if(x&(x-1) || y&(y-1) || z&(z-1))
        throw runtime_error("OpenCL3D_2Chemicals::Allocate : for wrap-around in OpenCL we require both dimensions to be powers of 2");
    this->AllocateBuffers(x,y,z,2);
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

void OpenCL3D_2Chemicals::InitWithBlobInCenter()
{
    vtkImageData *old_image = this->GetOldImage();
    vtkImageData *new_image = this->GetNewImage();
    assert(old_image);
    assert(new_image);

    const int X = old_image->GetDimensions()[0];
    const int Y = old_image->GetDimensions()[1];
    const int Z = old_image->GetDimensions()[2];
    const int NC = old_image->GetNumberOfScalarComponents();

    float* old_data = static_cast<float*>(old_image->GetScalarPointer());
    float* new_data = static_cast<float*>(new_image->GetScalarPointer());

    for(int x=0;x<X;x++)
    {
        for(int y=0;y<Y;y++)
        {
            for(int z=0;z<Z;z++)
            {
                if(hypot3(x-X/2,(y-Y/2)/1.5,z-Z/2)<=frand(2.0f,5.0f)) // start with a uniform field with an approximate sphere in the middle
                {
                    *vtk_at(new_data,x,y,z,0,X,Y,NC) = 0.0f;
                    *vtk_at(new_data,x,y,z,1,X,Y,NC) = 1.0f;
                    *vtk_at(old_data,x,y,z,0,X,Y,NC) = 0.0f;
                    *vtk_at(old_data,x,y,z,1,X,Y,NC) = 1.0f;
                }
                else 
                {
                    *vtk_at(new_data,x,y,z,0,X,Y,NC) = 1.0f;
                    *vtk_at(new_data,x,y,z,1,X,Y,NC) = 0.0f;
                    *vtk_at(old_data,x,y,z,0,X,Y,NC) = 1.0f;
                    *vtk_at(old_data,x,y,z,1,X,Y,NC) = 0.0f;
                }
            }
        }
    }
    this->GetNewImage()->Modified();
    this->WriteToOpenCLBuffers(); // old_data -> buffer1 
    this->timesteps_taken = 0;
}

void OpenCL3D_2Chemicals::Update(int n_steps)
{
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();

    // take approximately n_steps steps
    for(int it=0;it<(n_steps+1)/2;it++)
    {
        this->Update2Steps(); // take data from buffer1, leaves output in buffer1
        this->timesteps_taken += 2;
    }

    this->ReadFromOpenCLBuffers(); // buffer1 -> new_data = buffers[iCurrentBuffer]
    this->GetNewImage()->Modified();
    // N.B. we're not using BaseRD's render-buffer switching since we already have two OpenCL buffers
}
