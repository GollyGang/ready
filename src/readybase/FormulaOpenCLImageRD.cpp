/*  Copyright 2011-2021 The Ready Bunch

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
#include <algorithm>
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
    this->block_size[0] = 4;
    this->block_size[1] = 1;
    this->block_size[2] = 1;

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

struct InputsNeeded {
    vector<AppliedStencil> stencils_needed;
    set<InputPoint> cells_needed;
    map<string, int> gradient_mag_squared;
    bool using_x_pos;
    bool using_y_pos;
    bool using_z_pos;
    vector<string> deltas_needed;
};

// -------------------------------------------------------------------------

InputsNeeded DetectInputsNeeded(const string& formula, int num_chemicals, int dimensionality, const int block_size[3])
{
    InputsNeeded inputs_needed;

    const vector<string> formula_tokens = tokenize_for_keywords(formula);
    const vector<Stencil> known_stencils = GetKnownStencils(dimensionality);
    for (int i = 0; i < num_chemicals; i++)
    {
        const string chem = GetChemicalName(i);
        // assume we will need the central cell
        inputs_needed.cells_needed.insert({{ 0, 0, 0 }, chem });
        // assume we need delta_<chem> for the forward Euler step
        inputs_needed.deltas_needed.push_back(chem);
        // search for keywords that make use of stencils
        set<string> dependent_stencils;
        if (UsingKeyword(formula_tokens, "gradient_mag_squared_" + chem))
        {
            inputs_needed.gradient_mag_squared[chem] = dimensionality;
            switch (dimensionality)
            {
            default:
            case 3:
                dependent_stencils.insert("z_gradient_" + chem);
            case 2:
                dependent_stencils.insert("y_gradient_" + chem);
            case 1:
                dependent_stencils.insert("x_gradient_" + chem); // (N.B. no breaks)
            }
        }
        // search for keywords that are stencils
        for (const Stencil& stencil : known_stencils)
        {
            const string keyword = stencil.label + "_" + chem;
            if (UsingKeyword(formula_tokens, keyword) || dependent_stencils.find(keyword) != dependent_stencils.end())
            {
                const AppliedStencil applied_stencil{ stencil, chem };
                inputs_needed.stencils_needed.push_back(applied_stencil);
                // add the cell inputs needed for this stencil
                const set<InputPoint> input_points = applied_stencil.GetInputPoints();
                inputs_needed.cells_needed.insert(input_points.begin(), input_points.end());
            }
        }
        // search for direct access to neighbors, e.g. "a_nw"
        const int MAX_RADIUS = 10; // surely if the user wants something this big they should use a kernel?
        for (int x = -MAX_RADIUS; x <= MAX_RADIUS; x++)
        {
            for (int y = -MAX_RADIUS; y <= MAX_RADIUS; y++)
            {
                for (int z = -MAX_RADIUS; z <= MAX_RADIUS; z++)
                {
                    const InputPoint input_point{ {x, y, z}, chem };
                    if (UsingKeyword(formula_tokens, input_point.GetName()))
                    {
                        inputs_needed.cells_needed.insert(input_point);
                    }
                }
            }
        }
    }
    if (block_size[0] == 4)
    {
        // non-block-aligned inputs need other inputs: the two blocks that supply them
        vector<InputPoint> blocks_needed;
        for (const InputPoint& input_point : inputs_needed.cells_needed)
        {
            if (input_point.point.x % 4 != 0)
            {
                const pair<InputPoint, InputPoint> blocks = input_point.GetAlignedBlocks_Block411();
                blocks_needed.push_back(blocks.first);
                blocks_needed.push_back(blocks.second);
            }
        }
        inputs_needed.cells_needed.insert(blocks_needed.begin(), blocks_needed.end());
    }
    // detect if using x_pos, y_pos or z_pos
    inputs_needed.using_x_pos = UsingKeyword(formula_tokens, "x_pos");
    inputs_needed.using_y_pos = UsingKeyword(formula_tokens, "y_pos");
    inputs_needed.using_z_pos = UsingKeyword(formula_tokens, "z_pos");

    return inputs_needed;
}

// -------------------------------------------------------------------------

struct KernelOptions {
    KernelOptions(bool wrap, const string& indent, int data_type, const string& data_type_string,
                  const string& data_type_suffix, const int block_size[3])
        : wrap(wrap)
        , indent(indent)
        , data_type(data_type)
        , data_type_string(data_type_string)
        , data_type_suffix(data_type_suffix)
        , block_size{ block_size[0], block_size[1], block_size[2] }
    {}
    bool wrap;
    string indent;
    int data_type;
    string data_type_string;
    string data_type_suffix;
    const int block_size[3];
};

// -------------------------------------------------------------------------

void WriteCellsNeeded(ostringstream& kernel_source, const set<InputPoint>& cells_needed, const KernelOptions& options)
{
    // write code to retrieve the block-aligned inputs from global memory
    for (const InputPoint& input_point : cells_needed)
    {
        // central cell
        if (input_point.point.x == 0 && input_point.point.y == 0 && input_point.point.z == 0)
        {
            kernel_source << options.indent << options.data_type_string << " "
                          << input_point.GetDirectAccessCode(options.wrap, options.block_size) << ";\n";
            // the central cell is non-const to allow the user to set it directly - it appears in the forward-Euler step
        }
        // other block-aligned points
        else if (input_point.point.x % options.block_size[0] == 0)
        {
            kernel_source << options.indent << "const " << options.data_type_string << " "
                          << input_point.GetDirectAccessCode(options.wrap, options.block_size) << ";\n";
        }
    }
    if (options.block_size[0] == 4)
    {
        // write code to compute the non-block-aligned float4's from the block-aligned ones we have retrieved
        for (const InputPoint& input_point : cells_needed)
        {
            if (input_point.point.x % options.block_size[0] != 0)
            {
                // swizzle from the retrieved blocks
                kernel_source << options.indent << "const " << options.data_type_string << " " << input_point.GetName()
                    << " = (" << options.data_type_string << ")(" << input_point.GetSwizzled_Block411() << ");\n";
            }
        }
    }
}

// -------------------------------------------------------------------------

void WriteKeywords(ostringstream& kernel_source, const InputsNeeded& inputs_needed, const KernelOptions& options)
{
    kernel_source << options.indent << "// keywords needed for the formula:\n";
    // output the first part of the body
    kernel_source << options.indent << "const int index_x = get_global_id(0);\n";
    kernel_source << options.indent << "const int index_y = get_global_id(1);\n";
    kernel_source << options.indent << "const int index_z = get_global_id(2);\n";
    kernel_source << options.indent << "const int X = get_global_size(0);\n";
    kernel_source << options.indent << "const int Y = get_global_size(1);\n";
    kernel_source << options.indent << "const int Z = get_global_size(2);\n";
    kernel_source << options.indent << "const int index_here = X*(Y*index_z + index_y) + index_x;\n";
    // write code for the cells we need
    WriteCellsNeeded(kernel_source, inputs_needed.cells_needed, options);
    // write code for the stencils we need
    for (const AppliedStencil& applied_stencil : inputs_needed.stencils_needed)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " " << applied_stencil.GetCode() << ";\n";
    }
    // write code for x_pos, y_pos, z_pos if needed
    if (inputs_needed.using_x_pos)
    {
        if (options.block_size[0] == 4)
        {
            kernel_source << options.indent << "const " << options.data_type_string << " x_pos = (index_x + (" << options.data_type_string
                << ")(0.0" << options.data_type_suffix << ", 0.25" << options.data_type_suffix << ", 0.5" << options.data_type_suffix
                << ", 0.75" << options.data_type_suffix << ")) / X;\n";
        }
        else
        {
            kernel_source << options.indent << "const " << options.data_type_string << " x_pos = index_x / (" << options.data_type_string << ")(X); \n";
        }
    }
    if (inputs_needed.using_y_pos)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " y_pos = index_y / (" << options.data_type_string << ")(Y); \n";
    }
    if (inputs_needed.using_z_pos)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " z_pos = index_z / (" << options.data_type_string << ")(Z);\n";
    }
    // write code for gradient_mag_squared if needed
    for (const pair<string, int>& pair : inputs_needed.gradient_mag_squared)
    {
        const string& chem = pair.first;
        const int dimensionality = pair.second;
        kernel_source << options.indent << "const " << options.data_type_string << " gradient_mag_squared_" << chem
            << " = pow(x_gradient_" << chem << ", 2.0" << options.data_type_suffix << ")";
        if (dimensionality > 1)
        {
            kernel_source << " + pow(y_gradient_" << chem << ", 2.0" << options.data_type_suffix << ")";
            if (dimensionality > 2)
            {
                kernel_source << " + pow(z_gradient_" << chem << ", 2.0" << options.data_type_suffix << ")";
            }
        }
        kernel_source << ";\n";
    }
    // declare delta_a, etc. and initialize to zero
    for (const string& chem : inputs_needed.deltas_needed)
        kernel_source << options.indent << options.data_type_string << " delta_" << chem << " = 0.0" << options.data_type_suffix << ";\n";
    kernel_source << "\n";
}

// -------------------------------------------------------------------------

string AssembleKernelSource(const InputsNeeded& inputs_needed,
    const vector<AbstractRD::Parameter>& parameters,
    int num_chemicals,
    int dimensionality,
    const string& formula,
    const KernelOptions& options)
{
    ostringstream kernel_source;
    kernel_source << fixed << setprecision(6);
    if (options.data_type == VTK_DOUBLE)
    {
        kernel_source << "\
#ifdef cl_khr_fp64\n\
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable\n\
#elif defined(cl_amd_fp64)\n\
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable\n\
#else\n\
    #error \"Double precision floating point not supported on this OpenCL device. Choose another or contact the Ready team.\"\n\
#endif\n\n";
    }
    // output the function declaration
    kernel_source << "__kernel void rd_compute(";
    for (int i = 0; i < num_chemicals; i++)
    {
        kernel_source << "__global " << options.data_type_string << " *" << GetChemicalName(i) << "_in";
        kernel_source << ",";
    }
    for (int i = 0; i < num_chemicals; i++)
    {
        kernel_source << "__global " << options.data_type_string << " *" << GetChemicalName(i) << "_out";
        if (i < num_chemicals - 1)
        {
            kernel_source << ",";
        }
    }
    kernel_source << ")\n{\n";
    // add the parameters
    kernel_source << options.indent << "// parameters:\n";
    for (const AbstractRD::Parameter& parameter : parameters)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " " << parameter.name
            << " = " << setprecision(8) << parameter.value << options.data_type_suffix << ";\n";
    }
    // add a dx parameter for grid spacing if one is not already supplied
    const bool has_dx_parameter = find_if(parameters.begin(), parameters.end(),
        [](const AbstractRD::Parameter& param) { return param.name == "dx"; }) != parameters.end();
    if (!inputs_needed.stencils_needed.empty() && !has_dx_parameter)
    {
        kernel_source << options.indent << "const " << options.data_type_string << " dx = 1.0" << options.data_type_suffix << "; // grid spacing\n";
        // TODO: only need this if using a stencil that uses dx
    }
    kernel_source << "\n";
    // add the keywords we need
    WriteKeywords(kernel_source, inputs_needed, options);
    // add the formula
    kernel_source << options.indent << "// the formula:\n";
    istringstream iss(formula);
    string s;
    while (iss.good())
    {
        getline(iss, s);
        kernel_source << options.indent << s << "\n";
    }
    kernel_source << "\n";
    // add the forward-Euler step
    // TODO: only add this when delta_<chem> appears in the formula
    kernel_source << options.indent << "// forward-Euler update step:\n";
    for (int iC = 0; iC < num_chemicals; iC++)
    {
        kernel_source << options.indent << GetChemicalName(iC) << "_out[index_here] = "
            << GetChemicalName(iC) << " + timestep * delta_" << GetChemicalName(iC) << ";\n";
    }
    // TODO: timestep only needed if it appears in the formula or if we are doing forward-Euler for at least one chemical
    // finish up
    kernel_source << "}\n";

    return kernel_source.str();
}

// -------------------------------------------------------------------------

string FormulaOpenCLImageRD::AssembleKernelSourceFromFormula(const string& formula) const
{
    string full_data_type_string = this->data_type_string;
    if (this->block_size[0] == 4 && this->block_size[1] == 1 && this->block_size[2] == 1)
    {
        full_data_type_string += "4";
    }
    else if(this->block_size[0] == 1 && this->block_size[1] == 1 && this->block_size[2] == 1)
    {
    }
    else
    {
        throw runtime_error("unsupported block size in AssembleKernelSourceFromFormula");
    }

    const InputsNeeded inputs_needed = DetectInputsNeeded(formula, this->GetNumberOfChemicals(),
        this->GetArenaDimensionality(), this->block_size);

    const string indent = "    ";
    const KernelOptions options(this->wrap, indent, this->data_type, full_data_type_string, this->data_type_suffix, this->block_size);

    string amended_formula = formula;
    if (this->data_type == VTK_DOUBLE)
    {
        // float4 doesn't auto-convert to double4
        amended_formula = ReplaceAllSubstrings(amended_formula, "float4", full_data_type_string);
    }
    else if (this->data_type == VTK_FLOAT)
    {
        // float4 doesn't auto-convert to float
        amended_formula = ReplaceAllSubstrings(amended_formula, "float4", full_data_type_string);
        // double4 doesn't auto-convert to float4 or float
        amended_formula = ReplaceAllSubstrings(amended_formula, "double4", full_data_type_string);
        // double doesn't auto-convert to float4 or float
        amended_formula = ReplaceAllSubstrings(amended_formula, "double", full_data_type_string);
    }

    const string kernel_source = AssembleKernelSource(inputs_needed, this->parameters, this->GetNumberOfChemicals(),
        this->GetArenaDimensionality(), amended_formula, options);

    return kernel_source;
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
