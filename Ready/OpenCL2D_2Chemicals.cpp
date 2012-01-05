/*  Copyright 2011, 2012 The Ready Bunch

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
#include "utils.hpp"

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkImageData.h>

OpenCL2D_2Chemicals::OpenCL2D_2Chemicals()
{
    this->timestep = 1.0f;
    this->program_string = 
"__kernel void rd_compute(__global float4 *a_in,__global float4 *b_in,__global float4 *a_out,__global float4 *b_out)\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int i = X*(Y*z + y) + x;\n\
\n\
    const float4 a = a_in[i];\n\
    const float4 b = b_in[i];\n\
\n\
    // compute the Laplacians of a and b\n\
    const int xm1 = ((x-1+X) & (X-1)); // (assuming X and Y are powers of 2)\n\
    const int xp1 = ((x+1) & (X-1));\n\
    const int ym1 = ((y-1+Y) & (Y-1));\n\
    const int yp1 = ((y+1) & (Y-1));\n\
    const int iLeft =  X*(Y*z + y) + xm1;\n\
    const int iRight = X*(Y*z + y) + xp1;\n\
    const int iUp =    X*(Y*z + ym1) + x;\n\
    const int iDown =  X*(Y*z + yp1) + x;\n\
    const float4 a_left = a_in[iLeft];\n\
    const float4 a_right = a_in[iRight];\n\
    const float4 a_up = a_in[iUp];\n\
    const float4 a_down = a_in[iDown];\n\
    const float4 b_left = b_in[iLeft];\n\
    const float4 b_right = b_in[iRight];\n\
    const float4 b_up = b_in[iUp];\n\
    const float4 b_down = b_in[iDown];\n\
    // standard 5-point stencil:\n\
    const float4 laplacian_a = (float4)(a_up.x + a.y + a_down.x + a_left.w,\n\
                                    a_up.y + a.z + a_down.y + a.x,\n\
                                    a_up.z + a.w + a_down.z + a.y,\n\
                                    a_up.w + a_right.x + a_down.w + a.z) - 4.0f*a\n\
                                    + (float4)(1e-6f,1e-6f,1e-6f,1e-6f); // (kill denormals)\n\
    const float4 laplacian_b = (float4)(b_up.x + b.y + b_down.x + b_left.w,\n\
                                    b_up.y + b.z + b_down.y + b.x,\n\
                                    b_up.z + b.w + b_down.z + b.y,\n\
                                    b_up.w + b_right.x + b_down.w + b.z) - 4.0f*b\n\
                                    + (float4)(1e-6f,1e-6f,1e-6f,1e-6f); // (kill denormals)\n\
\n\
    // --------- change the code inside this block --------------\n\
    // compute the new rate of change\n\
    const float D_a = 0.082f;\n\
    const float D_b = 0.041f;\n\
    const float k = 0.064f;\n\
    const float F = 0.035f;\n\
    const float4 delta_a = D_a * laplacian_a - a*b*b + F*(1.0f-a);\n\
    const float4 delta_b = D_b * laplacian_b + a*b*b - (F+k)*b;\n\
    const float delta_t = 1.0f;\n\
    // ----------------------------------------------------------\n\
\n\
    // apply the change\n\
    a_out[i] = a + delta_t * delta_a;\n\
    b_out[i] = b + delta_t * delta_b;\n\
}";
    // TODO: parameterize the kernel code
}

void OpenCL2D_2Chemicals::Allocate(int x,int y)
{
    if(x&(x-1) || y&(y-1))
        throw runtime_error("OpenCL2D_2Chemicals::Allocate : for wrap-around in OpenCL we require both dimensions to be powers of 2");
    this->AllocateImages(x,y,1,2);
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

void OpenCL2D_2Chemicals::InitWithBlobInCenter()
{
    vtkImageData *a_image = this->images[0];
    vtkImageData *b_image = this->images[1];
    float* a_data = static_cast<float*>(a_image->GetScalarPointer());
    float* b_data = static_cast<float*>(b_image->GetScalarPointer());

    const int X = this->GetX();
    const int Y = this->GetY();
    for(int x=0;x<X;x++)
    {
        for(int y=0;y<Y;y++)
        {
            if(hypot2(x-X/2,(y-Y/2)/1.5)<=frand(2.0f,5.0f)) // start with a uniform field with an approximate circle in the middle
            {
                *vtk_at(a_data,x,y,0,X,Y) = 0.0f;
                *vtk_at(b_data,x,y,0,X,Y) = 1.0f;
            }
            else 
            {
                *vtk_at(a_data,x,y,0,X,Y) = 1.0f;
                *vtk_at(b_data,x,y,0,X,Y) = 0.0f;
            }
        }
    }
    a_image->Modified();
    b_image->Modified();
    this->WriteToOpenCLBuffers();
    this->timesteps_taken = 0;
}

void OpenCL2D_2Chemicals::Update(int n_steps)
{
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();

    // take approximately n_steps steps
    for(int it=0;it<(n_steps+1)/2;it++)
    {
        this->Update2Steps(); // take data from buffer1, leaves output in buffer1
        this->timesteps_taken += 2;
    }

    this->ReadFromOpenCLBuffers(); // buffer1 -> image
}
