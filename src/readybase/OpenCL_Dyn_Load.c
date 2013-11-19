/*
 * Copyright (C) 2010 - Alexandru Gagniuc - <mr.nuke.me@gmail.com>
 * This file is part of ElectroMag.
 *
 * ElectroMag is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ElectroMag is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with ElectroMag.  If not, see <http://www.gnu.org/licenses/>.
 */
// Source: http://code.google.com/p/electromag-with-cuda/source/browse/trunk/GPGPU_Segment/src/OpenCL_Dyn_Load.c

#if !defined(__APPLE__)
// (dynamic loading of OpenCL is not necessary with Mac OS 10.6+)

#include "OpenCL_Dyn_Load.h"

__clGetPlatformIDs                   *clGetPlatformIDs;
__clGetPlatformInfo                  *clGetPlatformInfo;
__clGetDeviceIDs                     *clGetDeviceIDs;
__clGetDeviceInfo                    *clGetDeviceInfo;
__clCreateContext                    *clCreateContext;
__clCreateContextFromType            *clCreateContextFromType;
__clRetainContext                    *clRetainContext;
__clReleaseContext                   *clReleaseContext;
__clGetContextInfo                   *clGetContextInfo;
__clCreateCommandQueue               *clCreateCommandQueue;
__clRetainCommandQueue               *clRetainCommandQueue;
__clReleaseCommandQueue              *clReleaseCommandQueue;
__clGetCommandQueueInfo              *clGetCommandQueueInfo;
__clCreateBuffer                     *clCreateBuffer;
__clCreateImage2D                    *clCreateImage2D;
__clCreateImage3D                    *clCreateImage3D;
__clRetainMemObject                  *clRetainMemObject;
__clReleaseMemObject                 *clReleaseMemObject;
__clGetSupportedImageFormats         *clGetSupportedImageFormats;
__clGetMemObjectInfo                 *clGetMemObjectInfo;
__clGetImageInfo                     *clGetImageInfo;
__clCreateSampler                    *clCreateSampler;
__clRetainSampler                    *clRetainSampler;
__clReleaseSampler                   *clReleaseSampler;
__clGetSamplerInfo                   *clGetSamplerInfo;
__clCreateProgramWithSource          *clCreateProgramWithSource;
__clCreateProgramWithBinary          *clCreateProgramWithBinary;
__clRetainProgram                    *clRetainProgram;
__clReleaseProgram                   *clReleaseProgram;
__clBuildProgram                     *clBuildProgram;
__clUnloadCompiler                   *clUnloadCompiler;
__clGetProgramInfo                   *clGetProgramInfo;
__clGetProgramBuildInfo              *clGetProgramBuildInfo;
__clCreateKernel                     *clCreateKernel;
__clCreateKernelsInProgram           *clCreateKernelsInProgram;
__clRetainKernel                     *clRetainKernel;
__clReleaseKernel                    *clReleaseKernel;
__clSetKernelArg                     *clSetKernelArg;
__clGetKernelInfo                    *clGetKernelInfo;
__clGetKernelWorkGroupInfo           *clGetKernelWorkGroupInfo;
__clWaitForEvents                    *clWaitForEvents;
__clGetEventInfo                     *clGetEventInfo;
__clRetainEvent                      *clRetainEvent;
__clReleaseEvent                     *clReleaseEvent;
__clGetEventProfilingInfo            *clGetEventProfilingInfo;
__clFlush                            *clFlush;
__clFinish                           *clFinish;
__clEnqueueReadBuffer                *clEnqueueReadBuffer;
__clEnqueueWriteBuffer               *clEnqueueWriteBuffer;
__clEnqueueCopyBuffer                *clEnqueueCopyBuffer;
__clEnqueueReadImage                 *clEnqueueReadImage;
__clEnqueueWriteImage                *clEnqueueWriteImage;
__clEnqueueCopyImage                 *clEnqueueCopyImage;
__clEnqueueCopyImageToBuffer         *clEnqueueCopyImageToBuffer;
__clEnqueueCopyBufferToImage         *clEnqueueCopyBufferToImage;
__clEnqueueMapBuffer                 *clEnqueueMapBuffer;
__clEnqueueMapImage                  *clEnqueueMapImage;
__clEnqueueUnmapMemObject            *clEnqueueUnmapMemObject;
__clEnqueueNDRangeKernel             *clEnqueueNDRangeKernel;
__clEnqueueTask                      *clEnqueueTask;
__clEnqueueNativeKernel              *clEnqueueNativeKernel;
__clEnqueueMarker                    *clEnqueueMarker;
__clEnqueueWaitForEvents             *clEnqueueWaitForEvents;
__clEnqueueBarrier                   *clEnqueueBarrier;
__clGetExtensionFunctionAddress      *clGetExtensionFunctionAddress;

/* OpenCL 1.1 stuff */
/* TJH commented this out, to avoid requiring 1.1
__clCreateSubBuffer                  *clCreateSubBuffer;
__clSetMemObjectDestructorCallback   *clSetMemObjectDestructorCallback;
__clCreateUserEvent                  *clCreateUserEvent;
__clSetUserEventStatus               *clSetUserEventStatus;
__clSetEventCallback                 *clSetEventCallback;
__clEnqueueReadBufferRect            *clEnqueueReadBufferRect;
__clEnqueueWriteBufferRect           *clEnqueueWriteBufferRect;
__clEnqueueCopyBufferRect            *clEnqueueCopyBufferRect;
*/

#if defined(_WIN32) || defined(_WIN64)

#include <Windows.h>

#ifdef UNICODE
static LPCWSTR __ClLibName = L"OpenCl.dll";
#else
static LPCSTR __ClLibName = "OpenCl.dll";
#endif

typedef HMODULE CL_LIBRARY;

