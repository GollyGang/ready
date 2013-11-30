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

#ifndef __OPENCLIMAGERD__
#define __OPENCLIMAGERD__

// local:
#include "ImageRD.hpp"
#include "OpenCL_MixIn.hpp"

/// Base class for implementations that use OpenCL.
class OpenCLImageRD : public ImageRD, public OpenCL_MixIn
{
    public:

        OpenCLImageRD(int opencl_platform,int opencl_device);
        OpenCLImageRD(int opencl_platform,int opencl_device, cl_context externalContext);

        virtual bool HasEditableFormula() const { return true; }

        virtual void GenerateInitialPattern();
        virtual void BlankImage();

        virtual void TestFormula(std::string program_string);

        virtual std::string GetKernel() const { return this->AssembleKernelSourceFromFormula(this->formula); } 

        virtual void SetValue(float x,float y,float z,float val,const Properties& render_settings);
        virtual void SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings);

        virtual void Undo();
        virtual void Redo();
        virtual void GetFromOpenCLBuffers( float* dest, int chemical_id );

    protected:

        virtual void CopyFromImage(vtkImageData* im);

        virtual void AllocateImages(int x,int y,int z,int nc);

        virtual void InternalUpdate(int n_steps);

        virtual void ReloadKernelIfNeeded();

        virtual void CreateOpenCLBuffers();
        virtual void WriteToOpenCLBuffersIfNeeded();
        virtual void ReadFromOpenCLBuffers();
};

#endif
