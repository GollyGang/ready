/*  Copyright 2011-2016 The Ready Bunch

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

// Local:
#include "InitialPatternGenerator.hpp"

InitialPatternGenerator::~InitialPatternGenerator()
{
    RemoveAllOverlays();
}

// ---------------------------------------------------------------------

void InitialPatternGenerator::RemoveAllOverlays()
{
    for (size_t i = 0; i < this->overlays.size(); i++)
    {
        delete this->overlays[i];
    }
    this->overlays.clear();
}

// ---------------------------------------------------------------------

void InitialPatternGenerator::ReadFromXML(vtkXMLDataElement* node)
{
    this->RemoveAllOverlays();
    if (node) // IPG is optional in the XML, default is none
    {
        for (int i = 0; i < node->GetNumberOfNestedElements(); i++)
        {
            this->overlays.push_back(new Overlay(node->GetNestedElement(i)));
        }
    }
}

// ---------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> InitialPatternGenerator::GetAsXML(bool generate_initial_pattern_when_loading) const
{
    // initial pattern generator
    vtkSmartPointer<vtkXMLDataElement> ipg = vtkSmartPointer<vtkXMLDataElement>::New();
    ipg->SetName("initial_pattern_generator");
    ipg->SetAttribute("apply_when_loading", generate_initial_pattern_when_loading ? "true" : "false");
    for (size_t i = 0; i < this->overlays.size(); i++)
    {
        ipg->AddNestedElement(this->overlays[i]->GetAsXML());
    }
    return ipg;
}

// ---------------------------------------------------------------------

void InitialPatternGenerator::CreateDefaultInitialPatternGenerator(size_t num_chemicals)
{
    RemoveAllOverlays();

    // this is ungainly, will need to improve later when we allow the user to edit the IPG through the Info Pane
    vtkSmartPointer<vtkXMLDataElement> ow = vtkSmartPointer<vtkXMLDataElement>::New();
    ow->SetName("overwrite");
    vtkSmartPointer<vtkXMLDataElement> c1 = vtkSmartPointer<vtkXMLDataElement>::New();
    c1->SetName("constant");
    c1->SetFloatAttribute("value", 1.0);
    vtkSmartPointer<vtkXMLDataElement> ew = vtkSmartPointer<vtkXMLDataElement>::New();
    ew->SetName("everywhere");

    vtkSmartPointer<vtkXMLDataElement> ov1 = vtkSmartPointer<vtkXMLDataElement>::New();
    ov1->SetName("overlay");
    ov1->SetAttribute("chemical", "a");
    ov1->AddNestedElement(ow);
    ov1->AddNestedElement(c1);
    ov1->AddNestedElement(ew);
    this->overlays.push_back(new Overlay(ov1));

    vtkSmartPointer<vtkXMLDataElement> wn = vtkSmartPointer<vtkXMLDataElement>::New();
    wn->SetName("white_noise");
    wn->SetFloatAttribute("low", 0.0);
    wn->SetFloatAttribute("high", 1.0);
    vtkSmartPointer<vtkXMLDataElement> p1 = vtkSmartPointer<vtkXMLDataElement>::New();
    p1->SetName("point3D");
    p1->SetFloatAttribute("x", 0.5);
    p1->SetFloatAttribute("y", 0);
    p1->SetFloatAttribute("z", 0);
    vtkSmartPointer<vtkXMLDataElement> p2 = vtkSmartPointer<vtkXMLDataElement>::New();
    p2->SetName("point3D");
    p2->SetFloatAttribute("x", 1.0);
    p2->SetFloatAttribute("y", 1.0);
    p2->SetFloatAttribute("z", 1.0);
    vtkSmartPointer<vtkXMLDataElement> r = vtkSmartPointer<vtkXMLDataElement>::New();
    r->SetName("rectangle");
    r->AddNestedElement(p1);
    r->AddNestedElement(p2);
    for(size_t iChem = 0; iChem < num_chemicals; iChem++)
    {
        vtkSmartPointer<vtkXMLDataElement> ov = vtkSmartPointer<vtkXMLDataElement>::New();
        ov->SetName("overlay");
        ov->SetAttribute("chemical", GetChemicalName(iChem).c_str());
        ov->AddNestedElement(ow);
        ov->AddNestedElement(wn);
        ov->AddNestedElement(r);
        this->overlays.push_back(new Overlay(ov));
    }
}
