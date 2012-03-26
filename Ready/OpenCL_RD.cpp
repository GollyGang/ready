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
#include "OpenCL_utils.hpp"
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

    // initialise the opencl things to null in case we fail to create them
    this->device_id = NULL;
    this->context = NULL;
    this->command_queue = NULL;
    this->kernel = NULL;

    if(LinkOpenCL()!= CL_SUCCESS)
        throw runtime_error("Failed to load dynamic library for OpenCL");
        
    this->iCurrentBuffer = 0;
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

/* static */ void OpenCL_RD::throwOnError(cl_int ret,const char* message)
{
    if(ret == CL_SUCCESS) return;

    ostringstream oss;
    oss << message << GetDescriptionOfOpenCLError(ret);
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
    this->command_queue = clCreateCommandQueue(this->context,this->device_id,0,&ret);
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
    size_t source_size = this->kernel_source.length();
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
        cl_int ret2 = clGetProgramBuildInfo(program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret2,"OpenCL_RD::ReloadKernelIfNeeded : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCL_RD::ReloadKernelIfNeeded : build failed (kernel saved as kernel.txt):\n\n" << build_log;
        throwOnError(ret,oss.str().c_str());
    }

    // create the kernel
    this->kernel = clCreateKernel(program,this->kernel_function_name.c_str(),&ret);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : kernel creation failed: ");

    // decide the size of the work-groups
    const size_t X = max(1,this->GetX() / this->GetBlockSizeX());
    const size_t Y = max(1,this->GetY() / this->GetBlockSizeY());
    const size_t Z = max(1,this->GetZ() / this->GetBlockSizeZ());
    this->global_range[0] = X;
    this->global_range[1] = Y;
    this->global_range[2] = Z;
    size_t wgs,returned_size;
    ret = clGetKernelWorkGroupInfo(this->kernel,this->device_id,CL_KERNEL_WORK_GROUP_SIZE,sizeof(size_t),&wgs,&returned_size);
    throwOnError(ret,"OpenCL_RD::ReloadKernelIfNeeded : retrieving kernel work group size failed: ");
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

    this->iCurrentBuffer = 0;
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueWriteBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::WriteToBuffers : buffer writing failed: ");
    }
}

