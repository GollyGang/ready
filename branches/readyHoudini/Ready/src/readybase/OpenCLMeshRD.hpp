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

#ifndef __OPENCLMESHRD__
#define __OPENCLMESHRD__

// local:
#include "MeshRD.hpp"
#include "OpenCL_MixIn.hpp"

/// Base class for mesh implementations that use OpenCL.
class OpenCLMeshRD : public MeshRD, public OpenCL_MixIn
{
    public:

        OpenCLMeshRD(int opencl_platform,int opencl_device);
        virtual ~OpenCLMeshRD();

        virtual void SetNumberOfChemicals(int n);

        virtual bool HasEditableFormula() const { return true; }

        virtual void CopyFromMesh(vtkUnstructuredGrid* mesh2);

        // we override the parameter access functions because changing the parameters requires rewriting the kernel
        virtual void AddParameter(const std::string& name,float val);
        virtual void DeleteParameter(int iParam);
        virtual void DeleteAllParameters();
        virtual void SetParameterName(int iParam,const std::string& s);
        virtual void SetParameterValue(int iParam,float val);

        virtual void GenerateInitialPattern();
        virtual void BlankImage();

        virtual void TestFormula(std::string program_string);
        virtual std::string GetKernel() const { return this->AssembleKernelSourceFromFormula(this->formula); } 

        virtual void SetValue(float x,float y,float z,float val,const Properties& render_settings);
        virtual void SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings);
        virtual void GetFromOpenCLBuffers( float* dest, int chemical_id );
        virtual void Undo();
        virtual void Redo();

    protected:

        virtual void InternalUpdate(int n_steps);

        virtual void ReloadKernelIfNeeded();

        virtual void CreateOpenCLBuffers();
        virtual void WriteToOpenCLBuffersIfNeeded();
        virtual void ReadFromOpenCLBuffers();
        virtual void ReleaseOpenCLBuffers();

    private:

        cl_mem clBuffer_cell_neighbor_indices;
        cl_mem clBuffer_cell_neighbor_weights;
};

#endif
