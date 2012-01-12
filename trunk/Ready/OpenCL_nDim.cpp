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
    this->kernel_part1 = "__kernel void rd_compute(";
    this->kernel_part2 = ")\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int Z = get_global_size(2);\n\
    const int i = X*(Y*z + y) + x;\n\
\n";
    this->kernel_part3 = "\
\n\
    // compute the Laplacians of each chemical\n\
    const int xm1 = ((x-1+X) & (X-1)); // wrap (assumes X is a power of 2)\n\
    const int xp1 = ((x+1) & (X-1));\n\
    const int ym1 = ((y-1+Y) & (Y-1));\n\
    const int yp1 = ((y+1) & (Y-1));\n\
    const int zm1 = ((z-1+Z) & (Z-1));\n\
    const int zp1 = ((z+1) & (Z-1));\n\
    const int i_left =  X*(Y*z + y) + xm1;\n\
    const int i_right = X*(Y*z + y) + xp1;\n\
    const int i_up =    X*(Y*z + ym1) + x;\n\
    const int i_down =  X*(Y*z + yp1) + x;\n\
    const int i_fore =  X*(Y*zm1 + y) + x;\n\
    const int i_back =  X*(Y*zp1 + y) + x;\n";
    this->kernel_part4 = "}";
    // provide a placeholder:
    this->SetRuleName("Gray-Scott");
    this->AddParameter("D_a",0.082f);
    this->AddParameter("D_b",0.041f);
    this->AddParameter("k",0.06f);
    this->AddParameter("F",0.035f);
    this->formula = "\
    delta_a = D_a * laplacian_a - a*b*b + F*(1.0f-a);\n\
    delta_b = D_b * laplacian_b + a*b*b - (F+k)*b;\n";
    this->timestep = 1.0f;
}

void OpenCL_nDim::Allocate(int x,int y,int z,int nc)
{
    if(x&(x-1) || y&(y-1) || z&(z-1))
        throw runtime_error("OpenCL_nDim::Allocate : for wrap-around in OpenCL we require all the dimensions to be powers of 2");
    OpenCL_RD::Allocate(x,y,z,nc);
    this->need_reload_formula = true;
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
