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
#include <vtkLinearSubdivisionFilter.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkPointSource.h>
#include <vtkDelaunay3D.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>
#include <vtkCleanPolyData.h>

// STL:
#include <stdexcept>
using namespace std;

// -------------------------------------------------------------------

/// Used to write vtkUnstructuredGrid to XML, with an added RD section containing rule information.
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
    this->cell_neighbor_indices = NULL;
    this->cell_neighbor_weights = NULL;
}

// ---------------------------------------------------------------------

MeshRD::~MeshRD()
{
    delete []this->cell_neighbor_indices;
    delete []this->cell_neighbor_weights;

    this->mesh->Delete();
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
                continue; // best for now to silently ignore this overlay, because the user has no way of editing the overlays (short of editing the file)
                //throw runtime_error("Overlay: chemical out of range: "+GetChemicalName(iC));

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

void MeshRD::CopyFromMesh(vtkUnstructuredGrid* mesh2)
{
    this->mesh->DeepCopy(mesh2);
    this->n_chemicals = this->mesh->GetCellData()->GetNumberOfArrays();

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
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();

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
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
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
            if(show_cell_edges)
            {
                actor->GetProperty()->EdgeVisibilityOn();
                actor->GetProperty()->SetEdgeColor(0,0,0);
            }
        }
        mapper->SetScalarModeToUseCellFieldData();
        mapper->SelectColorArray(activeChemical.c_str());
        mapper->SetLookupTable(lut);


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
    if(show_color_scale)
    {
        vtkSmartPointer<vtkScalarBarActor> scalar_bar = vtkSmartPointer<vtkScalarBarActor>::New();
        scalar_bar->SetLookupTable(lut);
        pRenderer->AddActor2D(scalar_bar);
    }

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
    this->CopyFromMesh(this->starting_pattern);
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

struct TNeighbor { vtkIdType iNeighbor; float diffusion_coefficient; };

void add_if_new(vector<TNeighbor>& neighbors,TNeighbor neighbor)
{
    for(vector<TNeighbor>::const_iterator it=neighbors.begin();it!=neighbors.end();it++)
        if(it->iNeighbor==neighbor.iNeighbor)
            return;
    neighbors.push_back(neighbor);
}

// ---------------------------------------------------------------------

void MeshRD::ComputeCellNeighbors()
{
    enum TNeighborhood { CELL_NEIGHBORS, VERTEX_NEIGHBORS };
    enum TDiffusionCoefficient { EQUAL, EUCLIDEAN_DISTANCE, BOUNDARY_SIZE };
    // TODO: when these possibilities have settled down, allow them to be specified as file options

    TNeighborhood neighborhood_type = VERTEX_NEIGHBORS;
    TDiffusionCoefficient diffusion_type = EQUAL;

    if(diffusion_type==BOUNDARY_SIZE && neighborhood_type!=CELL_NEIGHBORS)
        throw runtime_error("MeshRD::ComputeCellNeighbors : BOUNDARY_SIZE diffusion only works with CELL_NEIGHBORS neighborhood");

    vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    TNeighbor nbor;

    vector<vector<TNeighbor> > cell_neighbors; // the connectivity between cells; for each cell, what cells are its neighbors?
    this->max_neighbors = 0;
    for(vtkIdType iCell=0;iCell<this->mesh->GetNumberOfCells();iCell++)
    {
        vector<TNeighbor> neighbors;
        this->mesh->GetCellPoints(iCell,ptIds);
        vtkIdType npts = ptIds->GetNumberOfIds();
        switch(neighborhood_type)
        {
            case VERTEX_NEIGHBORS: // neighbors share a vertex
            {
                for(vtkIdType iPt=0;iPt<npts;iPt++)
                {
                    vtkSmartPointer<vtkIdList> edgeIds = vtkSmartPointer<vtkIdList>::New();
                    edgeIds->SetNumberOfIds(1);
                    edgeIds->SetId(0,ptIds->GetId(iPt));
                    this->mesh->GetCellNeighbors(iCell,edgeIds,cellIds);
                    int nNeighbors = cellIds->GetNumberOfIds();
                    for(vtkIdType iNeighbor=0;iNeighbor<nNeighbors;iNeighbor++)
                    {
                        nbor.iNeighbor = cellIds->GetId(iNeighbor);
                        switch(diffusion_type)
                        {
                            case EQUAL: nbor.diffusion_coefficient = 1.0f; break;
                            default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported diffusion type");
                        }
                        add_if_new(neighbors,nbor);
                    }
                }
            }
            break;
            case CELL_NEIGHBORS: // neighbors share an edge/face
            {
                switch(this->mesh->GetCellType(iCell))
                {
                    case VTK_POLYGON: // a 2D face, neighbors share an edge
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
                                nbor.iNeighbor = cellIds->GetId(iNeighbor);
                                switch(diffusion_type)
                                {
                                    case EQUAL: nbor.diffusion_coefficient = 1.0f; break;
                                    default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported diffusion type");
                                }
                                add_if_new(neighbors,nbor);
                            }
                        }
                    }
                    break;
                    case VTK_TETRA: // a 3D tetrahedral cell, neighbors share a triangular face
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
                                nbor.iNeighbor = cellIds->GetId(iNeighbor);
                                switch(diffusion_type)
                                {
                                    case EQUAL: nbor.diffusion_coefficient = 1.0f; break;
                                    default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported diffusion type");
                                }
                                add_if_new(neighbors,nbor);
                            }
                        }
                    }
                    break;
                    default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported cell type");
                }
            }        
            break;
            default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported neighborhood type");
        }
        // normalize the diffusion coefficients for this cell
        float coeff_sum=0.0f;
        for(int iN=0;iN<(int)neighbors.size();iN++)
            coeff_sum += neighbors[iN].diffusion_coefficient;
        for(int iN=0;iN<(int)neighbors.size();iN++)
            neighbors[iN].diffusion_coefficient /= coeff_sum;
        // store this list of neighbors
        cell_neighbors.push_back(neighbors);
        if((int)neighbors.size()>this->max_neighbors)
            this->max_neighbors = (int)neighbors.size();
    }
    // N.B. we don't intend to support cells of mixed dimensionality in one mesh

    // copy data to plain arrays
    this->cell_neighbor_indices = new int[this->mesh->GetNumberOfCells()*this->max_neighbors];
    this->cell_neighbor_weights = new float[this->mesh->GetNumberOfCells()*this->max_neighbors];
    for(int i=0;i<this->mesh->GetNumberOfCells();i++)
    {
        for(int j=0;j<(int)cell_neighbors[i].size();j++)
        {
            int k = i*this->max_neighbors + j;
            this->cell_neighbor_indices[k] = cell_neighbors[i][j].iNeighbor;
            this->cell_neighbor_weights[k] = cell_neighbors[i][j].diffusion_coefficient;
        }
        // fill any remaining slots with iCell,0.0
        for(int j=(int)cell_neighbors[i].size();j<max_neighbors;j++)
        {
            int k = i*this->max_neighbors + j;
            this->cell_neighbor_indices[k] = i;
            this->cell_neighbor_weights[k] = 0.0f;
        }
    }
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

