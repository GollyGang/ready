/*  Copyright 2011-2020 The Ready Bunch

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
#include <algorithm>
using namespace std;

// ------------------------------------------------------------------------------------------------

Overlay::Overlay(vtkXMLDataElement* node) : XML_Object(node)
{
    string s;
    read_required_attribute(node,"chemical",s);
    this->iTargetChemical = IndexFromChemicalName(s);
    const int n_nested = node->GetNumberOfNestedElements();
    if(n_nested<3)
        throw runtime_error("overlay : expected at least 3 nested elements (operation, fill, shape)");
    for(int i_nested=0;i_nested<n_nested;i_nested++)
    {
        vtkXMLDataElement *subnode = node->GetNestedElement(i_nested);
        // is this an operation element?
        unique_ptr<BaseOperation> pOp = BaseOperation::New(subnode);
        if(pOp) {
            this->op = move(pOp); // TODO: check if already supplied
            continue; // (save time parsing as other types)
        }
        // is this a fill element?
        unique_ptr<BaseFill> pFill = BaseFill::New(subnode);
        if(pFill) {
            this->fill = move(pFill); // TODO: check if already supplied
            continue;
        }
        // must be a shape element?
        unique_ptr<BaseShape> pShape = BaseShape::New(subnode);
        if(pShape)
            this->shapes.push_back(move(pShape));
        else throw runtime_error(string("Unknown overlay element: ")+subnode->GetName());
    }
    if(!this->op) throw runtime_error("overlay: missing operation element");
    if(!this->fill) throw runtime_error("overlay: missing fill element");
    if(this->shapes.empty()) throw runtime_error("overlay: missing shape element");
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

double Overlay::Apply(const vector<double>& vals, const AbstractRD& system, float x, float y, float z) const
{
    // copy the values into a scratchpad to allow the overlays to affect each other
    // e.g. one might write a constant value, the next double it
    vector<double> vals_scratchpad(vals);

    double& val = vals_scratchpad[this->iTargetChemical];
    for(int iShape=0;iShape<(int)this->shapes.size();iShape++)
    {
        if( this->shapes[iShape]->IsInside( x, y, z, system.GetX(), system.GetY(), system.GetZ(), system.GetArenaDimensionality() ) )
        {
            this->op->Apply( val, this->fill->GetValue(system, vals_scratchpad, x, y, z) );
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

        virtual void Apply(double& target,double value) const { target += value; }
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

        virtual void Apply(double& target,double value) const { target -= value; }
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

        virtual void Apply(double& target,double value) const { target = value; }
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

        virtual void Apply(double& target,double value) const { target *= value; }
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

        virtual void Apply(double& target,double value) const { target /= value; }
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

        virtual double GetValue(const AbstractRD& system, const vector<double>& vals, float x, float y, float z) const
        {
            return this->value;
        }

    protected:

        double value;
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

        virtual double GetValue(const AbstractRD& system,const vector<double>& vals, float x, float y, float z) const
        {
            if(this->iOtherChemical < 0 || this->iOtherChemical >= (int)vals.size())
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

        virtual double GetValue(const AbstractRD& system, const vector<double>& vals, float x, float y, float z) const
        {
            return system.GetParameterValueByName(this->parameter_name.c_str());
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

        virtual double GetValue(const AbstractRD& system, const vector<double>& vals, float x, float y, float z) const
        {
            return frand(this->low,this->high);
        }

    protected:

        double low,high;
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

        virtual double GetValue(const AbstractRD& system, const vector<double>& vals, float x, float y, float z) const
        {
            double rel_x = x/system.GetX();
            double rel_y = y/system.GetY();
            double rel_z = z/system.GetZ();
            // project this point onto the linear gradient axis
            double blen = hypot3(this->p2->x-this->p1->x,this->p2->y-this->p1->y,this->p2->z-this->p1->z);
            double bx = (this->p2->x-this->p1->x) / blen;
            double by = (this->p2->y-this->p1->y) / blen;
            double bz = (this->p2->z-this->p1->z) / blen;
            double dp = (rel_x-this->p1->x) * bx + (rel_y-this->p1->y) * by + (rel_z-this->p1->z) * bz; // dp = a.norm(b)
            double u = dp / blen; // [0,1]
            return this->val1 + (this->val2-this->val1) * u;
        }

    protected:

        double val1,val2;
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

        virtual double GetValue(const AbstractRD& system, const vector<double>& vals, float x, float y, float z) const
        {
            // convert p1 and p2 to absolute coordinates
            double rp1x = p1->x * system.GetX();
            double rp1y = p1->y * system.GetY();
            double rp1z = p1->z * system.GetZ();
            double rp2x = p2->x * system.GetX();
            double rp2y = p2->y * system.GetY();
            double rp2z = p2->z * system.GetZ();
            return val1 + (val2-val1) * hypot3(x-rp1x,y-rp1y,z-rp1z) / hypot3(rp2x-rp1x,rp2y-rp1y,rp2z-rp1z);
        }

    protected:

        double val1,val2;
        Point3D *p1,*p2;
};

class Gaussian : public BaseFill
{
    public:

        Gaussian(vtkXMLDataElement* node) : BaseFill(node)
        {
            read_required_attribute(node,"height",this->height);
            read_required_attribute(node,"sigma",this->sigma);
            if(node->GetNumberOfNestedElements()!=1)
                throw runtime_error("gasussian: expected one nested element (point3D)");
            this->center = new Point3D(node->GetNestedElement(0));
        }
        virtual ~Gaussian() { delete this->center; }

        static const char* GetTypeName() { return "gaussian"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Gaussian::GetTypeName());
            xml->SetFloatAttribute("height",this->height);
            xml->SetFloatAttribute("sigma",this->sigma);
            xml->AddNestedElement(this->center->GetAsXML());
            return xml;
        }

        virtual double GetValue(const AbstractRD& system, const vector<double>& vals, float x, float y, float z) const
        {
            // convert center to absolute coordinates
            double ax = center->x * system.GetX();
            double ay = center->y * system.GetY();
            double az = center->z * system.GetZ();
            double asigma = this->sigma * max(system.GetX(),max(system.GetY(),system.GetZ())); // (proportional to the largest dimension)
            double dist = hypot3(ax-x,ay-y,az-z);
            return this->height * exp( -dist*dist/(2.0f*asigma*asigma) );
        }

    protected:

        double height,sigma;
        Point3D *center;
};

class Sine : public BaseFill
{
    public:

        Sine(vtkXMLDataElement* node) : BaseFill(node)
        {
            read_required_attribute(node,"phase",this->phase);
            read_required_attribute(node,"amplitude",this->amplitude);
            if(node->GetNumberOfNestedElements()!=2)
                throw runtime_error("sine: expected two nested elements (point3D,point3D)");
            this->p1 = new Point3D(node->GetNestedElement(0));
            this->p2 = new Point3D(node->GetNestedElement(1));
        }
        virtual ~Sine() { delete this->p1; delete this->p2; }

        static const char* GetTypeName() { return "sine"; }

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const
        {
            vtkSmartPointer<vtkXMLDataElement> xml = vtkSmartPointer<vtkXMLDataElement>::New();
            xml->SetName(Sine::GetTypeName());
            xml->SetFloatAttribute("phase",this->phase);
            xml->SetFloatAttribute("amplitude",this->amplitude);
            xml->AddNestedElement(this->p1->GetAsXML());
            xml->AddNestedElement(this->p2->GetAsXML());
            return xml;
        }

        virtual double GetValue(const AbstractRD& system, const vector<double>& vals, float x, float y, float z) const
        {
            double rel_x = x/system.GetX();
            double rel_y = y/system.GetY();
            double rel_z = z/system.GetZ();
            // project this point onto the axis
            double blen = hypot3(this->p2->x-this->p1->x,this->p2->y-this->p1->y,this->p2->z-this->p1->z);
            double bx = (this->p2->x-this->p1->x) / blen;
            double by = (this->p2->y-this->p1->y) / blen;
            double bz = (this->p2->z-this->p1->z) / blen;
            double dp = (rel_x-this->p1->x) * bx + (rel_y-this->p1->y) * by + (rel_z-this->p1->z) * bz; // dp = a.norm(b)
            double u = dp / blen; // [0,1]
            return this->amplitude * sin( u * 2.0 * vtkMath::Pi() - this->phase );
        }

    protected:

        double phase,amplitude;
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
            double rel_x = x/X;
            double rel_y = y/Y;
            double rel_z = z/Z;
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
            double cx = this->c->x * X;
            double cy = this->c->y * Y;
            double cz = this->c->z * Z;
            double abs_radius = this->radius * max(X,max(Y,Z)); // (radius is proportional to the largest dimension)
            switch(dimensionality)
            {
                default:
                case 1: return sqrt(pow(x-cx, 2)) < abs_radius;
                case 2: return sqrt(pow(x-cx, 2)+pow(y-cy, 2)) < abs_radius;
                case 3: return sqrt(pow(x-cx, 2)+pow(y-cy, 2)+pow(z-cz, 2)) < abs_radius;
            }
        }

    protected:

        Point3D *c;
        double radius;
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

/* static */ unique_ptr<BaseOperation> BaseOperation::New(vtkXMLDataElement *node)
{
    string name(node->GetName());
    if(name==Overwrite::GetTypeName())         return make_unique<Overwrite>(node);
    else if(name==Add::GetTypeName())          return make_unique<Add>(node);
    else if(name==Subtract::GetTypeName())     return make_unique<Subtract>(node);
    else if(name==Multiply::GetTypeName())     return make_unique<Multiply>(node);
    else if(name==Divide::GetTypeName())       return make_unique<Divide>(node);
    else                                       return unique_ptr<BaseOperation>();
}

