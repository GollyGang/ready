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
    xml->SetFloatAttribute("x",this->x);
    xml->SetFloatAttribute("y",this->y);
    xml->SetFloatAttribute("z",this->z);
    return xml;
}

PointND::PointND(vtkXMLDataElement *node)
{
    if(!from_string(node->GetAttribute("x"),this->x))
        throw runtime_error("PointND::PointND : unable to read required attribute 'x'");
    if(!from_string(node->GetAttribute("y"),this->y))
        throw runtime_error("PointND::PointND : unable to read required attribute 'y'");
    if(!from_string(node->GetAttribute("z"),this->z))
        throw runtime_error("PointND::PointND : unable to read required attribute 'z'");
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

BaseOverlay::BaseOverlay(vtkXMLDataElement* node)
{
   string s;
    from_string(node->GetAttribute("fill_mode"),s);
    if(s=="Constant") this->fill_mode=Constant;
    else if(s=="WhiteNoise") this->fill_mode=WhiteNoise;
    else throw runtime_error("BaseOverlay::BaseOverlay : unsupported fill_mode");
    if(!from_string(node->GetAttribute("value1"),this->value1))
        throw runtime_error("BaseOverlay::BaseOverlay : failed to read required attribute 'value1'");
    if(this->fill_mode!=Constant && !from_string(node->GetAttribute("value2"),this->value2))
        throw runtime_error("BaseOverlay::BaseOverlay : failed to read required attribute 'value2'");
    from_string(node->GetAttribute("paste_mode"),s);
    if(s=="Overwrite") this->paste_mode=Overwrite;
    else if(s=="Add") this->paste_mode=Add;
    else throw runtime_error("BaseOverlay::BaseOverlay : unsupported paste_mode");
 }

RectangleOverlay::RectangleOverlay(vtkXMLDataElement* node) : BaseOverlay(node)
{
    if(!from_string(node->GetAttribute("iChemical"),this->iChemical))
        throw runtime_error("RectangleOverlay::RectangleOverlay : failed to read required attribute 'iChemical'");
    if(node->GetNumberOfNestedElements()!=2)
        throw runtime_error("RectangleOverlay::RectangleOverlay : should have two corners");
    this->corner1 = PointND(node->GetNestedElement(0));
    this->corner2 = PointND(node->GetNestedElement(1));
}