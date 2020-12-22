/*  Copyright 2011-2020 The Ready Bunch

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
#include "stencils.hpp"
#include "utils.hpp"

// STL:
#include <set>
#include <sstream>
#include <string>
using namespace std;

// VTK:
#include <vtkXMLUtilities.h>

// -------------------------------------------------------------------------

FormulaOpenCLImageRD::FormulaOpenCLImageRD(int opencl_platform,int opencl_device,int data_type)
    : OpenCLImageRD(opencl_platform,opencl_device,data_type)
{
    // these settings are used in File > New Pattern
    this->SetRuleName("Gray-Scott");
    this->AddParameter("timestep",1.0f);
    this->AddParameter("D_a",0.082f);
    this->AddParameter("D_b",0.041f);
    this->AddParameter("K",0.06f);
    this->AddParameter("F",0.035f);
    this->SetFormula("\
delta_a = D_a * laplacian_a - a*b*b + F*(1.0"+this->data_type_suffix+"-a);\n\
delta_b = D_b * laplacian_b + a*b*b - (F+K)*b;");
}

// -------------------------------------------------------------------------

struct KeywordOptions {
    bool wrap;
    string indent;
    string data_type_string;
    string data_type_suffix;
    vector<AppliedStencil> stencils_needed;
    set<InputPoint> inputs_needed;
};

// -------------------------------------------------------------------------

string GetIndexString(int val, const string& coord, const string& coord_capital, bool wrap)
{
    ostringstream oss;
    const string index = "index_" + coord;
    if (val == 0)
    {
        oss << index;
    }
    else if (wrap)
    {
        oss << "((" << index << showpos << val << " + " << coord_capital << ") & (" << coord_capital << " - 1))";
    }
    else
    {
        oss << "min(" << coord_capital << "-1, max(0, " << index << showpos << val << "))";
    }
    return oss.str();
}

void AddKeywords_Block411(ostringstream& kernel_source, const KeywordOptions& options)
{
    // retrieve the values needed
    for (const InputPoint& input_point : options.inputs_needed)
    {
        if (input_point.point.x == 0 && input_point.point.y == 0 && input_point.point.z == 0)
        {
            continue; // central cell has already been retrieved
        }
        const string index_x = GetIndexString(input_point.point.x, "x", "X", options.wrap);
        const string index_y = GetIndexString(input_point.point.y, "y", "Y", options.wrap);
        const string index_z = GetIndexString(input_point.point.z, "z", "Z", options.wrap);
        kernel_source << options.indent << "const " << options.data_type_string << "4 " << input_point.GetName()
            << " = " << input_point.chem << "_in[X * (Y * " << index_z << " + " << index_y << ") + " << index_x << "];\n";
    }
    // compute the keywords needed
    for (const AppliedStencil& applied_stencil : options.stencils_needed)
    {
        kernel_source << options.indent << "const " << options.data_type_string << "4 " << applied_stencil.GetName()
            << " = (" << options.data_type_string << "4)(";
        for (int iSlot = 0; iSlot < 4; iSlot++)
        {
            for (int iStencilPoint = 0; iStencilPoint < applied_stencil.stencil.points.size(); iStencilPoint++)
            {
                kernel_source << applied_stencil.stencil.points[iStencilPoint].GetCode(iSlot, applied_stencil.chem);
                if (iStencilPoint < applied_stencil.stencil.points.size()-1)
                {
                    kernel_source << " + ";
                }
            }
            if (iSlot < 3)
            {
                kernel_source << ",\n" << options.indent << options.indent;
            }
        }
        kernel_source << ") / (" << applied_stencil.stencil.GetDivisorCode() << ");\n";
    }
}

// -------------------------------------------------------------------------

std::string FormulaOpenCLImageRD::AssembleKernelSourceFromFormula(std::string formula) const
{
    const string indent = "    ";
    const int NC = this->GetNumberOfChemicals();

    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    if( this->data_type == VTK_DOUBLE ) {
        kernel_source << "\
#ifdef cl_khr_fp64\n\
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable\n\
#elif defined(cl_amd_fp64)\n\
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable\n\
#else\n\
    #error \"Double precision floating point not supported on this OpenCL device. Choose another or contact the Ready team.\"\n\
#endif\n\n";
    }
    // output the function definition
    kernel_source << "__kernel void rd_compute(";
    for(int i=0;i<NC;i++)
        kernel_source << "__global " << this->data_type_string << "4 *" << GetChemicalName(i) << "_in,";
    for(int i=0;i<NC;i++)
    {
        kernel_source << "__global " << this->data_type_string << "4 *" << GetChemicalName(i) << "_out";
        if(i<NC-1)
            kernel_source << ",";
    }
    kernel_source << ")\n{\n";
    // output the first part of the body
    kernel_source << indent << "const int index_x = get_global_id(0);\n";
    kernel_source << indent << "const int index_y = get_global_id(1);\n";
    kernel_source << indent << "const int index_z = get_global_id(2);\n";
    kernel_source << indent << "const int X = get_global_size(0);\n";
    kernel_source << indent << "const int Y = get_global_size(1);\n";
    kernel_source << indent << "const int Z = get_global_size(2);\n";
    kernel_source << indent << "const int index_here = X*(Y*index_z + index_y) + index_x;\n\n";
    for (int i = 0; i < NC; i++)
    {
        kernel_source << indent << this->data_type_string << "4 " << GetChemicalName(i)
                      << " = " << GetChemicalName(i) << "_in[index_here];\n";
        // (non-const, to allow the user to assign directly to it if wanted)
    }
    kernel_source << "\n";
    // search the formula for keywords
    KeywordOptions options{ this->wrap, indent, this->data_type_string, this->data_type_suffix };
    vector<Stencil> known_stencils = GetKnownStencils();
    for (int i = 0; i < NC; i++)
    {
        const string chem = GetChemicalName(i);
        for (const Stencil& stencil : known_stencils)
        {
            if (formula.find(stencil.label + "_" + chem) != string::npos) // TODO: parse properly
            {
                AppliedStencil applied_stencil{ stencil, chem }; // TODO: use this->GetArenaDimensionality()
                options.stencils_needed.push_back(applied_stencil);
                set<InputPoint> input_points = applied_stencil.GetInputPoints_Block411();
                options.inputs_needed.insert(input_points.begin(), input_points.end());
            }
        }
        // search for direct access to neighbors, e.g. "a_nw"
        for (int x = -2; x <= 2; x++)
        {
            for (int y = -2; y <= 2; y++)
            {
                for (int z = -2; z <= 2; z++)
                {
                    InputPoint input_point{ {x, y, z}, chem };
                    const string input_point_neighbor_name = input_point.GetName();
                    if (formula.find(input_point_neighbor_name) != string::npos) // TODO: parse properly
                    {
                        options.inputs_needed.insert(input_point);
                    }
                }
            }
        }
    }
    // add the parameters (assume all float for now)
    for (int i = 0; i < (int)this->parameters.size(); i++)
        kernel_source << indent << "const " << this->data_type_string << "4 " << this->parameters[i].first
                      << " = " << setprecision(8) << this->parameters[i].second << this->data_type_suffix << ";\n";
    // add a dx parameter for grid spacing if one is not already supplied
    const bool has_dx_parameter = find_if(this->parameters.begin(), this->parameters.end(),
        [](const pair<string, float>& param) { return param.first == "dx"; }) != this->parameters.end();
    if (!options.stencils_needed.empty() && !has_dx_parameter)
    {
        kernel_source << indent << "const " << options.data_type_string << " dx = 1.0" << options.data_type_suffix << "; // grid spacing\n";
    }
    kernel_source << "\n";
    // add the keywords we found
    AddKeywords_Block411(kernel_source, options);
    kernel_source << "\n";
    // add delta_a, etc.
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << this->data_type_string << "4 delta_" << GetChemicalName(iC) << " = 0.0" << this->data_type_suffix << ";\n";
    kernel_source << "\n";
    // the formula
    istringstream iss(formula);
    string s;
    while(iss.good())
    {
        getline(iss,s);
        kernel_source << indent << s << "\n";
    }
    // the last part of the kernel
    kernel_source << "\n";
    for(int iC=0;iC<NC;iC++)
        kernel_source << indent << GetChemicalName(iC) << "_out[index_here] = " << GetChemicalName(iC) << " + timestep * delta_" << GetChemicalName(iC) << ";\n";
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
    //this->TestFormula(formula); // will throw on error
    this->SetFormula(formula); // (won't throw yet)
}

// -------------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> FormulaOpenCLImageRD::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<vtkXMLDataElement> rd = OpenCLImageRD::GetAsXML(generate_initial_pattern_when_loading);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    // formula
    vtkSmartPointer<vtkXMLDataElement> formula = vtkSmartPointer<vtkXMLDataElement>::New();
    formula->SetName("formula");
    formula->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    string f = this->GetFormula();
    f = ReplaceAllSubstrings(f, "\n", "\n        "); // indent the lines
    formula->SetCharacterData(f.c_str(), (int)f.length());
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
