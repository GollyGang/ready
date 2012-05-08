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
#include "FormulaOpenCLImageRD.hpp"
#include "utils.hpp"

// STL:
#include <string>
#include <sstream>
using namespace std;

// VTK:
#include <vtkXMLUtilities.h>

// -------------------------------------------------------------------------

FormulaOpenCLImageRD::FormulaOpenCLImageRD()
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

std::string FormulaOpenCLImageRD::AssembleKernelSourceFromFormula(std::string formula) const
{
    const string indent = "    ";
    const int NC = this->GetNumberOfChemicals();
    const int NDIRS = 6;
    const string dir[NDIRS]={"left","right","up","down","fore","back"};

    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    // output the function definition
    kernel_source << "__kernel void rd_compute(";
    for(int i=0;i<NC;i++)
        kernel_source << "__global float4 *" << GetChemicalName(i) << "_in,";
    for(int i=0;i<NC;i++)
    {
        kernel_source << "__global float4 *" << GetChemicalName(i) << "_out";
        if(i<NC-1)
            kernel_source << ",";
    }
    // output the first part of the body
    kernel_source << ")\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const int z = get_global_id(2);\n\
    const int X = get_global_size(0);\n\
    const int Y = get_global_size(1);\n\
    const int Z = get_global_size(2);\n\
    const int i_here = X*(Y*z + y) + x;\n\
\n";
    for(int i=0;i<NC;i++)
        kernel_source << indent << "float4 " << GetChemicalName(i) << " = " << GetChemicalName(i) << "_in[i_here];\n"; // "float4 a = a_in[i_here];"
    // output the Laplacian part of the body
    kernel_source << "\
\n\
    // compute the Laplacians of each chemical\n";
    if(this->wrap)
        kernel_source << "\
    const int xm1 = ((x-1+X) & (X-1)); // wrap (assumes X is a power of 2)\n\
    const int xp1 = ((x+1) & (X-1));\n\
    const int ym1 = ((y-1+Y) & (Y-1));\n\
    const int yp1 = ((y+1) & (Y-1));\n\
    const int zm1 = ((z-1+Z) & (Z-1));\n\
    const int zp1 = ((z+1) & (Z-1));\n";
    else
        kernel_source << "\
    const int xm1 = max(0,x-1);\n\
    const int ym1 = max(0,y-1);\n\
    const int zm1 = max(0,z-1);\n\
    const int xp1 = min(X-1,x+1);\n\
    const int yp1 = min(Y-1,y+1);\n\
    const int zp1 = min(Z-1,z+1);\n";
    kernel_source << "\
    const int i_left =  X*(Y*z + y) + xm1;\n\
    const int i_right = X*(Y*z + y) + xp1;\n\
    const int i_up =    X*(Y*z + ym1) + x;\n\
    const int i_down =  X*(Y*z + yp1) + x;\n\
    const int i_fore =  X*(Y*zm1 + y) + x;\n\
    const int i_back =  X*(Y*zp1 + y) + x;\n";
    for(int iC=0;iC<NC;iC++)
        for(int iDir=0;iDir<NDIRS;iDir++)
            kernel_source << indent << "float4 " << GetChemicalName(iC) << "_" << dir[iDir] << " = " << GetChemicalName(iC) << "_in[i_" << dir[iDir] << "];\n";
    for(int iC=0;iC<NC;iC++)
    {
        kernel_source << indent << "float4 laplacian_" << GetChemicalName(iC) << " = (float4)(" << GetChemicalName(iC) << "_up.x + " 
            << GetChemicalName(iC) << ".y + " << GetChemicalName(iC) << "_down.x + " << GetChemicalName(iC) << "_left.w + " 
            << GetChemicalName(iC) << "_fore.x + " << GetChemicalName(iC) << "_back.x,\n";
        kernel_source << indent << GetChemicalName(iC) << "_up.y + " << GetChemicalName(iC) << ".z + " << GetChemicalName(iC) 
            << "_down.y + " << GetChemicalName(iC) << ".x + " << GetChemicalName(iC) << "_fore.y + " << GetChemicalName(iC) << "_back.y,\n";
        kernel_source << indent << GetChemicalName(iC) << "_up.z + " << GetChemicalName(iC) << ".w + " << GetChemicalName(iC) 
            << "_down.z + " << GetChemicalName(iC) << ".y + " << GetChemicalName(iC) << "_fore.z + " << GetChemicalName(iC) << "_back.z,\n";
        kernel_source << indent << GetChemicalName(iC) << "_up.w + " << GetChemicalName(iC) << "_right.x + " << GetChemicalName(iC) 
            << "_down.w + " << GetChemicalName(iC) << ".z + " << GetChemicalName(iC) << "_fore.w + " << GetChemicalName(iC) 
            << "_back.w) - 6.0f*" << GetChemicalName(iC) << "\n";
        kernel_source << indent << "+ (float4)(1e-6f,1e-6f,1e-6f,1e-6f); // (kill denormals)\n";
    }
    kernel_source << "\n";
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << "float4 delta_" << GetChemicalName(iC) << "=(float4)(0.0f,0.0f,0.0f,0.0f);\n";
    kernel_source << "\n";
    // the parameters (assume all float for now)
    for(int i=0;i<(int)this->parameters.size();i++)
        kernel_source << indent << "float " << this->parameters[i].first << " = " << this->parameters[i].second << "f;\n";
    // the formula
    istringstream iss(formula);
    string s;
    while(iss.good())
    {
        getline(iss,s);
        kernel_source << indent << s << "\n";
    }
    // the last part of the kernel
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << GetChemicalName(iC) << "_out[i_here] = " << GetChemicalName(iC) << " + timestep * delta_" << GetChemicalName(iC) << ";\n";
    kernel_source << "}\n";
    return kernel_source.str();
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    OpenCLImageRD::InitializeFromXML(rd,warn_to_update);

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

vtkSmartPointer<vtkXMLDataElement> FormulaOpenCLImageRD::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCLImageRD::GetAsXML();

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

void FormulaOpenCLImageRD::SetParameterValue(int iParam,float val)
{
    AbstractRD::SetParameterValue(iParam,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::SetParameterName(int iParam,const string& s)
{
    AbstractRD::SetParameterName(iParam,s);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::AddParameter(const std::string& name,float val)
{
    AbstractRD::AddParameter(name,val);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::DeleteParameter(int iParam)
{
    AbstractRD::DeleteParameter(iParam);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::DeleteAllParameters()
{
    AbstractRD::DeleteAllParameters();
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------

void FormulaOpenCLImageRD::SetWrap(bool w)
{
    AbstractRD::SetWrap(w);
    this->need_reload_formula = true;
}

// -------------------------------------------------------------------------
