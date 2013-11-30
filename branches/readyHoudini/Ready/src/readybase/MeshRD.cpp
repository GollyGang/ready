/*  Copyright 2011, 2012, 2013 The Ready Bunch

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
#include "IO_XML.hpp"
#include "MeshRD.hpp"
#include "overlays.hpp"
#include "Properties.hpp"
#include "scene_items.hpp"
#include "utils.hpp"

// VTK:
#include <vtkActor.h>
#include <vtkAssignAttribute.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkCellLocator.h>
#include <vtkContourFilter.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkCubeSource.h>
#include <vtkCutter.h>
#include <vtkDataSetMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkExtractEdges.h>
#include <vtkFloatArray.h>
#include <vtkGenericCell.h>
#include <vtkGeometryFilter.h>
#include <vtkIdList.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMergeFilter.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPointSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRearrangeFields.h>
#include <vtkRenderer.h>
#include <vtkReverseSense.h>
#include <vtkScalarBarActor.h>
#include <vtkThreshold.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkWarpScalar.h>
#include <vtkXMLDataElement.h>

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
    this->undo_stack.clear();
    this->InternalUpdate(n_steps);

    this->timesteps_taken += n_steps;

    this->mesh->Modified();
}

// ---------------------------------------------------------------------

void MeshRD::SetNumberOfChemicals(int n)
{
    for(int iChem=0;iChem<this->n_chemicals;iChem++)
    {
        this->mesh->GetCellData()->RemoveArray(GetChemicalName(iChem).c_str());
    }
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

void MeshRD::SaveFile(const char* filename,const Properties& render_settings,bool generate_initial_pattern_when_loading) const
{
    vtkSmartPointer<RD_XMLUnstructuredGridWriter> iw = vtkSmartPointer<RD_XMLUnstructuredGridWriter>::New();
    iw->SetSystem(this);
    iw->SetRenderSettings(&render_settings);
    if(generate_initial_pattern_when_loading)
        iw->GenerateInitialPatternWhenLoading();
    iw->SetFileName(filename);
    iw->SetDataModeToBinary(); // workaround for http://www.vtk.org/Bug/view.php?id=13382
    #if VTK_MAJOR_VERSION >= 6
        iw->SetInputData(this->mesh);
    #else
        iw->SetInput(this->mesh);
    #endif
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
    this->undo_stack.clear();
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
    this->undo_stack.clear();
    this->mesh->DeepCopy(mesh2);
    this->n_chemicals = this->mesh->GetCellData()->GetNumberOfArrays();

    if(this->cell_locator)
    {
        this->cell_locator->Delete();
        this->cell_locator = NULL;
    }

    this->ComputeCellNeighbors(this->neighborhood_type,this->neighborhood_range,
        this->neighborhood_weight_type);
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
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();
    float surface_r,surface_g,surface_b;
    render_settings.GetProperty("surface_color").GetColor(surface_r,surface_g,surface_b);
    bool slice_3D = render_settings.GetProperty("slice_3D").GetBool();
    string slice_3D_axis = render_settings.GetProperty("slice_3D_axis").GetAxis();
    float slice_3D_position = render_settings.GetProperty("slice_3D_position").GetFloat();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());

    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetRampToLinear();
    lut->SetScaleToLinear();
    lut->SetTableRange(low,high);
    lut->SetSaturationRange(low_sat,high_sat);
    lut->SetHueRange(low_hue,high_hue);
    lut->SetValueRange(low_val,high_val);
    lut->Build();

    if(this->mesh->GetCellType(0)==VTK_POLYGON)
    {
        // add the mesh actor
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->ImmediateModeRenderingOn();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        if(use_wireframe && !slice_3D) // full wireframe mode: all internal edges
        {
            // explicitly extract the edges - the default mapper only shows the outside surface
            vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
            #if VTK_MAJOR_VERSION >= 6
                edges->SetInputData(this->mesh);
            #else
                edges->SetInput(this->mesh);
            #endif
            mapper->SetInputConnection(edges->GetOutputPort());
            mapper->SetScalarModeToUseCellFieldData();
        }
        else if(slice_3D) // partial wireframe mode: only external surface edges
        {
            vtkSmartPointer<vtkGeometryFilter> geom = vtkSmartPointer<vtkGeometryFilter>::New();
            #if VTK_MAJOR_VERSION >= 6
                geom->SetInputData(this->mesh);
            #else
                geom->SetInput(this->mesh);
            #endif
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
                #if VTK_MAJOR_VERSION >= 6
                    to_point_data->SetInputData(this->mesh);
                #else
                    to_point_data->SetInput(this->mesh);
                #endif
                mapper->SetInputConnection(to_point_data->GetOutputPort());
                mapper->SetScalarModeToUsePointFieldData();
            }
            else
            {
                #if VTK_MAJOR_VERSION >= 6
                    mapper->SetInputData(this->mesh);
                #else
                    mapper->SetInput(this->mesh);
                #endif
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
    else if(use_image_interpolation)
    {
        // show a contour
        vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
        #if VTK_MAJOR_VERSION >= 6
            assign_attribute->SetInputData(this->mesh);
        #else
            assign_attribute->SetInput(this->mesh);
        #endif
        assign_attribute->Assign(activeChemical.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
        vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
        to_point_data->SetInputConnection(assign_attribute->GetOutputPort());
        vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
        surface->SetInputConnection(to_point_data->GetOutputPort());
        surface->SetValue(0,contour_level);
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(surface->GetOutputPort());
        mapper->ImmediateModeRenderingOn();
        mapper->ScalarVisibilityOff();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(surface_r,surface_g,surface_b);  
        actor->GetProperty()->SetAmbient(0.1);
        actor->GetProperty()->SetDiffuse(0.7);
        actor->GetProperty()->SetSpecular(0.2);
        actor->GetProperty()->SetSpecularPower(3);
        if(use_wireframe)
            actor->GetProperty()->SetRepresentationToWireframe();
        /*vtkSmartPointer<vtkProperty> bfprop = vtkSmartPointer<vtkProperty>::New();
        actor->SetBackfaceProperty(bfprop);
        bfprop->SetColor(0.3,0.3,0.3);
        bfprop->SetAmbient(0.3);
        bfprop->SetDiffuse(0.6);
        bfprop->SetSpecular(0.1);*/ // TODO: re-enable this if can get correct normals
        actor->PickableOff();
        pRenderer->AddActor(actor);
    }
    else // visualise the cells
    {
        vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
        #if VTK_MAJOR_VERSION >= 6
            assign_attribute->SetInputData(this->mesh);
        #else
            assign_attribute->SetInput(this->mesh);
        #endif
        assign_attribute->Assign(activeChemical.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        threshold->SetInputConnection(assign_attribute->GetOutputPort());
        threshold->ThresholdByUpper(contour_level);
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputConnection(threshold->GetOutputPort());
        mapper->SetLookupTable(lut);
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        if(show_cell_edges)
        {
            actor->GetProperty()->EdgeVisibilityOn();
            actor->GetProperty()->SetEdgeColor(0,0,0); // could be a user option
        }
        if(use_wireframe)
            actor->GetProperty()->SetRepresentationToWireframe();
        actor->PickableOff();
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
            #if VTK_MAJOR_VERSION >= 6
                to_point_data->SetInputData(this->mesh);
            #else
                to_point_data->SetInput(this->mesh);
            #endif
            cutter->SetInputConnection(to_point_data->GetOutputPort());
            mapper->SetScalarModeToUsePointFieldData();
        }
        else
        {
            #if VTK_MAJOR_VERSION >= 6
                cutter->SetInputData(this->mesh);
            #else
                cutter->SetInput(this->mesh);
            #endif
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
        AddScalarBar(pRenderer,lut);
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

    // add a phase plot
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        this->AddPhasePlot( pRenderer,GetX()/(high-low),low,high,
                            this->mesh->GetBounds()[0],
                            this->mesh->GetBounds()[3]+GetY()*0.1f,
                            this->mesh->GetBounds()[4],
                            iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void MeshRD::AddPhasePlot(vtkRenderer* pRenderer,float scaling,float low,float high,float posX,float posY,float posZ,
    int iChemX,int iChemY,int iChemZ)
{
    iChemX = max( 0, min( iChemX, this->GetNumberOfChemicals()-1 ) );
    iChemY = max( 0, min( iChemY, this->GetNumberOfChemicals()-1 ) );
    iChemZ = max( 0, min( iChemZ, this->GetNumberOfChemicals()-1 ) );
    
    vtkSmartPointer<vtkPointSource> points = vtkSmartPointer<vtkPointSource>::New();
    points->SetNumberOfPoints(this->GetNumberOfCells());
    points->SetRadius(0);

    vtkSmartPointer<vtkRearrangeFields> rearrange_fieldsX = vtkSmartPointer<vtkRearrangeFields>::New();
    #if VTK_MAJOR_VERSION >= 6
        rearrange_fieldsX->SetInputData(this->mesh);
    #else
        rearrange_fieldsX->SetInput(this->mesh);
    #endif
    rearrange_fieldsX->AddOperation(vtkRearrangeFields::MOVE,GetChemicalName(iChemX).c_str(),vtkRearrangeFields::CELL_DATA,vtkRearrangeFields::POINT_DATA);
    vtkSmartPointer<vtkAssignAttribute> assign_attributeX = vtkSmartPointer<vtkAssignAttribute>::New();
    assign_attributeX->SetInputConnection(rearrange_fieldsX->GetOutputPort());
    assign_attributeX->Assign(GetChemicalName(iChemX).c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    vtkSmartPointer<vtkMergeFilter> mergeX = vtkSmartPointer<vtkMergeFilter>::New();
    mergeX->SetGeometryConnection(points->GetOutputPort());
    mergeX->SetScalarsConnection(assign_attributeX->GetOutputPort());
    vtkSmartPointer<vtkWarpScalar> warpX = vtkSmartPointer<vtkWarpScalar>::New();
    warpX->UseNormalOn();
    warpX->SetNormal(1,0,0);
    warpX->SetInputConnection(mergeX->GetOutputPort());
    warpX->SetScaleFactor(scaling);

    vtkSmartPointer<vtkRearrangeFields> rearrange_fieldsY = vtkSmartPointer<vtkRearrangeFields>::New();
    #if VTK_MAJOR_VERSION >= 6
        rearrange_fieldsY->SetInputData(this->mesh);
    #else
        rearrange_fieldsY->SetInput(this->mesh);
    #endif
    rearrange_fieldsY->AddOperation(vtkRearrangeFields::MOVE,GetChemicalName(iChemY).c_str(),vtkRearrangeFields::CELL_DATA,vtkRearrangeFields::POINT_DATA);
    vtkSmartPointer<vtkAssignAttribute> assign_attributeY = vtkSmartPointer<vtkAssignAttribute>::New();
    assign_attributeY->SetInputConnection(rearrange_fieldsY->GetOutputPort());
    assign_attributeY->Assign(GetChemicalName(iChemY).c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    vtkSmartPointer<vtkMergeFilter> mergeY = vtkSmartPointer<vtkMergeFilter>::New();
    mergeY->SetGeometryConnection(warpX->GetOutputPort());
    mergeY->SetScalarsConnection(assign_attributeY->GetOutputPort());
    vtkSmartPointer<vtkWarpScalar> warpY = vtkSmartPointer<vtkWarpScalar>::New();
    warpY->UseNormalOn();
    warpY->SetNormal(0,1,0);
    warpY->SetInputConnection(mergeY->GetOutputPort());
    warpY->SetScaleFactor(scaling);

    vtkSmartPointer<vtkVertexGlyphFilter> glyph = vtkSmartPointer<vtkVertexGlyphFilter>::New();

    float offsetZ = 0.0f;
    if(this->GetNumberOfChemicals()>2)
    {
        vtkSmartPointer<vtkRearrangeFields> rearrange_fieldsZ = vtkSmartPointer<vtkRearrangeFields>::New();
        #if VTK_MAJOR_VERSION >= 6
            rearrange_fieldsZ->SetInputData(this->mesh);
        #else
            rearrange_fieldsZ->SetInput(this->mesh);
        #endif
        rearrange_fieldsZ->AddOperation(vtkRearrangeFields::MOVE,GetChemicalName(iChemZ).c_str(),vtkRearrangeFields::CELL_DATA,vtkRearrangeFields::POINT_DATA);
        vtkSmartPointer<vtkAssignAttribute> assign_attributeZ = vtkSmartPointer<vtkAssignAttribute>::New();
        assign_attributeZ->SetInputConnection(rearrange_fieldsZ->GetOutputPort());
        assign_attributeZ->Assign(GetChemicalName(iChemZ).c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
        vtkSmartPointer<vtkMergeFilter> mergeZ = vtkSmartPointer<vtkMergeFilter>::New();
        mergeZ->SetGeometryConnection(warpY->GetOutputPort());
        mergeZ->SetScalarsConnection(assign_attributeZ->GetOutputPort());
        vtkSmartPointer<vtkWarpScalar> warpZ = vtkSmartPointer<vtkWarpScalar>::New();
        warpZ->UseNormalOn();
        warpZ->SetNormal(0,0,1);
        warpZ->SetInputConnection(mergeZ->GetOutputPort());
        warpZ->SetScaleFactor(scaling);

        glyph->SetInputConnection(warpZ->GetOutputPort());

        offsetZ = low*scaling;
    }
    else
    {
        glyph->SetInputConnection(warpY->GetOutputPort());
    }

    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
    trans->Scale(1,1,-1);
    vtkSmartPointer<vtkTransformFilter> transFilter = vtkSmartPointer<vtkTransformFilter>::New();
    transFilter->SetTransform(trans);
    transFilter->SetInputConnection(glyph->GetOutputPort());

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transFilter->GetOutputPort());
    mapper->ScalarVisibilityOff();
    mapper->ImmediateModeRenderingOn();
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbient(1);
    actor->GetProperty()->SetPointSize(1);
    actor->PickableOff();
    actor->SetPosition(posX-low*scaling,posY-low*scaling,posZ+offsetZ);
    pRenderer->AddActor(actor);

    // also add the axes
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX+scaling*(high-low),posY,posY,posZ,posZ);
        axis->SetRanges(low,high,0,0,0,0);
        axis->UseRangesOn();
        axis->YAxisVisibilityOff();
        axis->ZAxisVisibilityOff();
        axis->SetXLabel(GetChemicalName(iChemX).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX,posY,posY+(high-low)*scaling,posZ,posZ);
        axis->SetRanges(0,0,low,high,0,0);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->ZAxisVisibilityOff();
        axis->SetYLabel(GetChemicalName(iChemY).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
    if(this->GetNumberOfChemicals()>2)
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX,posY,posY,posZ,posZ-scaling*(high-low));
        axis->SetRanges(0,0,0,0,low,high);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->YAxisVisibilityOff();
        axis->SetZLabel(GetChemicalName(iChemZ).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
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

struct TNeighbor { vtkIdType iNeighbor; float weight; };

void add_if_new(vector<TNeighbor>& neighbors,TNeighbor neighbor)
{
    for(vector<TNeighbor>::const_iterator it=neighbors.begin();it!=neighbors.end();it++)
        if(it->iNeighbor==neighbor.iNeighbor)
            return;
    neighbors.push_back(neighbor);
}

bool IsEdgeNeighbor(vtkUnstructuredGrid *grid,vtkIdType iCell1,vtkIdType iCell2)
{
    vtkSmartPointer<vtkIdList> cellIds = vtkSmartPointer<vtkIdList>::New();
    vtkCell* pCell = grid->GetCell(iCell1);
    for(int iEdge=0;iEdge<pCell->GetNumberOfEdges();iEdge++)
    {
        vtkIdList *vertIds = pCell->GetEdge(iEdge)->GetPointIds();
        grid->GetCellNeighbors(iCell1,vertIds,cellIds);
        if(cellIds->IsId(iCell2)>=0)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------

void MeshRD::ComputeCellNeighbors(TNeighborhood neighborhood_type,int range,TWeight weight_type)
{
    // TODO: for now we treat LAPLACIAN weights the same as EQUAL weights, not sure what to do with this on arbitrary meshes
    if(range!=1)
        throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported range");
    if(!this->mesh->IsHomogeneous())
        throw runtime_error("MeshRD::ComputeCellNeighbors : mixed cell types not supported");

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
                vtkSmartPointer<vtkIdList> vertIds = vtkSmartPointer<vtkIdList>::New();
                vertIds->SetNumberOfIds(1);
                // first try to add neighbors that are also edge-neighbors of the previously added cell
                size_t n_previously;
                do {
                    n_previously = neighbors.size();
                    for(vtkIdType iPt=0;iPt<npts;iPt++)
                    {
                        vertIds->SetId(0,ptIds->GetId(iPt));
                        this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                        for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                        {
                            nbor.iNeighbor = cellIds->GetId(iNeighbor);
                            switch(weight_type)
                            {
                                case EQUAL: nbor.weight = 1.0f; break;
                                case LAPLACIAN: nbor.weight = 1.0f; break;
                                default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported weight type");
                            }
                            if(neighbors.empty() || IsEdgeNeighbor(this->mesh,neighbors.back().iNeighbor,nbor.iNeighbor))
                                add_if_new(neighbors,nbor);
                        }
                    }
                } while(neighbors.size() > n_previously);
                // add any remaining neighbors (in case mesh is non-manifold)
                for(vtkIdType iPt=0;iPt<npts;iPt++)
                {
                    vertIds->SetId(0,ptIds->GetId(iPt));
                    this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                    for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                    {
                        nbor.iNeighbor = cellIds->GetId(iNeighbor);
                        switch(weight_type)
                        {
                            case EQUAL: nbor.weight = 1.0f; break;
                            case LAPLACIAN: nbor.weight = 1.0f; break;
                            default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported weight type");
                        }
                        add_if_new(neighbors,nbor);
                    }
                }
            }
            break;
            case EDGE_NEIGHBORS: // neighbors share an edge
            {
                vtkCell* pCell = this->mesh->GetCell(iCell);
                for(int iEdge=0;iEdge<pCell->GetNumberOfEdges();iEdge++)
                {
                    vtkIdList *vertIds = pCell->GetEdge(iEdge)->GetPointIds();
                    this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                    for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                    {
                        nbor.iNeighbor = cellIds->GetId(iNeighbor);
                        switch(weight_type)
                        {
                            case EQUAL: nbor.weight = 1.0f; break;
                            case LAPLACIAN: nbor.weight = 1.0f; break;
                            default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported weight type");
                        }
                        add_if_new(neighbors,nbor);
                    }
                }
            }        
            break;
            case FACE_NEIGHBORS:
            {
                vtkCell* pCell = this->mesh->GetCell(iCell);
                for(int iEdge=0;iEdge<pCell->GetNumberOfFaces();iEdge++)
                {
                    vtkIdList *vertIds = pCell->GetFace(iEdge)->GetPointIds();
                    this->mesh->GetCellNeighbors(iCell,vertIds,cellIds);
                    for(vtkIdType iNeighbor=0;iNeighbor<cellIds->GetNumberOfIds();iNeighbor++)
                    {
                        nbor.iNeighbor = cellIds->GetId(iNeighbor);
                        switch(weight_type)
                        {
                            case EQUAL: nbor.weight = 1.0f; break;
                            case LAPLACIAN: nbor.weight = 1.0f; break;
                            default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported weight type");
                        }
                        add_if_new(neighbors,nbor);
                    }
                }
            }
            break;
            default: throw runtime_error("MeshRD::ComputeCellNeighbors : unsupported neighborhood type");
        }
        // normalize the weights for this cell
        float weight_sum=0.0f;
        for(int iN=0;iN<(int)neighbors.size();iN++)
            weight_sum += neighbors[iN].weight;
        weight_sum = max(weight_sum,1e-5f); // avoid div0
        for(int iN=0;iN<(int)neighbors.size();iN++)
            neighbors[iN].weight /= weight_sum;
        // store this list of neighbors
        cell_neighbors.push_back(neighbors);
        if((int)neighbors.size()>this->max_neighbors)
            this->max_neighbors = (int)neighbors.size();
        this->max_neighbors = max(1,this->max_neighbors); // avoid error in case of unconnected cells or single cell
    }

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
            this->cell_neighbor_weights[k] = cell_neighbors[i][j].weight;
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
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    string activeChemical = render_settings.GetProperty("active_chemical").GetChemical();
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();

    // 2D meshes will get returned unchanged, meshes with 3D cells will have their contour returned
    if(this->mesh->GetCellType(0)==VTK_POLYGON)
    {
        vtkSmartPointer<vtkDataSetSurfaceFilter> geom = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        #if VTK_MAJOR_VERSION >= 6
            geom->SetInputData(this->mesh);
        #else
            geom->SetInput(this->mesh);
        #endif
        geom->Update();
        out->DeepCopy(geom->GetOutput());
    }
    else if(use_image_interpolation)
    {
        vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
        #if VTK_MAJOR_VERSION >= 6
            assign_attribute->SetInputData(this->mesh);
        #else
            assign_attribute->SetInput(this->mesh);
        #endif
        assign_attribute->Assign(activeChemical.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
        vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New();
        to_point_data->SetInputConnection(assign_attribute->GetOutputPort());
        vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
        surface->SetInputConnection(to_point_data->GetOutputPort());
        surface->SetValue(0,contour_level);
        surface->Update();
        out->DeepCopy(surface->GetOutput());
    }
    else
    {
        vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
        #if VTK_MAJOR_VERSION >= 6
            assign_attribute->SetInputData(this->mesh);
        #else
            assign_attribute->SetInput(this->mesh);
        #endif
        assign_attribute->Assign(activeChemical.c_str(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        threshold->SetInputConnection(assign_attribute->GetOutputPort());
        threshold->ThresholdByUpper(contour_level);
        vtkSmartPointer<vtkDataSetSurfaceFilter> geom = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
        geom->SetInputConnection(threshold->GetOutputPort());
        geom->Update();
        out->DeepCopy(geom->GetOutput());
    }
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
    throw runtime_error("MeshRD::GetAs2DImage() : no 2D image available");
}

// ---------------------------------------------------------------------

float MeshRD::GetValue(float x,float y,float z,const Properties& render_settings)
{
    this->CreateCellLocatorIfNeeded();

    double p[3]={x,y,z},cp[3],dist2;
    vtkIdType iCell;
    int subId;
    this->cell_locator->FindClosestPoint(p,cp,iCell,subId,dist2);
    
    if(iCell<0)
        return 0.0f;

    int iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    return vtkFloatArray::SafeDownCast(this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str()))->GetValue(iCell);
}

// --------------------------------------------------------------------------------

void MeshRD::SetValue(float x,float y,float z,float val,const Properties& render_settings)
{
    this->CreateCellLocatorIfNeeded();

    double p[3]={x,y,z},cp[3],dist2;
    vtkIdType iCell;
    int subId;
    this->cell_locator->FindClosestPoint(p,cp,iCell,subId,dist2);
    
    if(iCell<0)
        return;

    int iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    float *pCell = vtkFloatArray::SafeDownCast(this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str()))->GetPointer(iCell);
    this->StorePaintAction(iChemical,iCell,*pCell);
    *pCell = val;
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

    double p[3] = {x,y,z};

    int iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    for(vtkIdType i=0;i<cells->GetNumberOfIds();i++)
    {
        int iCell = cells->GetId(i);
        vtkIdType npts,*pts;
        this->mesh->GetCellPoints(iCell,npts,pts);
        // set this cell if any of its points are inside
        for(vtkIdType iPt=0;iPt<npts;iPt++)
        {
            if(vtkMath::Distance2BetweenPoints(this->mesh->GetPoint(pts[iPt]),p)<r*r)
            {
                float *pCell = vtkFloatArray::SafeDownCast(this->mesh->GetCellData()->GetArray(GetChemicalName(iChemical).c_str()))->GetPointer(iCell);
                this->StorePaintAction(iChemical,iCell,*pCell);
                *pCell = val;
                break;
            }
        }
    }
    this->mesh->Modified();
}

// --------------------------------------------------------------------------------

void MeshRD::GetFromOpenCLBuffers( float* dest, int chemical_id )
{
	//do nothing for now, TODO, provide an implementation.
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

void MeshRD::FlipPaintAction(PaintAction& cca)
{
    float *pCell = vtkFloatArray::SafeDownCast(this->mesh->GetCellData()->GetArray(GetChemicalName(cca.iChemical).c_str()))->GetPointer(cca.iCell);
    float old_val = *pCell;
    *pCell = cca.val;
    cca.val = old_val;
    cca.done = !cca.done;
    this->mesh->Modified();
}

// --------------------------------------------------------------------------------

void MeshRD::GetMesh(vtkUnstructuredGrid* mesh) const
{
    mesh->DeepCopy(this->mesh);
}

// --------------------------------------------------------------------------------
