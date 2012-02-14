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
#include "Properties.hpp"

// STL:
#include <string>
#include <stdexcept>
using namespace std;

void Properties::InitializeFromXML(vtkXMLDataElement *node)
{    
    for(int i=0;i<node->GetNumberOfNestedElements();i++)
    {
        vtkXMLDataElement *prop = node->GetNestedElement(i);
        string prop_name,prop_type;
        read_required_attribute(prop,"name",prop_name);
        read_required_attribute(prop,"type",prop_type);
        if(prop_type=="float")
        {
            float f;
            read_required_attribute(prop,"value",f);
            this->float_properties[prop_name] = f;
        }
        else if(prop_type=="int")
        {
            int i;
            read_required_attribute(prop,"value",i);
            this->int_properties[prop_name] = i;
        }
        else if(prop_type=="bool")
        {
            int i;
            read_required_attribute(prop,"value",i);
            this->bool_properties[prop_name] = (i==1);
        }
        else if(prop_type=="float3")
        {
            vector<float> f3(3);
            read_required_attribute(prop,"a",f3[0]);
            read_required_attribute(prop,"b",f3[1]);
            read_required_attribute(prop,"c",f3[2]);
            this->float3_properties[prop_name] = f3;
        }
        else throw runtime_error("Properties::InitializeFromXML : unrecognised type: "+prop_type);
    }
}

vtkSmartPointer<vtkXMLDataElement> Properties::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> root = vtkSmartPointer<vtkXMLDataElement>::New();
    root->SetName(this->name.c_str());
    for(map<string,float>::const_iterator it=this->float_properties.begin();it!=this->float_properties.end();it++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
        node->SetName("prop");
        node->SetAttribute("name",it->first.c_str());
        node->SetAttribute("type","float");
        node->SetFloatAttribute("value",it->second);
        root->AddNestedElement(node);
    }
    for(map<string,int>::const_iterator it=this->int_properties.begin();it!=this->int_properties.end();it++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
        node->SetName("prop");
        node->SetAttribute("name",it->first.c_str());
        node->SetAttribute("type","int");
        node->SetIntAttribute("value",it->second);
        root->AddNestedElement(node);
    }
    for(map<string,bool>::const_iterator it=this->bool_properties.begin();it!=this->bool_properties.end();it++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
        node->SetName("prop");
        node->SetAttribute("name",it->first.c_str());
        node->SetAttribute("type","bool");
        node->SetIntAttribute("value",it->second?1:0);
        root->AddNestedElement(node);
    }
    for(map<string,vector<float> >::const_iterator it=this->float3_properties.begin();it!=this->float3_properties.end();it++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
        node->SetName("prop");
        node->SetAttribute("name",it->first.c_str());
        node->SetAttribute("type","float3");
        node->SetFloatAttribute("a",it->second[0]);
        node->SetFloatAttribute("b",it->second[1]);
        node->SetFloatAttribute("c",it->second[2]);
        root->AddNestedElement(node);
    }
    return root;
}

float Properties::GetFloat(const std::string &name) const
{
    if(this->float_properties.count(name)) 
        return this->float_properties.find(name)->second;
    throw runtime_error("Properties::GetFloat: unknown property: "+name);
}

int Properties::GetInt(const std::string &name) const
{
    if(this->int_properties.count(name)) 
        return this->int_properties.find(name)->second;
    throw runtime_error("Properties::GetInt: unknown property: "+name);
}

bool Properties::GetBool(const std::string &name) const
{
    if(this->bool_properties.count(name)) 
        return this->bool_properties.find(name)->second;
    throw runtime_error("Properties::GetBool: unknown property: "+name);
}

int Properties::GetNumberOfProperties() const
{
    return int(this->float_properties.size() + this->int_properties.size() + this->bool_properties.size() + this->float3_properties.size());
}

std::string Properties::GetPropertyName(int i) const
{
    if(i<0 || i>=this->GetNumberOfProperties()) throw runtime_error("Properties::GetPropertyName : out of range");
    if(i<(int)this->float_properties.size())
    {
        map<string,float>::const_iterator it = this->float_properties.begin();
        advance(it,i);
        return it->first;
    }
    i -= (int)this->float_properties.size();
    if(i<(int)this->int_properties.size())
    {
        map<string,int>::const_iterator it = this->int_properties.begin();
        advance(it,i);
        return it->first;
    }
    i -= (int)this->int_properties.size();
    if(i<(int)this->bool_properties.size())
    {
        map<string,bool>::const_iterator it = this->bool_properties.begin();
        advance(it,i);
        return it->first;
    }
    i -= (int)this->bool_properties.size();
    if(i<(int)this->float3_properties.size())
    {
        map<string,vector<float> >::const_iterator it = this->float3_properties.begin();
        advance(it,i);
        return it->first;
    }
    throw runtime_error("Properties::GetPropertyName : internal error");
}

std::string Properties::GetPropertyType(int i) const
{
    if(i<0 || i>=this->GetNumberOfProperties()) throw runtime_error("Properties::GetPropertyType : out of range");
    if(i<(int)this->float_properties.size())
        return "float";
    i -= (int)this->float_properties.size();
    if(i<(int)this->int_properties.size())
        return "int";
    i -= (int)this->int_properties.size();
    if(i<(int)this->bool_properties.size())
        return "bool";
    i -= (int)this->bool_properties.size();
    if(i<(int)this->float3_properties.size())
        return "float3";
    throw runtime_error("Properties::GetPropertyType : internal error");
}

void Properties::GetFloat3(const std::string& name,float &a,float &b,float &c) const
{
    if(this->float3_properties.count(name)) 
    {
        const vector<float> &f3 = this->float3_properties.find(name)->second;
        a = f3[0]; b = f3[1]; c = f3[2];
    }
    throw runtime_error("Properties::GetFloat3: unknown property: "+name);
}

void Properties::Set(const std::string& name,float a,float b,float c)
{
    vector<float> f3(3);
    f3[0]=a; f3[1]=b; f3[2]=c;
    this->float3_properties[name] = f3;
}