void OpenCL_RD::TestFormula(std::string formula)
{
    this->need_reload_context = true;
    this->ReloadContextIfNeeded();

    string kernel_source = this->AssembleKernelSourceFromFormula(formula);

    cl_int ret;

    // create the program
    const char *source = kernel_source.c_str();
    size_t source_size = kernel_source.length();
    cl_program program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCL_RD::TestProgram : Failed to create program with source: ");

    // build the program
    ret = clBuildProgram(program,1,&this->device_id,NULL,NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        const int MAX_BUILD_LOG = 10000;
        char build_log[MAX_BUILD_LOG];
        size_t build_log_length;
        cl_int ret2 = clGetProgramBuildInfo(program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret2,"OpenCL_RD::TestProgram : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCL_RD::TestProgram : build failed: (kernel saved as kernel.txt)\n\n" << build_log;
        throwOnError(ret,oss.str().c_str());
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
            report << GetPlatformInfoIdAsString(i) << " : " << info << "\n";
        }

        const size_t MAX_DEVICES = 10;
        cl_device_id devices_available[MAX_DEVICES];
        cl_uint num_devices;
        clGetDeviceIDs(platforms_available[iPlatform],CL_DEVICE_TYPE_ALL,MAX_DEVICES,devices_available,&num_devices);
        report << "\nFound " << num_devices << " device(s) on this platform.\n";

        const int N_CHAR_RETURNING_IDS = 6;
        cl_device_info charReturningIds[N_CHAR_RETURNING_IDS] = {CL_DEVICE_EXTENSIONS,CL_DEVICE_NAME,CL_DEVICE_PROFILE,CL_DEVICE_VENDOR,
            CL_DEVICE_VERSION,CL_DRIVER_VERSION};
        const int N_UINT_RETURNING_IDS = 13;
        cl_device_info uintReturningIds[N_UINT_RETURNING_IDS] = {CL_DEVICE_ADDRESS_BITS,CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,CL_DEVICE_MAX_CLOCK_FREQUENCY,
            CL_DEVICE_MAX_COMPUTE_UNITS,CL_DEVICE_MAX_CONSTANT_ARGS,CL_DEVICE_MAX_READ_IMAGE_ARGS,CL_DEVICE_MAX_SAMPLERS,CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
            CL_DEVICE_MAX_WRITE_IMAGE_ARGS,CL_DEVICE_MEM_BASE_ADDR_ALIGN,CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
            CL_DEVICE_VENDOR_ID};
        const int N_BOOL_RETURNING_IDS = 5;
        cl_device_info boolReturningIds[N_BOOL_RETURNING_IDS] = {CL_DEVICE_AVAILABLE,CL_DEVICE_COMPILER_AVAILABLE,CL_DEVICE_ENDIAN_LITTLE,CL_DEVICE_ERROR_CORRECTION_SUPPORT,
            CL_DEVICE_IMAGE_SUPPORT};
        const int N_ULONG_RETURNING_IDS = 5;
        cl_device_info ulongReturningIds[N_ULONG_RETURNING_IDS] = {CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,CL_DEVICE_GLOBAL_MEM_SIZE,CL_DEVICE_LOCAL_MEM_SIZE,CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,
            CL_DEVICE_MAX_MEM_ALLOC_SIZE};
        const int N_SIZET_RETURNING_IDS = 8;
        cl_device_info sizetReturningIds[N_SIZET_RETURNING_IDS] = {CL_DEVICE_IMAGE2D_MAX_HEIGHT,CL_DEVICE_IMAGE2D_MAX_WIDTH,CL_DEVICE_IMAGE3D_MAX_DEPTH,CL_DEVICE_IMAGE3D_MAX_HEIGHT,
            CL_DEVICE_IMAGE3D_MAX_WIDTH,CL_DEVICE_MAX_PARAMETER_SIZE,CL_DEVICE_MAX_WORK_GROUP_SIZE,CL_DEVICE_PROFILING_TIMER_RESOLUTION};

        cl_uint uint_value;
        cl_bool bool_value;
        cl_ulong ulong_value;
        size_t sizet_value;
        for(unsigned int iDevice=0;iDevice<num_devices;iDevice++)
        {
            report << "Device " << iDevice+1 << ":\n";
            for(int i=0;i<N_CHAR_RETURNING_IDS;i++)
            {
                clGetDeviceInfo(devices_available[iDevice],charReturningIds[i],MAX_INFO_LENGTH,info,&info_length);
                report << GetDeviceInfoIdAsString(charReturningIds[i]) << " : " << info << "\n";
            }
            for(int i=0;i<N_UINT_RETURNING_IDS;i++)
            {
                clGetDeviceInfo(devices_available[iDevice],uintReturningIds[i],sizeof(uint_value),&uint_value,&info_length);
                report << GetDeviceInfoIdAsString(uintReturningIds[i]) << " : " << uint_value << "\n";
            }
            for(int i=0;i<N_BOOL_RETURNING_IDS;i++)
            {
                clGetDeviceInfo(devices_available[iDevice],boolReturningIds[i],sizeof(bool_value),&bool_value,&info_length);
                report << GetDeviceInfoIdAsString(boolReturningIds[i]) << " : " << (bool_value?"yes":"no") << "\n";
            }
            for(int i=0;i<N_ULONG_RETURNING_IDS;i++)
            {
                clGetDeviceInfo(devices_available[iDevice],ulongReturningIds[i],sizeof(ulong_value),&ulong_value,&info_length);
                report << GetDeviceInfoIdAsString(ulongReturningIds[i]) << " : " << ulong_value << "\n";
            }
            for(int i=0;i<N_SIZET_RETURNING_IDS;i++)
            {
                clGetDeviceInfo(devices_available[iDevice],sizetReturningIds[i],sizeof(sizet_value),&sizet_value,&info_length);
                report << GetDeviceInfoIdAsString(sizetReturningIds[i]) << " : " << sizet_value << "\n";
            }
            // CL_DEVICE_MAX_WORK_ITEM_SIZES:
            size_t dim[3];
            clGetDeviceInfo(devices_available[iDevice],CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(dim),dim,&info_length);
            report << GetDeviceInfoIdAsString(CL_DEVICE_MAX_WORK_ITEM_SIZES) << " : " << dim[0] << ", " << dim[1] << ", " << dim[2] << "\n";
            // CL_DEVICE_TYPE:
            cl_device_type device_type;
            clGetDeviceInfo(devices_available[iDevice],CL_DEVICE_TYPE,sizeof(device_type),&device_type,&info_length);
            ostringstream oss;
            if(device_type&CL_DEVICE_TYPE_CPU) oss << "CPU";
            if(device_type&CL_DEVICE_TYPE_GPU) { if(!oss.str().empty()) { oss << " & "; } oss << "GPU"; }
            if(device_type&CL_DEVICE_TYPE_ACCELERATOR) { if(!oss.str().empty()) { oss << " & "; } oss << "ACCELERATOR"; }
            if(device_type&CL_DEVICE_TYPE_DEFAULT) { if(!oss.str().empty()) { oss << " & "; } oss << "DEFAULT"; }
            report << GetDeviceInfoIdAsString(CL_DEVICE_TYPE) << " : " << oss.str() << "\n";
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

void OpenCL_RD::SetParameterValue(int iParam,float val)
{
    BaseRD::SetParameterValue(iParam,val);
    this->need_reload_formula = true;
}

void OpenCL_RD::SetParameterName(int iParam,const string& s)
{
    BaseRD::SetParameterName(iParam,s);
    this->need_reload_formula = true;
}

void OpenCL_RD::AddParameter(const std::string& name,float val)
{
    BaseRD::AddParameter(name,val);
    this->need_reload_formula = true;
}

void OpenCL_RD::DeleteParameter(int iParam)
{
    BaseRD::DeleteParameter(iParam);
    this->need_reload_formula = true;
}

void OpenCL_RD::DeleteAllParameters()
{
    BaseRD::DeleteAllParameters();
    this->need_reload_formula = true;
}

void OpenCL_RD::GenerateInitialPattern()
{
	BaseRD::GenerateInitialPattern();
    this->WriteToOpenCLBuffers();
}

void OpenCL_RD::BlankImage()
{
	BaseRD::BlankImage();
    this->WriteToOpenCLBuffers();
}

void OpenCL_RD::Allocate(int x,int y,int z,int nc)
{
    if(x&(x-1) || y&(y-1) || z&(z-1))
        throw runtime_error("OpenCL_RD::Allocate : for wrap-around in OpenCL we require all the dimensions to be powers of 2");
    BaseRD::Allocate(x,y,z,nc);
    this->need_reload_formula = true;
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->CreateOpenCLBuffers();
}

void OpenCL_RD::Update(int n_steps)
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
                throwOnError(ret,"OpenCL_RD::Update : clSetKernelArg failed: ");
            }
        }
        ret = clEnqueueNDRangeKernel(this->command_queue,this->kernel, 3, NULL, this->global_range, this->local_range, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::Update : clEnqueueNDRangeKernel failed: ");
        this->iCurrentBuffer = 1 - this->iCurrentBuffer;
    }
    this->timesteps_taken += n_steps;

    // read from opencl buffers into our image
    const unsigned long MEM_SIZE = sizeof(float) * this->GetX() * this->GetY() * this->GetZ();
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = static_cast<float*>(this->images[ic]->GetScalarPointer());
        cl_int ret = clEnqueueReadBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCL_RD::Update : buffer reading failed: ");
        this->images[ic]->Modified();
    }
}
