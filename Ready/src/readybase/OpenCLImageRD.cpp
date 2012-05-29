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
#include "OpenCLImageRD.hpp"
#include "OpenCL_utils.hpp"
#include "utils.hpp"
using namespace OpenCL_utils;

// STL:
#include <vector>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <cassert>
using namespace std;

// VTK:
#include <vtkImageData.h>
#include <vtkMath.h>

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::ReloadKernelIfNeeded()
{
    if(!this->need_reload_formula) return;

    cl_int ret;

    // create the program
    this->kernel_source = this->AssembleKernelSourceFromFormula(this->formula);
    const char *source = this->kernel_source.c_str();
    size_t source_size = this->kernel_source.length();
    cl_program program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCLImageRD::ReloadKernelIfNeeded : Failed to create program with source: ");

    // make a set of options to pass to the compiler
    ostringstream options;
    //options << "-cl-denorms-are-zero -cl-fast-relaxed-math";

    // build the program
    ret = clBuildProgram(program,1,&this->device_id,options.str().c_str(),NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        const int MAX_BUILD_LOG = 10000;
        char build_log[MAX_BUILD_LOG];
        size_t build_log_length;
        cl_int ret2 = clGetProgramBuildInfo(program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret2,"OpenCLImageRD::ReloadKernelIfNeeded : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCLImageRD::ReloadKernelIfNeeded : build failed (kernel saved as kernel.txt):\n\n" << build_log;
        throwOnError(ret,oss.str().c_str());
    }

    // create the kernel
    if(this->kernel) clReleaseKernel(this->kernel);
    this->kernel = clCreateKernel(program,this->kernel_function_name.c_str(),&ret);
    throwOnError(ret,"OpenCLImageRD::ReloadKernelIfNeeded : kernel creation failed: ");

    this->global_range[0] = max(1,vtkMath::Round(this->GetX()) / this->GetBlockSizeX());
    this->global_range[1] = max(1,vtkMath::Round(this->GetY()) / this->GetBlockSizeY());
    this->global_range[2] = max(1,vtkMath::Round(this->GetZ()) / this->GetBlockSizeZ());
    // (we let the local work group size be automatically decided, seems to be faster and more flexible that way)

    this->need_reload_formula = false;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::CreateOpenCLBuffers()
{
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();
    const int NC = this->GetNumberOfChemicals();

    cl_int ret;

    for(int io=0;io<2;io++) // we create two buffers for each chemical, and switch between them
    {
        this->buffers[io].resize(NC);
        for(int ic=0;ic<NC;ic++)
        {
            this->buffers[io][ic] = clCreateBuffer(this->context, CL_MEM_READ_WRITE, MEM_SIZE, NULL, &ret);
            throwOnError(ret,"OpenCLImageRD::CreateOpenCLBuffers : buffer creation failed: ");
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::WriteToOpenCLBuffers()
{
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();

    this->iCurrentBuffer = 0;
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueWriteBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCLImageRD::WriteToOpenCLBuffers : buffer writing failed: ");
    }
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::CopyFromImage(vtkImageData* im)
{
    ImageRD::CopyFromImage(im);
    this->WriteToOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::GenerateInitialPattern()
{
    ImageRD::GenerateInitialPattern();
    this->WriteToOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::BlankImage()
{
    ImageRD::BlankImage();
    this->WriteToOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::AllocateImages(int x,int y,int z,int nc)
{
    if(x&(x-1) || y&(y-1) || z&(z-1))
        throw runtime_error("OpenCLImageRD::AllocateImages : for wrap-around in OpenCL we require all the dimensions to be powers of 2");
    ImageRD::AllocateImages(x,y,z,nc);
    this->need_reload_formula = true;
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::InternalUpdate(int n_steps)
{
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();

    cl_int ret;
    int iBuffer;
    const int NC = this->GetNumberOfChemicals();

    for(int it=0;it<n_steps;it++)
    {
        for(int io=0;io<2;io++) // first input buffers (io=0) then output buffers (io=1)
        {
            iBuffer = (this->iCurrentBuffer+io)%2;
            for(int ic=0;ic<NC;ic++)
            {
                // a_in, b_in, ... a_out, b_out ...
                ret = clSetKernelArg(this->kernel, io*NC+ic, sizeof(cl_mem), (void *)&this->buffers[iBuffer][ic]);
                throwOnError(ret,"OpenCLImageRD::InternalUpdate : clSetKernelArg failed: ");
            }
        }
        ret = clEnqueueNDRangeKernel(this->command_queue,this->kernel, 3, NULL, this->global_range, NULL, 0, NULL, NULL);
        throwOnError(ret,"OpenCLImageRD::InternalUpdate : clEnqueueNDRangeKernel failed: ");
        this->iCurrentBuffer = 1 - this->iCurrentBuffer;
    }

    this->ReadFromOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::ReadFromOpenCLBuffers()
{
    // read from opencl buffers into our image
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueReadBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCLImageRD::ReadFromOpenCLBuffers : buffer reading failed: ");
    }
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLImageRD::TestFormula(std::string program_string)
{
    this->TestKernel(this->AssembleKernelSourceFromFormula(program_string));
}

// ----------------------------------------------------------------------------------------------------------------
