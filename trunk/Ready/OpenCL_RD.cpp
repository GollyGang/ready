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
#include "OpenCL_RD.hpp"
#include "utils.hpp"

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

    if(LinkOpenCL()!= CL_SUCCESS)
        throw runtime_error("Failed to load dynamic library for OpenCL");
}

OpenCL_RD::~OpenCL_RD()
{
    clReleaseContext(this->context);
    clReleaseCommandQueue(this->command_queue);
    clReleaseKernel(this->kernel);
    for(int io=0;io<2;io++)
        for(int i=0;i<(int)this->buffers[io].size();i++)
            clReleaseMemObject(this->buffers[io][i]);
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

int OpenCL_RD::GetPlatform() const
{
    return this->iPlatform;
}

int OpenCL_RD::GetDevice() const
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
            throw runtime_error("OpenCL_RD::ReloadContextIfNeeded : too few platforms available");
        platform_id = platforms_available[this->iPlatform];
    }

    // retrieve our chosen device
    {
        const int MAX_DEVICES = 10;
        cl_device_id devices_available[MAX_DEVICES];
        cl_uint num_devices;
        ret = clGetDeviceIDs(platform_id,CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
        throwOnError(ret,"OpenCL_RD::ReloadContextIfNeeded : Failed to retrieve device IDs: ");
        if(this->iDevice >= (int)num_devices)
            throw runtime_error("OpenCL_RD::ReloadContextIfNeeded : too few devices available");
        this->device_id = devices_available[this->iDevice];
    }

    // create the context
    this->context = clCreateContext(NULL,1,&this->device_id,NULL,NULL,&ret);
    throwOnError(ret,"OpenCL_RD::ReloadContextIfNeeded : Failed to create context: ");

    // create the command queue
    this->command_queue = clCreateCommandQueue(this->context,this->device_id,NULL,&ret);
    throwOnError(ret,"OpenCL_RD::ReloadContextIfNeeded : Failed to create command queue: ");

    this->need_reload_context = false; 
}

void OpenCL_RD::ReloadKernelIfNeeded()
{
    if(!this->need_reload_formula) return;

    cl_int ret;

    // create the program
    this->kernel_source = this->AssembleKernelSourceFromFormula(this->formula);
    const char *source = this->kernel_source.c_str();
    size_t source_size = this->kernel_source.length()+1;
    cl_program program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : Failed to create program with source: ");

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
        ret = clGetProgramBuildInfo(program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : retrieving program build log failed: ");
        ostringstream oss;
        oss << "OpenCL_RD::ReloadKernelIfNeeded : build failed:\n\n" << build_log;
        throw runtime_error(oss.str().c_str());
    }

    // create the kernel
    this->kernel = clCreateKernel(program,this->kernel_function_name.c_str(),&ret);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : kernel creation failed: ");

    // decide the size of the work-groups
    const size_t X = this->GetX() / 4; // using float4 in kernels
    const size_t Y = this->GetY();
    const size_t Z = this->GetZ();
    this->global_range[0] = X;
    this->global_range[1] = Y;
    this->global_range[2] = Z;
    size_t wgs,returned_size;
    ret = clGetKernelWorkGroupInfo(this->kernel,this->device_id,CL_KERNEL_WORK_GROUP_SIZE,sizeof(size_t),&wgs,&returned_size);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : retrieving kernel work group size failed: ");
    // TODO: allow user override (this value isn't always optimal)
    if(wgs&(wgs-1)) 
        throw runtime_error("OpenCL_RD::ReloadKernelIfNeeded : expecting CL_KERNEL_WORK_GROUP_SIZE to be a power of 2");
    // spread the work group over the dimensions, preferring x over y and y over z because of memory alignment
    size_t wgx,wgy,wgz;
    wgx = min(X,wgs);
    wgy = min(Y,wgs/wgx);
    wgz = min(Z,wgs/(wgx*wgy));
    // TODO: give user control over the work group shape?
    if(X%wgx || Y%wgy || Z%wgz)
        throw runtime_error("OpenCL_RD::ReloadKernelIfNeeded : work group size doesn't divide into grid dimensions");
    this->local_range[0] = wgx;
    this->local_range[1] = wgy;
    this->local_range[2] = wgz;

    this->need_reload_formula = false;
}

