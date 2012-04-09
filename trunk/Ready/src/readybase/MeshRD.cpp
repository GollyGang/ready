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
#include "MeshRD.hpp"
#include "IO_XML.hpp"

// VTK:
#include <vtkPolyData.h>
#include <vtkXMLDataElement.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h> // DEBUG

// ---------------------------------------------------------------------

MeshRD::MeshRD()
{
    //this->starting_pattern = vtkPolyData::New();

    // DEBUG:
    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->Update();
    this->mesh = vtkPolyData::New();
    this->mesh->DeepCopy(sphere->GetOutput());
}

// ---------------------------------------------------------------------

MeshRD::~MeshRD()
{
    this->mesh->Delete();
    //this->starting_pattern->Delete();
}

// ---------------------------------------------------------------------

void MeshRD::InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update)
{
    // TODO
}

// ---------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> MeshRD::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = vtkSmartPointer<vtkXMLDataElement>::New();
    rd->SetName("RD");
    rd->SetAttribute("format_version","1");
    // (Use this for when the format changes so much that the user will get better results if they update their Ready. File reading will still proceed but may fail.) 

    return rd;

    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::Update(int n_steps)
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::SetNumberOfChemicals(int n)
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::SaveFile(const char* filename,const Properties& render_settings) const
{
    vtkSmartPointer<RD_XMLPolyDataWriter> iw = vtkSmartPointer<RD_XMLPolyDataWriter>::New();
    iw->SetSystem(this);
    iw->SetRenderSettings(&render_settings);
    iw->SetFileName(filename);
    iw->SetInput(this->mesh);
    iw->Write();
}

// ---------------------------------------------------------------------

void MeshRD::GenerateInitialPattern()
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::BlankImage()
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::CopyFromMesh(vtkPolyData* pd)
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings)
{
    // TODO

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInput(this->mesh);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    pRenderer->AddActor(actor);
}

// ---------------------------------------------------------------------

void MeshRD::SaveStartingPattern()
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::RestoreStartingPattern()
{
    // TODO
}

// ---------------------------------------------------------------------

float MeshRD::SampleAt(int x,int y,int z,int ic)
{
    return 0.0f;

    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::InternalUpdate(int n_steps)
{
    // TODO
}

// ---------------------------------------------------------------------
