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
#include "BaseOverlay.hpp"
#include "utils.hpp"

// STL:
#include <stdexcept>
using namespace std;

void RectangleOverlay::Apply(int iChemical,const PointND& at,float& value) const
{
    if(iChemical != this->iChemical) return; // leave other channels untouched
    if(!at.InRect(this->corner1,this->corner2)) return; // leave points outside untouched
    
    float f;
    switch(this->fill_mode)
    {
        case Constant: f = this->value1; break;
        case WhiteNoise: f = frand(this->value1,this->value2); break;
    }
    switch(this->paste_mode)
    {
        case Overwrite: value = f; break;
        case Add: value += f; break;
    }
}

vtkSmartPointer<vtkXMLDataElement> PointND::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
    xml->SetName("PointND");
    xml->SetIntAttribute("x",this->x);
    xml->SetIntAttribute("y",this->y);
    xml->SetIntAttribute("z",this->z);
    return xml;
}

vtkSmartPointer<vtkXMLDataElement> RectangleOverlay::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
    xml->SetName(RectangleOverlay::GetXMLName());
    xml->SetIntAttribute("iChemical",this->iChemical);
    xml->SetFloatAttribute("value1",this->value1);
    if(this->fill_mode!=Constant)
        xml->SetFloatAttribute("value2",this->value2);
    switch(this->fill_mode) {
        case Constant: xml->SetAttribute("fill_mode","Constant"); break;
        case WhiteNoise: xml->SetAttribute("fill_mode","WhiteNoise"); break;
    }
    switch(this->paste_mode) {
        case Overwrite: xml->SetAttribute("paste_mode","Overwrite"); break;
        case Add: xml->SetAttribute("paste_mode","Add"); break;
    }
    xml->AddNestedElement(this->corner1.GetAsXML());
    xml->AddNestedElement(this->corner2.GetAsXML());
    return xml;
}

RectangleOverlay::RectangleOverlay(vtkXMLDataElement* node)
{
    // (check name matches?)
    if(!from_string(node->GetAttribute("iChemical"),this->iChemical))
        throw runtime_error("RectangleOverlay::RectangleOverlay : failed to read required attribute 'iChemical'");
}