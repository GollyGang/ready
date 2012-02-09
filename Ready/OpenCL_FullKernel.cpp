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
#include "OpenCL_FullKernel.hpp"
#include "utils.hpp"

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkImageData.h>
#include <vtkXMLDataElement.h>

OpenCL_FullKernel::OpenCL_FullKernel()
{
    this->SetRuleName("Full kernel example");
    this->SetFormula("__kernel void rd_compute(__global float* a_in,__global float* a_out) {}");
    this->block_size[0]=1;
    this->block_size[1]=1;
    this->block_size[2]=1;
    this->SetTimestep(1.0f);
}

std::string OpenCL_FullKernel::AssembleKernelSourceFromFormula(std::string formula) const
{
    return formula; // here the formula is a full OpenCL kernel
}
