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
#include "overlays.hpp"
#include "ImageRD.hpp"
#include "utils.hpp"

// VTK:
#include <vtkXMLDataElement.h>
#include <vtkImageData.h>
#include <vtkMath.h>

// STL:
#include <stdexcept>
using namespace std;

// ------------------------------------------------------------------------------------------------

/// Base class for a mathematical operation to be carried out at a particular location in the RD system.
class BaseOperation : public XML_Object
{
    public:

        virtual ~BaseOperation() {}

        /// construct when we don't know the derived type (returns NULL if name is unknown)
        static BaseOperation* New(vtkXMLDataElement* node);

        virtual void Apply(float& target,float value) const =0;

    protected:
        
        /// can construct from an XML node
        BaseOperation(vtkXMLDataElement* node) : XML_Object(node) {}
};

/// Base class for different ways of specifying values at a particular location in the RD system.
class BaseFill : public XML_Object
{
    public:

        virtual ~BaseFill() {}

        /// construct when we don't know the derived type (returns NULL if name is unknown)
        static BaseFill* New(vtkXMLDataElement* node);

        /// what value would this fill type be at the given location, given the existing data
        virtual float GetValue(AbstractRD *system,vector<float> vals,float x,float y,float z) const =0;

    protected:

        /// can construct from an XML node
        BaseFill(vtkXMLDataElement* node) : XML_Object(node) {}
};
 
/// Base class for different shapes that we can draw onto the RD system.
class BaseShape : public XML_Object
{
    public:

        virtual ~BaseShape() {}

        /// construct when we don't know the derived type (returns NULL if name is unknown)
        static BaseShape* New(vtkXMLDataElement* node);

        virtual bool IsInside(float x,float y,float z,float X,float Y,float Z,int dimensionality) const =0;

    protected:

        /// can construct from an XML node
        BaseShape(vtkXMLDataElement* node) : XML_Object(node) {}
};

// ------------------------------------------------------------------------------------------------

Overlay::Overlay(vtkXMLDataElement* node) : XML_Object(node), op(NULL), fill(NULL)
{
    string s;
    read_required_attribute(node,"chemical",s);
    this->iTargetChemical = IndexFromChemicalName(s);
    const int n_nested = node->GetNumberOfNestedElements();
    if(n_nested<3)
        throw runtime_error("overlay : expected at least 3 nested elements (operation, fill, shape)");
    BaseShape *pShape;
    BaseOperation *pOp;
    BaseFill *pFill;
    for(int i_nested=0;i_nested<n_nested;i_nested++)
    {
        vtkXMLDataElement *subnode = node->GetNestedElement(i_nested);
        // is this an operation element?
        pOp = BaseOperation::New(subnode);
        if(pOp) { 
            this->op = pOp; // TODO: check if already supplied
            continue; // (save time parsing as other types)
        }
        // is this a fill element?
        pFill = BaseFill::New(subnode);
        if(pFill) {
            this->fill = pFill; // TODO: check if already supplied
            continue;
        }
        // must be a shape element?
        pShape = BaseShape::New(subnode);
        if(pShape)
            this->shapes.push_back(pShape);
        else throw runtime_error(string("Unknown overlay element: ")+subnode->GetName());
    }
    if(this->op == NULL) throw runtime_error("overlay: missing operation element");
    if(this->fill == NULL) throw runtime_error("overlay: missing fill element");
    if(this->shapes.empty()) throw runtime_error("overlay: missing shape element");
}

Overlay::~Overlay()
{
    delete this->op;
    delete this->fill;
    for(int i=0;i<(int)this->shapes.size();i++)
        delete this->shapes[i];
}

vtkSmartPointer<vtkXMLDataElement> Overlay::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
    xml->SetName(Overlay::GetTypeName());
    xml->SetAttribute("chemical",GetChemicalName(this->iTargetChemical).c_str());
    xml->AddNestedElement(this->op->GetAsXML());
    xml->AddNestedElement(this->fill->GetAsXML());
    for(int i=0;i<(int)this->shapes.size();i++)
        xml->AddNestedElement(this->shapes[i]->GetAsXML());
    return xml;
}

float Overlay::Apply(vector<float> vals,AbstractRD* system,float x,float y,float z) const
{
    float val = vals[this->iTargetChemical];
    for(int iShape=0;iShape<(int)this->shapes.size();iShape++)
    {
        if( this->shapes[iShape]->IsInside( x, y, z, system->GetX(), system->GetY(), system->GetZ(), system->GetArenaDimensionality() ) )
        {
            this->op->Apply( val, this->fill->GetValue(system,vals,x,y,z) );
            vals[this->iTargetChemical] = val; // in case there are multiple shapes at this location in this overlay
        }
    }
    return val;
}

// --------------------------------------------------------------------------------------------------