/* static */ unique_ptr<BaseFill> BaseFill::New(vtkXMLDataElement* node)
{
    string name(node->GetName());
    if(name==Constant::GetTypeName())             return make_unique<Constant>(node);
    else if(name==WhiteNoise::GetTypeName())      return make_unique<WhiteNoise>(node);
    else if(name==OtherChemical::GetTypeName())   return make_unique<OtherChemical>(node);
    else if(name==Parameter::GetTypeName())       return make_unique<Parameter>(node);
    else if(name==LinearGradient::GetTypeName())  return make_unique<LinearGradient>(node);
    else if(name==RadialGradient::GetTypeName())  return make_unique<RadialGradient>(node);
    else if(name==Gaussian::GetTypeName())        return make_unique<Gaussian>(node);
    else if(name==Sine::GetTypeName())            return make_unique<Sine>(node);
    else                                          return unique_ptr<BaseFill>();
}

/* static */ unique_ptr<BaseShape> BaseShape::New(vtkXMLDataElement* node)
{
    string name(node->GetName());
    if(name==Everywhere::GetTypeName())        return make_unique<Everywhere>(node);
    else if(name==Rectangle::GetTypeName())    return make_unique<Rectangle>(node);
    else if(name==Circle::GetTypeName())       return make_unique<Circle>(node);
    else if(name==Pixel::GetTypeName())        return make_unique<Pixel>(node);
    else                                       return unique_ptr<BaseShape>();
}
