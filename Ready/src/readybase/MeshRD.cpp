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
#include <vtkCellDataToPointData.h>
#include <vtkCellLocator.h>

// STL:
#include <stdexcept>
using namespace std;

// ---------------------------------------------------------------------

MeshRD::MeshRD()
{
    this->starting_pattern = vtkUnstructuredGrid::New();
    this->mesh = vtkUnstructuredGrid::New();
    this->cell_neighbor_indices = NULL;
    this->cell_neighbor_weights = NULL;
    this->cell_locator = NULL;
}

// ---------------------------------------------------------------------

MeshRD::~MeshRD()
{
    delete []this->cell_neighbor_indices;
    delete []this->cell_neighbor_weights;

    this->mesh->Delete();
    this->starting_pattern->Delete();
    this->n_chemicals = 0;

    if(this->cell_locator)
        this->cell_locator->Delete();
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

    if(this->cell_locator)
    {
        this->cell_locator->Delete();
        this->cell_locator = NULL;
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
    string activeChemical = render_settings.GetProperty("active_chemical").GetChemical();
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_bounding_box = render_settings.GetProperty("show_bounding_box").GetBool();

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
    lut->Build();

    // add the mesh actor
    {
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->ImmediateModeRenderingOn();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        if(use_wireframe && !slice_3D) // full wireframe mode: all internal edges
        {
            // explicitly extract the edges - the default mapper only shows the outside surface
            vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
            edges->SetInput(this->mesh);
            mapper->SetInputConnection(edges->GetOutputPort());
            mapper->SetScalarModeToUseCellFieldData();
        }
        else if(slice_3D) // partial wireframe mode: only external surface edges
        {
            vtkSmartPointer<vtkGeometryFilter> geom = vtkSmartPointer<vtkGeometryFilter>::New();
            geom->SetInput(this->mesh);
            vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
            edges->SetInputConnection(geom->GetOutputPort());
            mapper->SetInputConnection(edges->GetOutputPort());
            mapper->SetScalarModeToUseCellFieldData();
        }
        else // non-wireframe mode: shows filled external surface
        {
            if(use_image_interpolation)
            {
                vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
                to_point_data->SetInput(this->mesh);
                mapper->SetInputConnection(to_point_data->GetOutputPort());
                mapper->SetScalarModeToUsePointFieldData();
            }
            else
            {
                mapper->SetInput(this->mesh);
                mapper->SetScalarModeToUseCellFieldData();
            }
            if(show_cell_edges)
            {
                actor->GetProperty()->EdgeVisibilityOn();
                actor->GetProperty()->SetEdgeColor(0,0,0); // could be a user option
            }
        }
        mapper->SelectColorArray(activeChemical.c_str());
        mapper->SetLookupTable(lut);
        mapper->UseLookupTableScalarRangeOn();

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
        cutter->SetCutFunction(plane);
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(cutter->GetOutputPort());
        mapper->ImmediateModeRenderingOn();
        if(use_image_interpolation)
        {
            vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
            to_point_data->SetInput(this->mesh);
            cutter->SetInputConnection(to_point_data->GetOutputPort());
            mapper->SetScalarModeToUsePointFieldData();
        }
        else
        {
            cutter->SetInput(this->mesh);
            mapper->SetScalarModeToUseCellFieldData();
        }
        mapper->SelectColorArray(activeChemical.c_str());
        mapper->SetLookupTable(lut);
        mapper->UseLookupTableScalarRangeOn();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->LightingOff();
        if(show_cell_edges)
        {
            actor->GetProperty()->EdgeVisibilityOn();
            actor->GetProperty()->SetEdgeColor(0,0,0); // could be a user option
        }
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
    if(show_bounding_box)
    {
        vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
        box->SetBounds(this->mesh->GetBounds());

        vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
        edges->SetInputConnection(box->GetOutputPort());

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(edges->GetOutputPort());
        mapper->ImmediateModeRenderingOn();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0,0,0);  
        actor->GetProperty()->SetAmbient(1);

        actor->PickableOff();
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
        coeff_sum = max(coeff_sum,1e-5f); // avoid div0
        for(int iN=0;iN<(int)neighbors.size();iN++)
            neighbors[iN].diffusion_coefficient /= coeff_sum;
        // store this list of neighbors
        cell_neighbors.push_back(neighbors);
        if((int)neighbors.size()>this->max_neighbors)
            this->max_neighbors = (int)neighbors.size();
    }
    // N.B. we don't intend to support cells of mixed dimensionality in one mesh

    // copy data to plain arrays
    if(this->cell_neighbor_indices) delete []this->cell_neighbor_indices;
    if(this->cell_neighbor_weights) delete []this->cell_neighbor_weights;
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

float MeshRD::GetValue(float x,float y,float z,const Properties& render_settings)
{
    this->CreateCellLocatorIfNeeded();

    double p[3]={x,y,z};
    vtkIdType iCell = this->cell_locator->FindCell(p);

    if(iCell<0)
        return 0.0f;

    int iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    return vtkFloatArray::SafeDownCast(this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str()))->GetValue(iCell);
}

// --------------------------------------------------------------------------------

void MeshRD::SetValue(float x,float y,float z,float val,const Properties& render_settings)
{
    this->CreateCellLocatorIfNeeded();

    double p[3]={x,y,z};
    vtkIdType iCell = this->cell_locator->FindCell(p);

    if(iCell<0)
        return;

    int iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    vtkFloatArray::SafeDownCast(this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str()))->SetValue(iCell,val);
    this->mesh->Modified();
}

// --------------------------------------------------------------------------------

void MeshRD::SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings)
{
    this->CreateCellLocatorIfNeeded();

    double *dataset_bbox = this->mesh->GetBounds();
    r *= hypot3(dataset_bbox[1]-dataset_bbox[0],dataset_bbox[3]-dataset_bbox[2],dataset_bbox[5]-dataset_bbox[4]);

    double bbox[6]={x-r,x+r,y-r,y+r,z-r,z+r};
    vtkSmartPointer<vtkIdList> cells = vtkSmartPointer<vtkIdList>::New();
    this->cell_locator->FindCellsWithinBounds(bbox,cells);

    int iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    for(vtkIdType i=0;i<cells->GetNumberOfIds();i++)
    {
        int iCell = cells->GetId(i);
        vtkIdType npts,*pts;
        this->mesh->GetCellPoints(iCell,npts,pts);
        // get a point at the centre of the cell (need a location to sample the overlays)
        double cp[3] = {0,0,0};
        for(vtkIdType iPt=0;iPt<npts;iPt++)
            for(int xyz=0;xyz<3;xyz++)
                cp[xyz] += this->mesh->GetPoint(pts[iPt])[xyz];
        for(int xyz=0;xyz<3;xyz++)
            cp[xyz] /= npts;
        if(hypot3(cp[0]-x,cp[1]-y,cp[2]-z)<r)
            vtkFloatArray::SafeDownCast(this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str()))->SetValue(iCell,val);
    }
    this->mesh->Modified();
}

// --------------------------------------------------------------------------------

void MeshRD::CreateCellLocatorIfNeeded()
{
    if(this->cell_locator) return;

    this->cell_locator = vtkCellLocator::New();
    this->cell_locator->SetDataSet(this->mesh);
    this->cell_locator->SetTolerance(0.0001);
    this->cell_locator->BuildLocator();
}

// --------------------------------------------------------------------------------
