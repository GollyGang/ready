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

#ifndef __OPENCL_RD__
#define __OPENCL_RD__

// local:
#include "BaseRD.hpp"

// OpenCL:
#ifdef __APPLE__
    // OpenCL is linked at start up time on Mac OS 10.6+
    #include <OpenCL/opencl.h>
#else
    // OpenCL is loaded dynamically on Windows and Linux
    #include "OpenCL_Dyn_Load.h"
#endif

// base class for those RD implementations that use OpenCL
class OpenCL_RD : public BaseRD
{
    public:

        OpenCL_RD();
        ~OpenCL_RD();

        virtual bool HasEditableFormula() const { return true; }

        void SetPlatform(int i);
        void SetDevice(int i);
        int GetPlatform() const;
        int GetDevice() const;
        static std::string GetOpenCLDiagnostics();
        static int GetNumberOfPlatforms();
        static int GetNumberOfDevices(int iPlatform);
        static std::string GetPlatformDescription(int iPlatform);
        static std::string GetDeviceDescription(int iPlatform,int iDevice);

        virtual void SetParameterValue(int iParam,float val);
        virtual void SetParameterName(int iParam,std::string s);
        virtual void SetTimestep(float t);

        virtual void TestFormula(std::string s);

        virtual void CopyFromImage(vtkImageData* im);

		virtual void GenerateInitialPattern();
		virtual void BlankImage();
		
        virtual void Allocate(int x,int y,int z,int nc);

        virtual void Update(int n_steps);

    protected:

        virtual std::string AssembleKernelSourceFromFormula(std::string formula) const =0;

        void ReloadContextIfNeeded();
        void ReloadKernelIfNeeded();

        void CreateOpenCLBuffers();
        void WriteToOpenCLBuffers();
        void ReadFromOpenCLBuffers();

        void Update2Steps();

        static cl_int LinkOpenCL();
        static void throwOnError(cl_int ret,const char* message);
        static const char* descriptionOfError(cl_int err);

    protected:

        std::string kernel_source;

    private:

        // OpenCL things for re-use
        cl_device_id device_id;
        cl_context context;
        cl_command_queue command_queue;
        cl_kernel kernel;
        std::vector<cl_mem> buffers[2];
        size_t global_range[3],local_range[3];

        std::string kernel_function_name;

        int iPlatform,iDevice;
        bool need_reload_context;
};

#endif
