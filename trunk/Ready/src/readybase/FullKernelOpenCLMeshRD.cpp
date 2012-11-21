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
#include "FullKernelOpenCLMeshRD.hpp"
#include "utils.hpp"

// STL:
#include <string>
using namespace std;

// VTK:
#include <vtkXMLUtilities.h>
#include <vtkUnstructuredGrid.h>

// ---------------------------------------------------------------------------------------------------------

FullKernelOpenCLMeshRD::FullKernelOpenCLMeshRD(int opencl_platform,int opencl_device)
    : OpenCLMeshRD(opencl_platform,opencl_device)
{
    this->SetRuleName("Full kernel example");
    this->SetFormula("__kernel void rd_compute() {}");
}

// ---------------------------------------------------------------------------------------------------------

FullKernelOpenCLMeshRD::FullKernelOpenCLMeshRD(const OpenCLMeshRD& source) 
    : OpenCLMeshRD(source.GetPlatform(),source.GetDevice())
{
    this->SetFormula(source.GetKernel());

    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    source.GetMesh(mesh);
    this->CopyFromMesh(mesh);

    this->SetRuleName(source.GetRuleName());
    this->SetDescription(source.GetDescription());

    this->ReadInitialPatternGeneratorFromXML(source.GetAsXML(false)->FindNestedElementWithName("initial_pattern_generator"));

    // TODO: copy starting pattern?
}

// ---------------------------------------------------------------------------------------------------------

string FullKernelOpenCLMeshRD::AssembleKernelSourceFromFormula(std::string formula) const
{
    return formula; // here the formula is a full OpenCL kernel
}

// ---------------------------------------------------------------------------------------------------------

void FullKernelOpenCLMeshRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    OpenCLMeshRD::InitializeFromXML(rd,warn_to_update);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // kernel:
    vtkSmartPointer<vtkXMLDataElement> xml_kernel = rule->FindNestedElementWithName("kernel");
    if(!xml_kernel) throw runtime_error("kernel node not found in file");
    string formula = trim_multiline_string(xml_kernel->GetCharacterData());

    // number_of_chemicals:
    read_required_attribute(xml_kernel,"number_of_chemicals",this->n_chemicals);

    // do this last, because it requires everything else to be set up first
    this->TestFormula(formula); // will throw on error but won't set
    this->SetFormula(formula); // will set but won't throw
}

// ---------------------------------------------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> FullKernelOpenCLMeshRD::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCLMeshRD::GetAsXML(generate_initial_pattern_when_loading);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    vtkSmartPointer<vtkXMLDataElement> kernel = vtkSmartPointer<vtkXMLDataElement>::New();
    kernel->SetName("kernel");
    kernel->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    kernel->SetCharacterData(this->GetFormula().c_str(),(int)this->GetFormula().length());
    rule->AddNestedElement(kernel);

    return rd;
}

// ---------------------------------------------------------------------------------------------------------
