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

// a 1D or 2D or 3D integer point
class PointND
{
    public:
    
        PointND() {}
        PointND(int xval) : x(xval),y(0),z(0) {}
        PointND(int xval,int yval) : x(xval),y(yval),z(0) {}
        PointND(int xval,int yval,int zval) : x(xval),y(yval),z(zval) {}
        
        bool InRect(const PointND& corner1,const PointND& corner2) const 
        { 
            return x>=corner1.x && y>=corner1.y && z>=corner1.z &&
                   x<=corner2.x && y<=corner2.y && z<=corner2.z; 
        }

        vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

    public:
    
        int x,y,z;
};

// an overlay is a shape to be drawn on top of an image (think: stacked transparencies)
class BaseOverlay
{
    public: // type definitions
    
        enum TPasteMode { Overwrite, Add };
        enum TFillMode { Constant, WhiteNoise };
        
    public:
    
        BaseOverlay() {}
        BaseOverlay(float val1,TPasteMode pm)
            : fill_mode(Constant),value1(val1),paste_mode(pm) {}
        BaseOverlay(float val1,float val2,TPasteMode pm)
            : fill_mode(WhiteNoise),value1(val1),value2(val2),paste_mode(pm) {}
            
        BaseOverlay(const BaseOverlay& a) 
            : value1(a.value1),value2(a.value2),paste_mode(a.paste_mode),fill_mode(a.fill_mode) {}
        BaseOverlay& operator=(const BaseOverlay& a) { 
            this->value1 = a.value1; this->value2 = a.value2; this->paste_mode = a.paste_mode; this->fill_mode = a.fill_mode; return *this; 
        }
        
        virtual void Apply(int iChemical,const PointND& at,float& value) const =0;

        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const =0;

    protected:
    
        float value1,value2;
        TPasteMode paste_mode;
        TFillMode fill_mode;
};
 
// single-channel axis-aligned-rectangle overlay with a fixed location
class RectangleOverlay : public BaseOverlay
{
        
    public:
    
        RectangleOverlay(vtkXMLDataElement* node);
        RectangleOverlay(PointND corner_1,PointND corner_2,int iChem,float val1,TPasteMode pm)
            : BaseOverlay(val1,pm),iChemical(iChem),corner1(corner_1),corner2(corner_2) {}
        RectangleOverlay(PointND corner_1,PointND corner_2,int iChem,float val1,float val2,TPasteMode pm)
            : BaseOverlay(val1,val2,pm),iChemical(iChem),corner1(corner_1),corner2(corner_2) {}
            
        RectangleOverlay(const RectangleOverlay& a) 
            : BaseOverlay(a),iChemical(a.iChemical),corner1(a.corner1),corner2(a.corner2) {}
        RectangleOverlay& operator=(const RectangleOverlay& a) { 
            this->iChemical = a.iChemical; this->corner1 = a.corner1; this->corner2 = a.corner2; return *this; 
        }

        virtual void Apply(int iChemical,const PointND& at,float& value) const;
        
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        static const char* GetXMLName() { return "RectangleOverlay"; }
        
    protected:
    
        int iChemical;
        PointND corner1,corner2;
};
