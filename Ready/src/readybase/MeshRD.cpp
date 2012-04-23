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
#include <vtkGeometryFilter.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkObjectFactory.h>

// STL:
#include <stdexcept>
using namespace std;

// -------------------------------------------------------------------

class RD_XMLUnstructuredGridWriter : public vtkXMLUnstructuredGridWriter
{
    public:

        vtkTypeMacro(RD_XMLUnstructuredGridWriter, vtkXMLUnstructuredGridWriter);
        static RD_XMLUnstructuredGridWriter* New();

        void SetSystem(const MeshRD* rd_system);
        void SetRenderSettings(const Properties* settings) { this->render_settings = settings; }

    protected:  

        RD_XMLUnstructuredGridWriter() : system(NULL) {} 

        static vtkSmartPointer<vtkXMLDataElement> BuildRDSystemXML(MeshRD* system);

        virtual int WritePrimaryElement(ostream& os,vtkIndent indent);

    protected:

        const MeshRD* system;
        const Properties* render_settings;
};

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
    this->n_chemicals = 0;
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
    for(int iChem=0;iChem<this->n_chemicals;iChem++)
        this->mesh->GetCellData()->RemoveArray(GetChemicalName(iChem).c_str());
    this->n_chemicals = n;
    for(int iChem=0;iChem<this->n_chemicals;iChem++)
    {
        vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(this->mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        this->mesh->GetCellData()->AddArray(scalars);
    }

    this->buffer->DeepCopy(this->mesh);
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
    float cp[3];
    vtkFloatingPointType *bounds = this->mesh->GetBounds();
    for(vtkIdType iCell=0;iCell<this->mesh->GetNumberOfCells();iCell++)
    {
        this->mesh->GetCellPoints(iCell,npts,pts);
        // get a point at the centre of the cell (need a location to sample the overlays)
        cp[0]=cp[1]=cp[2]=0.0f;
        for(vtkIdType iPt=0;iPt<npts;iPt++)
            for(int xyz=0;xyz<3;xyz++)
                cp[xyz] += this->mesh->GetPoint(pts[iPt])[xyz]-bounds[xyz*2+0];
        for(int xyz=0;xyz<3;xyz++)
            cp[xyz] /= npts;
        for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
        {
            Overlay* overlay = this->initial_pattern_generator[iOverlay];

            int iC = overlay->GetTargetChemical();
            if(iC<0 || iC>=this->GetNumberOfChemicals())
                throw runtime_error("Overlay: chemical out of range: "+GetChemicalName(iC));

            float val;
            vector<float> vals(this->GetNumberOfChemicals());
            for(int i=0;i<this->GetNumberOfChemicals();i++)
            {
                vtkFloatArray *scalars = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(i).c_str()) );
                if(!scalars)
                    throw runtime_error("MeshRD::GenerateInitialPattern : failed to cast cell data to vtkFloatArray");
                vals[i] = scalars->GetValue(iCell);
                if(i==iC) val = vals[i];
            }
            vtkFloatArray *scalars = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(iC).c_str()) );
            if(!scalars)
                throw runtime_error("MeshRD::GenerateInitialPattern : failed to cast cell data to vtkFloatArray");
            scalars->SetValue(iCell,overlay->Apply(vals,this,cp[0],cp[1],cp[2]));
        }
    }
    this->mesh->Modified();
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

void MeshRD::BlankImage()
{
    for(int iChem=0;iChem<this->n_chemicals;iChem++)
    {
        vtkFloatArray *scalars = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(iChem).c_str()) );
        scalars->FillComponent(0,0.0);
    }
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

// DEBUG: some things used for making datasets, will remove before the next release
#include <vtkPlatonicSolidSource.h>
#include <vtkButterflySubdivisionFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPointSource.h>
#include <vtkDelaunay3D.h>

