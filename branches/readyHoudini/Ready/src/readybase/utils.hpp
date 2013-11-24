/*  Copyright 2011, 2012, 2013 The Ready Bunch

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

#ifndef __UTILS__
#define __UTILS__

// STL:
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>

// VTK:
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>

double get_time_in_seconds();

float frand(float lower,float upper);

double hypot2(double x,double y);

double hypot3(double x,double y,double z);

// http://www.doc.ic.ac.uk/~akf/handel-c/cgi-bin/forum.cgi?msg=551 
#define STRING_FROM_LITERAL(a) #a
#define STR(a) STRING_FROM_LITERAL(a)

template <typename T> std::string to_string(const T& t) 
{ 
    std::ostringstream oss; 
    oss << t; 
    return oss.str(); 
} 

template <typename T> bool from_string(const std::string& s,T& val) 
{ 
    std::istringstream iss(s); 
    iss >> val; 
    return !iss.fail(); 
} 
// we provide a specialized version of from_string for string types, to avoid the problem of multiple space-separated words
template <> bool from_string<std::string> (const std::string& s,std::string& val);

float* vtk_at(float* origin,int x,int y,int z,int X,int Y);

template <typename T> 
void read_required_attribute(vtkXMLDataElement* e,const std::string& name,T& val) 
{ 
    const char *str = e->GetAttribute(name.c_str());
    if(!str || !from_string(str,val))
        throw std::runtime_error(to_string(e->GetName())+" : failed to read required attribute: "+name);
} 

std::string GetChemicalName(int i); ///< a, b, c, ... z, aa, ab, ...
int IndexFromChemicalName(const std::string& s);

/// Read a multiline string, outputting whitespace-trimmed lines.
std::string trim_multiline_string(const char* s);

/// Abstract interface for XML objects.
/** Allows objects to be saved/loaded to/from an XML representation. */
class XML_Object
{
    public:

        XML_Object(const vtkXMLDataElement* node) {}
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const =0;
};

/// Interpolate between two RGB colors, first converting to HSV and converting the result back to RGB
void InterpolateInHSV(const float r1,const float g1,const float b1,const float r2,const float g2,const float b2,const float u,float& r,float& g,float& b);

#endif
