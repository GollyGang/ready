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
class BaseRD;

// VTK:
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLDataElement.h>
#include <vtkObjectFactory.h>
#include <vtkXMLDataElement.h>
#include <vtkSmartPointer.h>

class RD_XMLWriter : public vtkXMLImageDataWriter
{
    public:

        vtkTypeMacro(RD_XMLWriter, vtkXMLImageDataWriter);
        static RD_XMLWriter* New();

        void SetSystem(BaseRD* rd_system);

    protected:  

        RD_XMLWriter() : system(NULL) {} 

        static vtkSmartPointer<vtkXMLDataElement> BuildRDSystemXML(BaseRD* system);

        virtual int WritePrimaryElement(ostream& os,vtkIndent indent)
        {
            BuildRDSystemXML(this->system)->PrintXML(os,indent);
            return vtkXMLImageDataWriter::WritePrimaryElement(os,indent);
        }

    protected:

        BaseRD *system;
};

class RD_XMLReader : public vtkXMLImageDataReader
{
    public:

        vtkTypeMacro(RD_XMLReader, vtkXMLImageDataReader);
        static RD_XMLReader* New();

        std::string GetType();
        std::string GetName();
        void SetSystemFromXML(BaseRD* rd_system);

    protected:  

        RD_XMLReader() {} 

};