void MeshRD::CopyFromMesh(vtkUnstructuredGrid* mesh2)
{
    this->mesh->DeepCopy(mesh2);

    // DEBUG: make a tetrahedral mesh by delaunay tetrahedralization on a random point cloud
    if(0)
    {
        vtkSmartPointer<vtkPointSource> pts = vtkSmartPointer<vtkPointSource>::New();
        pts->SetNumberOfPoints(200);
        vtkSmartPointer<vtkDelaunay3D> del = vtkSmartPointer<vtkDelaunay3D>::New();
        del->SetInputConnection(pts->GetOutputPort());
        del->Update();
        this->mesh->DeepCopy(del->GetOutput());
    }

    // DEBUG: make a geodesic sphere by subdividing an icosahedron
    if(0)
    {
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
    }
     
    // DEBUG
    if(0)
    {
        // assign some cell data
        this->n_chemicals = 2;
        for(int iChem=0;iChem<this->n_chemicals;iChem++)
        {
            vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
            scalars->SetNumberOfComponents(1);
            scalars->SetNumberOfTuples(this->mesh->GetNumberOfCells());
            scalars->SetName(GetChemicalName(iChem).c_str());
            scalars->FillComponent(0,((iChem%2)?0.8:0.2));
            this->mesh->GetCellData()->AddArray(scalars);
        }
    }

    this->buffer->DeepCopy(this->mesh);

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
    string activeChemical = render_settings.GetProperty("active_chemical").GetChemical();
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();

    bool slice_3D = render_settings.GetProperty("slice_3D").GetBool();
    string slice_3D_axis = render_settings.GetProperty("slice_3D_axis").GetAxis();
    float slice_3D_position = render_settings.GetProperty("slice_3D_position").GetFloat();

    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetRampToLinear();
    lut->SetScaleToLinear();
    lut->SetTableRange(low,high);
    lut->SetSaturationRange(low_sat,high_sat);
    lut->SetHueRange(low_hue,high_hue);
    lut->SetValueRange(low_val,high_val);

    // add the mesh actor
    {
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        if(use_wireframe && !slice_3D) // full wireframe mode: all internal edges
        {
            // explicitly extract the edges - the default mapper only shows the outside surface
            vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
            edges->SetInput(this->mesh);
            mapper->SetInputConnection(edges->GetOutputPort());
        }
        else if(slice_3D) // partial wireframe mode: only external surface edges
        {
            vtkSmartPointer<vtkGeometryFilter> geom = vtkSmartPointer<vtkGeometryFilter>::New();
            geom->SetInput(this->mesh);
            vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
            edges->SetInputConnection(geom->GetOutputPort());
            mapper->SetInputConnection(edges->GetOutputPort());
        }
        else // non-wireframe mode: shows filled external surface
        {
            mapper->SetInput(this->mesh);
        }
        mapper->SetScalarModeToUseCellFieldData();
        mapper->SelectColorArray(activeChemical.c_str());
        mapper->SetLookupTable(lut);

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);

        pRenderer->AddActor(actor);
    }

    // add a slice
    if(slice_3D)
    {
        vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
        vtkFloatingPointType *bounds = this->mesh->GetBounds();
        plane->SetOrigin(slice_3D_position*(bounds[1]-bounds[0])+bounds[0],
                         slice_3D_position*(bounds[3]-bounds[2])+bounds[2],
                         slice_3D_position*(bounds[5]-bounds[4])+bounds[4]);
        if(slice_3D_axis=="x")
            plane->SetNormal(1,0,0);
        else if(slice_3D_axis=="y")
            plane->SetNormal(0,1,0);
        else
            plane->SetNormal(0,0,1);
        vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
        cutter->SetInput(this->mesh);
        cutter->SetCutFunction(plane);
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(cutter->GetOutputPort());
        mapper->SetScalarModeToUseCellFieldData();
        mapper->SelectColorArray(activeChemical.c_str());
        mapper->SetLookupTable(lut);
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->LightingOff();
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
    // for now, a hard-coded heat equation
    vtkFloatArray *source;
    vtkFloatArray *target;
    for(int iStep=0;iStep<n_steps;iStep++)
    {
        for(int iChem=0;iChem<this->n_chemicals;iChem++)
        {
            if(iStep%2)
            {
                source = vtkFloatArray::SafeDownCast( this->buffer->GetCellData()->GetArray(GetChemicalName(iChem).c_str()) );
                target = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(iChem).c_str()) );
            }
            else
            {
                source = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(iChem).c_str()) );
                target = vtkFloatArray::SafeDownCast( this->buffer->GetCellData()->GetArray(GetChemicalName(iChem).c_str()) );
            }
            for(vtkIdType iCell=0;iCell<(int)this->cell_neighbors.size();iCell++)
            {
                float val = source->GetValue(iCell);
                for(vtkIdType iNeighbor=0;iNeighbor<(int)this->cell_neighbors[iCell].size();iNeighbor++)
                    val += source->GetValue(this->cell_neighbors[iCell][iNeighbor]);
                val /= this->cell_neighbors[iCell].size() + 1;
                target->SetValue(iCell,val);
            }
        }
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
                        vtkSmartPointer<vtkIdList> edgeIds = vtkSmartPointer<vtkIdList>::New();
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
            case VTK_TETRA: // 3D cell, neighbors share a triangular face
                {
                    for(vtkIdType iPt=0;iPt<npts;iPt++)
                    {
                        vtkSmartPointer<vtkIdList> faceIds = vtkSmartPointer<vtkIdList>::New();
                        faceIds->SetNumberOfIds(3);
                        faceIds->SetId(0,ptIds->GetId(iPt));
                        faceIds->SetId(1,ptIds->GetId((iPt+1)%npts));
                        faceIds->SetId(2,ptIds->GetId((iPt+2)%npts));
                        this->mesh->GetCellNeighbors(iCell,faceIds,cellIds);
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
            // TODO: allow for 1D (VTK_LINE) and other kinds of 3D cells?
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

void MeshRD::GetAsMesh(vtkPolyData *out, const Properties &render_settings) const
{
    vtkSmartPointer<vtkGeometryFilter> geom = vtkSmartPointer<vtkGeometryFilter>::New();
    geom->SetInput(this->mesh);
    geom->Update();
    out->DeepCopy(geom->GetOutput());
    // 2D meshes will get returned unchanged, meshes with 3D cells will have their outer surface returned
}

// ---------------------------------------------------------------------

int MeshRD::GetArenaDimensionality() const
{
    vtkFloatingPointType epsilon = 1e-4;
    vtkFloatingPointType *bounds = this->mesh->GetBounds();
    int dimensionality = 0;
    for(int xyz=0;xyz<3;xyz++)
        if(bounds[xyz*2+1]-bounds[xyz*2+0] > epsilon)
            dimensionality++;
    return dimensionality;
    // TODO: rotate datasets on input such that if dimensionality=2 then all z=constant, and if dimensionality=1 then all y=constant and all z=constant
}

// ---------------------------------------------------------------------

vtkStandardNewMacro(RD_XMLUnstructuredGridWriter);

// ---------------------------------------------------------------------

void RD_XMLUnstructuredGridWriter::SetSystem(const MeshRD* rd_system) 
{ 
    this->system = rd_system; 
}

// ---------------------------------------------------------------------

int RD_XMLUnstructuredGridWriter::WritePrimaryElement(ostream& os,vtkIndent indent)
{
    vtkSmartPointer<vtkXMLDataElement> xml = this->system->GetAsXML();
    xml->AddNestedElement(this->render_settings->GetAsXML());
    xml->PrintXML(os,indent);
    return vtkXMLUnstructuredGridWriter::WritePrimaryElement(os,indent);
}

// ---------------------------------------------------------------------
