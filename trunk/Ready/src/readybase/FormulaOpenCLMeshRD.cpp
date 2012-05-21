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
#include "FormulaOpenCLMeshRD.hpp"
#include "utils.hpp"

// STL:
#include <string>
#include <sstream>
using namespace std;

// VTK:
#include <vtkXMLUtilities.h>

// -------------------------------------------------------------------------

FormulaOpenCLMeshRD::FormulaOpenCLMeshRD()
{
    this->SetRuleName("Gray-Scott");
    this->AddParameter("timestep",1.0f);
    this->AddParameter("D_a",0.082f);
    this->AddParameter("D_b",0.041f);
    this->AddParameter("k",0.06f);
    this->AddParameter("F",0.035f);
    this->SetFormula("\
    delta_a = D_a * laplacian_a - a*b*b + F*(1.0f-a);\n\
    delta_b = D_b * laplacian_b + a*b*b - (F+k)*b;\n");
}

// -------------------------------------------------------------------------

std::string FormulaOpenCLMeshRD::AssembleKernelSourceFromFormula(std::string formula) const
{
    const string indent = "    ";
    const int NC = this->GetNumberOfChemicals();

    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    // output the function definition
    kernel_source << "__kernel void rd_compute(";
    for(int i=0;i<NC;i++)
        kernel_source << "__global float *" << GetChemicalName(i) << "_in,";
    for(int i=0;i<NC;i++)
        kernel_source << "__global float *" << GetChemicalName(i) << "_out,";
    kernel_source << "__global int* neighbor_indices,__global float* neighbor_weights,const int max_neighbors)\n";
    // output the body
    kernel_source << "{\n";
    kernel_source << indent << "const int x = get_global_id(0);\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "float " << GetChemicalName(i) << " = " << GetChemicalName(i) << "_in[x];\n";
    // compute the laplacians
    for(int i=0;i<NC;i++)
        kernel_source << indent << "float laplacian_" << GetChemicalName(i) << " = 0.0f;\n";
    kernel_source << indent << "int offset = x * max_neighbors;\n";
    kernel_source << indent << "for(int i=0;i<max_neighbors;i++)\n" << indent << "{\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << indent << "laplacian_" << GetChemicalName(i) << " += " << GetChemicalName(i) << "_in[neighbor_indices[offset+i]] * neighbor_weights[offset+i];\n";
    kernel_source << indent << "}\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "laplacian_" << GetChemicalName(i) << " -= " << GetChemicalName(i) << ";\n";
    kernel_source << indent << "// scale the Laplacians to be more similar to the 2D square grid version, so the same parameters work\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "laplacian_" << GetChemicalName(i) << " *= 4.0f;\n";
    kernel_source << indent << "// avoid denormals\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "laplacian_" << GetChemicalName(i) << " += 1e-6f;\n";
    // the parameters (assume all float for now)
    kernel_source << "\n" << indent << "// parameters:\n";
    for(int i=0;i<(int)this->parameters.size();i++)
        kernel_source << indent << "float " << this->parameters[i].first << " = " << this->parameters[i].second << "f;\n";
    // the update step
    kernel_source << "\n" << indent << "// update step:\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "float delta_" << GetChemicalName(i) << " = 0.0f;\n";
    kernel_source << this->formula;
    for(int i=0;i<NC;i++)
        kernel_source << indent << GetChemicalName(i) << "_out[x] = " << GetChemicalName(i) << " + timestep * delta_" << GetChemicalName(i) << ";\n";
    kernel_source << "}\n";
    return kernel_source.str();
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    OpenCLMeshRD::InitializeFromXML(rd,warn_to_update);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // formula:
    vtkSmartPointer<vtkXMLDataElement> xml_formula = rule->FindNestedElementWithName("formula");
    if(!xml_formula) throw runtime_error("formula node not found in file");

    // number_of_chemicals:
    read_required_attribute(xml_formula,"number_of_chemicals",this->n_chemicals);

    string formula = trim_multiline_string(xml_formula->GetCharacterData());
    this->TestFormula(formula); // will throw on error
    this->SetFormula(formula); // (won't throw yet)
}

// -------------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> FormulaOpenCLMeshRD::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCLMeshRD::GetAsXML();

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    // formula
    vtkSmartPointer<vtkXMLDataElement> formula = vtkSmartPointer<vtkXMLDataElement>::New();
    formula->SetName("formula");
    formula->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    formula->SetCharacterData(this->GetFormula().c_str(),(int)this->GetFormula().length());
    rule->AddNestedElement(formula);

    return rd;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::SetParameterValue(int iParam,float val)
{
    AbstractRD::SetParameterValue(iParam,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::SetParameterName(int iParam,const string& s)
{
    AbstractRD::SetParameterName(iParam,s);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::AddParameter(const std::string& name,float val)
{
    AbstractRD::AddParameter(name,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::DeleteParameter(int iParam)
{
    AbstractRD::DeleteParameter(iParam);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLMeshRD::DeleteAllParameters()
{
    AbstractRD::DeleteAllParameters();
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------