class Point3D : public XML_Object
{
    public:
    
        Point3D(vtkXMLDataElement *node);
        
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        static const char* GetTypeName() { return "point3D"; }

    public:
    
        float x,y,z;
};

Point3D::Point3D(vtkXMLDataElement *node) : XML_Object(node)
{
    read_required_attribute(node,"x",this->x);
    read_required_attribute(node,"y",this->y);
    read_required_attribute(node,"z",this->z);
}

vtkSmartPointer<vtkXMLDataElement> Point3D::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
    xml->SetName(Point3D::GetTypeName());
    xml->SetFloatAttribute("x",this->x);
    xml->SetFloatAttribute("y",this->y);
    xml->SetFloatAttribute("z",this->z);
    return xml;
}
// -------------------------- the derived types ----------------------------------

// -------- operations: -----------

class Add : public BaseOperation
{
    public:

        Add(vtkXMLDataElement* node) : BaseOperation(node) {}

        static const char* GetTypeName() { return "add"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Add::GetTypeName());
            return xml;
        }

        virtual void Apply(float& target,float value) const { target += value; }
};

class Subtract : public BaseOperation
{
    public:

        Subtract(vtkXMLDataElement* node) : BaseOperation(node) {}

        static const char* GetTypeName() { return "subtract"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Subtract::GetTypeName());
            return xml;
        }

        virtual void Apply(float& target,float value) const { target -= value; }
};

class Overwrite : public BaseOperation
{
    public:

        Overwrite(vtkXMLDataElement* node) : BaseOperation(node) {}

        static const char* GetTypeName() { return "overwrite"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Overwrite::GetTypeName());
            return xml;
        }

        virtual void Apply(float& target,float value) const { target = value; }
};

class Multiply : public BaseOperation
{
    public:

        Multiply(vtkXMLDataElement* node) : BaseOperation(node) {}

        static const char* GetTypeName() { return "multiply"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Multiply::GetTypeName());
            return xml;
        }

        virtual void Apply(float& target,float value) const { target *= value; }
};

class Divide : public BaseOperation
{
    public:

        Divide(vtkXMLDataElement* node) : BaseOperation(node) {}

        static const char* GetTypeName() { return "divide"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Divide::GetTypeName());
            return xml;
        }

        virtual void Apply(float& target,float value) const { target /= value; }
};

// -------- fill methods: -----------

class Constant : public BaseFill
{
    public:

        Constant(vtkXMLDataElement* node) : BaseFill(node)
        {
            read_required_attribute(node,"value",this->value);
        }

        static const char* GetTypeName() { return "constant"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Constant::GetTypeName());
            xml->SetFloatAttribute("value",this->value);
            return xml;
        }

        virtual float GetValue(AbstractRD *system,vector<float> vals,float x,float y,float z) const
        {
            return this->value;
        }

    protected:

        float value;
};

class OtherChemical : public BaseFill
{
    public:

        OtherChemical(vtkXMLDataElement* node) : BaseFill(node)
        {
            string s;
            read_required_attribute(node,"chemical",s);
            this->iOtherChemical = IndexFromChemicalName(s);
        }

        static const char* GetTypeName() { return "other_chemical"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(OtherChemical::GetTypeName());
            xml->SetAttribute("chemical",GetChemicalName(this->iOtherChemical).c_str());
            return xml;
        }

        virtual float GetValue(AbstractRD *system,vector<float> vals,float x,float y,float z) const
        {
            if(this->iOtherChemical<0 || this->iOtherChemical>=vals.size())
                throw runtime_error("OtherChemical:GetValue : chemical out of range");
            return vals[this->iOtherChemical];
        }

    protected:

        int iOtherChemical;
};

class Parameter : public BaseFill
{
    public:

        Parameter(vtkXMLDataElement* node) : BaseFill(node)
        {
            read_required_attribute(node,"name",this->parameter_name);
        }

        static const char* GetTypeName() { return "parameter"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Parameter::GetTypeName());
            xml->SetAttribute("name",this->parameter_name.c_str());
            return xml;
        }

        virtual float GetValue(AbstractRD *system,vector<float> vals,float x,float y,float z) const
        {
            return system->GetParameterValueByName(this->parameter_name.c_str());
        }

    protected:

        string parameter_name;
};

class WhiteNoise : public BaseFill
{
    public:

        WhiteNoise(vtkXMLDataElement* node) : BaseFill(node)
        {
            read_required_attribute(node,"low",this->low);
            read_required_attribute(node,"high",this->high);
        }

        static const char* GetTypeName() { return "white_noise"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(WhiteNoise::GetTypeName());
            xml->SetFloatAttribute("low",this->low);
            xml->SetFloatAttribute("high",this->high);
            return xml;
        }

        virtual float GetValue(AbstractRD *system,vector<float> vals,float x,float y,float z) const
        {
            return frand(this->low,this->high);
        }

