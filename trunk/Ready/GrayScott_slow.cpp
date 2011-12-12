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
#include "GrayScott_slow.hpp"

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
    this->r_a = 0.082f;
    this->r_b = 0.041f;
    // for spots:
    this->k = 0.064f;
    this->f = 0.035f;
}

void GrayScott_slow::Allocate(int x,int y)
{
    for(int iB=0;iB<2;iB++)
    {
        assert(!this->buffer[iB]);
        this->buffer[iB] = vtkImageData::New();
        this->buffer[iB]->SetNumberOfScalarComponents(2);
        this->buffer[iB]->SetScalarTypeToFloat();
        this->buffer[iB]->SetDimensions(x,y,1);
        this->buffer[iB]->AllocateScalars();
    }
}

void GrayScott_slow::Update(int n_steps)
{
    for(int iStep=0;iStep<n_steps;iStep++)
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
            int x_prev = (x-1+X)%X;
            int x_next = (x+1)%X;
            for(int y=0;y<Y;y++)
            {
                int y_prev = (y-1+Y)%Y;
                int y_next = (y+1)%Y;

                float aval = *vtk_at(old_data,x,y,0,0,X,Y,NC);
                float bval = *vtk_at(old_data,x,y,0,1,X,Y,NC);

                // compute the Laplacians of a and b
                float dda = *vtk_at(old_data,x,y_prev,0,0,X,Y,NC) +
                            *vtk_at(old_data,x,y_next,0,0,X,Y,NC) +
                            *vtk_at(old_data,x_prev,y,0,0,X,Y,NC) + 
                            *vtk_at(old_data,x_next,y,0,0,X,Y,NC) - 4*aval;
                float ddb = *vtk_at(old_data,x,y_prev,0,1,X,Y,NC) +
                            *vtk_at(old_data,x,y_next,0,1,X,Y,NC) +
                            *vtk_at(old_data,x_prev,y,0,1,X,Y,NC) + 
                            *vtk_at(old_data,x_next,y,0,1,X,Y,NC) - 4*bval;

                // compute the new rate of change of a and b
                float da = this->r_a * dda - aval*bval*bval + this->f*(1-aval);
                float db = this->r_b * ddb + aval*bval*bval - (this->f+this->k)*bval;

                // apply the change
                aval += this->timestep * da;
                bval += this->timestep * db;
                *vtk_at(new_data,x,y,0,0,X,Y,NC) = aval;
                *vtk_at(new_data,x,y,0,1,X,Y,NC) = bval;
            }
        }

        this->SwitchBuffers();
        this->timesteps_taken++;
    }
}

void GrayScott_slow::InitWithBlobInCenter()
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
    this->SwitchBuffers();
}
