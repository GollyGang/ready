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

// STL:
#include <stdexcept>
#include <cassert>
using namespace std;

GrayScott_slow::GrayScott_slow() : data(NULL)
{
    this->f = 0.003f; // TODO
    this->k = 0.004f;
}

void GrayScott_slow::Allocate(int x,int y)
{
    this->X = x;
    this->Y = y;
    this->data = new float[2*x*y];
}

GrayScott_slow::~GrayScott_slow()
{
    delete data;
}

void GrayScott_slow::Update(int n_steps)
{
    // TODO
}

float GrayScott_slow::GetAt(int x,int y,int iChemical)
{
    assert(this->data);
    return rand()/float(RAND_MAX); // DEBUG
    //return this->data[ iChemical*this->Y*this->X + y*this->X + x ];
}

void GrayScott_slow::InitWithBlobInCenter()
{
    // TODO
}
