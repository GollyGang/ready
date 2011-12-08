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
    assert(!this->image_data);
    this->image_data = vtkImageData::New();
    this->image_data->SetNumberOfScalarComponents(2);
    this->image_data->SetScalarTypeToFloat();
    this->image_data->SetDimensions(x,y,1);
    this->image_data->AllocateScalars();
}

GrayScott_slow::~GrayScott_slow()
{
}

void GrayScott_slow::Update(int n_steps)
{
    assert(this->image_data);
    // TODO
}

void GrayScott_slow::InitWithBlobInCenter()
{
    assert(this->image_data);
    int X = this->image_data->GetDimensions()[0];
    int Y = this->image_data->GetDimensions()[1];
    for(int x=0;x<X;x++)
    {
        for(int y=0;y<Y;y++)
        {
            if(_hypot(x-X/2,(y-Y/2)/1.5)<=frand(2,5)) // start with a uniform field with an approximate circle in the middle
            {
                this->image_data->SetScalarComponentFromFloat(x,y,0,0,0.0f);
                this->image_data->SetScalarComponentFromFloat(x,y,0,1,1.0f);
            }
            else 
            {
                this->image_data->SetScalarComponentFromFloat(x,y,0,0,1.0f);
                this->image_data->SetScalarComponentFromFloat(x,y,0,1,0.0f);
            }
        }
    }
}
