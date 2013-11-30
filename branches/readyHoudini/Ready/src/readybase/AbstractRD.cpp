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
#include "AbstractRD.hpp"
#include "overlays.hpp"

// STL:
using namespace std;

// SSE:
#include <xmmintrin.h>

// ---------------------------------------------------------------------

AbstractRD::AbstractRD()
{
    this->timesteps_taken = 0;
    this->need_reload_formula = true;
    this->is_modified = false;
    this->wrap = true;

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
    this->ClearInitialPatternGenerator();
}

// ---------------------------------------------------------------------

void AbstractRD::ClearInitialPatternGenerator()
{
    for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
        delete this->initial_pattern_generator[iOverlay];
    this->initial_pattern_generator.clear();
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

void AbstractRD::SetParameterValueByName(const std::string& name, float value)
{
    for(int iParam=0;iParam<(int)this->parameters.size();iParam++)
        if(this->parameters[iParam].first == name)
		{
            this->parameters[iParam].second = value;
			this->is_modified = true;
			return;
		}
    throw runtime_error("ImageRD::SetParameterValueByName : parameter name not found: "+name);
}

// ---------------------------------------------------------------------

void AbstractRD::QueueReloadFormula()
{
	this->need_reload_formula = true;
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

void AbstractRD::InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update)
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
    this->ReadInitialPatternGeneratorFromXML(rd->FindNestedElementWithName("initial_pattern_generator"));
}

// ---------------------------------------------------------------------

void AbstractRD::ReadInitialPatternGeneratorFromXML(vtkXMLDataElement* node)
{
    this->ClearInitialPatternGenerator();
    if(node) // optional, default is none
    {
        for(int i=0;i<node->GetNumberOfNestedElements();i++)
             this->initial_pattern_generator.push_back(new Overlay(node->GetNestedElement(i)));
    }
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
    description->SetCharacterData(this->GetDescription().c_str(),(int)this->GetDescription().length());
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

    // initial pattern generator
    vtkSmartPointer<vtkXMLDataElement> initial_pattern_generator = vtkSmartPointer<vtkXMLDataElement>::New();
    initial_pattern_generator->SetName("initial_pattern_generator");
    initial_pattern_generator->SetAttribute("apply_when_loading",generate_initial_pattern_when_loading?"true":"false");
    for(int i=0;i<(int)this->initial_pattern_generator.size();i++)
        initial_pattern_generator->AddNestedElement(this->initial_pattern_generator[i]->GetAsXML());
    rd->AddNestedElement(initial_pattern_generator);

    return rd;
}

// ---------------------------------------------------------------------

void AbstractRD::CreateDefaultInitialPatternGenerator()
{
    this->ClearInitialPatternGenerator();
    // this is ungainly, will need to improve later when we allow the user to edit the IPG through the Info Pane
    vtkSmartPointer<vtkXMLDataElement> ow = vtkSmartPointer<vtkXMLDataElement>::New();
    ow->SetName("overwrite");
    vtkSmartPointer<vtkXMLDataElement> c1 = vtkSmartPointer<vtkXMLDataElement>::New();
    c1->SetName("constant");
    c1->SetFloatAttribute("value",1.0);
    vtkSmartPointer<vtkXMLDataElement> ew = vtkSmartPointer<vtkXMLDataElement>::New();
    ew->SetName("everywhere");

    vtkSmartPointer<vtkXMLDataElement> ov1 = vtkSmartPointer<vtkXMLDataElement>::New();
    ov1->SetName("overlay");
    ov1->SetAttribute("chemical","a");
    ov1->AddNestedElement(ow);
    ov1->AddNestedElement(c1);
    ov1->AddNestedElement(ew);
    this->initial_pattern_generator.push_back(new Overlay(ov1));

    vtkSmartPointer<vtkXMLDataElement> wn = vtkSmartPointer<vtkXMLDataElement>::New();
    wn->SetName("white_noise");
    wn->SetFloatAttribute("low",0.0);
    wn->SetFloatAttribute("high",1.0);
    vtkSmartPointer<vtkXMLDataElement> p1 = vtkSmartPointer<vtkXMLDataElement>::New();
    p1->SetName("point3D");
    p1->SetFloatAttribute("x",0.5);
    p1->SetFloatAttribute("y",0);
    p1->SetFloatAttribute("z",0);
    vtkSmartPointer<vtkXMLDataElement> p2 = vtkSmartPointer<vtkXMLDataElement>::New();
    p2->SetName("point3D");
    p2->SetFloatAttribute("x",1.0);
    p2->SetFloatAttribute("y",1.0);
    p2->SetFloatAttribute("z",1.0);
    vtkSmartPointer<vtkXMLDataElement> r = vtkSmartPointer<vtkXMLDataElement>::New();
    r->SetName("rectangle");
    r->AddNestedElement(p1);
    r->AddNestedElement(p2);
    for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
    {
        vtkSmartPointer<vtkXMLDataElement> ov = vtkSmartPointer<vtkXMLDataElement>::New();
        ov->SetName("overlay");
        ov->SetAttribute("chemical",GetChemicalName(iChem).c_str());
        ov->AddNestedElement(ow);
        ov->AddNestedElement(wn);
        ov->AddNestedElement(r);
        this->initial_pattern_generator.push_back(new Overlay(ov));
    }
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