void OpenCL_RD::CreateOpenCLBuffers()
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
            throwOnError(ret,"OpenCL_RD::CreateBuffers : buffer creation failed: ");
        }
    }
}

void OpenCL_RD::WriteToOpenCLBuffers()
{
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();

    const int io = 0;
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueWriteBuffer(this->command_queue,this->buffers[io][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::WriteToBuffers : buffer writing failed: ");
    }
}

void OpenCL_RD::ReadFromOpenCLBuffers()
{
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();

    const int io = 0;
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueReadBuffer(this->command_queue,this->buffers[io][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::ReadFromBuffers : buffer reading failed: ");
        this->images[ic]->Modified();
    }
}

void OpenCL_RD::Update2Steps()
{
    // (buffer-switching)

    cl_int ret;

    const int NC = this->GetNumberOfChemicals();

    for(int io=0;io<2;io++)
    {
        for(int ic=0;ic<NC;ic++)
        {
            ret = clSetKernelArg(this->kernel, io*NC+ic, sizeof(cl_mem), (void *)&this->buffers[io][ic]); // 0=input, 1=output
            throwOnError(ret,"OpenCL_RD::Update2Steps : clSetKernelArg failed: ");
        }
    }
    ret = clEnqueueNDRangeKernel(this->command_queue,this->kernel, 3, NULL, this->global_range, this->local_range, 0, NULL, NULL);
    throwOnError(ret,"OpenCL_RD::Update2Steps : clEnqueueNDRangeKernel failed: ");

    for(int io=0;io<2;io++)
    {
        for(int ic=0;ic<NC;ic++)
        {
            ret = clSetKernelArg(this->kernel, io*NC+ic, sizeof(cl_mem), (void *)&this->buffers[1-io][ic]); // 1=input, 0=output
            throwOnError(ret,"OpenCL_RD::Update2Steps : clSetKernelArg failed: ");
        }
    }
    ret = clEnqueueNDRangeKernel(this->command_queue,this->kernel, 3, NULL, this->global_range, this->local_range, 0, NULL, NULL);
    throwOnError(ret,"OpenCL_RD::Update2Steps : clEnqueueNDRangeKernel failed: ");
}

string Chem(int i) { return to_string((char)('a'+i)); } // a, b, c, ...

std::string OpenCL_RD::AssembleKernelSourceFromFormula(std::string formula) const
{
    const string indent = "    ";
    const int NC = this->GetNumberOfChemicals();
    const int NDIRS = 6;
    const string dir[NDIRS]={"left","right","up","down","fore","back"};

    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    // output the function definition
    kernel_source << "__kernel void rd_compute(";
    for(int i=0;i<NC;i++)
        kernel_source << "__global float4 *" << Chem(i) << "_in,";
    for(int i=0;i<NC;i++)
    {
        kernel_source << "__global float4 *" << Chem(i) << "_out";
        if(i<NC-1)
            kernel_source << ",";
    }
    // output the first part of the body
    kernel_source << ")\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int Z = get_global_size(2);\n\
    const int i = X*(Y*z + y) + x;\n\
\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "float4 " << Chem(i) << " = " << Chem(i) << "_in[i];\n"; // "float4 a = a_in[i];"
    // output the Laplacian part of the body
    kernel_source << "\
\n\
    // compute the Laplacians of each chemical\n\
    const int xm1 = ((x-1+X) & (X-1)); // wrap (assumes X is a power of 2)\n\
    const int xp1 = ((x+1) & (X-1));\n\
    const int ym1 = ((y-1+Y) & (Y-1));\n\
    const int yp1 = ((y+1) & (Y-1));\n\
    const int zm1 = ((z-1+Z) & (Z-1));\n\
    const int zp1 = ((z+1) & (Z-1));\n\
    const int i_left =  X*(Y*z + y) + xm1;\n\
    const int i_right = X*(Y*z + y) + xp1;\n\
    const int i_up =    X*(Y*z + ym1) + x;\n\
    const int i_down =  X*(Y*z + yp1) + x;\n\
    const int i_fore =  X*(Y*zm1 + y) + x;\n\
    const int i_back =  X*(Y*zp1 + y) + x;\n";
    for(int iC=0;iC<NC;iC++)
        for(int iDir=0;iDir<NDIRS;iDir++)
            kernel_source << indent << "float4 " << Chem(iC) << "_" << dir[iDir] << " = " << Chem(iC) << "_in[i_" << dir[iDir] << "];\n";
    for(int iC=0;iC<NC;iC++)
    {
        kernel_source << indent << "float4 laplacian_" << Chem(iC) << " = (float4)(" << Chem(iC) << "_up.x + " << Chem(iC) << ".y + " << Chem(iC) << "_down.x + " << Chem(iC) << "_left.w + " << Chem(iC) << "_fore.x + " << Chem(iC) << "_back.x,\n";
        kernel_source << indent << Chem(iC) << "_up.y + " << Chem(iC) << ".z + " << Chem(iC) << "_down.y + " << Chem(iC) << ".x + " << Chem(iC) << "_fore.y + " << Chem(iC) << "_back.y,\n";
        kernel_source << indent << Chem(iC) << "_up.z + " << Chem(iC) << ".w + " << Chem(iC) << "_down.z + " << Chem(iC) << ".y + " << Chem(iC) << "_fore.z + " << Chem(iC) << "_back.z,\n";
        kernel_source << indent << Chem(iC) << "_up.w + " << Chem(iC) << "_right.x + " << Chem(iC) << "_down.w + " << Chem(iC) << ".z + " << Chem(iC) << "_fore.w + " << Chem(iC) << "_back.w) - 6.0f*" << Chem(iC) << "\n";
        kernel_source << indent << "+ (float4)(1e-6f,1e-6f,1e-6f,1e-6f); // (kill denormals)\n";
    }
    kernel_source << "\n";
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << "float4 delta_" << Chem(iC) << ";\n";
    kernel_source << "\n";
    // the parameters (assume all float for now)
    for(int i=0;i<(int)this->parameters.size();i++)
        kernel_source << indent << "float " << this->parameters[i].first << " = " << this->parameters[i].second << "f;\n";
    // the timestep
    kernel_source << indent << "float delta_t = " << this->timestep << "f;\n";
    // the formula
    istringstream iss(formula);
    string s;
    while(iss.good())
    {
        getline(iss,s);
        kernel_source << indent << s << "\n";
    }
    // the last part of the kernel
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << Chem(iC) << "_out[i] = " << Chem(iC) << " + delta_t * delta_" << Chem(iC) << ";\n";
    kernel_source << "}\n";
    return kernel_source.str();
}

void OpenCL_RD::TestFormula(std::string formula)
{
    this->need_reload_context = true;
    this->ReloadContextIfNeeded();

    string kernel_source = this->AssembleKernelSourceFromFormula(formula);

    cl_int ret;

    // create the program
    const char *source = kernel_source.c_str();
    size_t source_size = kernel_source.length()+1;
    cl_program program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCL_RD::TestProgram : Failed to create program with source: ");

    // build the program
    ret = clBuildProgram(program,1,&this->device_id,NULL,NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        const int MAX_BUILD_LOG = 10000;
        char build_log[MAX_BUILD_LOG];
        size_t build_log_length;
        ret = clGetProgramBuildInfo(program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret,"OpenCL_RD::TestProgram : retrieving program build log failed: ");
        ostringstream oss;
        oss << "OpenCL_RD::TestProgram : build failed:\n\n" << build_log;
        throw runtime_error(oss.str().c_str());
    }
}

void OpenCL_RD::CopyFromImage(vtkImageData* im)
{
    BaseRD::CopyFromImage(im);
    this->WriteToOpenCLBuffers();
}

/* static */ std::string OpenCL_RD::GetOpenCLDiagnostics() // report on OpenCL without throwing exceptions
{
    // TODO: make this report more readable, retrieve numeric data too
    ostringstream report;

    if(LinkOpenCL()!= CL_SUCCESS)
    {
        report << "Failed to load dynamic library for OpenCL";
        return report.str();
    }

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    if(ret != CL_SUCCESS || num_platforms==0)
    {
        report << "No OpenCL platforms available";
        // currently only likely to see this when running in a virtualized OS, where an opencl.dll is found but doesn't work
        return report.str();
    }

    report << "Found " << num_platforms << " platform(s):\n";

    for(unsigned int iPlatform=0;iPlatform<num_platforms;iPlatform++)
    {
        report << "Platform " << iPlatform+1 << ":\n";
        const size_t MAX_INFO_LENGTH = 1000;
        char info[MAX_INFO_LENGTH];
        size_t info_length;
        for(cl_platform_info i=CL_PLATFORM_PROFILE;i<=CL_PLATFORM_EXTENSIONS;i++)
        {
            clGetPlatformInfo(platforms_available[iPlatform],i,MAX_INFO_LENGTH,info,&info_length);
            report << i << " : " << info << "\n";
        }

        const size_t MAX_DEVICES = 10;
        cl_device_id devices_available[MAX_DEVICES];
        cl_uint num_devices;
        clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
        report << "\nFound " << num_devices << " device(s) on this platform.\n";
        for(unsigned int iDevice=0;iDevice<num_devices;iDevice++)
        {
            report << "Device " << iDevice+1 << ":\n";
            for(cl_device_info i=CL_DEVICE_NAME;i<=CL_DEVICE_EXTENSIONS;i++)
            {
                clGetDeviceInfo(devices_available[iDevice],i,MAX_INFO_LENGTH,info,&info_length);
                report << i << " : " << info << "\n";
            }
        }
        report << "\n";
    }

    return report.str();
}

/* static */ int OpenCL_RD::GetNumberOfPlatforms()
{
    if(LinkOpenCL() != CL_SUCCESS)
        return 0;

    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_RD::GetNumberOfPlatforms : clGetPlatformIDs failed: ");
    return num_platforms;
}

/* static */ int OpenCL_RD::GetNumberOfDevices(int iPlatform)
{
    if(LinkOpenCL() != CL_SUCCESS)
        return 0;

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_RD::GetNumberOfDevices : clGetPlatformIDs failed: ");

    const size_t MAX_DEVICES = 10;
    cl_device_id devices_available[MAX_DEVICES];
    cl_uint num_devices;
    ret = clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
    throwOnError(ret,"OpenCL_RD::GetNumberOfDevices : clGetDeviceIDs failed: ");

    return num_devices;
}

/* static */ string OpenCL_RD::GetPlatformDescription(int iPlatform)
{
    LinkOpenCL();

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_RD::GetPlatformDescription : clGetPlatformIDs failed: ");

    ostringstream oss;
    const size_t MAX_INFO_LENGTH = 1000;
    char info[MAX_INFO_LENGTH];
    size_t info_length;
    ret = clGetPlatformInfo(platforms_available[iPlatform],CL_PLATFORM_NAME,
        MAX_INFO_LENGTH,info,&info_length);
    throwOnError(ret,"OpenCL_RD::GetPlatformDescription : clGetPlatformInfo failed: ");
    string platform_name = info;
    platform_name = platform_name.substr(platform_name.find_first_not_of(" \n\r\t"));
    oss << platform_name;
    return oss.str();
}

/* static */ string OpenCL_RD::GetDeviceDescription(int iPlatform,int iDevice)
{
    LinkOpenCL();

    // get available OpenCL platforms
    const size_t MAX_PLATFORMS = 10;
    cl_platform_id platforms_available[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int ret = clGetPlatformIDs(MAX_PLATFORMS,platforms_available,&num_platforms);
    throwOnError(ret,"OpenCL_RD::GetDeviceDescription : clGetPlatformIDs failed: ");

    const size_t MAX_INFO_LENGTH = 1000;
    char info[MAX_INFO_LENGTH];
    size_t info_length;

    ostringstream oss;
    const size_t MAX_DEVICES = 10;
    cl_device_id devices_available[MAX_DEVICES];
    cl_uint num_devices;
    ret = clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,
        MAX_DEVICES,devices_available,&num_devices);
    throwOnError(ret,"OpenCL_RD::GetDeviceDescription : clGetDeviceIDs failed: ");
    ret = clGetDeviceInfo(devices_available[iDevice],CL_DEVICE_NAME,
        MAX_INFO_LENGTH,info,&info_length);
    throwOnError(ret,"OpenCL_RD::GetDeviceDescription : clGetDeviceInfo failed: ");
    string device_name = info;
    device_name = device_name.substr(device_name.find_first_not_of(" \n\r\t"));
    oss << device_name;
    return oss.str();
}

/* static */ cl_int OpenCL_RD::LinkOpenCL()
{
#ifdef __APPLE__
    return CL_SUCCESS;
#else
    return clLibLoad();
#endif
}

// http://www.khronos.org/message_boards/viewtopic.php?f=37&t=2107
/* static */ const char* OpenCL_RD::descriptionOfError(cl_int err) 
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
