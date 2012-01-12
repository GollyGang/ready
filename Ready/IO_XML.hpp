// local
class BaseRD;

// VTK:
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLDataElement.h>
#include <vtkObjectFactory.h>
#include <vtkXMLDataElement.h>

class RD_XMLWriter : public vtkXMLImageDataWriter
{
    public:

        vtkTypeMacro(RD_XMLWriter, vtkXMLImageDataWriter);
        static RD_XMLWriter* New();

        void SetSystem(BaseRD* rd_system) { this->system = rd_system; }

    protected:  

        RD_XMLWriter() : system(NULL) {} 

        static void WriteRDSystemXML(BaseRD* system,ostream& os,vtkIndent indent);

        int WritePrimaryElement(ostream& os,vtkIndent indent)
        {
            WriteRDSystemXML(this->system,os,indent);
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

        void SetFromXML(BaseRD* rd_system);

    protected:  

        RD_XMLReader() {} 

};

