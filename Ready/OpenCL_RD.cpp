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
#include "OpenCL_RD.hpp"

// STL:
#include <vector>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <cassert>
using namespace std;

// VTK:
#include <vtkImageData.h>

OpenCL_RD::OpenCL_RD()
{
    this->iPlatform = 0;
    this->iDevice = 0;
    this->need_reload_context = true;
    this->kernel_function_name = "rd_compute";

    this->context = NULL;
    this->device = NULL;
    this->command_queue = NULL;
    this->kernel = NULL;
    this->buffer1 = NULL;
    this->buffer2 = NULL;
}

OpenCL_RD::~OpenCL_RD()
{
    this->DeleteOpenCLBuffers();
    delete this->context;
    delete this->device;
    delete this->command_queue;
    delete this->kernel;
}

void OpenCL_RD::SetPlatform(int i)
{
    if(i != this->iPlatform)
        this->need_reload_context = true;
    this->iPlatform = i;
}

void OpenCL_RD::SetDevice(int i)
{
    if(i != this->iDevice)
        this->need_reload_context = true;
    this->iDevice = i;
}

int OpenCL_RD::GetPlatform()
{
    return this->iPlatform;
}

int OpenCL_RD::GetDevice()
{
    return this->iDevice;
}

void OpenCL_RD::ReloadContextIfNeeded()
{
    if(!this->need_reload_context) return;

    cl_int ret;

    vector<cl::Platform> platforms_available;
    ret = cl::Platform::get(&platforms_available);
    if(ret != CL_SUCCESS)
        throw runtime_error("OpenCL_RD::ReloadContextIfNeeded : cl::Platform::get() failed");
    if(this->iPlatform>=(int)platforms_available.size())
        throw runtime_error("OpenCL_RD::ReloadContextIfNeeded : too few platforms available");
    cl::Platform &platform = platforms_available[this->iPlatform];

    cl_context_properties cps[3] = { 
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)(platform)(), 
        0 
    };
    this->context = new cl::Context( CL_DEVICE_TYPE_ALL, cps);

    vector<cl::Device> devices_available = this->context->getInfo<CL_CONTEXT_DEVICES>();
    if(this->iDevice>=(int)devices_available.size())
        throw runtime_error("OpenCL_RD::ReloadContextIfNeeded : too few devices available");
    this->device = new cl::Device(devices_available[this->iDevice]);

    this->command_queue = new cl::CommandQueue(*this->context,*this->device);

    this->need_reload_context = false;
}

void OpenCL_RD::ReloadKernelIfNeeded()
{
    if(!this->need_reload_program) return;

    cl_int ret;

    cl::Program::Sources source(1, std::make_pair(this->program_string.c_str(), this->program_string.length()+1));

    // Make program of the source code in the context
    cl::Program program(*this->context, source);

    // Build program for our selected device
    vector<cl::Device> devices;
    devices.push_back(*this->device);
    ret = program.build(devices, NULL, NULL, NULL);
    if(ret != CL_SUCCESS)
    {
        throw runtime_error("OpenCL_RD::ReloadKernelIfNeeded : program.build() failed:\n\n" + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*this->device));
    }

    // Make kernel
    this->kernel = new cl::Kernel(program, this->kernel_function_name.c_str(),&ret);
    if(ret != CL_SUCCESS)
    {
        // e.g. -46 = CL_INVALID_KERNEL_NAME : function name not found in kernel
        ostringstream oss;
        oss << "OpenCL_RD::ReloadKernelIfNeeded : Kernel() failed: " << ret;
        throw runtime_error(oss.str().c_str());
    }

    // make work-items queue
    const int X = this->GetOldImage()->GetDimensions()[0];
    const int Y = this->GetOldImage()->GetDimensions()[1];
    const int Z = this->GetOldImage()->GetDimensions()[2];
    this->global_range = cl::NDRange(X,Y,Z);
    this->local_range = cl::NDRange(1,1,1); // TODO: some way of sensibly choosing this

    this->need_reload_program = false;
}

