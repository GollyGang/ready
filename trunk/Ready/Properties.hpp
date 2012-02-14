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
#include "utils.hpp"

// STL:
#include <vector>
#include <string>
#include <map>

// a set of property types that can be saved to XML and loaded from XML
class Properties : public XML_Object
{
    public: 

        Properties(std::string name) : XML_Object(NULL),name(name) {}
        void InitializeFromXML(vtkXMLDataElement* node);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        // for accessing properties generically
        int GetNumberOfProperties() const;
        std::string GetPropertyName(int i) const;
        std::string GetPropertyType(int i) const; // currently: float, int, bool, float3

        // retrieve the value of a property
        float GetFloat(const std::string& name) const;
        int GetInt(const std::string& name) const;
        bool GetBool(const std::string& name) const;
        void GetFloat3(const std::string& name,float &a,float &b,float &c) const;

        // set the value of a property (will add it if it doesn't already exist)
        void Set(const std::string& name,float f) { this->float_properties[name] = f; }
        void Set(const std::string& name,int i) { this->int_properties[name] = i; }
        void Set(const std::string& name,bool b) { this->bool_properties[name] = b; }
        void Set(const std::string& name,float a,float b,float c);

    protected:
    
        std::string name; // used for saving to XML

        std::map<std::string,float> float_properties;
        std::map<std::string,int> int_properties;
        std::map<std::string,bool> bool_properties;
        std::map<std::string,std::vector<float> > float3_properties;
};
