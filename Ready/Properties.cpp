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

Property::Property(vtkXMLDataElement* node) : XML_Object(node)
{    
    this->name = string(node->GetName());
    read_required_attribute(node,"type",this->type);
    if(this->type=="float")
        read_required_attribute(node,"value",this->f1);
    else if(this->type=="int")
        read_required_attribute(node,"value",this->i);
    else if(this->type=="bool")
    {
        read_required_attribute(node,"value",this->i);
        this->b = (this->i==1);
    }
    else if(this->type=="color")
    {
        read_required_attribute(node,"r",this->f1);
        read_required_attribute(node,"g",this->f2);
        read_required_attribute(node,"b",this->f3);
    }
    else if(this->type=="chemical")
        read_required_attribute(node,"value",this->s);
    else throw runtime_error("Property::InitializeFromXML : unrecognised type: "+this->type);
}

vtkSmartPointer<vtkXMLDataElement> Property::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
    node->SetName(this->name.c_str());
    node->SetAttribute("type",this->type.c_str());
    if(this->type=="float")
        node->SetFloatAttribute("value",this->f1);
    else if(this->type=="int")
        node->SetIntAttribute("value",this->i);
    else if(this->type=="bool")
        node->SetIntAttribute("value",this->b?1:0);
    else if(this->type=="color")
    {
        node->SetFloatAttribute("r",f1);
        node->SetFloatAttribute("g",f2);
        node->SetFloatAttribute("b",f3);
    }
    else if(this->type=="chemical")
        node->SetAttribute("value",this->s.c_str());
    else throw runtime_error("Property::GetAsXML : unrecognised type: "+this->type);
    return node;
}

// ==============================================================================================

void Properties::OverwriteFromXML(vtkXMLDataElement *node)
{    
    this->set_name = string(node->GetName());
    for(int i=0;i<node->GetNumberOfNestedElements();i++)
    {
        Property prop(node->GetNestedElement(i));
        // does this property already exist?
        if(IsProperty(prop.GetName()))
        {
            this->GetProperty(prop.GetName()) = prop;
        }
        else
        {
            this->properties.push_back(prop); 
            // could give warning that we don't know what this property is - it could be from the future, or a typo
        }
    }
}

vtkSmartPointer<vtkXMLDataElement> Properties::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> root = vtkSmartPointer<vtkXMLDataElement>::New();
    root->SetName(this->set_name.c_str());
    for(vector<Property>::const_iterator it=this->properties.begin();it!=this->properties.end();it++)
        root->AddNestedElement(it->GetAsXML());
    return root;
}

const Property& Properties::GetProperty(const std::string& name) const
{
    for(vector<Property>::const_iterator it=this->properties.begin();it!=this->properties.end();it++)
    {
        if(it->GetName()==name)
            return *it;
    }
    throw runtime_error("Properties::GetProperty : unrecognised property: "+name);
}

Property& Properties::GetProperty(const std::string& name)
{
    for(vector<Property>::iterator it=this->properties.begin();it!=this->properties.end();it++)
    {
        if(it->GetName()==name)
            return *it;
    }
    throw runtime_error("Properties::GetProperty : unrecognised property: "+name);
}

void Properties::AddProperty(Property p)
{
    this->properties.push_back(p);
}

bool Properties::IsProperty(const std::string& name)
{
    for(vector<Property>::const_iterator it=this->properties.begin();it!=this->properties.end();it++)
    {
        if(it->GetName()==name)
            return true;
    }
    return false;
}
