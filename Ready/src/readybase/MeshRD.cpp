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
#include "overlays.hpp"

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
#include <vtkIdList.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkTetra.h>
#include <vtkCellArray.h>

// STL:
#include <stdexcept>
using namespace std;

// ---------------------------------------------------------------------

MeshRD::MeshRD()
{
    this->starting_pattern = vtkUnstructuredGrid::New();
    this->mesh = vtkUnstructuredGrid::New();
    this->buffer = vtkUnstructuredGrid::New();
}

// ---------------------------------------------------------------------

MeshRD::~MeshRD()
{
    this->mesh->Delete();
    this->buffer->Delete();
    this->starting_pattern->Delete();
}

// ---------------------------------------------------------------------

void MeshRD::Update(int n_steps)
{
    this->InternalUpdate(n_steps);

    this->timesteps_taken += n_steps;

    this->mesh->Modified();
}

// ---------------------------------------------------------------------

void MeshRD::SetNumberOfChemicals(int n)
{
    // TODO
}

// ---------------------------------------------------------------------

void MeshRD::SaveFile(const char* filename,const Properties& render_settings) const
{
    vtkSmartPointer<RD_XMLUnstructuredGridWriter> iw = vtkSmartPointer<RD_XMLUnstructuredGridWriter>::New();
    iw->SetSystem(this);
    iw->SetRenderSettings(&render_settings);
    iw->SetFileName(filename);
    iw->SetInput(this->mesh);
    iw->Write();
}

// ---------------------------------------------------------------------

void MeshRD::GenerateInitialPattern()
{
    this->BlankImage();

    vtkIdType npts,*pts;
    float x,y,z;
    vtkFloatingPointType *p;
    vtkFloatArray *scalars = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetScalars() );
    vtkFloatingPointType *bounds = this->mesh->GetBounds();
    for(vtkIdType iCell=0;iCell<this->mesh->GetNumberOfCells();iCell++)
    {
        this->mesh->GetCellPoints(iCell,npts,pts);
        // get a point at the centre of the cell (need a location to sample the overlays)
        x=y=z=0.0f;
        for(vtkIdType iPt=0;iPt<npts;iPt++)
        {
            p = this->mesh->GetPoint(pts[iPt]);
            x+=p[0]-bounds[0]; y+=p[1]-bounds[2]; z+=p[2]-bounds[4];
        }
        x/=npts; y/=npts; z/=npts;
        for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
        {
            Overlay* overlay = this->initial_pattern_generator[iOverlay];

            int iC = overlay->GetTargetChemical();
            if(iC<0 || iC>=this->GetNumberOfChemicals())
                throw runtime_error("Overlay: chemical out of range: "+GetChemicalName(iC));

            float val = scalars->GetTuple(iCell)[iC];
            vector<float> vals(this->GetNumberOfChemicals());
            for(int i=0;i<this->GetNumberOfChemicals();i++)
                vals[i] = scalars->GetTuple(iCell)[i];
            scalars->SetComponent(iCell,iC,overlay->Apply(vals,this,x,y,z));
        }
    }
    this->mesh->Modified();
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

void MeshRD::BlankImage()
{
    vtkFloatArray *scalars = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetScalars() );
    for(vtkIdType iCell=0;iCell<this->mesh->GetNumberOfCells();iCell++)
        scalars->FillComponent(iCell,0.0);
    this->mesh->Modified();
}

// ---------------------------------------------------------------------

float MeshRD::GetX() const
{
    return this->mesh->GetBounds()[1]-this->mesh->GetBounds()[0];
}

// ---------------------------------------------------------------------

float MeshRD::GetY() const
{
    return this->mesh->GetBounds()[3]-this->mesh->GetBounds()[2];
}

// ---------------------------------------------------------------------

float MeshRD::GetZ() const
{
    return this->mesh->GetBounds()[5]-this->mesh->GetBounds()[4];
}

// ---------------------------------------------------------------------

// DEBUG
#include <vtkPlatonicSolidSource.h>
#include <vtkButterflySubdivisionFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkMinimalStandardRandomSequence.h>

