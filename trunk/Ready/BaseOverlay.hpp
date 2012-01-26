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

// a 1D or 2D or 3D float point
class PointND
{
    public:
    
        PointND() {}
        PointND(float x,float y,float z) :x(x),y(y),z(z) {}
        PointND(vtkXMLDataElement *node);
        
        bool InRect(const PointND& corner1,const PointND& corner2) const 
        { 
            return x>=corner1.x && y>=corner1.y && z>=corner1.z &&
                   x<=corner2.x && y<=corner2.y && z<=corner2.z; 
        }

        vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

    protected:
    
        float x,y,z;
};

// an overlay is a shape to be drawn on top of an image (think: stacked transparencies)
class BaseOverlay
{
    public: // type definitions
    
        enum TPasteMode { Overwrite, Add };
        enum TFillMode { Constant, WhiteNoise };
        
    public:
    
        // apply the overlay at the specified pixel for the specified chemical
        virtual void Apply(int iChemical,const PointND& at,float& value) const =0;

        // for saving to file, get the overlay as an XML element
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

    protected:

        BaseOverlay(vtkXMLDataElement* node);
        BaseOverlay(float val1,TPasteMode pm)
            : fill_mode(Constant),value1(val1),paste_mode(pm) {}
        BaseOverlay(float val1,float val2,TPasteMode pm)
            : fill_mode(WhiteNoise),value1(val1),value2(val2),paste_mode(pm) {}
            
    protected:
    
        float value1,value2;
        TPasteMode paste_mode;
        TFillMode fill_mode;
};
 
// single-channel axis-aligned-rectangle overlay with a fixed location
// (spatial coordinates are [0,1] i.e. relative to the image size)
class RectangleOverlay : public BaseOverlay
{
    public:
    
        // can initialise from XML, for loading from file
        RectangleOverlay(vtkXMLDataElement* node);
        // can initialise manually
        RectangleOverlay(PointND corner_1,PointND corner_2,int iChem,float val1,TPasteMode pm)
            : BaseOverlay(val1,pm),iChemical(iChem),corner1(corner_1),corner2(corner_2) {}
        RectangleOverlay(PointND corner_1,PointND corner_2,int iChem,float val1,float val2,TPasteMode pm)
            : BaseOverlay(val1,val2,pm),iChemical(iChem),corner1(corner_1),corner2(corner_2) {}
            
        virtual void Apply(int iChemical,const PointND& at,float& value) const;
        
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        static const char* GetXMLName() { return "RectangleOverlay"; }
        
    protected:
    
        int iChemical;
        PointND corner1,corner2;
};
