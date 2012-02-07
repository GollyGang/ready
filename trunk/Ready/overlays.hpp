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

// VTK:
#include <vtkSmartPointer.h>
#include <vtkXMLDataElement.h>

// local:
class BaseRD;

// TODO: split this into separate files when matured

// XML object interface
class XML_Object
{
    public:

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const =0;
};

// general mechanism for allowing re-use of XML nodes through label="..." and ref="..."
// TODO: this is not yet functional!
class Reusable_XML_Object : public XML_Object
{
    protected:

        Reusable_XML_Object(vtkXMLDataElement *node);

        std::string GetLabel() const { return this->label; }

    protected:

        std::string label;
};

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
            
        // apply the overlay
        void Apply(BaseRD *system,int x,int y,int z) const;

        // for saving to file, get the overlay as an XML element
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        static const char* GetTypeName() { return "overlay"; }

    protected:
    
        int iTargetChemical;
        BaseOperation *op;   // e.g. overwrite, add, multiply, etc.
        BaseFill *fill;     // e.g. constant value, white noise, named parameter, other chemical, etc.
        BaseShape *shape;  // e.g. rectangle, sphere, scattered shapes, etc.

    private:

        Overlay();          // not implemented
        Overlay(Overlay&); // not implemented
};
