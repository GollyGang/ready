/*  Copyright 2011-2018 The Ready Bunch

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
#include "AbstractRD.hpp"
#include "overlays.hpp"

// STL:
using namespace std;

// SSE:
#include <xmmintrin.h>

// ---------------------------------------------------------------------

AbstractRD::AbstractRD(int data_type)
{
    this->timesteps_taken = 0;
    this->need_reload_formula = true;
    this->is_modified = false;
    this->wrap = true;
    this->InternalSetDataType(data_type);

    this->neighborhood_type = VERTEX_NEIGHBORS;
    this->neighborhood_range = 1;
    this->neighborhood_weight_type = LAPLACIAN;

    #if defined(USE_SSE)
        // disable accurate handling of denormals and zeros, for speed
        #if (defined(__i386__) || defined(__x64_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_IX86))
         int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
         int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
         _mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
        #endif
    #endif // (USE_SSE)

    this->canonical_neighborhood_type_identifiers[VERTEX_NEIGHBORS] = "vertex";
    this->canonical_neighborhood_type_identifiers[EDGE_NEIGHBORS] = "edge";
    this->canonical_neighborhood_type_identifiers[FACE_NEIGHBORS] = "face";
    for(map<TNeighborhood,string>::const_iterator it = this->canonical_neighborhood_type_identifiers.begin();it != this->canonical_neighborhood_type_identifiers.end();it++)
        this->recognized_neighborhood_type_identifiers[it->second] = it->first;
    this->canonical_neighborhood_weight_identifiers[EQUAL] = "equal";
    this->canonical_neighborhood_weight_identifiers[LAPLACIAN] = "laplacian";
    for(map<TWeight,string>::const_iterator it = this->canonical_neighborhood_weight_identifiers.begin();it != this->canonical_neighborhood_weight_identifiers.end();it++)
        this->recognized_neighborhood_weight_identifiers[it->second] = it->first;
}

// ---------------------------------------------------------------------

AbstractRD::~AbstractRD()
{
}

// ---------------------------------------------------------------------

void AbstractRD::SetFormula(string s)
{
    if(s != this->formula)
        this->need_reload_formula = true;
    this->formula = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetRuleName(std::string s)
{
    this->rule_name = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetDescription(std::string s)
{
    this->description = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

int AbstractRD::GetNumberOfParameters() const
{
    return (int)this->parameters.size();
}

// ---------------------------------------------------------------------

std::string AbstractRD::GetParameterName(int iParam) const
{
    return this->parameters[iParam].first;
}

// ---------------------------------------------------------------------

float AbstractRD::GetParameterValue(int iParam) const
{
    return this->parameters[iParam].second;
}

// ---------------------------------------------------------------------

float AbstractRD::GetParameterValueByName(const std::string& name) const
{
    for(int iParam=0;iParam<(int)this->parameters.size();iParam++)
        if(this->parameters[iParam].first == name)
            return this->parameters[iParam].second;
    throw runtime_error("ImageRD::GetParameterValueByName : parameter name not found: "+name);
}

// ---------------------------------------------------------------------

void AbstractRD::AddParameter(const std::string& name,float val)
{
    this->parameters.push_back(make_pair(name,val));
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::DeleteParameter(int iParam)
{
    this->parameters.erase(this->parameters.begin()+iParam);
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::DeleteAllParameters()
{
    this->parameters.clear();
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetParameterName(int iParam,const string& s)
{
    this->parameters[iParam].first = s;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

void AbstractRD::SetParameterValue(int iParam,float val)
{
    this->parameters[iParam].second = val;
    this->is_modified = true;
}

// ---------------------------------------------------------------------

bool AbstractRD::IsParameter(const string& name) const
{
    for(int i=0;i<(int)this->parameters.size();i++)
        if(this->parameters[i].first == name)
            return true;
    return false;
}

// ---------------------------------------------------------------------

void AbstractRD::SetModified(bool m)
{
    this->is_modified = m;
}

// ---------------------------------------------------------------------

void AbstractRD::SetFilename(const string& s)
{
    this->filename = s;
}

// ---------------------------------------------------------------------

void AbstractRD::InitializeFromXML(vtkXMLDataElement* rd, bool& warn_to_update)
{
    string str;
    const char *s;
    float f;
    int i;

    // check whether we should warn the user that they need to update Ready
    {
        read_required_attribute(rd,"format_version",i);
        warn_to_update = (i>4);
        // (we will still proceed and try to read the file but it might fail or give poor results)
    }

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // rule_name:
    read_required_attribute(rule,"name",str);
    this->SetRuleName(str);

    // wrap-around
    s = rule->GetAttribute("wrap");
    if(!s) this->wrap = true;
    else this->wrap = (string(s)=="1");

    // neighborhood specifiers

    s = rule->GetAttribute("neighborhood_type");
    if(!s) this->neighborhood_type = VERTEX_NEIGHBORS;
    else if(this->recognized_neighborhood_type_identifiers.find(s)==this->recognized_neighborhood_type_identifiers.end())
        throw runtime_error("Unrecognized neighborhood_type");
    else this->neighborhood_type = this->recognized_neighborhood_type_identifiers[s];

    s = rule->GetAttribute("neighborhood_range");
    if(!s) this->neighborhood_range = 1;
    else {
        istringstream iss(s);
        iss >> this->neighborhood_range;
    }
    if(neighborhood_range!=1)
        throw runtime_error("Unsupported neighborhood range");

    s = rule->GetAttribute("neighborhood_weight");
    if(!s) this->neighborhood_weight_type = LAPLACIAN;
    else if(this->recognized_neighborhood_weight_identifiers.find(s)==this->recognized_neighborhood_weight_identifiers.end())
        throw runtime_error("Unrecognized neighborhood_weight");
    else this->neighborhood_weight_type = this->recognized_neighborhood_weight_identifiers[s];

    // parameters:
    this->DeleteAllParameters();
    for(int i=0;i<rule->GetNumberOfNestedElements();i++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = rule->GetNestedElement(i);
        if(string(node->GetName())!="param") continue;
        string name;
        s = node->GetAttribute("name");
        if(!s) throw runtime_error("Failed to read param attribute: name");
        name = trim_multiline_string(s);
        s = node->GetCharacterData();
        if(!s || !from_string(s,f)) throw runtime_error("Failed to read param value");
        this->AddParameter(name,f);
    }

    // description:
    vtkSmartPointer<vtkXMLDataElement> xml_description = rd->FindNestedElementWithName("description");
    if(!xml_description) this->SetDescription(""); // optional, default is empty string
    else this->SetDescription(trim_multiline_string(xml_description->GetCharacterData()));

    // initial_pattern_generator:
    this->initial_pattern_generator.ReadFromXML(rd->FindNestedElementWithName("initial_pattern_generator"));
}

// ---------------------------------------------------------------------

// TODO: ImageRD could inherit from XML_Object (but as VTKFile element, not RD element!)
vtkSmartPointer<vtkXMLDataElement> AbstractRD::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<vtkXMLDataElement> rd = vtkSmartPointer<vtkXMLDataElement>::New();
    rd->SetName("RD");
    rd->SetIntAttribute("format_version",4);
    // (Use this for when the format changes so much that the user will get better results if they update their Ready. File reading will still proceed but may fail.) 

    // description
    vtkSmartPointer<vtkXMLDataElement> description = vtkSmartPointer<vtkXMLDataElement>::New();
    description->SetName("description");
    string desc = this->GetDescription();
    desc = ReplaceAllSubstrings(desc, "\n", "\n      "); // indent the lines
    description->SetCharacterData(desc.c_str(), (int)desc.length());
    rd->AddNestedElement(description);

    // rule
    vtkSmartPointer<vtkXMLDataElement> rule = vtkSmartPointer<vtkXMLDataElement>::New();
    rule->SetName("rule");
    rule->SetAttribute("name",this->GetRuleName().c_str());
    rule->SetAttribute("type",this->GetRuleType().c_str());
    if(this->HasEditableWrapOption())
        rule->SetIntAttribute("wrap",this->GetWrap()?1:0);
    rule->SetAttribute("neighborhood_type",this->canonical_neighborhood_type_identifiers.find(this->neighborhood_type)->second.c_str());
    rule->SetIntAttribute("neighborhood_range",this->neighborhood_range);
    rule->SetAttribute("neighborhood_weight",this->canonical_neighborhood_weight_identifiers.find(this->neighborhood_weight_type)->second.c_str());
    for(int i=0;i<this->GetNumberOfParameters();i++)    // parameters
    {
        vtkSmartPointer<vtkXMLDataElement> param = vtkSmartPointer<vtkXMLDataElement>::New();
        param->SetName("param");
        param->SetAttribute("name",this->GetParameterName(i).c_str());
        string s = to_string(this->GetParameterValue(i));
        param->SetCharacterData(s.c_str(),(int)s.length());
        rule->AddNestedElement(param);
    }
    rd->AddNestedElement(rule);

    rd->AddNestedElement(initial_pattern_generator.GetAsXML(generate_initial_pattern_when_loading));

    return rd;
}

// ---------------------------------------------------------------------

void AbstractRD::CreateDefaultInitialPatternGenerator()
{
    this->initial_pattern_generator.CreateDefaultInitialPatternGenerator(this->GetNumberOfChemicals());
}

// ---------------------------------------------------------------------

bool AbstractRD::CanUndo() const
{
    return !this->undo_stack.empty() && this->undo_stack.front().done;
}

// ---------------------------------------------------------------------

bool AbstractRD::CanRedo() const
{
    return !this->undo_stack.empty() && !this->undo_stack.back().done;
}

// ---------------------------------------------------------------------

void AbstractRD::SetUndoPoint()
{
    // paint events are treated as a block until (e.g.) mouse up calls this function
    if(!this->undo_stack.empty())
        this->undo_stack.back().last_of_group = true;
}

// ---------------------------------------------------------------------

void AbstractRD::Undo()
{
    if(!this->CanUndo()) throw runtime_error("AbstractRD::Undo() : attempt to undo when undo not possible");

    // find the last done paint action, undo backwards from there until the previous last_of_group (exclusive)
    vector<PaintAction>::reverse_iterator rit;
    for(rit=this->undo_stack.rbegin();!rit->done;rit++); // skip over actions that have already been undone
    while(true)
    {
        this->FlipPaintAction(*rit);
        rit++;
        if(rit==this->undo_stack.rend() || rit->last_of_group)
            break;
    }
}

// ---------------------------------------------------------------------

void AbstractRD::Redo()
{
    if(!this->CanRedo()) throw runtime_error("AbstractRD::Redo() : attempt to redo when redo not possible");

    // find the first undone paint action, redo forwards from there until the next last_of_group (inclusive)
    vector<PaintAction>::iterator it;
    for(it=this->undo_stack.begin();it->done;it++); // skip over actions that have already been done
    do
    {
        this->FlipPaintAction(*it);
        if(it->last_of_group) 
            break;
        it++;
    } while(it!=this->undo_stack.end());
}

// ---------------------------------------------------------------------

void AbstractRD::StorePaintAction(int iChemical,int iCell,float old_val)
{
    // forget all stored undone actions
    while(!this->undo_stack.empty() && !this->undo_stack.back().done)
        this->undo_stack.pop_back();
    // add the new paint action
    PaintAction pa;
    pa.iCell = iCell;
    pa.iChemical = iChemical;
    pa.val = old_val; // (the cell itself stores the new val, we just need the old one)
    pa.done = true;
    pa.last_of_group = false;
    this->undo_stack.push_back(pa);
}

// ---------------------------------------------------------------------

std::string AbstractRD::GetNeighborhoodType() const
{
    return this->canonical_neighborhood_type_identifiers.find(this->neighborhood_type)->second;
}

// ---------------------------------------------------------------------

int AbstractRD::GetNeighborhoodRange() const
{
    return this->neighborhood_range;
}

// ---------------------------------------------------------------------

std::string AbstractRD::GetNeighborhoodWeight() const
{
    return this->canonical_neighborhood_weight_identifiers.find(this->neighborhood_weight_type)->second;
}

// ---------------------------------------------------------------------

int AbstractRD::GetDataType() const
{
    return this->data_type;
}

// ---------------------------------------------------------------------

void AbstractRD::SetDataType(int type)
{
    this->InternalSetDataType(type);
    this->SetNumberOfChemicals(this->n_chemicals); // we use this because it causes the data to be reallocated
    this->GenerateInitialPattern(); // TODO: would be nice to keep current pattern somehow
}

// ---------------------------------------------------------------------

void AbstractRD::InternalSetDataType(int type)
{
    switch( type ) {
        default:
        case VTK_FLOAT:
            this->data_type = VTK_FLOAT;
            this->data_type_size = sizeof( float );
            this->data_type_string = "float";
            this->data_type_suffix = "f";
            break;
        case VTK_DOUBLE:
            this->data_type = VTK_DOUBLE;
            this->data_type_size = sizeof( double );
            this->data_type_string = "double";
            this->data_type_suffix = "";
    }
}

// ---------------------------------------------------------------------

