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

// STL:
#include <string>
#include <vector>

// VTK:
#include <vtkSmartPointer.h>
#include <vtkXMLDataElement.h>

// local:
#include "utils.hpp"
class AbstractRD;

// internal:
class BaseOperation;
class BaseFill;
class BaseShape;

// an overlay is a filled shape to be drawn on top of an image (think: stacked transparencies)
class Overlay : public XML_Object
{
    public:
    
        // can construct from an XML node
        Overlay(vtkXMLDataElement* node);
        ~Overlay();
            
        // for saving to file, get the overlay as an XML element
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        static const char* GetTypeName() { return "overlay"; }

        int GetTargetChemical() const { return this->iTargetChemical; }

        void Apply(float& val,AbstractRD* system,int x,int y,int z) const;

    protected:
    
        int iTargetChemical;
        BaseOperation *op;                 // e.g. overwrite, add, multiply, etc.
        BaseFill *fill;                   // e.g. constant value, white noise, named parameter, other chemical, etc.
        std::vector<BaseShape*> shapes;  // e.g. rectangle, sphere, scattered shapes, etc.

    private:

        Overlay();          // not implemented
        Overlay(Overlay&); // not implemented
};
