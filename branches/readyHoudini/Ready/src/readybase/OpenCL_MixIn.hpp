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

#ifndef __OPENCLMIXIN__
#define __OPENCLMIXIN__

// OpenCL:
#if defined( __APPLE__ )
    // OpenCL is linked at start up time on Mac OS 10.6+
    #include <OpenCL/opencl.h>
#elif defined( __EXTERNAL_OPENCL__ )
	// compiling for use as a plugin into an app that provides its own openCL context.
	#include <CL/opencl.h>
#else
    // OpenCL is loaded dynamically on Windows and Linux
    #include "OpenCL_Dyn_Load.h"
#endif

// STL:
#include <vector>
#include <string>

/// OpenCL functionality, for adding to those implementations that use it.
class OpenCL_MixIn
{
    public:

        OpenCL_MixIn(int opencl_platform,int opencl_device);
		OpenCL_MixIn(int opencl_platform,int opencl_device, cl_context externalContext);
        virtual ~OpenCL_MixIn();
    
        void SetPlatform(int i);
        void SetDevice(int i);
        int GetPlatform() const;
        int GetDevice() const;

    protected:

        virtual std::string AssembleKernelSourceFromFormula(std::string formula) const =0;
        void ImportExternalContext( cl_context externalContext );
        void ReloadContextIfNeeded();
        virtual void ReloadKernelIfNeeded() =0;

        virtual void CreateOpenCLBuffers() =0;
        virtual void WriteToOpenCLBuffersIfNeeded() =0;
        virtual void ReadFromOpenCLBuffers() =0;
        virtual void ReleaseOpenCLBuffers();

        /// Test a kernel string for errors on the current device.
        void TestKernel(std::string s);

    protected:

        cl_context context;
        cl_device_id device_id;
        cl_program program;
        cl_kernel kernel;
        std::string kernel_function_name;
        size_t global_range[3];

        cl_command_queue command_queue;

        bool need_reload_context,external_context,need_write_to_opencl_buffers;

        std::vector<cl_mem> buffers[2];
        int iCurrentBuffer;

        std::string kernel_source;

    private:

        int iPlatform,iDevice;
};

#endif
