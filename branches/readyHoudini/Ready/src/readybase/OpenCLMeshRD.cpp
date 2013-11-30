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

// local:
#include "OpenCLMeshRD.hpp"
#include "OpenCL_utils.hpp"
using namespace OpenCL_utils;
#include "utils.hpp"

// STL:
#include <string>
#include <sstream>
using namespace std;

// VTK:
#include <vtkMath.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>

// -------------------------------------------------------------------------

OpenCLMeshRD::OpenCLMeshRD(int opencl_platform,int opencl_device)
    : OpenCL_MixIn(opencl_platform,opencl_device)
{
    this->clBuffer_cell_neighbor_indices = NULL;
    this->clBuffer_cell_neighbor_weights = NULL;
}

// -------------------------------------------------------------------------

OpenCLMeshRD::~OpenCLMeshRD()
{
    clReleaseMemObject(this->clBuffer_cell_neighbor_indices);
    clReleaseMemObject(this->clBuffer_cell_neighbor_weights);
}

// -------------------------------------------------------------------------

void OpenCLMeshRD::SetNumberOfChemicals(int n)
{
    MeshRD::SetNumberOfChemicals(n);
    this->need_reload_formula = true;
    this->CreateOpenCLBuffers();
}

// -------------------------------------------------------------------------

