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
    this->SetRuleName("Gray-Scott");
    this->AddParameter("D_a",0.082f);
    this->AddParameter("D_b",0.041f);
    this->AddParameter("k",0.06f);
    this->AddParameter("F",0.035f);
    this->SetFormula("\
    delta_a = D_a * laplacian_a - a*b*b + F*(1.0f-a);\n\
    delta_b = D_b * laplacian_b + a*b*b - (F+k)*b;\n");
    this->SetTimestep(1.0f);
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
    this->is_modified = true;
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
    this->is_modified = true;
}

string Chem(int i) { return to_string((char)('a'+i)); } // a, b, c, ...

std::string OpenCL_nDim::AssembleKernelSourceFromFormula(std::string formula) const
{
    const string indent = "    ";
    const int NC = this->GetNumberOfChemicals();
    const int NDIRS = 6;
    const string dir[NDIRS]={"left","right","up","down","fore","back"};

    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    // output the function definition
    kernel_source << "__kernel void rd_compute(";
    for(int i=0;i<NC;i++)
        kernel_source << "__global float4 *" << Chem(i) << "_in,";
    for(int i=0;i<NC;i++)
    {
        kernel_source << "__global float4 *" << Chem(i) << "_out";
        if(i<NC-1)
            kernel_source << ",";
    }
    // output the first part of the body
    kernel_source << ")\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int Z = get_global_size(2);\n\
    const int i_here = X*(Y*z + y) + x;\n\
\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "float4 " << Chem(i) << " = " << Chem(i) << "_in[i_here];\n"; // "float4 a = a_in[i_here];"
    // output the Laplacian part of the body
    kernel_source << "\
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
    for(int iC=0;iC<NC;iC++)
        for(int iDir=0;iDir<NDIRS;iDir++)
            kernel_source << indent << "float4 " << Chem(iC) << "_" << dir[iDir] << " = " << Chem(iC) << "_in[i_" << dir[iDir] << "];\n";
    for(int iC=0;iC<NC;iC++)
    {
        kernel_source << indent << "float4 laplacian_" << Chem(iC) << " = (float4)(" << Chem(iC) << "_up.x + " << Chem(iC) << ".y + " << Chem(iC) << "_down.x + " << Chem(iC) << "_left.w + " << Chem(iC) << "_fore.x + " << Chem(iC) << "_back.x,\n";
        kernel_source << indent << Chem(iC) << "_up.y + " << Chem(iC) << ".z + " << Chem(iC) << "_down.y + " << Chem(iC) << ".x + " << Chem(iC) << "_fore.y + " << Chem(iC) << "_back.y,\n";
        kernel_source << indent << Chem(iC) << "_up.z + " << Chem(iC) << ".w + " << Chem(iC) << "_down.z + " << Chem(iC) << ".y + " << Chem(iC) << "_fore.z + " << Chem(iC) << "_back.z,\n";
        kernel_source << indent << Chem(iC) << "_up.w + " << Chem(iC) << "_right.x + " << Chem(iC) << "_down.w + " << Chem(iC) << ".z + " << Chem(iC) << "_fore.w + " << Chem(iC) << "_back.w) - 6.0f*" << Chem(iC) << "\n";
        kernel_source << indent << "+ (float4)(1e-6f,1e-6f,1e-6f,1e-6f); // (kill denormals)\n";
    }
    kernel_source << "\n";
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << "float4 delta_" << Chem(iC) << ";\n";
    kernel_source << "\n";
    // the parameters (assume all float for now)
    for(int i=0;i<(int)this->parameters.size();i++)
        kernel_source << indent << "float " << this->parameters[i].first << " = " << this->parameters[i].second << "f;\n";
    // the timestep
    kernel_source << indent << "float delta_t = " << this->timestep << "f;\n";
    // the formula
    istringstream iss(formula);
    string s;
    while(iss.good())
    {
        getline(iss,s);
        kernel_source << indent << s << "\n";
    }
    // the last part of the kernel
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << Chem(iC) << "_out[i_here] = " << Chem(iC) << " + delta_t * delta_" << Chem(iC) << ";\n";
    kernel_source << "}\n";
    return kernel_source.str();
}

