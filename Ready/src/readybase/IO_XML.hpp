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

// VTK:
#include <vtkXMLImageDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>

// -------------------------------------------------------------------

/// Reads *.vti files, our extended version of VTK's XML format for vtkImageData.
class RD_XMLImageReader : public vtkXMLImageDataReader
{
    public:

        vtkTypeMacro(RD_XMLImageReader, vtkXMLImageDataReader);
        static RD_XMLImageReader* New();

        std::string GetType();
        std::string GetName();
        vtkXMLDataElement* GetRDElement();
        bool ShouldGenerateInitialPatternWhenLoading();

    protected:  

        RD_XMLImageReader() {} 

};

// -------------------------------------------------------------------

/// Reads *.vtu files, our extended version of VTK's XML format for vtkUnstructuredGrid.
class RD_XMLUnstructuredGridReader : public vtkXMLUnstructuredGridReader
{
    public:

        vtkTypeMacro(RD_XMLUnstructuredGridReader, vtkXMLUnstructuredGridReader);
        static RD_XMLUnstructuredGridReader* New();

        std::string GetType();
        std::string GetName();
        vtkXMLDataElement* GetRDElement();
        bool ShouldGenerateInitialPatternWhenLoading();

    protected:  

        RD_XMLUnstructuredGridReader() {} 

};

// -------------------------------------------------------------------