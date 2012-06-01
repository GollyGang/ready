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
#include "AbstractRD.hpp"
#include "overlays.hpp"

// STL:
using namespace std;

// ---------------------------------------------------------------------

AbstractRD::AbstractRD()
{
    this->timesteps_taken = 0;
    this->need_reload_formula = true;
    this->is_modified = false;
    this->wrap = true;
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
        warn_to_update = (i>2);
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
    this->ClearInitialPatternGenerator();
    vtkSmartPointer<vtkXMLDataElement> xml_initial_pattern_generator = rd->FindNestedElementWithName("initial_pattern_generator");
    if(xml_initial_pattern_generator) // optional, default is none
    {
        for(int i=0;i<xml_initial_pattern_generator->GetNumberOfNestedElements();i++)
             this->initial_pattern_generator.push_back(new Overlay(xml_initial_pattern_generator->GetNestedElement(i)));
    }
}

// ---------------------------------------------------------------------

// TODO: ImageRD could inherit from XML_Object (but as VTKFile element, not RD element!)
vtkSmartPointer<vtkXMLDataElement> AbstractRD::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = vtkSmartPointer<vtkXMLDataElement>::New();
    rd->SetName("RD");
    rd->SetIntAttribute("format_version",2);
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