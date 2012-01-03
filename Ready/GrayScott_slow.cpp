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
    this->r_a = 0.082f;
    this->r_b = 0.041f;
    // for spots:
    this->k = 0.064f;
    this->f = 0.035f;
    this->buffer_image = NULL;
}

void GrayScott_slow::Allocate(int x,int y)
{
    this->AllocateImage(x,y,1,2);
    // also allocate our buffer image
    this->buffer_image = AllocateVTKImage(x,y,1,2);
}

GrayScott_slow::~GrayScott_slow()
{
    if(this->buffer_image)
        this->buffer_image->Delete();
}

void GrayScott_slow::Update(int n_steps)
{
    // take approximately n_steps
    for(int iStepPair=0;iStepPair<(n_steps+1)/2;iStepPair++)
    {
        for(int iStep=0;iStep<2;iStep++)
        {
            vtkImageData *from_im,*to_im;
            switch(iStep) {
                case 0: from_im = this->GetImage(); to_im = this->buffer_image; break;
                case 1: from_im = this->buffer_image; to_im = this->GetImage(); break;
            }

            const int X = from_im->GetDimensions()[0];
            const int Y = from_im->GetDimensions()[1];
            const int NC = from_im->GetNumberOfScalarComponents();

            float* from_data = static_cast<float*>(from_im->GetScalarPointer());
            float* to_data = static_cast<float*>(to_im->GetScalarPointer());

            for(int x=0;x<X;x++)
            {
                int x_prev = (x-1+X)%X;
                int x_next = (x+1)%X;
                for(int y=0;y<Y;y++)
                {
                    int y_prev = (y-1+Y)%Y;
                    int y_next = (y+1)%Y;

                    float aval = *vtk_at(from_data,x,y,0,0,X,Y,NC);
                    float bval = *vtk_at(from_data,x,y,0,1,X,Y,NC);

                    // compute the Laplacians of a and b
                    // 5-point stencil:
                    float dda = *vtk_at(from_data,x,y_prev,0,0,X,Y,NC) +
                                *vtk_at(from_data,x,y_next,0,0,X,Y,NC) +
                                *vtk_at(from_data,x_prev,y,0,0,X,Y,NC) + 
                                *vtk_at(from_data,x_next,y,0,0,X,Y,NC) - 4*aval;
                    float ddb = *vtk_at(from_data,x,y_prev,0,1,X,Y,NC) +
                                *vtk_at(from_data,x,y_next,0,1,X,Y,NC) +
                                *vtk_at(from_data,x_prev,y,0,1,X,Y,NC) + 
                                *vtk_at(from_data,x_next,y,0,1,X,Y,NC) - 4*bval;
     
                    // compute the new rate of change of a and b
                    float da = this->r_a * dda - aval*bval*bval + this->f*(1-aval);
                    float db = this->r_b * ddb + aval*bval*bval - (this->f+this->k)*bval;

                    // apply the change
                    *vtk_at(to_data,x,y,0,0,X,Y,NC) = aval + this->timestep * da;
                    *vtk_at(to_data,x,y,0,1,X,Y,NC) = bval + this->timestep * db;
                }
            }
        }
        this->timesteps_taken+=2;
    }
    this->GetImage()->Modified();
}

void GrayScott_slow::InitWithBlobInCenter()
{
    vtkImageData *image = this->GetImage();
    assert(image);

    const int X = image->GetDimensions()[0];
    const int Y = image->GetDimensions()[1];
    const int NC = image->GetNumberOfScalarComponents();

    float* data = static_cast<float*>(image->GetScalarPointer());

    for(int x=0;x<X;x++)
    {
        for(int y=0;y<Y;y++)
        {
            if(hypot2(x-X/2,(y-Y/2)/1.5)<=frand(2.0f,5.0f)) // start with a uniform field with an approximate circle in the middle
            {
                *vtk_at(data,x,y,0,0,X,Y,NC) = 0.0f;
                *vtk_at(data,x,y,0,1,X,Y,NC) = 1.0f;
            }
            else 
            {
                *vtk_at(data,x,y,0,0,X,Y,NC) = 1.0f;
                *vtk_at(data,x,y,0,1,X,Y,NC) = 0.0f;
            }
        }
    }
    this->GetImage()->Modified();
    this->timesteps_taken = 0;
}