void OpenCLMeshRD::SetParameterValue(int iParam,float val)
{
    AbstractRD::SetParameterValue(iParam,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void OpenCLMeshRD::SetParameterName(int iParam,const string& s)
{
    AbstractRD::SetParameterName(iParam,s);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void OpenCLMeshRD::AddParameter(const std::string& name,float val)
{
    AbstractRD::AddParameter(name,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void OpenCLMeshRD::DeleteParameter(int iParam)
{
    AbstractRD::DeleteParameter(iParam);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void OpenCLMeshRD::DeleteAllParameters()
{
    AbstractRD::DeleteAllParameters();
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void OpenCLMeshRD::InternalUpdate(int n_steps)
{
    this->ReloadContextIfNeeded();
    this->ReloadKernelIfNeeded();
    this->WriteToOpenCLBuffersIfNeeded();

    cl_int ret;
    int iBuffer;
    const int NC = this->GetNumberOfChemicals();

    // pass the neighbor indices and weights as parameters for the kernel
    ret = clSetKernelArg(this->kernel, 2*NC + 0, sizeof(cl_mem), (void *)&this->clBuffer_cell_neighbor_indices);
    throwOnError(ret,"OpenCLMeshRD::InternalUpdate : clSetKernelArg failed on indices array: ");
    ret = clSetKernelArg(this->kernel, 2*NC + 1, sizeof(cl_mem), (void *)&this->clBuffer_cell_neighbor_weights);
    throwOnError(ret,"OpenCLMeshRD::InternalUpdate : clSetKernelArg failed on weights array: ");
    ret = clSetKernelArg(this->kernel, 2*NC + 2, sizeof(int), &this->max_neighbors);
    throwOnError(ret,"OpenCLMeshRD::InternalUpdate : clSetKernelArg failed on max_neighbors parameter: ");

    for(int it=0;it<n_steps;it++)
    {
        for(int io=0;io<2;io++) // first input buffers (io=0) then output buffers (io=1)
        {
            iBuffer = (this->iCurrentBuffer+io)%2;
            for(int ic=0;ic<NC;ic++)
            {
                // a_in, b_in, ... a_out, b_out ...
                ret = clSetKernelArg(this->kernel, io*NC+ic, sizeof(cl_mem), &this->buffers[iBuffer][ic]);
                throwOnError(ret,"OpenCLMeshRD::InternalUpdate : clSetKernelArg failed on buffer: ");
            }
        }
        ret = clEnqueueNDRangeKernel(this->command_queue,this->kernel, 3, NULL, this->global_range, NULL, 0, NULL, NULL);
        throwOnError(ret,"OpenCLMeshRD::InternalUpdate : clEnqueueNDRangeKernel failed: ");
        this->iCurrentBuffer = 1 - this->iCurrentBuffer;
    }

    this->ReadFromOpenCLBuffers();
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::ReloadKernelIfNeeded()
{
    if(!this->need_reload_formula) return;

    if(this->n_chemicals==0)
        throw runtime_error("OpenCLMeshRD::ReloadKernelIfNeeded : zero chemicals");

    cl_int ret;

    // create the program
    this->kernel_source = this->AssembleKernelSourceFromFormula(this->formula);
    const char *source = this->kernel_source.c_str();
    size_t source_size = this->kernel_source.length();
    clReleaseProgram(this->program);
    this->program = clCreateProgramWithSource(this->context,1,&source,&source_size,&ret);
    throwOnError(ret,"OpenCLMeshRD::ReloadKernelIfNeeded : Failed to create program with source: ");

    // build the program
    ret = clBuildProgram(this->program,1,&this->device_id,"-cl-denorms-are-zero -cl-fast-relaxed-math",NULL,NULL);
    if(ret != CL_SUCCESS)
    {
        const int MAX_BUILD_LOG = 10000;
        char build_log[MAX_BUILD_LOG];
        size_t build_log_length;
        cl_int ret2 = clGetProgramBuildInfo(this->program,this->device_id,CL_PROGRAM_BUILD_LOG,MAX_BUILD_LOG,build_log,&build_log_length);
        throwOnError(ret2,"OpenCLMeshRD::ReloadKernelIfNeeded : retrieving program build log failed: ");
        { ofstream out("kernel.txt"); out << kernel_source; }
        ostringstream oss;
        oss << "OpenCLMeshRD::ReloadKernelIfNeeded : build failed (kernel saved as kernel.txt):\n\n" << build_log;
        throwOnError(ret,oss.str().c_str());
    }

    // create the kernel
    clReleaseKernel(this->kernel);
    this->kernel = clCreateKernel(this->program,this->kernel_function_name.c_str(),&ret);
    throwOnError(ret,"OpenCLMeshRD::ReloadKernelIfNeeded : kernel creation failed: ");

    // TODO: round this up to an abundant number to enable many choices for division by local workgroup range?
    this->global_range[0] = this->mesh->GetNumberOfCells();
    this->global_range[1] = 1;
    this->global_range[2] = 1;
    // (we let the local work group size be automatically decided, seems to be faster and more flexible that way)

    this->need_reload_formula = false;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::CreateOpenCLBuffers()
{
    this->ReloadContextIfNeeded();

    const unsigned long MEM_SIZE = sizeof(float) * (unsigned long)this->mesh->GetNumberOfCells();
    const int NC = this->GetNumberOfChemicals();

    this->ReleaseOpenCLBuffers();

    cl_int ret;

    // create two buffers for each chemical (we will switch between them)
    for(int io=0;io<2;io++)
    {
        this->buffers[io].resize(NC);
        for(int ic=0;ic<NC;ic++)
        {
            this->buffers[io][ic] = clCreateBuffer(this->context, CL_MEM_READ_WRITE, MEM_SIZE, NULL, &ret);
            throwOnError(ret,"OpenCLMeshRD::CreateOpenCLBuffers : buffer creation failed: ");
        }
    }
    
    // create a buffer for the indices of the neighbors of each cell
    const unsigned long NBORS_INDICES_SIZE = sizeof(int) * (unsigned long)this->mesh->GetNumberOfCells() * this->max_neighbors;
    this->clBuffer_cell_neighbor_indices = clCreateBuffer(this->context, CL_MEM_READ_ONLY, NBORS_INDICES_SIZE, NULL, &ret);
    throwOnError(ret,"OpenCLMeshRD::CreateOpenCLBuffers : neighbor_indices buffer creation failed: ");

    // create a buffer for the diffusion coefficients of the neighbors of each cell
    const unsigned long NBORS_WEIGHTS_SIZE = sizeof(float) * (unsigned long)this->mesh->GetNumberOfCells() * this->max_neighbors;
    this->clBuffer_cell_neighbor_weights = clCreateBuffer(this->context, CL_MEM_READ_ONLY, NBORS_WEIGHTS_SIZE, NULL, &ret);
    throwOnError(ret,"OpenCLMeshRD::CreateOpenCLBuffers : neighbor_weights buffer creation failed: ");
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::WriteToOpenCLBuffersIfNeeded()
{
    if(!this->need_write_to_opencl_buffers) return;

    if(this->buffers[0].empty())
        this->CreateOpenCLBuffers();

    cl_int ret;
    const unsigned long MEM_SIZE = sizeof(float) * (int)this->mesh->GetNumberOfCells();
    this->iCurrentBuffer = 0;
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(ic).c_str()) )->GetPointer(0);
        ret = clEnqueueWriteBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCLMeshRD::WriteToOpenCLBuffers : buffer writing failed: ");
    }

    // fill indices buffer
    const unsigned long NBORS_INDICES_SIZE = sizeof(int) * (unsigned long)this->mesh->GetNumberOfCells() * this->max_neighbors;
    ret = clEnqueueWriteBuffer(this->command_queue,this->clBuffer_cell_neighbor_indices, CL_TRUE, 0, NBORS_INDICES_SIZE, this->cell_neighbor_indices, 0, NULL, NULL);
    throwOnError(ret,"OpenCLMeshRD::WriteToOpenCLBuffers : buffer writing failed: ");

    // fill weights buffer
    const unsigned long NBORS_WEIGHTS_SIZE = sizeof(float) * (unsigned long)this->mesh->GetNumberOfCells() * this->max_neighbors;
    ret = clEnqueueWriteBuffer(this->command_queue,this->clBuffer_cell_neighbor_weights, CL_TRUE, 0, NBORS_WEIGHTS_SIZE, this->cell_neighbor_weights, 0, NULL, NULL);
    throwOnError(ret,"OpenCLMeshRD::WriteToOpenCLBuffers : buffer writing failed: ");

    this->need_write_to_opencl_buffers = false;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::ReadFromOpenCLBuffers()
{
    // read from opencl buffers into our mesh data
    const unsigned long MEM_SIZE = sizeof(float) * (int)this->mesh->GetNumberOfCells();
    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
    {
        float* data = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(ic).c_str()) )->GetPointer(0);
        cl_int ret = clEnqueueReadBuffer(this->command_queue,this->buffers[this->iCurrentBuffer][ic], CL_TRUE, 0, MEM_SIZE, data, 0, NULL, NULL);
        throwOnError(ret,"OpenCLMeshRD::ReadFromOpenCLBuffers : buffer reading failed: ");
    }
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::CopyFromMesh(vtkUnstructuredGrid* mesh2)
{
    MeshRD::CopyFromMesh(mesh2);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::TestFormula(std::string program_string)
{
    this->TestKernel(this->AssembleKernelSourceFromFormula(program_string));
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::GenerateInitialPattern()
{
    MeshRD::GenerateInitialPattern();
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::BlankImage()
{
    MeshRD::BlankImage();
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::ReleaseOpenCLBuffers()
{
    OpenCL_MixIn::ReleaseOpenCLBuffers();
    clReleaseMemObject(this->clBuffer_cell_neighbor_indices);
    clReleaseMemObject(this->clBuffer_cell_neighbor_weights);
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::SetValue(float x,float y,float z,float val,const Properties& render_settings)
{
    MeshRD::SetValue(x,y,z,val,render_settings);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings)
{
    MeshRD::SetValuesInRadius(x,y,z,r,val,render_settings);
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::GetFromOpenCLBuffers( float* dest, int chemical_id )
{
	//do nothing for now, TODO, provide an implementation.
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::Undo()
{
    MeshRD::Undo();
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------

void OpenCLMeshRD::Redo()
{
    MeshRD::Redo();
    this->need_write_to_opencl_buffers = true;
}

// ----------------------------------------------------------------------------------------------------------------
