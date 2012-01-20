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
#include "GrayScott_slow.hpp"
#include "utils.hpp"

// stdlib:
#include <stdlib.h>
#include <math.h>

// STL:
#include <stdexcept>
#include <cassert>
using namespace std;

// VTK:
#include <vtkImageData.h>

GrayScott_slow::GrayScott_slow()
{
    this->timestep = 1.0f;
    this->rule_name = "Gray-Scott";
    this->AddParameter("r_a",0.082f);
    this->AddParameter("r_b",0.041f);
    // for spots:
    this->AddParameter("k",0.064f);
    this->AddParameter("F",0.035f);
}

void GrayScott_slow::Allocate(int x,int y,int z,int nc)
{
    if(z!=1) throw runtime_error("GrayScott_slow::Allocate : this implementation is 2D only"); 
    if(nc!=2) throw runtime_error("GrayScott_slow::Allocate : this implementation is for 2 chemicals only"); 
    BaseRD::Allocate(x,y,1,2);
    // also allocate our buffer images
    this->DeleteBuffers();
    this->buffer_images.resize(2);
    for(int i=0;i<2;i++)
        this->buffer_images[i] = AllocateVTKImage(x,y,1);
}

GrayScott_slow::~GrayScott_slow()
{
    this->DeleteBuffers();
}

void GrayScott_slow::DeleteBuffers()
{
    for(int i=0;i<(int)this->buffer_images.size();i++)
    {
        if(this->buffer_images[i])
            this->buffer_images[i]->Delete();
    }
}

void GrayScott_slow::Update(int n_steps)
{
    const int X = this->GetX();
    const int Y = this->GetY();

    float r_a = this->GetParameterValue(0);
    float r_b = this->GetParameterValue(1);
    float k = this->GetParameterValue(2);
    float F = this->GetParameterValue(3);

    // take approximately n_steps
    for(int iStepPair=0;iStepPair<(n_steps+1)/2;iStepPair++)
    {
        for(int iStep=0;iStep<2;iStep++)
        {
            float *old_a,*new_a,*old_b,*new_b;
            switch(iStep) {
                case 0: old_a = static_cast<float*>(this->images[0]->GetScalarPointer()); 
                        old_b = static_cast<float*>(this->images[1]->GetScalarPointer()); 
                        new_a = static_cast<float*>(this->buffer_images[0]->GetScalarPointer()); 
                        new_b = static_cast<float*>(this->buffer_images[1]->GetScalarPointer()); 
                        break;
                case 1: old_a = static_cast<float*>(this->buffer_images[0]->GetScalarPointer()); 
                        old_b = static_cast<float*>(this->buffer_images[1]->GetScalarPointer()); 
                        new_a = static_cast<float*>(this->images[0]->GetScalarPointer()); 
                        new_b = static_cast<float*>(this->images[1]->GetScalarPointer()); 
                        break;
            }

            for(int x=0;x<X;x++)
            {
                int x_prev = (x-1+X)%X;
                int x_next = (x+1)%X;
                for(int y=0;y<Y;y++)
                {
                    int y_prev = (y-1+Y)%Y;
                    int y_next = (y+1)%Y;

                    float aval = *vtk_at(old_a,x,y,0,X,Y);
                    float bval = *vtk_at(old_b,x,y,0,X,Y);

                    // compute the Laplacians of a and b
                    // 5-point stencil:
                    float dda = *vtk_at(old_a,x,y_prev,0,X,Y) +
                                *vtk_at(old_a,x,y_next,0,X,Y) +
                                *vtk_at(old_a,x_prev,y,0,X,Y) + 
                                *vtk_at(old_a,x_next,y,0,X,Y) - 4*aval;
                    float ddb = *vtk_at(old_b,x,y_prev,0,X,Y) +
                                *vtk_at(old_b,x,y_next,0,X,Y) +
                                *vtk_at(old_b,x_prev,y,0,X,Y) + 
                                *vtk_at(old_b,x_next,y,0,X,Y) - 4*bval;
     
                    // compute the new rate of change of a and b
                    float da = r_a * dda - aval*bval*bval + F*(1-aval);
                    float db = r_b * ddb + aval*bval*bval - (F+k)*bval;

                    // apply the change
                    *vtk_at(new_a,x,y,0,X,Y) = aval + this->timestep * da;
                    *vtk_at(new_b,x,y,0,X,Y) = bval + this->timestep * db;
                }
            }
        }
        this->timesteps_taken+=2;
    }
    this->images[0]->Modified();
    this->images[1]->Modified();
    this->is_modified = true;
}

void GrayScott_slow::InitWithBlobInCenter()
{
    const int X = this->GetX();
    const int Y = this->GetY();

    vtkImageData *a_image = this->images[0];
    vtkImageData *b_image = this->images[1];
    float* a_data = static_cast<float*>(a_image->GetScalarPointer());
    float* b_data = static_cast<float*>(b_image->GetScalarPointer());

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
    this->timesteps_taken = 0;
    this->is_modified = true;
}
