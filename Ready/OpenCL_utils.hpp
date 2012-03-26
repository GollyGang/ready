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

// OpenCL:
#ifdef __APPLE__
    // OpenCL is linked at start up time on Mac OS 10.6+
    #include <OpenCL/opencl.h>
    // we need these defs from OpenCL_Dyn_Load.h because they
    // are not in the OpenCL headers in Mac OS 10.6
    #define CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF       0x1034
    #define CL_DEVICE_HOST_UNIFIED_MEMORY               0x1035
    #define CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR          0x1036
    #define CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT         0x1037
    #define CL_DEVICE_NATIVE_VECTOR_WIDTH_INT           0x1038
    #define CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG          0x1039
    #define CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT         0x103A
    #define CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE        0x103B
    #define CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF          0x103C
    #define CL_DEVICE_OPENCL_C_VERSION                  0x103D
#else
    // OpenCL is loaded dynamically on Windows and Linux
    #include "OpenCL_Dyn_Load.h"
#endif

// STL:
#include <string>

std::string GetOpenCLDiagnostics();
int GetNumberOfPlatforms();
int GetNumberOfDevices(int iPlatform);
std::string GetPlatformDescription(int iPlatform);
std::string GetDeviceDescription(int iPlatform,int iDevice);
const char* GetDescriptionOfOpenCLError(cl_int err);
const char* GetPlatformInfoIdAsString(cl_platform_info i);
const char* GetDeviceInfoIdAsString(cl_device_info i);
cl_int LinkOpenCL();
void throwOnError(cl_int ret,const char* message);