void MeshRD::CopyFromMesh(vtkUnstructuredGrid* mesh2)
{
    this->mesh->DeepCopy(mesh2);
    this->buffer->DeepCopy(this->mesh);

    // DEBUG:
    if(0)
    {
        // make a geodesic sphere by subdividing an icosahedron
        vtkSmartPointer<vtkPlatonicSolidSource> icosahedron = vtkSmartPointer<vtkPlatonicSolidSource>::New();
        icosahedron->SetSolidTypeToIcosahedron();
        vtkSmartPointer<vtkButterflySubdivisionFilter> butterfly = vtkSmartPointer<vtkButterflySubdivisionFilter>::New();
        butterfly->SetInputConnection(icosahedron->GetOutputPort());
        butterfly->SetNumberOfSubdivisions(3);
        butterfly->Update();
        this->mesh->SetPoints(butterfly->GetOutput()->GetPoints());
        this->mesh->SetCells(VTK_POLYGON,butterfly->GetOutput()->GetPolys());

        // push the vertices out into the shape of a sphere
        vtkFloatingPointType p[3];
        for(int i=0;i<this->mesh->GetNumberOfPoints();i++)
        {
            this->mesh->GetPoint(i,p);
            vtkMath::Normalize(p);
            this->mesh->GetPoints()->SetPoint(i,p);
        }
        
        // assign some cell data
        vtkFloatArray *scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(this->mesh->GetNumberOfCells());
        this->mesh->GetCellData()->SetScalars(scalars);
        vtkSmartPointer<vtkMinimalStandardRandomSequence> random = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
        for(int i=0;i<this->mesh->GetNumberOfCells();i++)
        {
            random->Next();
            scalars->SetValue(i,random->GetValue()*0.1f);
            if(i==0)
                scalars->SetValue(i,1.0f);
        }

        this->buffer->DeepCopy(this->mesh);
    }

    this->ComputeCellNeighbors();
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

    // add the mesh actor
    {
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInput(this->mesh);
        mapper->SetScalarModeToUseCellData();
        mapper->SetLookupTable(lut);

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);

        pRenderer->AddActor(actor);
    }

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

void MeshRD::InternalUpdate(int n_steps)
{
    vtkFloatArray *source = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetScalars() );
    vtkFloatArray *target = vtkFloatArray::SafeDownCast( this->buffer->GetCellData()->GetScalars() );
    for(int iStep=0;iStep<n_steps;iStep++)
    {
        for(vtkIdType iCell=0;iCell<(int)this->cell_neighbors.size();iCell++)
        {
            // for now, a hard-coded heat equation
            float val = source->GetValue(iCell);
            for(vtkIdType iNeighbor=0;iNeighbor<(int)this->cell_neighbors[iCell].size();iNeighbor++)
                val += source->GetValue(this->cell_neighbors[iCell][iNeighbor]);
            val /= this->cell_neighbors[iCell].size() + 1;
            target->SetValue(iCell,val);
        }
        std::swap(source,target);
    }
    if(n_steps%2)
        this->mesh->DeepCopy(this->buffer);
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

void MeshRD::ComputeCellNeighbors()
{
    this->cell_neighbors.clear();
    vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkIdList> edgeIds = vtkSmartPointer<vtkIdList>::New();
    for(vtkIdType iCell=0;iCell<this->mesh->GetNumberOfCells();iCell++)
    {
        vector<vtkIdType> neighbors;
        this->mesh->GetCellPoints(iCell,ptIds);
        vtkIdType npts = ptIds->GetNumberOfIds();
        switch(this->mesh->GetCellType(iCell))
        {
            case VTK_POLYGON: // 2D face, neighbors share an edge
                {
                    for(vtkIdType iPt=0;iPt<npts;iPt++)
                    {
                        edgeIds->SetNumberOfIds(2);
                        edgeIds->SetId(0,ptIds->GetId(iPt));
                        edgeIds->SetId(1,ptIds->GetId((iPt+1)%npts));
                        this->mesh->GetCellNeighbors(iCell,edgeIds,cellIds);
                        int nNeighbors = cellIds->GetNumberOfIds();
                        for(vtkIdType iNeighbor=0;iNeighbor<nNeighbors;iNeighbor++)
                        {
                            vtkIdType id = cellIds->GetId(iNeighbor);
                            neighbors.push_back(id);
                        }
                    }
                    this->cell_neighbors.push_back(neighbors);
                }
                break;
            // TODO: allow for 1D (VTK_LINE) and 3D (VTK_HEXAHEDRON?) cells
            default:
                throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported cell type");
        }
    }
    // N.B. we don't intend to support cells of mixed dimensionality in one mesh
}

// ---------------------------------------------------------------------

int MeshRD::GetNumberOfCells() const
{
	return this->mesh->GetNumberOfCells();
}

// ---------------------------------------------------------------------