cl_int CL_LOAD_LIBRARY(CL_LIBRARY *pInstance)
{
    *pInstance = LoadLibrary(__ClLibName);
    if (*pInstance == NULL)
    {
        return CL_DEVICE_NOT_FOUND;
    }
    return CL_SUCCESS;
}

#define GET_PROC(name)                                          \
        name = (__##name *)GetProcAddress(ClLib, #name);        \
        if (name == NULL) return CL_DEVICE_NOT_AVAILABLE

#elif defined(__unix__) || defined(__APPLE__) || defined(__MACOSX)

#include <dlfcn.h>

#if defined(__APPLE__) || defined(__MACOSX)
static char __ClLibName[] = "/usr/lib/libOpenCL.dylib";
#else
static char __ClLibName[] = "libOpenCL.so";
#endif

typedef void * CL_LIBRARY;

cl_int CL_LOAD_LIBRARY(CL_LIBRARY *pInstance)
{
    *pInstance = dlopen(__ClLibName, RTLD_NOW);
    if (*pInstance == NULL)
    {
        return CL_DEVICE_NOT_FOUND;
    }
    return CL_SUCCESS;
}

#define GET_PROC(name)                                          \
        name = (__##name *)(size_t)dlsym(ClLib, #name);                 \
        if (name == NULL) return CL_DEVICE_NOT_AVAILABLE

#endif


cl_int CL_API_CALL clLibLoad()
{
    CL_LIBRARY ClLib;
    cl_int result;
    result = CL_LOAD_LIBRARY(&ClLib);
    if (result != CL_SUCCESS)
    {
        return result;
    }
    GET_PROC(clGetPlatformIDs                   );
    GET_PROC(clGetPlatformInfo                  );
    GET_PROC(clGetDeviceIDs                     );
    GET_PROC(clGetDeviceInfo                    );
    GET_PROC(clCreateContext                    );
    GET_PROC(clCreateContextFromType            );
    GET_PROC(clRetainContext                    );
    GET_PROC(clReleaseContext                   );
    GET_PROC(clGetContextInfo                   );
    GET_PROC(clCreateCommandQueue               );
    GET_PROC(clRetainCommandQueue               );
    GET_PROC(clReleaseCommandQueue              );
    GET_PROC(clGetCommandQueueInfo              );
    GET_PROC(clCreateBuffer                     );
    GET_PROC(clCreateImage2D                    );
    GET_PROC(clCreateImage3D                    );
    GET_PROC(clRetainMemObject                  );
    GET_PROC(clReleaseMemObject                 );
    GET_PROC(clGetSupportedImageFormats         );
    GET_PROC(clGetMemObjectInfo                 );
    GET_PROC(clGetImageInfo                     );
    GET_PROC(clCreateSampler                    );
    GET_PROC(clRetainSampler                    );
    GET_PROC(clReleaseSampler                   );
    GET_PROC(clGetSamplerInfo                   );
    GET_PROC(clCreateProgramWithSource          );
    GET_PROC(clCreateProgramWithBinary          );
    GET_PROC(clRetainProgram                    );
    GET_PROC(clReleaseProgram                   );
    GET_PROC(clBuildProgram                     );
    GET_PROC(clUnloadCompiler                   );
    GET_PROC(clGetProgramInfo                   );
    GET_PROC(clGetProgramBuildInfo              );
    GET_PROC(clCreateKernel                     );
    GET_PROC(clCreateKernelsInProgram           );
    GET_PROC(clRetainKernel                     );
    GET_PROC(clReleaseKernel                    );
    GET_PROC(clSetKernelArg                     );
    GET_PROC(clGetKernelInfo                    );
    GET_PROC(clGetKernelWorkGroupInfo           );
    GET_PROC(clWaitForEvents                    );
    GET_PROC(clGetEventInfo                     );
    GET_PROC(clRetainEvent                      );
    GET_PROC(clReleaseEvent                     );
    GET_PROC(clGetEventProfilingInfo            );
    GET_PROC(clFlush                            );
    GET_PROC(clFinish                           );
    GET_PROC(clEnqueueReadBuffer                );
    GET_PROC(clEnqueueWriteBuffer               );
    GET_PROC(clEnqueueCopyBuffer                );
    GET_PROC(clEnqueueReadImage                 );
    GET_PROC(clEnqueueWriteImage                );
    GET_PROC(clEnqueueCopyImage                 );
    GET_PROC(clEnqueueCopyImageToBuffer         );
    GET_PROC(clEnqueueCopyBufferToImage         );
    GET_PROC(clEnqueueMapBuffer                 );
    GET_PROC(clEnqueueMapImage                  );
    GET_PROC(clEnqueueUnmapMemObject            );
    GET_PROC(clEnqueueNDRangeKernel             );
    GET_PROC(clEnqueueTask                      );
    GET_PROC(clEnqueueNativeKernel              );
    GET_PROC(clEnqueueMarker                    );
    GET_PROC(clEnqueueWaitForEvents             );
    GET_PROC(clEnqueueBarrier                   );
    GET_PROC(clGetExtensionFunctionAddress      );

    /* Load OpenCL 1.1  stuff*/
    // TJH commented this all out, to avoid requiring 1.1
    //GET_PROC(clCreateSubBuffer                  );
    //GET_PROC(clSetMemObjectDestructorCallback   );
    //GET_PROC(clCreateUserEvent                  );
    //GET_PROC(clSetUserEventStatus               );
    //GET_PROC(clSetEventCallback                 );
    //GET_PROC(clEnqueueReadBufferRect            );
    //GET_PROC(clEnqueueWriteBufferRect           );
    //GET_PROC(clEnqueueCopyBufferRect            );

    return CL_SUCCESS;
}

#endif