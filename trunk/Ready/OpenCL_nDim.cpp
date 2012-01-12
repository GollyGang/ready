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
#include "OpenCL_nDim.hpp"
#include "utils.hpp"

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkImageData.h>
#include <vtkXMLDataElement.h>

OpenCL_nDim::OpenCL_nDim()
{
    this->timestep = 1.0f;
    this->pre_formula_kernel = 
"__kernel void rd_compute(__global float4 *a_in,__global float4 *b_in,__global float4 *a_out,__global float4 *b_out)\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int Z = get_global_size(2);\n\
    const int i = X*(Y*z + y) + x;\n\
\n\
    const float4 a = a_in[i];\n\
    const float4 b = b_in[i];\n\
\n\
    // compute the Laplacians of u and v\n\
    const int xm1 = ((x-1+X) & (X-1)); // wrap (assumes X is a power of 2)\n\
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
    const float4 a_left = a_in[iLeft];\n\
    const float4 a_right = a_in[iRight];\n\
    const float4 a_up = a_in[iUp];\n\
    const float4 a_down = a_in[iDown];\n\
    const float4 a_fore = a_in[iFore];\n\
    const float4 a_back = a_in[iBack];\n\
    const float4 b_left = b_in[iLeft];\n\
    const float4 b_right = b_in[iRight];\n\
    const float4 b_up = b_in[iUp];\n\
    const float4 b_down = b_in[iDown];\n\
    const float4 b_fore = b_in[iFore];\n\
    const float4 b_back = b_in[iBack];\n\
    // standard 7-point stencil:\n\
    const float4 laplacian_a = (float4)(a_up.x + a.y + a_down.x + a_left.w + a_fore.x + a_back.x,\n\
                                    a_up.y + a.z + a_down.y + a.x + a_fore.y + a_back.y,\n\
                                    a_up.z + a.w + a_down.z + a.y + a_fore.z + a_back.z,\n\
                                    a_up.w + a_right.x + a_down.w + a.z + a_fore.w + a_back.w) - 6.0f*a\n\
                                    + (float4)(1e-6f,1e-6f,1e-6f,1e-6f); // (kill denormals)\n\
    const float4 laplacian_b = (float4)(b_up.x + b.y + b_down.x + b_left.w + b_fore.x + b_back.x,\n\
                                    b_up.y + b.z + b_down.y + b.x + b_fore.y + b_back.y,\n\
                                    b_up.z + b.w + b_down.z + b.y + b_fore.z + b_back.z,\n\
                                    b_up.w + b_right.x + b_down.w + b.z + b_fore.w + b_back.w) - 6.0f*b\n\
                                    + (float4)(1e-6f,1e-6f,1e-6f,1e-6f); // (kill denormals)\n\
    \n\
    float4 delta_a,delta_b;\n\n\
    // compute the new rate of change\n";
    this->post_formula_kernel = "\n\
    // apply the change\n\
    a_out[i] = a + delta_t * delta_a;\n\
    b_out[i] = b + delta_t * delta_b;\n}";
    // provide a placeholder:
    this->program_string = this->pre_formula_kernel + "\
    float D_a = 0.082f;\n\
    float D_b = 0.041f;\n\
    float k = 0.06f;\n\
    float F = 0.035f;\n\
    delta_a = D_a * laplacian_a - a*b*b + F*(1.0f-a);\n\
    delta_b = D_b * laplacian_b + a*b*b - (F+k)*b;\n\
    float delta_t = 1.0f;\n" + this->post_formula_kernel;
}

void OpenCL_nDim::Allocate(int x,int y,int z,int nc)
{
    if(x&(x-1) || y&(y-1) || z&(z-1))
        throw runtime_error("OpenCL_nDim::Allocate : for wrap-around in OpenCL we require all the dimensions to be powers of 2");
    OpenCL_RD::Allocate(x,y,z,nc);
    this->need_reload_program = true;
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

void OpenCL_nDim::InitWithBlobInCenter()
{
    vector<float*> image_data(this->GetNumberOfChemicals());
    for(int i=0;i<this->GetNumberOfChemicals();i++)
        image_data[i] = static_cast<float*>(this->images[i]->GetScalarPointer());

    const int X = this->GetX();
    const int Y = this->GetY();
    const int Z = this->GetZ();
    for(int x=0;x<X;x++)
    {
        for(int y=0;y<Y;y++)
        {
            for(int z=0;z<Z;z++)
            {
                if(hypot3(x-X/2,(y-Y/2)/1.5,z-Z/2)<=frand(2.0f,5.0f)) // start with a uniform field with an approximate circle in the middle
                {
                    for(int i=0;i<this->GetNumberOfChemicals();i++)
                        *vtk_at(image_data[i],x,y,z,X,Y) = (float)(i%2);
                }
                else 
                {
                    for(int i=0;i<this->GetNumberOfChemicals();i++)
                        *vtk_at(image_data[i],x,y,z,X,Y) = (float)(1-(i%2));
                }
            }
        }
    }
    for(int i=0;i<this->GetNumberOfChemicals();i++)
        this->images[i]->Modified();
    this->WriteToOpenCLBuffers();
    this->timesteps_taken = 0;
}

void OpenCL_nDim::Update(int n_steps)
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