void OpenCL_RD::CreateOpenCLBuffers()
{
    assert(!this->buffer1);
    assert(!this->buffer2);

    vtkImageData *old_image = this->GetOldImage();
    vtkImageData *new_image = this->GetNewImage();
    assert(old_image);
    assert(new_image);

    const int X = old_image->GetDimensions()[0];
    const int Y = old_image->GetDimensions()[1];
    const int Z = old_image->GetDimensions()[2];
    const int NC = old_image->GetNumberOfScalarComponents();
    const unsigned long MEM_SIZE = sizeof(float) * X * Y * Z * NC;

    cl_int ret;

    this->buffer1 = new cl::Buffer(*this->context, CL_MEM_READ_ONLY, MEM_SIZE, NULL, &ret);
    if(ret != CL_SUCCESS)
    {
        ostringstream oss;
        oss << "OpenCL_RD::CreateBuffers : Buffer() failed: " << ret;
        throw runtime_error(oss.str().c_str());
    }

    this->buffer2 = new cl::Buffer(*this->context, CL_MEM_READ_ONLY, MEM_SIZE, NULL, &ret);
    if(ret != CL_SUCCESS)
    {
        ostringstream oss;
        oss << "OpenCL_RD::CreateBuffers : Buffer() failed: " << ret;
        throw runtime_error(oss.str().c_str());
    }
}

void OpenCL_RD::WriteToOpenCLBuffers()
{
    assert(this->buffer1);
    assert(this->buffer2);

    vtkImageData *old_image = this->GetOldImage();
    vtkImageData *new_image = this->GetNewImage();
    assert(old_image);
    assert(new_image);

    const int X = old_image->GetDimensions()[0];
    const int Y = old_image->GetDimensions()[1];
    const int Z = old_image->GetDimensions()[2];
    const int NC = old_image->GetNumberOfScalarComponents();

    float* old_data = static_cast<float*>(old_image->GetScalarPointer());
    float* new_data = static_cast<float*>(new_image->GetScalarPointer());
    const unsigned long MEM_SIZE = sizeof(float) * X * Y * Z * NC;

    cl_int ret;

    ret = this->command_queue->enqueueWriteBuffer(*this->buffer1, CL_TRUE, 0, MEM_SIZE, old_data);
    if(ret != CL_SUCCESS)
    {
        ostringstream oss;
        oss << "OpenCL_RD::WriteToBuffers : enqueueWriteBuffer() failed: " << ret;
        throw runtime_error(oss.str().c_str());
    }
}

void OpenCL_RD::ReadFromOpenCLBuffers()
{
    assert(this->buffer1);
    assert(this->buffer2);

    vtkImageData *old_image = this->GetOldImage();
    vtkImageData *new_image = this->GetNewImage();
    assert(old_image);
    assert(new_image);

    const int X = old_image->GetDimensions()[0];
    const int Y = old_image->GetDimensions()[1];
    const int Z = old_image->GetDimensions()[2];
    const int NC = old_image->GetNumberOfScalarComponents();

    float* old_data = static_cast<float*>(old_image->GetScalarPointer());
    float* new_data = static_cast<float*>(new_image->GetScalarPointer());
    const unsigned long MEM_SIZE = sizeof(float) * X * Y * Z * NC;

    cl_int ret;

    ret = this->command_queue->enqueueReadBuffer(*this->buffer1, CL_TRUE, 0, MEM_SIZE, new_data);
    if(ret != CL_SUCCESS)
    {
        ostringstream oss;
        oss << "OpenCL_RD::ReadFromBuffers : enqueueReadBuffer() failed: " << ret;
        throw runtime_error(oss.str().c_str());
    }
}

void OpenCL_RD::DeleteOpenCLBuffers()
{
    delete this->buffer1;
    delete this->buffer2;
}

void OpenCL_RD::Update2Steps()
{
    // (buffer-switching)

    cl_int ret;

    this->kernel->setArg(0, this->buffer1); // input
    this->kernel->setArg(1, this->buffer2); // output
    ret = this->command_queue->enqueueNDRangeKernel(*this->kernel, cl::NullRange, this->global_range, this->local_range);
    if(ret != CL_SUCCESS)
    {
        ostringstream oss;
        oss << "OpenCL_RD::Update2Steps : enqueueNDRangeKernel() failed: " << ret;
        throw runtime_error(oss.str().c_str());
    }

    this->kernel->setArg(0, this->buffer2); // input
    this->kernel->setArg(1, this->buffer1); // output
    ret = this->command_queue->enqueueNDRangeKernel(*this->kernel, cl::NullRange, this->global_range, this->local_range);
    if(ret != CL_SUCCESS)
    {
        ostringstream oss;
        oss << "OpenCL_RD::Update2Steps : enqueueNDRangeKernel() failed: " << ret;
        throw runtime_error(oss.str().c_str());
    }
}
