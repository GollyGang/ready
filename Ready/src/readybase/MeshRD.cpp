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
#include "MeshRD.hpp"
#include "IO_XML.hpp"
#include "utils.hpp"
#include "Properties.hpp"

// VTK:
#include <vtkPolyData.h>
#include <vtkXMLDataElement.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkMath.h>
#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>
#include <vtkCubeSource.h>
#include <vtkExtractEdges.h>
#include <vtkProperty.h>
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include <vtkMinimalStandardRandomSequence.h>

// STL:
#include <stdexcept>
using namespace std;

// ---------------------------------------------------------------------

MeshRD::MeshRD()
{
    this->starting_pattern = vtkPolyData::New();
    this->mesh = vtkPolyData::New();
}

// ---------------------------------------------------------------------

MeshRD::~MeshRD()
{
    this->mesh->Delete();
    this->starting_pattern->Delete();
}

// ---------------------------------------------------------------------

void MeshRD::Update(int n_steps)
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::SetNumberOfChemicals(int n)
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::SaveFile(const char* filename,const Properties& render_settings) const
{
    vtkSmartPointer<RD_XMLPolyDataWriter> iw = vtkSmartPointer<RD_XMLPolyDataWriter>::New();
    iw->SetSystem(this);
    iw->SetRenderSettings(&render_settings);
    iw->SetFileName(filename);
    iw->SetInput(this->mesh);
    iw->Write();
}

// ---------------------------------------------------------------------

void MeshRD::GenerateInitialPattern()
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::BlankImage()
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::CopyFromMesh(vtkPolyData* pd)
{
    this->mesh->DeepCopy(pd);
}

// ---------------------------------------------------------------------

void MeshRD::InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float r,g,b,low_hue,low_sat,low_val,high_hue,high_sat,high_val;
    render_settings.GetProperty("color_low").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&low_hue,&low_sat,&low_val);
    render_settings.GetProperty("color_high").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&high_hue,&high_sat,&high_val);
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();

    int iFirstChem=0,iLastChem=this->GetNumberOfChemicals();
    if(!show_multiple_chemicals) { iFirstChem = iActiveChemical; iLastChem = iFirstChem+1; }
    
    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetRampToLinear();
    lut->SetScaleToLinear();
    lut->SetTableRange(low,high);
    lut->SetSaturationRange(low_sat,high_sat);
    lut->SetHueRange(low_hue,high_hue);
    lut->SetValueRange(low_val,high_val);
    lut->Build();

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInput(this->mesh);
    mapper->SetScalarModeToUseCellData();
    mapper->SetLookupTable(lut);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    pRenderer->AddActor(actor);

    // also add a scalar bar to show how the colors correspond to values
    vtkSmartPointer<vtkScalarBarActor> scalar_bar = vtkSmartPointer<vtkScalarBarActor>::New();
    scalar_bar->SetLookupTable(lut);
    pRenderer->AddActor2D(scalar_bar);

    // add the bounding box
    {
        vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
        box->SetBounds(this->mesh->GetBounds());

        vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
        edges->SetInputConnection(box->GetOutputPort());

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(edges->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0,0,0);  
        actor->GetProperty()->SetAmbient(1);

        pRenderer->AddActor(actor);
    }
}

// ---------------------------------------------------------------------

void MeshRD::SaveStartingPattern()
{
    this->starting_pattern->DeepCopy(this->mesh);
}

// ---------------------------------------------------------------------

void MeshRD::RestoreStartingPattern()
{
    this->mesh->DeepCopy(this->starting_pattern);
}

// ---------------------------------------------------------------------

float MeshRD::SampleAt(int x,int y,int z,int ic)
{
    return 0.0f;

    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::InternalUpdate(int n_steps)
{
    // TODO
}

// ---------------------------------------------------------------------
void MeshRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    AbstractRD::InitializeFromXML(rd,warn_to_update);

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // formula:
    vtkSmartPointer<vtkXMLDataElement> xml_formula = rule->FindNestedElementWithName("formula");
    if(!xml_formula) throw runtime_error("formula node not found in file");

    // number_of_chemicals:
    read_required_attribute(xml_formula,"number_of_chemicals",this->n_chemicals);

    string formula = trim_multiline_string(xml_formula->GetCharacterData());
    this->TestFormula(formula); // will throw on error
    this->SetFormula(formula); // (won't throw yet)
}

// ---------------------------------------------------------------------

vtkSmartPointer<vtkXMLDataElement> MeshRD::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = AbstractRD::GetAsXML();

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found");

    // formula
    vtkSmartPointer<vtkXMLDataElement> formula = vtkSmartPointer<vtkXMLDataElement>::New();
    formula->SetName("formula");
    formula->SetIntAttribute("number_of_chemicals",this->GetNumberOfChemicals());
    formula->SetCharacterData(this->GetFormula().c_str(),(int)this->GetFormula().length());
    rule->AddNestedElement(formula);

    return rd;
}

// ---------------------------------------------------------------------