    protected:

        float low,high;
};

class LinearGradient : public BaseFill
{
    public:

        LinearGradient(vtkXMLDataElement* node) : BaseFill(node)
        {
            read_required_attribute(node,"val1",this->val1);
            read_required_attribute(node,"val2",this->val2);
            if(node->GetNumberOfNestedElements()!=2)
                throw runtime_error("linear_gradient: expected two nested elements (point3D,point3D)");
            this->p1 = new Point3D(node->GetNestedElement(0));
            this->p2 = new Point3D(node->GetNestedElement(1));
        }
        virtual ~LinearGradient() { delete this->p1; delete this->p2; }

        static const char* GetTypeName() { return "linear_gradient"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(LinearGradient::GetTypeName());
            xml->SetFloatAttribute("val1",this->val1);
            xml->SetFloatAttribute("val2",this->val2);
            xml->AddNestedElement(this->p1->GetAsXML());
            xml->AddNestedElement(this->p2->GetAsXML());
            return xml;
        }

        virtual float GetValue(AbstractRD *system,vector<float> vals,float x,float y,float z) const
        {
            float rel_x = x/system->GetX();
            float rel_y = y/system->GetY();
            float rel_z = z/system->GetZ();
            // project this point onto the linear gradient axis
            return this->val1 + (this->val2-this->val1) 
                * ( (rel_x-this->p1->x)*(this->p2->x-this->p1->x) 
                  + (rel_y-this->p1->y)*(this->p2->y-this->p1->y) 
                  + (rel_z-this->p1->z)*(this->p2->z-this->p1->z) )
                / hypot3(this->p2->x-this->p1->x,this->p2->y-this->p1->y,this->p2->z-this->p1->z) ;
        }

    protected:

        float val1,val2;
        Point3D *p1,*p2;
};

class RadialGradient : public BaseFill
{
    public:

        RadialGradient(vtkXMLDataElement* node) : BaseFill(node)
        {
            read_required_attribute(node,"val1",this->val1);
            read_required_attribute(node,"val2",this->val2);
            if(node->GetNumberOfNestedElements()!=2)
                throw runtime_error("radial_gradient: expected two nested elements (point3D,point3D)");
            this->p1 = new Point3D(node->GetNestedElement(0));
            this->p2 = new Point3D(node->GetNestedElement(1));
        }
        virtual ~RadialGradient() { delete this->p1; delete this->p2; }

        static const char* GetTypeName() { return "radial_gradient"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(RadialGradient::GetTypeName());
            xml->SetFloatAttribute("val1",this->val1);
            xml->SetFloatAttribute("val2",this->val2);
            xml->AddNestedElement(this->p1->GetAsXML());
            xml->AddNestedElement(this->p2->GetAsXML());
            return xml;
        }

        virtual float GetValue(AbstractRD *system,vector<float> vals,float x,float y,float z) const
        {
            // convert p1 and p2 to absolute coordinates
            float rp1x = p1->x * system->GetX();
            float rp1y = p1->y * system->GetY();
            float rp1z = p1->z * system->GetZ();
            float rp2x = p2->x * system->GetX();
            float rp2y = p2->y * system->GetY();
            float rp2z = p2->z * system->GetZ();
            return val1 + (val2-val1) * hypot3(x-rp1x,y-rp1y,z-rp1z) / hypot3(rp2x-rp1x,rp2y-rp1y,rp2z-rp1z);
        }

    protected:

        float val1,val2;
        Point3D *p1,*p2;
};

// -------- shapes: -----------

class Everywhere : public BaseShape
{
    public:

        Everywhere(vtkXMLDataElement* node) : BaseShape(node) {}

        static const char* GetTypeName() { return "everywhere"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Everywhere::GetTypeName());
            return xml;
        }

        virtual bool IsInside(float x,float y,float z,float X,float Y,float Z,int dimensionality) const 
        { 
            return true; 
        }
};

class Rectangle : public BaseShape
{
    public:

        Rectangle(vtkXMLDataElement* node) : BaseShape(node)
        {
            if(node->GetNumberOfNestedElements()!=2)
                throw runtime_error("rectangle: expected two nested elements (point3D,point3D)");
            this->a = new Point3D(node->GetNestedElement(0));
            this->b = new Point3D(node->GetNestedElement(1));
        }
        virtual ~Rectangle() { delete this->a; delete this->b; }

