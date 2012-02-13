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
class wxVTKRenderWindowInteractor;

// readybase:
#include "utils.hpp"
class BaseRD;

// STL:
#include <map>
#include <string>

class Properties : public XML_Object
{
    public: 

        Properties(std::string name) : XML_Object(NULL),name(name) {}
        Properties(vtkXMLDataElement* node);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        void Set(const std::string& name,float f) { this->float_properties[name] = f; }
        void Set(const std::string& name,int i) { this->int_properties[name] = i; }
        void Set(const std::string& name,bool b) { this->bool_properties[name] = b; }
        float GetFloat(const std::string& name) const;
        int GetInt(const std::string& name) const;
        bool GetBool(const std::string& name) const;

    protected:
    
        std::string name;

        std::map<std::string,float> float_properties;
        std::map<std::string,int> int_properties;
        std::map<std::string,bool> bool_properties;
};

void InitializeVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings);