void MeshRD::GetAs2DImage(vtkImageData *out,const Properties& render_settings) const
{
    // TODO
}

// ---------------------------------------------------------------------

/* static */ void MeshRD::GetGeodesicSphere(int n_subdivisions,vtkUnstructuredGrid *mesh,int n_chems)
{
    vtkSmartPointer<vtkPlatonicSolidSource> icosahedron = vtkSmartPointer<vtkPlatonicSolidSource>::New();
    icosahedron->SetSolidTypeToIcosahedron();
    vtkSmartPointer<vtkLinearSubdivisionFilter> subdivider = vtkSmartPointer<vtkLinearSubdivisionFilter>::New();
    subdivider->SetInputConnection(icosahedron->GetOutputPort());
    subdivider->SetNumberOfSubdivisions(n_subdivisions);
    subdivider->Update();
    mesh->SetPoints(subdivider->GetOutput()->GetPoints());
    mesh->SetCells(VTK_POLYGON,subdivider->GetOutput()->GetPolys());

    // push the vertices out into the shape of a sphere
    vtkFloatingPointType p[3];
    for(int i=0;i<mesh->GetNumberOfPoints();i++)
    {
        mesh->GetPoint(i,p);
        vtkMath::Normalize(p);
        mesh->GetPoints()->SetPoint(i,p);
    }

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

/* static */ void MeshRD::GetTetrahedralMesh(int n_points,vtkUnstructuredGrid *mesh,int n_chems)
{
    // TODO: we could make any number of shapes here but we need a more general mechanism, 
    // e.g. input a closed surface, scatter points inside, tetrahedralize

    // make a tetrahedral mesh by delaunay tetrahedralization on a point cloud
    vtkSmartPointer<vtkPointSource> pts = vtkSmartPointer<vtkPointSource>::New();
    pts->SetNumberOfPoints(n_points);
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(2,1,1); // just to make it a bit more interesting we stretch the points in one direction
    vtkSmartPointer<vtkTransformPolyDataFilter> trans = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    trans->SetTransform(transform);
    trans->SetInputConnection(pts->GetOutputPort());
    vtkSmartPointer<vtkDelaunay3D> del = vtkSmartPointer<vtkDelaunay3D>::New();
    del->SetInputConnection(trans->GetOutputPort());
    del->Update();
    mesh->DeepCopy(del->GetOutput());

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

/* static */ void MeshRD::GetTorus(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    float radius1 = 2.0f;
    float radius2 = 3.0f;
    vtkSmartPointer<vtkTransform> r1 = vtkSmartPointer<vtkTransform>::New();
    vtkSmartPointer<vtkTransform> r2 = vtkSmartPointer<vtkTransform>::New();
    double p[3],p2[3];
    for(int x=0;x<nx;x++)
    {
        p[0]=p[1]=p[2]=0;
        // translate, rotate
        p[1] += radius1;
        r1->TransformPoint(p,p);
        // rotate the transform further for next time
        r1->RotateX(360.0/nx);
        for(int y=0;y<ny;y++)
        {
            // translate
            p2[0] = p[0];
            p2[1] = p[1] + radius2;
            p2[2] = p[2];
            // rotate
            r2->TransformPoint(p2,p2);
            pts->InsertNextPoint(p2);
            // rotate the transform further for next time
            r2->RotateZ(360.0/ny);
            // make a quad 
            cells->InsertNextCell(4);
            cells->InsertCellPoint(x*ny+y);
            cells->InsertCellPoint(x*ny+(y+1)%ny);
            cells->InsertCellPoint(((x+1)%nx)*ny+(y+1)%ny);
            cells->InsertCellPoint(((x+1)%nx)*ny+y);
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

/* static */ void MeshRD::GetTriangularMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y;
        for(int x=0;x<nx;x++)
        {
            p[0] = ((y%2)?0.5:0) + x;
            pts->InsertNextPoint(p);
            if(y%2 && x<nx-1)
            {
                cells->InsertNextCell(3);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint((y-1)*nx+x+1);
                cells->InsertCellPoint((y-1)*nx+x);
                cells->InsertNextCell(3);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint(y*nx+x+1);
                cells->InsertCellPoint((y-1)*nx+x+1);
                if(y<ny-1)
                {
                    cells->InsertNextCell(3);
                    cells->InsertCellPoint(y*nx+x);
                    cells->InsertCellPoint((y+1)*nx+x);
                    cells->InsertCellPoint((y+1)*nx+x+1);
                    cells->InsertNextCell(3);
                    cells->InsertCellPoint(y*nx+x);
                    cells->InsertCellPoint(y*nx+x+1);
                    cells->InsertCellPoint((y+1)*nx+x+1);
                }
            }
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

/* static */ void MeshRD::GetHexagonalMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y;
        for(int x=0;x<nx;x++)
        {
            p[0] = ((y%2)?0.5:0) + x;
            pts->InsertNextPoint(p);
            if(y%2 && x%3==2 && y<ny-1)
            {
                cells->InsertNextCell(6);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint((y-1)*nx+x);
                cells->InsertCellPoint((y-1)*nx+x-1);
                cells->InsertCellPoint(y*nx+x-2);
                cells->InsertCellPoint((y+1)*nx+x-1);
                cells->InsertCellPoint((y+1)*nx+x);
            }
            else if(y%2==0 && x%3==1 && y>0 && y<ny-1 && x>1)
            {
                cells->InsertNextCell(6);
                cells->InsertCellPoint(y*nx+x);
                cells->InsertCellPoint((y-1)*nx+x-1);
                cells->InsertCellPoint((y-1)*nx+x-2);
                cells->InsertCellPoint(y*nx+x-2);
                cells->InsertCellPoint((y+1)*nx+x-2);
                cells->InsertCellPoint((y+1)*nx+x-1);
            }
        }
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
}

// ---------------------------------------------------------------------

/// A two-dimensional triangle, used in MeshRD::GetPenroseRhombiTiling().
struct Tri {
    double ax,ay,bx,by,cx,cy;
    Tri(double ax2,double ay2,double bx2,double by2,double cx2,double cy2)
        : ax(ax2), ay(ay2), bx(bx2), by(by2), cx(cx2), cy(cy2) {}
};

/// The indices of a triangle, used in MeshRD::GetPenroseRhombiTiling().
struct TriIndices {
    double ind[3];
};

/* static */ void MeshRD::GetPenroseTiling(int n_subdivisions,int type,vtkUnstructuredGrid* mesh,int n_chems)
{
    // Many thanks to Jeff Preshing: http://preshing.com/20110831/penrose-tiling-explained

    const int RHOMBI = 0;
    const int DARTS_AND_KITES = 1;

    vector<Tri> red_tris[2],blue_tris[2]; // each list has two buffers
    int iCurrentBuffer = 0;

    double goldenRatio = (1.0 + sqrt(5.0)) / 2.0;

    // start with 10 red triangles in a wheel, to get a nice circular shape (with 5-fold rotational symmetry)
    // (any correctly-tiled starting pattern will work too)
    double angle_step = 2.0 * 3.1415926535 / 10.0;
    for(int i=0;i<10;i++)
    {
        double angle1 = angle_step * (i + i%2);
        double angle2 = angle_step * (i + 1 - i%2);
        switch(type) {
            default:
            case RHOMBI: red_tris[iCurrentBuffer].push_back(Tri(0,0,cos(angle1),sin(angle1),cos(angle2),sin(angle2))); break;
            case DARTS_AND_KITES: red_tris[iCurrentBuffer].push_back(Tri(cos(angle1),sin(angle1),0,0,cos(angle2),sin(angle2))); break;
        }
    }

    // subdivide
    double px,py,qx,qy,rx,ry;
    for(int i=0;i<n_subdivisions;i++)
    {
        int iTargetBuffer = 1-iCurrentBuffer;
        red_tris[iTargetBuffer].clear();
        blue_tris[iTargetBuffer].clear();
        // each red triangle becomes a smaller red and a blue
        for(vector<Tri>::const_iterator it = red_tris[iCurrentBuffer].begin();it!=red_tris[iCurrentBuffer].end();it++)
        {
            switch(type)
            {
                default:
                case RHOMBI:
                    px = it->ax + (it->bx - it->ax) / goldenRatio;
                    py = it->ay + (it->by - it->ay) / goldenRatio;
                    red_tris[iTargetBuffer].push_back(Tri(it->cx,it->cy,px,py,it->bx,it->by));
                    blue_tris[iTargetBuffer].push_back(Tri(px,py,it->cx,it->cy,it->ax,it->ay));
                    break;
                case DARTS_AND_KITES:
                    qx = it->ax + (it->bx - it->ax) / goldenRatio;
                    qy = it->ay + (it->by - it->ay) / goldenRatio;
                    rx = it->bx + (it->cx - it->bx) / goldenRatio;
                    ry = it->by + (it->cy - it->by) / goldenRatio;
                    blue_tris[iTargetBuffer].push_back(Tri(rx,ry,qx,qy,it->bx,it->by));
                    red_tris[iTargetBuffer].push_back(Tri(qx,qy,it->ax,it->ay,rx,ry));
                    red_tris[iTargetBuffer].push_back(Tri(it->cx,it->cy,it->ax,it->ay,rx,ry));
                    break;
            }
        }
        // each blue triangle becomes a smaller red and two blues
        for(vector<Tri>::const_iterator it = blue_tris[iCurrentBuffer].begin();it!=blue_tris[iCurrentBuffer].end();it++)
        {
            switch(type)
            {
                default:
                case RHOMBI:
                    qx = it->bx + (it->ax - it->bx) / goldenRatio;
                    qy = it->by + (it->ay - it->by) / goldenRatio;
                    rx = it->bx + (it->cx - it->bx) / goldenRatio;
                    ry = it->by + (it->cy - it->by) / goldenRatio;
                    red_tris[iTargetBuffer].push_back(Tri(rx,ry,qx,qy,it->ax,it->ay));
                    blue_tris[iTargetBuffer].push_back(Tri(rx,ry,it->cx,it->cy,it->ax,it->ay));
                    blue_tris[iTargetBuffer].push_back(Tri(qx,qy,rx,ry,it->bx,it->by));
                    break;
                case DARTS_AND_KITES:
                    px = it->cx + (it->ax - it->cx) / goldenRatio;
                    py = it->cy + (it->ay - it->cy) / goldenRatio;
                    blue_tris[iTargetBuffer].push_back(Tri(it->bx,it->by,px,py,it->ax,it->ay));
                    red_tris[iTargetBuffer].push_back(Tri(px,py,it->cx,it->cy,it->bx,it->by));
                    break;
            }
        }
        iCurrentBuffer = iTargetBuffer;
    }

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    // merge coincident vertices and merge abutting triangles into quads
    double tol = hypot2(red_tris[iCurrentBuffer][0].ax-red_tris[iCurrentBuffer][0].bx,red_tris[iCurrentBuffer][0].ay-red_tris[iCurrentBuffer][0].by)/100.0;
    vector<pair<double,double> > verts;
    vector<TriIndices> tris;
    vector<Tri> all_tris(red_tris[iCurrentBuffer]);
    all_tris.insert(all_tris.end(),blue_tris[iCurrentBuffer].begin(),blue_tris[iCurrentBuffer].end());
    for(vector<Tri>::const_iterator it = all_tris.begin();it!=all_tris.end();it++)
    {
        TriIndices tri;
        for(int i=0;i<3;i++)
        {
            switch(i)
            {
                case 0: px = it->ax; py = it->ay; break;
                case 1: px = it->bx; py = it->by; break;
                case 2: px = it->cx; py = it->cy; break;
            }
            // have we seen px,py before?
            int iPt;
            bool found = false;
            for(int j=0;j<(int)verts.size();j++)
            {
                if(hypot2(verts[j].first-px,verts[j].second-py) < tol)
                {
                    iPt = j;
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                verts.push_back(make_pair(px,py));
                pts->InsertNextPoint(px,py,0);
                iPt = (int)verts.size()-1;
            }
            tri.ind[i] = iPt;
        }
        // is this the other half of a triangle we've seen previously?
        for(vector<TriIndices>::const_iterator it2 = tris.begin();it2!=tris.end();it2++)
        {
            if( (it2->ind[1] == tri.ind[1] && it2->ind[2] == tri.ind[2]) || (it2->ind[2] == tri.ind[1] && it2->ind[1] == tri.ind[2]) )
            {
                cells->InsertNextCell(4);
                cells->InsertCellPoint(tri.ind[0]);
                cells->InsertCellPoint(tri.ind[1]);
                cells->InsertCellPoint(it2->ind[0]);
                cells->InsertCellPoint(tri.ind[2]);
            }
        }
        // go ahead and add the tri now
        tris.push_back(tri);
    }
    
    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,cells);

    // allocate the chemicals arrays
    for(int iChem=0;iChem<n_chems;iChem++)
    {
        vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
        scalars->SetNumberOfComponents(1);
        scalars->SetNumberOfTuples(mesh->GetNumberOfCells());
        scalars->SetName(GetChemicalName(iChem).c_str());
        scalars->FillComponent(0,0.0f);
        mesh->GetCellData()->AddArray(scalars);
    }
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
