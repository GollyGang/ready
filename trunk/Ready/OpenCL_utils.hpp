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
#else
    // OpenCL is loaded dynamically on Windows and Linux
    #include "OpenCL_Dyn_Load.h"
#endif

const char* GetDescriptionOfOpenCLError(cl_int err);
const char* GetPlatformInfoIdAsString(cl_platform_info i);
const char* GetDeviceInfoIdAsString(cl_device_info i);

