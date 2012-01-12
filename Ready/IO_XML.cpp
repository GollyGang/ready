// local:
#include "IO_XML.hpp"
#include "BaseRD.hpp"
#include "utils.hpp"

// VTK:
#include <vtkXMLUtilities.h>
#include <vtkSmartPointer.h>
#include <vtkXMLDataParser.h>

// STL:
#include <string>
#include <vector>
#include <sstream>
using namespace std;

// stdlib:
#include <stdio.h>

vtkStandardNewMacro(RD_XMLWriter);
vtkStandardNewMacro(RD_XMLReader);

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

void SetSystemFromXML(BaseRD* system,vtkXMLDataElement* rd)
{
    string rule_name,rule_description,pattern_description,formula;
    vector<pair<string,float> > params;
    float timestep;

    // first load everything into local variables (in case there's a problem)
    {
        const char *s;
        vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
        if(!rule) throw runtime_error("rule node not found in file");
        s = rule->GetAttribute("name");
        if(!s) throw runtime_error("Failed to read rule attribute: name");
        rule_name = read_string(s);
        s = rule->GetAttribute("timestep");
        if(!s || !read_float(s,timestep)) throw runtime_error("Failed to read rule attribute: timestep");
        vtkSmartPointer<vtkXMLDataElement> xml_rule_description = rule->FindNestedElementWithName("description");
        if(!xml_rule_description) rule_description=""; // optional, default is empty string
        else rule_description = read_string(xml_rule_description->GetCharacterData());
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
            if(!s || !read_float(s,val)) throw runtime_error("Failed to read param value");
            params.push_back(make_pair(name,val));
        }
        vtkSmartPointer<vtkXMLDataElement> xml_formula = rule->FindNestedElementWithName("formula");
        if(!xml_formula) throw runtime_error("formula node not found in file");
        formula = read_string(xml_formula->GetCharacterData());
        vtkSmartPointer<vtkXMLDataElement> xml_pattern_description = rd->FindNestedElementWithName("pattern_description");
        if(!xml_pattern_description) pattern_description=""; // optional, default is empty string
        else pattern_description = read_string(xml_pattern_description->GetCharacterData());
    }

    // set the system properties from our local variables
    system->SetRuleName(rule_name);
    system->SetRuleDescription(rule_description);
    system->SetPatternDescription(pattern_description);
    system->SetTimestep(timestep);
    system->DeleteAllParameters();
    for(int i=0;i<(int)params.size();i++)
        system->AddParameter(params[i].first,params[i].second);
    system->TestProgram(formula); // will throw on error
    system->SetProgram(formula);
}

void RD_XMLReader::SetFromXML(BaseRD* rd_system)
{
    vtkSmartPointer<vtkXMLDataElement> root = this->XMLParser->GetRootElement();
    if(!root) throw runtime_error("No XML found in file");
    vtkSmartPointer<vtkXMLDataElement> rd = root->FindNestedElementWithName("RD");
    if(!rd) throw runtime_error("RD node not found in file");
    SetSystemFromXML(rd_system,rd);
}

void RD_XMLWriter::WriteRDSystemXML(BaseRD* system,ostream& os,vtkIndent indent)
{
    // TODO
}
