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
#include "IO_XML.hpp"
#include "BaseRD.hpp"
#include "utils.hpp"
#include "BaseOverlay.hpp"

// VTK:
#include <vtkXMLUtilities.h>
#include <vtkSmartPointer.h>
#include <vtkXMLDataParser.h>
#include <vtkImageData.h>

// STL:
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
using namespace std;

// stdlib:
#include <stdio.h>

vtkStandardNewMacro(RD_XMLWriter);
vtkStandardNewMacro(RD_XMLReader);

void RD_XMLWriter::SetSystem(BaseRD* rd_system) 
{ 
    this->system = rd_system; 
    this->SetInput(rd_system->GetImage());
}

// read a multiline string, outputting whitespace-trimmed lines
string read_string(const char* s)
{
    const char *whitespace = " \r\t\n";
    istringstream iss(s);
    ostringstream oss;
    string str;
    while(iss.good())
    {
        getline(iss,str);
        // trim whitespace at start and end
        if(str.find_first_not_of(whitespace)==string::npos)
            continue; // skip whitespace-only lines
        str = str.substr(str.find_first_not_of(whitespace),str.find_last_not_of(whitespace)+1);
        if(!oss.str().empty()) // insert a newline if there have been previous lines
            oss << "\n";
        oss << str;
    }
    return oss.str();
}

void LoadInitialPatternGenerator(vtkXMLDataElement *ipg,BaseRD* system)
{
    // delete all the overlays (TODO: BaseRD should do this for us)
    {
        for(int i=0;i<(int)system->GetInitialPatternGenerator().size();i++)
            delete system->GetInitialPatternGenerator()[i];
        system->GetInitialPatternGenerator().clear();
    }
    for(int i=0;i<ipg->GetNumberOfNestedElements();i++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = ipg->GetNestedElement(i);

        if(to_string(node->GetName())==to_string(RectangleOverlay::GetXMLName())) // TODO: use factory pattern
        {
            system->GetInitialPatternGenerator().push_back(new RectangleOverlay(node));
        }
        else
        {
            // TODO: unknown overlay type from the future or from a mistake: should continue anyway but might want to warn the user
        }
    }
}

string RD_XMLReader::GetType()
{
    this->Update();
    vtkSmartPointer<vtkXMLDataElement> root = this->XMLParser->GetRootElement();
    if(!root) throw runtime_error("No XML found in file");
    vtkSmartPointer<vtkXMLDataElement> rd = root->FindNestedElementWithName("RD");
    if(!rd) throw runtime_error("RD node not found in file");
    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");
    const char *s = rule->GetAttribute("type");
    if(!s) throw runtime_error("Failed to read rule attribute: type");
    return string(s);
}

string RD_XMLReader::GetName()
{
    this->Update();
    vtkSmartPointer<vtkXMLDataElement> root = this->XMLParser->GetRootElement();
    if(!root) throw runtime_error("No XML found in file");
    vtkSmartPointer<vtkXMLDataElement> rd = root->FindNestedElementWithName("RD");
    if(!rd) throw runtime_error("RD node not found in file");
    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");
    const char *s = rule->GetAttribute("name");
    if(!s) throw runtime_error("Failed to read rule attribute: name");
    return string(s);
}

void RD_XMLReader::SetSystemFromXML(BaseRD* system)
{
    vtkSmartPointer<vtkXMLDataElement> root = this->XMLParser->GetRootElement();
    if(!root) throw runtime_error("No XML found in file");
    vtkSmartPointer<vtkXMLDataElement> rd = root->FindNestedElementWithName("RD");
    if(!rd) throw runtime_error("RD node not found in file");

    const char *s;
    float f;
    int i;

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");
    // rule_name:
    s = rule->GetAttribute("name");
    if(!s) throw runtime_error("Failed to read rule attribute: name");
    system->SetRuleName(read_string(s));
    // number_of_chemicals:
    if(system->HasEditableFormula())
    {
        s = rule->GetAttribute("number_of_chemicals");
        if(!s || !from_string(s,i)) throw runtime_error("Failed to read rule attribute: number_of_chemicals");
        system->SetNumberOfChemicals(i);
    }
    // timestep:
    s = rule->GetAttribute("timestep");
    if(!s || !from_string(s,f)) throw runtime_error("Failed to read rule attribute: timestep");
    system->SetTimestep(f);
    // rule_description:
    vtkSmartPointer<vtkXMLDataElement> xml_rule_description = rule->FindNestedElementWithName("description");
    if(!xml_rule_description) system->SetRuleDescription(""); // optional, default is empty string
    else system->SetRuleDescription(read_string(xml_rule_description->GetCharacterData()));
    // parameters:
    system->DeleteAllParameters();
    for(int i=0;i<rule->GetNumberOfNestedElements();i++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = rule->GetNestedElement(i);
        if(string(node->GetName())!="param") continue;
        string name;
        float val;
        s = node->GetAttribute("name");
        if(!s) throw runtime_error("Failed to read param attribute: name");
        name = read_string(s);
        s = node->GetCharacterData();
        if(!s || !from_string(s,val)) throw runtime_error("Failed to read param value");
        system->AddParameter(name,val);
    }
    // formula:
    if(system->HasEditableFormula())
    {
        vtkSmartPointer<vtkXMLDataElement> xml_formula = rule->FindNestedElementWithName("formula");
        if(!xml_formula) throw runtime_error("formula node not found in file");
        string formula = read_string(xml_formula->GetCharacterData());
        system->TestFormula(formula); // will throw on error
        system->SetFormula(formula); // (won't throw yet)
    }
    // pattern_description:
    vtkSmartPointer<vtkXMLDataElement> xml_pattern_description = rd->FindNestedElementWithName("pattern_description");
    if(!xml_pattern_description) system->SetPatternDescription(""); // optional, default is empty string
    else system->SetPatternDescription(read_string(xml_pattern_description->GetCharacterData()));
    // initial_pattern_generator:
    vtkSmartPointer<vtkXMLDataElement> xml_initial_pattern_generator = rd->FindNestedElementWithName("initial_pattern_generator");
    if(xml_initial_pattern_generator) // optional, default is none
        LoadInitialPatternGenerator(xml_initial_pattern_generator,system);
}

