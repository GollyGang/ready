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
#include "ImageRD.hpp"
#include "OpenCL_MixIn.hpp"

/// Base class for implementations that use OpenCL.
class OpenCLImageRD : public ImageRD, public OpenCL_MixIn
{
    public:

        OpenCLImageRD();
        virtual ~OpenCLImageRD();

        virtual bool HasEditableFormula() const { return true; }

        virtual void TestFormula(std::string s);

		virtual void GenerateInitialPattern();
		virtual void BlankImage();
		
    protected:

        virtual void CopyFromImage(vtkImageData* im);

        virtual void AllocateImages(int x,int y,int z,int nc);

        virtual void InternalUpdate(int n_steps);

        virtual std::string AssembleKernelSourceFromFormula(std::string formula) const =0;

        void ReloadKernelIfNeeded();

        void CreateOpenCLBuffers();
        void WriteToOpenCLBuffers();

    protected:

        std::string kernel_source;

        std::vector<cl_mem> buffers[2];
        int iCurrentBuffer;
};

#endif
