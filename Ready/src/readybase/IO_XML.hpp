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

// local
class ImageRD;
class MeshRD;
class Properties;

// VTK:
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>

// -------------------------------------------------------------------

class RD_XMLImageWriter : public vtkXMLImageDataWriter
{
    public:

        vtkTypeMacro(RD_XMLImageWriter, vtkXMLImageDataWriter);
        static RD_XMLImageWriter* New();

        void SetSystem(const ImageRD* rd_system);
        void SetRenderSettings(const Properties* settings) { this->render_settings = settings; }

    protected:  

        RD_XMLImageWriter() : system(NULL) {} 

        static vtkSmartPointer<vtkXMLDataElement> BuildRDSystemXML(ImageRD* system);

        virtual int WritePrimaryElement(ostream& os,vtkIndent indent);

    protected:

        const ImageRD* system;
        const Properties* render_settings;
};

// -------------------------------------------------------------------

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

class RD_XMLPolyDataWriter : public vtkXMLPolyDataWriter
{
    public:

        vtkTypeMacro(RD_XMLPolyDataWriter, vtkXMLPolyDataWriter);
        static RD_XMLPolyDataWriter* New();

        void SetSystem(const MeshRD* rd_system);
        void SetRenderSettings(const Properties* settings) { this->render_settings = settings; }

    protected:  

        RD_XMLPolyDataWriter() : system(NULL) {} 

        static vtkSmartPointer<vtkXMLDataElement> BuildRDSystemXML(MeshRD* system);

        virtual int WritePrimaryElement(ostream& os,vtkIndent indent);

    protected:

        const MeshRD* system;
        const Properties* render_settings;
};

// -------------------------------------------------------------------

class RD_XMLPolyDataReader : public vtkXMLPolyDataReader
{
    public:

        vtkTypeMacro(RD_XMLPolyDataReader, vtkXMLPolyDataReader);
        static RD_XMLPolyDataReader* New();

        std::string GetType();
        std::string GetName();
        vtkXMLDataElement* GetRDElement();
        bool ShouldGenerateInitialPatternWhenLoading();

    protected:  

        RD_XMLPolyDataReader() {} 

};

// -------------------------------------------------------------------
