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
#include "OpenCL_FullKernel.hpp"
#include "utils.hpp"

// STL:
#include <string>
using namespace std;

// VTK:
#include <vtkXMLUtilities.h>

OpenCL_FullKernel::OpenCL_FullKernel()
{
    this->SetRuleName("Full kernel example");
    this->SetFormula("__kernel void rd_compute(__global float* a_in,__global float* a_out) {}");
    this->block_size[0]=1;
    this->block_size[1]=1;
    this->block_size[2]=1;
}

string OpenCL_FullKernel::AssembleKernelSourceFromFormula(std::string formula) const
{
    return formula; // here the formula is a full OpenCL kernel
}

void OpenCL_FullKernel::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    OpenCL_RD::InitializeFromXML(rd,warn_to_update);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // kernel:
    vtkSmartPointer<vtkXMLDataElement> xml_kernel = rule->FindNestedElementWithName("kernel");
    if(!xml_kernel) throw runtime_error("kernel node not found in file");
    string formula = trim_multiline_string(xml_kernel->GetCharacterData());
    read_required_attribute(xml_kernel,"block_size_x",this->block_size[0]);
    read_required_attribute(xml_kernel,"block_size_y",this->block_size[1]);
    read_required_attribute(xml_kernel,"block_size_z",this->block_size[2]);

    // number_of_chemicals:
    read_required_attribute(xml_kernel,"number_of_chemicals",this->n_chemicals);

    // do this last, because it requires everything else to be set up first
    this->TestFormula(formula); // will throw on error but won't set
    this->SetFormula(formula); // will set but won't throw
}

vtkSmartPointer<vtkXMLDataElement> OpenCL_FullKernel::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCL_RD::GetAsXML();

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    vtkSmartPointer<vtkXMLDataElement> kernel = vtkSmartPointer<vtkXMLDataElement>::New();
    kernel->SetName("kernel");
    kernel->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    kernel->SetIntAttribute("block_size_x",this->block_size[0]);
    kernel->SetIntAttribute("block_size_y",this->block_size[1]);
    kernel->SetIntAttribute("block_size_z",this->block_size[2]);
    ostringstream oss;
    #if defined(unix) || defined(__unix__) || defined(__unix)
        vtkXMLUtilities::EncodeString(this->GetFormula().c_str(),VTK_ENCODING_UNKNOWN,oss,VTK_ENCODING_UNKNOWN,true);
    #else
        oss << this->GetFormula();
    #endif
    kernel->SetCharacterData(oss.str().c_str(),(int)oss.str().length());
    rule->AddNestedElement(kernel);

    return rd;
}