vtkSmartPointer<vtkXMLDataElement> RD_XMLWriter::BuildRDSystemXML(BaseRD* system)
{
    vtkSmartPointer<vtkXMLDataElement> rd = vtkSmartPointer<vtkXMLDataElement>::New();
    rd->SetName("RD");
    {
        vtkSmartPointer<vtkXMLDataElement> rule = vtkSmartPointer<vtkXMLDataElement>::New();
        rule->SetName("rule");
        bool is_inbuilt = !system->HasEditableFormula();
        if(is_inbuilt)
            rule->SetAttribute("type","inbuilt");
        else
            rule->SetAttribute("type","formula");
        rule->SetAttribute("name",system->GetRuleName().c_str());
        if(!is_inbuilt)
            rule->SetAttribute("number_of_chemicals",to_string(system->GetNumberOfChemicals()).c_str());
        rule->SetAttribute("timestep",to_string(system->GetTimestep()).c_str());
        {
            vtkSmartPointer<vtkXMLDataElement> rule_description = vtkSmartPointer<vtkXMLDataElement>::New();
            rule_description->SetName("description");
            rule_description->SetCharacterData(system->GetRuleDescription().c_str(),(int)system->GetRuleDescription().length());
            rule->AddNestedElement(rule_description);
        }
        for(int i=0;i<system->GetNumberOfParameters();i++)
        {
            vtkSmartPointer<vtkXMLDataElement> param = vtkSmartPointer<vtkXMLDataElement>::New();
            param->SetName("param");
            param->SetAttribute("name",system->GetParameterName(i).c_str());
            string s = to_string(system->GetParameterValue(i));
            param->SetCharacterData(s.c_str(),(int)s.length());
            rule->AddNestedElement(param);
        }
        if(!is_inbuilt)
        {
            vtkSmartPointer<vtkXMLDataElement> formula = vtkSmartPointer<vtkXMLDataElement>::New();
            formula->SetName("formula");
            formula->SetCharacterData(system->GetFormula().c_str(),(int)system->GetFormula().length());
            rule->AddNestedElement(formula);
        }
        rd->AddNestedElement(rule);
    }
    {
        vtkSmartPointer<vtkXMLDataElement> pattern_description = vtkSmartPointer<vtkXMLDataElement>::New();
        pattern_description->SetName("pattern_description");
        pattern_description->SetCharacterData(system->GetPatternDescription().c_str(),(int)system->GetPatternDescription().length());
        rd->AddNestedElement(pattern_description);
    }
    {
        vtkSmartPointer<vtkXMLDataElement> initial_pattern_generator = vtkSmartPointer<vtkXMLDataElement>::New();
        initial_pattern_generator->SetName("initial_pattern_generator");
        const vector<BaseOverlay*>& overlays = system->GetInitialPatternGenerator();
        for(int i=0;i<(int)overlays.size();i++)
            initial_pattern_generator->AddNestedElement(overlays[i]->GetAsXML());
        rd->AddNestedElement(initial_pattern_generator);
    }
    return rd;
}

bool RD_XMLReader::ShouldGenerateInitialPatternWhenLoading()
{
    this->Update();
    vtkSmartPointer<vtkXMLDataElement> root = this->XMLParser->GetRootElement();
    if(!root) throw runtime_error("No XML found in file");
    vtkSmartPointer<vtkXMLDataElement> rd = root->FindNestedElementWithName("RD");
    if(!rd) throw runtime_error("RD node not found in file");
    vtkSmartPointer<vtkXMLDataElement> initial_pattern_generator = rd->FindNestedElementWithName("initial_pattern_generator");
    if(!initial_pattern_generator) return false; // (element is optional)
    int i;
    const char *s = initial_pattern_generator->GetAttribute("apply_when_loading");
    bool read_ok = s && from_string(s,i);
    return read_ok && i==1;
}
