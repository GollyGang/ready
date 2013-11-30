/*  Copyright 2011, 2012, 2013 The Ready Bunch

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
#include "OpenCL_MixIn.hpp"
#include "OpenCL_utils.hpp"
using namespace OpenCL_utils;

// STL:
#include <stdexcept>
#include <fstream>
#include <sstream>
using namespace std;

// ---------------------------------------------------------------------------

OpenCL_MixIn::OpenCL_MixIn(int opencl_platform,int opencl_device)
{
    this->iPlatform = opencl_platform;
    this->iDevice = opencl_device;
    this->need_reload_context = true;
    this->external_context = false;
    this->need_write_to_opencl_buffers = true;
    this->kernel_function_name = "rd_compute";

    // initialise the opencl things to null in case we fail to create them
    this->device_id = NULL;
    this->context = NULL;
    this->command_queue = NULL;
    this->kernel = NULL;
    this->program = NULL;

    if(LinkOpenCL()!= CL_SUCCESS)
        throw runtime_error("Failed to load dynamic library for OpenCL");

    this->iCurrentBuffer = 0;
}

// ---------------------------------------------------------------------------

OpenCL_MixIn::OpenCL_MixIn( int opencl_platform, int opencl_device, cl_context externalContext )
{
    this->iPlatform = opencl_platform;
    this->iDevice = opencl_device;
    this->context = externalContext;
    this->external_context = true;
    this->need_reload_context = true;
    this->need_write_to_opencl_buffers = true;
    this->kernel_function_name = "rd_compute";

    // initialise the opencl things to null in case we fail to create them
    this->device_id = NULL;
    this->context = NULL;
    this->command_queue = NULL;
    this->kernel = NULL;
    this->program = NULL;

    if(LinkOpenCL()!= CL_SUCCESS)
        throw runtime_error("Failed to load dynamic library for OpenCL");

    this->iCurrentBuffer = 0;
}

// ---------------------------------------------------------------------------

OpenCL_MixIn::~OpenCL_MixIn()
{
    clFlush(this->command_queue);
    clFinish(this->command_queue);
    clReleaseKernel(this->kernel);
    clReleaseProgram(this->program);
    for(int i=0;i<2;i++)
        for(vector<cl_mem>::const_iterator it = this->buffers[i].begin();it!=this->buffers[i].end();it++)
            clReleaseMemObject(*it);
    clReleaseCommandQueue(this->command_queue);
    clReleaseContext(this->context);
}

// ---------------------------------------------------------------------------

void OpenCL_MixIn::SetPlatform(int i)
{
    if(i != this->iPlatform)
        this->need_reload_context = true;
    this->iPlatform = i;
}

// ---------------------------------------------------------------------------

void OpenCL_MixIn::SetDevice(int i)
{
    if(i != this->iDevice)
        this->need_reload_context = true;
    this->iDevice = i;
}

// ---------------------------------------------------------------------------

int OpenCL_MixIn::GetPlatform() const
{
    return this->iPlatform;
}

// ---------------------------------------------------------------------------

int OpenCL_MixIn::GetDevice() const
{
    return this->iDevice;
}

// ---------------------------------------------------------------------------

void OpenCL_MixIn::ReloadContextIfNeeded()
{
    if(!this->need_reload_context) return;

    cl_int ret;

    // retrieve our chosen platform
    cl_platform_id platform_id;
    {
        const int MAX_PLATFORMS = 10;
        cl_platform_id platforms_available[MAX_PLATFORMS];
        cl_uint num_platforms;
        ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
        if(ret != CL_SUCCESS || num_platforms==0)
        {
            throw runtime_error("No OpenCL platforms available");
            // currently only likely to see this when running in a virtualized OS, where an opencl.dll is found but doesn't work
        }
        if(this->iPlatform >= (int)num_platforms)
            throw runtime_error("OpenCL_MixIn::ReloadContextIfNeeded : too few platforms available");
        platform_id = platforms_available[this->iPlatform];
    }

    // retrieve our chosen device
    {
        const int MAX_DEVICES = 10;
        cl_device_id devices_available[MAX_DEVICES];
        cl_uint num_devices;
        ret = clGetDeviceIDs(platform_id,CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
        throwOnError(ret,"OpenCL_MixIn::ReloadContextIfNeeded : Failed to retrieve device IDs: ");
        if(this->iDevice >= (int)num_devices)
            throw runtime_error("OpenCL_MixIn::ReloadContextIfNeeded : too few devices available");
        this->device_id = devices_available[this->iDevice];
    }
    
    // If using external context then we don't need to create a new one
    // Probably need to get the device_id from the external context via the getInfo method
    // CL_QUEUE_DEVICE
    if (!this->external_context)
    {
        // create the context
        clReleaseContext(this->context);
        this->context = clCreateContext(NULL,1,&this->device_id,NULL,NULL,&ret);
        throwOnError(ret,"OpenCL_MixIn::ReloadContextIfNeeded : Failed to create context: ");
    }
    
    // create the command queue
    clReleaseCommandQueue(this->command_queue);
    this->command_queue = clCreateCommandQueue(this->context,this->device_id,0,&ret);
    throwOnError(ret,"OpenCL_MixIn::ReloadContextIfNeeded : Failed to create command queue: ");

    this->need_reload_context = false;
}

// -------------------------------------------------------------------------

void OpenCL_MixIn::TestKernel(std::string kernel_source)
{
    this->need_reload_context = true;
    this->ReloadContextIfNeeded();

    cl_int ret;

    // create the program
    const char *source = kernel_source.c_str();
    size_t source_size = kernel_source.length();
    cl_program temp_program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCL_MixIn::TestKernel : Failed to create program with source: ");

    // build the program
    ret = clBuildProgram(temp_program,1,&this->device_id,"-cl-denorms-are-zero -cl-fast-relaxed-math",NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        const int MAX_BUILD_LOG = 10000;
        char build_log[MAX_BUILD_LOG];
        size_t build_log_length;
        cl_int ret2 = clGetProgramBuildInfo(temp_program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret2,"OpenCL_MixIn::TestKernel : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCL_MixIn::TestKernel : build failed: (kernel saved as kernel.txt)\n\n" << build_log;
        throwOnError(ret,oss.str().c_str());
    }
    clReleaseProgram(temp_program);
}

// -----------------------------------------------------------------------

void OpenCL_MixIn::ReleaseOpenCLBuffers()
{
    for(int i=0;i<2;i++)
        for(vector<cl_mem>::const_iterator it = this->buffers[i].begin();it!=this->buffers[i].end();it++)
            clReleaseMemObject(*it);
}

// -----------------------------------------------------------------------
