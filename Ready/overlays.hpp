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

class Point3D : public XML_Object
{
    public:
    
        Point3D(vtkXMLDataElement *node);
        
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        static const char* GetTypeName() { return "Point3D"; }

    public:
    
        float x,y,z;
};

class BaseOperation : public Reusable_XML_Object
{
    public:

        // construct when we don't know the derived type
        static BaseOperation* New(vtkXMLDataElement* node);

        virtual void Apply(float& target,float value) const =0;

    protected:
        
        // can construct from an XML node
        BaseOperation(vtkXMLDataElement* node) : Reusable_XML_Object(node) {}
};

class BaseFill : public Reusable_XML_Object
{
    public:

        // construct when we don't know the derived type
        static BaseFill* New(vtkXMLDataElement* node);

        // what value would this fill type be at the given location, given the existing image as in system
        virtual float GetValue(BaseRD *system,int x,int y,int z) const =0;

    protected:

        // can construct from an XML node
        BaseFill(vtkXMLDataElement* node) : Reusable_XML_Object(node) {}
};
 
class BaseShape : public Reusable_XML_Object
{
    public:

        // construct when we don't know the derived type
        static BaseShape* New(vtkXMLDataElement* node);

        virtual bool IsInside(float x,float y,float z,int dimensionality) const =0;

    protected:

        // can construct from an XML node
        BaseShape(vtkXMLDataElement* node) : Reusable_XML_Object(node) {}
};

// an overlay is a filled shape to be drawn on top of an image (think: stacked transparencies)
class Overlay : public XML_Object
{
    public:
    
        // can construct from an XML node
        Overlay(vtkXMLDataElement* node);
            
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
};
