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
#include <cassert>

// a property can have one of several types
class Property : public XML_Object
{
    public:

        Property(vtkXMLDataElement* node);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        Property(const std::string& name,float f) : XML_Object(NULL), name(name), type("float"), f1(f) {}
        Property(const std::string& name,int i) : XML_Object(NULL), name(name), type("int"), i(i) {}
        Property(const std::string& name,bool b) : XML_Object(NULL), name(name), type("bool"), b(b) {}
        Property(const std::string& name,const std::string& type,float a,float b,float c) // e.g. "color"
            : XML_Object(NULL), name(name), type(type), f1(a), f2(b), f3(c) {}
        Property(const std::string& name,const std::string& type,const std::string& c) // e.g. "chemical"
            : XML_Object(NULL), name(name), type(type), s(c) {}

        std::string GetName() const { return this->name; }
        std::string GetType() const { return this->type; }

        float GetFloat() const { assert(type=="float"); return this->f1; }
        int GetInt() const { assert(type=="int"); return this->i; }
        bool GetBool() const { assert(type=="bool"); return this->b; }
        void GetColor(float& r,float& g,float& b) const { assert(type=="color"); r=this->f1; g=this->f2; b=this->f3; }
        const std::string& GetChemical() const { assert(type=="chemical"); return this->s; }

        void SetFloat(float f) { assert(type=="float"); this->f1=f; }
        void SetInt(int i) { assert(type=="int"); this->i = i; }
        void SetBool(bool b) { assert(type=="bool"); this->b = b; }
        void SetColor(float r,float g,float b) { assert(type=="color"); this->f1=r; this->f2=g; this->f3=b; }
        void SetChemical(const std::string& s) { assert(type=="chemical"); this->s = s; }

    protected:

        std::string name;
        std::string type;
        float f1,f2,f3;
        int i;
        bool b;
        std::string s;
};

// a set of property types that can be saved to XML and loaded from XML
class Properties : public XML_Object
{
    public: 

        Properties(std::string set_name) : XML_Object(NULL),set_name(set_name) {}
        void OverwriteFromXML(vtkXMLDataElement* node);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        int GetNumberOfProperties() const { return (int)this->properties.size(); }
        const Property& GetProperty(int i) const { return this->properties[i]; }
        const Property& GetProperty(const std::string& name) const;
        Property& GetProperty(const std::string& name);
        void AddProperty(Property p);
        bool IsProperty(const std::string& name);
        void DeleteAllProperties() { this->properties.clear(); }

    protected:
    
        std::vector<Property> properties;
        std::string set_name; // used for saving to XML
};
