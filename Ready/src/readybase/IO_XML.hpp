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
class Properties;

// VTK:
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>

class RD_XMLWriter : public vtkXMLImageDataWriter
{
    public:

        vtkTypeMacro(RD_XMLWriter, vtkXMLImageDataWriter);
        static RD_XMLWriter* New();

        void SetSystem(ImageRD* rd_system);
        void SetRenderSettings(Properties* settings) { this->render_settings = settings; }

    protected:  

        RD_XMLWriter() : system(NULL) {} 

        static vtkSmartPointer<vtkXMLDataElement> BuildRDSystemXML(ImageRD* system);

        virtual int WritePrimaryElement(ostream& os,vtkIndent indent);

    protected:

        ImageRD *system;
        Properties* render_settings;
};

class RD_XMLReader : public vtkXMLImageDataReader
{
    public:

        vtkTypeMacro(RD_XMLReader, vtkXMLImageDataReader);
        static RD_XMLReader* New();

        std::string GetType();
        std::string GetName();
        vtkXMLDataElement* GetRDElement();
        bool ShouldGenerateInitialPatternWhenLoading();

    protected:  

        RD_XMLReader() {} 

};