        static const char* GetTypeName() { return "rectangle"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Rectangle::GetTypeName());
            xml->AddNestedElement(this->a->GetAsXML());
            xml->AddNestedElement(this->b->GetAsXML());
            return xml;
        }

        virtual bool IsInside(float x,float y,float z,float X,float Y,float Z,int dimensionality) const
        {
            float rel_x = x/X;
            float rel_y = y/Y;
            float rel_z = z/Z;
            switch(dimensionality)
            {
                default:
                case 1: return rel_x>=this->a->x && rel_x<=this->b->x;
                case 2: return rel_x>=this->a->x && rel_x<=this->b->x && 
                               rel_y>=this->a->y && rel_y<=this->b->y;
                case 3: return rel_x>=this->a->x && rel_x<=this->b->x && 
                               rel_y>=this->a->y && rel_y<=this->b->y && 
                               rel_z>=this->a->z && rel_z<=this->b->z;
            }
        }

    protected:

        Point3D *a,*b;
};

class Circle : public BaseShape
{
    public:

        Circle(vtkXMLDataElement* node) : BaseShape(node)
        {
            read_required_attribute(node,"radius",this->radius);
            if(node->GetNumberOfNestedElements()!=1)
                throw runtime_error("circle: expected one nested element (point3D)");
            this->c = new Point3D(node->GetNestedElement(0));
        }
        virtual ~Circle() { delete this->c; }

        static const char* GetTypeName() { return "circle"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Circle::GetTypeName());
            xml->SetFloatAttribute("radius",this->radius);
            xml->AddNestedElement(this->c->GetAsXML());
            return xml;
        }

        virtual bool IsInside(float x,float y,float z,float X,float Y,float Z,int dimensionality) const
        {
            // convert the center and radius to absolute coordinates
            float cx = this->c->x * X;
            float cy = this->c->y * Y;
            float cz = this->c->z * Z;
            float abs_radius = this->radius * max(X,max(Y,Z)); // (radius is proportional to the largest dimension)
            switch(dimensionality)
            {
                default:
                case 1: return sqrt(pow(x-cx,2.0f)) < abs_radius;
                case 2: return sqrt(pow(x-cx,2.0f)+pow(y-cy,2.0f)) < abs_radius;
                case 3: return sqrt(pow(x-cx,2.0f)+pow(y-cy,2.0f)+pow(z-cz,2.0f)) < abs_radius;
            }
        }

    protected:

        Point3D *c;
        float radius;
};

class Pixel : public BaseShape
{
    public:

        Pixel(vtkXMLDataElement* node) : BaseShape(node)
        {
            read_required_attribute(node,"x",this->px);
            read_required_attribute(node,"y",this->py);
            read_required_attribute(node,"z",this->pz);
        }

        static const char* GetTypeName() { return "pixel"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Pixel::GetTypeName());
            xml->SetIntAttribute("x",this->px);
            xml->SetIntAttribute("y",this->py);
            xml->SetIntAttribute("z",this->pz);
            return xml;
        }

        virtual bool IsInside(float x,float y,float z,float X,float Y,float Z,int dimensionality) const
        {
            switch(dimensionality)
            {
                default:
                case 1: return vtkMath::Round(x)==this->px;
                case 2: return vtkMath::Round(x)==this->px && vtkMath::Round(y)==this->py;
                case 3: return vtkMath::Round(x)==this->px && vtkMath::Round(y)==this->py && vtkMath::Round(z)==this->pz;
            }
        }

    protected:

        int px,py,pz;
};

// -------------------------------------------------------------------------------

// when you create a new derived class, add it to the appropriate factory method here:

/* static */ BaseOperation* BaseOperation::New(vtkXMLDataElement *node)
{
    string name(node->GetName());
    if(name==Overwrite::GetTypeName())         return new Overwrite(node);
    else if(name==Add::GetTypeName())          return new Add(node);
    else if(name==Subtract::GetTypeName())     return new Subtract(node);
    else if(name==Multiply::GetTypeName())     return new Multiply(node);
    else if(name==Divide::GetTypeName())       return new Divide(node);
    else                                       return NULL;
}

/* static */ BaseFill* BaseFill::New(vtkXMLDataElement* node)
{
    string name(node->GetName());
    if(name==Constant::GetTypeName())             return new Constant(node);
    else if(name==WhiteNoise::GetTypeName())      return new WhiteNoise(node);
    else if(name==OtherChemical::GetTypeName())   return new OtherChemical(node);
    else if(name==Parameter::GetTypeName())       return new Parameter(node);
    else if(name==LinearGradient::GetTypeName())  return new LinearGradient(node);
    else if(name==RadialGradient::GetTypeName())  return new RadialGradient(node);
    else                                          return NULL;
}

/* static */ BaseShape* BaseShape::New(vtkXMLDataElement* node)
{
    string name(node->GetName());
    if(name==Everywhere::GetTypeName())        return new Everywhere(node);
    else if(name==Rectangle::GetTypeName())    return new Rectangle(node);
    else if(name==Circle::GetTypeName())       return new Circle(node);
    else if(name==Pixel::GetTypeName())        return new Pixel(node);
    else                                       return NULL;
}
