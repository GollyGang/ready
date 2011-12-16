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
    this->program = NULL;
    this->source = NULL;
}

OpenCL_RD::~OpenCL_RD()
{
    this->DeleteOpenCLBuffers();
    delete this->context;
    delete this->device;
    delete this->command_queue;
    delete this->kernel;
    delete this->program;
    delete this->source;
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

void OpenCL_RD::throwOnError(cl_int ret,const char* message)
{
    if(ret == CL_SUCCESS) return;

    ostringstream oss;
    oss << message << descriptionOfError(ret);
    throw runtime_error(oss.str().c_str());
}

void OpenCL_RD::ReloadContextIfNeeded()
{
    if(!this->need_reload_context) return;

    cl_int ret;

    vector<cl::Platform> platforms_available;
    ret = cl::Platform::get(&platforms_available);
    if(ret != CL_SUCCESS || platforms_available.empty())
    {
        throw runtime_error("No OpenCL platforms available");
        // currently only likely to see this when running in a virtualized OS, where an opencl.dll is found but doesn't work
    }
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

    this->source = new cl::Program::Sources(1, std::make_pair(this->program_string.c_str(), this->program_string.length()+1));

    // Make program of the source code in the context
    this->program = new cl::Program(*this->context, *this->source);

    // Build program for our selected device
    vector<cl::Device> devices;
    devices.push_back(*this->device);
    ret = this->program->build(devices, NULL, NULL, NULL);
    if(ret != CL_SUCCESS)
    {
        throw runtime_error("OpenCL_RD::ReloadKernelIfNeeded : program.build() failed:\n\n" 
            + this->program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*this->device));
    }

    // Make kernel
    this->kernel = new cl::Kernel(*this->program, this->kernel_function_name.c_str(),&ret);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : Kernel() failed: ");

    // decide the size of the work-groups
    const int X = this->GetOldImage()->GetDimensions()[0];
    const int Y = this->GetOldImage()->GetDimensions()[1];
    const int Z = this->GetOldImage()->GetDimensions()[2];
    this->global_range = cl::NDRange(X,Y,Z);
    int wgs = this->kernel->getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(*this->device);
    // TODO: allow user override (this value isn't always optimal)
    if(wgs&(wgs-1)) 
        throw runtime_error("OpenCL_RD::ReloadKernelIfNeeded : expecting CL_KERNEL_WORK_GROUP_SIZE to be a power of 2");
    // spread the work group over the dimensions, preferring x over y and y over z because of memory alignment
    int wgx,wgy,wgz;
    wgx = min(X,wgs);
    wgy = min(Y,wgs/wgx);
    wgz = min(Z,wgs/(wgx*wgy));
    // TODO: give user control over the work group shape?
    if(X%wgx || Y%wgy || Z%wgz)
        throw runtime_error("OpenCL_RD::ReloadKernelIfNeeded : work group size doesn't divide into grid dimensions");
    this->local_range = cl::NDRange(wgx,wgy,wgz);

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

    this->buffer1 = new cl::Buffer(*this->context, 0, MEM_SIZE, NULL, &ret);
    throwOnError(ret,"OpenCL_RD::CreateBuffers : Buffer() failed: ");

    this->buffer2 = new cl::Buffer(*this->context, 0, MEM_SIZE, NULL, &ret);
    throwOnError(ret,"OpenCL_RD::CreateBuffers : Buffer() failed: ");
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
    const size_t MEM_SIZE = sizeof(float) * X * Y * Z * NC;

    cl_int ret;

    ret = this->command_queue->enqueueWriteBuffer(*this->buffer1, CL_TRUE, 0, MEM_SIZE, old_data);
    throwOnError(ret,"OpenCL_RD::WriteToBuffers : enqueueWriteBuffer() failed: ");
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
    const size_t MEM_SIZE = sizeof(float) * X * Y * Z * NC;

    cl_int ret;

    ret = this->command_queue->enqueueReadBuffer(*this->buffer1, CL_TRUE, 0, MEM_SIZE, new_data);
    throwOnError(ret,"OpenCL_RD::ReadFromBuffers : enqueueReadBuffer() failed: ");
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

    ret = this->kernel->setArg(0, *this->buffer1); // input
    throwOnError(ret,"OpenCL_RD::Update2Steps : setArg() failed: ");
    ret = this->kernel->setArg(1, *this->buffer2); // output
    throwOnError(ret,"OpenCL_RD::Update2Steps : setArg() failed: ");
    ret = this->command_queue->enqueueNDRangeKernel(*this->kernel, cl::NullRange, this->global_range, this->local_range);
    throwOnError(ret,"OpenCL_RD::Update2Steps : enqueueNDRangeKernel() failed: ");

    ret = this->kernel->setArg(0, *this->buffer2); // input
    throwOnError(ret,"OpenCL_RD::Update2Steps : setArg() failed: ");
    ret = this->kernel->setArg(1, *this->buffer1); // output
    throwOnError(ret,"OpenCL_RD::Update2Steps : setArg() failed: ");
    ret = this->command_queue->enqueueNDRangeKernel(*this->kernel, cl::NullRange, this->global_range, this->local_range);
    throwOnError(ret,"OpenCL_RD::Update2Steps : enqueueNDRangeKernel() failed: ");
}

// http://www.khronos.org/message_boards/viewtopic.php?f=37&t=2107
const char* OpenCL_RD::descriptionOfError(cl_int err) 
{
    switch (err) {
        case CL_SUCCESS:                            return "Success!";
        case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
        case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
        case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
        case CL_OUT_OF_RESOURCES:                   return "Out of resources";
        case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
        case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
        case CL_MAP_FAILURE:                        return "Map failure";
        case CL_INVALID_VALUE:                      return "Invalid value";
        case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
        case CL_INVALID_PLATFORM:                   return "Invalid platform";
        case CL_INVALID_DEVICE:                     return "Invalid device";
        case CL_INVALID_CONTEXT:                    return "Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
        case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
        case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
        case CL_INVALID_SAMPLER:                    return "Invalid sampler";
        case CL_INVALID_BINARY:                     return "Invalid binary";
        case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
        case CL_INVALID_PROGRAM:                    return "Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
        case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
        case CL_INVALID_KERNEL:                     return "Invalid kernel";
        case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
        case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
        case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
        case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
        case CL_INVALID_EVENT:                      return "Invalid event";
        case CL_INVALID_OPERATION:                  return "Invalid operation";
        case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
        case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
        case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
        default: return "Unknown";
    }
}
