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
#include "MeshGenerators.hpp"
#include "utils.hpp"

// VTK:
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPlatonicSolidSource.h>
#include <vtkLinearSubdivisionFilter.h>
#include <vtkMath.h>
#include <vtkFloatArray.h>
#include <vtkPointSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkCellData.h>
#include <vtkDelaunay2D.h>
#include <vtkDelaunay3D.h>
#include <vtkCellArray.h>
#include <vtkTriangle.h>
#include <vtkGenericCell.h>

// STL:
#include <vector>
using namespace std;

// stdlib:
#include <math.h>

// ---------------------------------------------------------------------

void MeshGenerators::GetGeodesicSphere(int n_subdivisions,vtkUnstructuredGrid *mesh,int n_chems)
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
    const float scale = 100.0f; // we make the sphere larger to make <pixel> access more useful
    vtkFloatingPointType p[3];
    for(int i=0;i<mesh->GetNumberOfPoints();i++)
    {
        mesh->GetPoint(i,p);
        vtkMath::Normalize(p);
        mesh->GetPoints()->SetPoint(i,p[0]*scale,p[1]*scale,p[2]*scale);
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

void MeshGenerators::GetTetrahedralMesh(int n_points,vtkUnstructuredGrid *mesh,int n_chems)
{
    // TODO: we could make any number of shapes here but we need a more general mechanism, 
    // e.g. input a closed surface, scatter points inside, tetrahedralize

    // make a tetrahedral mesh by delaunay tetrahedralization on a point cloud
    vtkSmartPointer<vtkPointSource> pts = vtkSmartPointer<vtkPointSource>::New();
    pts->SetNumberOfPoints(n_points);
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(200,100,100); // just to make it a bit more interesting we stretch the points in one direction
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

void MeshGenerators::GetTorus(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    float radius1 = nx; // we scale the torus to give <pixel> access a better chance of being useful
    float radius2 = radius1 * 1.5f; // could allow user to change the proportions
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

void MeshGenerators::GetTriangularMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    const double scale = 2.0;
    const double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y * scale;
        for(int x=0;x<nx;x++)
        {
            p[0] = ( ((y%2)?0.5:0) + x) * scale;
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

void MeshGenerators::GetRhombilleTiling(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    const double scale = 2.0;
    const double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y * scale;
        for(int x=0;x<nx;x++)
        {
            p[0] = (((y%2)?0.5:0) + x) * scale;
            pts->InsertNextPoint(p);
            if(y%2 && x%3==2 && y<ny-1)
            {
                cells->InsertNextCell(4);
                cells->InsertCellPoint(y*nx+x);       // a
                cells->InsertCellPoint((y-1)*nx+x);   // b
                cells->InsertCellPoint((y-1)*nx+x-1); // c
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint((y-1)*nx+x-1); // c
                cells->InsertCellPoint(y*nx+x-2);     // d
                cells->InsertCellPoint((y+1)*nx+x-1); // e
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint((y+1)*nx+x-1); // e 
                cells->InsertCellPoint((y+1)*nx+x);   // f 
                cells->InsertCellPoint(y*nx+x);       // a
                cells->InsertCellPoint(y*nx+x-1);     // center
            }
            else if(y%2==0 && x%3==1 && y>0 && y<ny-1 && x>1)
            {
                cells->InsertNextCell(4);
                cells->InsertCellPoint(y*nx+x);       // a
                cells->InsertCellPoint((y-1)*nx+x-1); // b
                cells->InsertCellPoint((y-1)*nx+x-2); // c
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint((y-1)*nx+x-2); // c
                cells->InsertCellPoint(y*nx+x-2);     // d
                cells->InsertCellPoint((y+1)*nx+x-2); // e
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertNextCell(4);
                cells->InsertCellPoint(y*nx+x-1);     // center
                cells->InsertCellPoint((y+1)*nx+x-2); // e
                cells->InsertCellPoint((y+1)*nx+x-1); // f
                cells->InsertCellPoint(y*nx+x);       // a
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

void MeshGenerators::GetHexagonalMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
{
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    const double scale = 2.0;
    const double th = sqrt(3.0)/2.0; // height of an equilateral triangle with edge length 1

    double p[3]={0,0,0};
    for(int y=0;y<ny;y++)
    {
        p[1] = th*y * scale;
        for(int x=0;x<nx;x++)
        {
            p[0] = (((y%2)?0.5:0) + x) * scale;
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

void MeshGenerators::GetPenroseTiling(int n_subdivisions,int type,vtkUnstructuredGrid* mesh,int n_chems)
{
    // Many thanks to Jeff Preshing: http://preshing.com/20110831/penrose-tiling-explained

    const int RHOMBI = 0;
    const int DARTS_AND_KITES = 1;

    // we keep a list of the 'red' and 'blue' Robinson triangles and use 'deflation' (decomposition)
    vector<Tri> red_tris[2],blue_tris[2]; // each list has two buffers
    int iCurrentBuffer = 0;

    double goldenRatio = (1.0 + sqrt(5.0)) / 2.0;

    // start with 10 red triangles in a wheel, to get a nice circular shape (with 5-fold rotational symmetry)
    // (any correctly-tiled starting pattern will work too)
    double angle_step = 2.0 * 3.1415926535 / 10.0;
    const double scale = pow( goldenRatio, n_subdivisions );
    for(int i=0;i<10;i++)
    {
        double angle1 = angle_step * (i + i%2);
        double angle2 = angle_step * (i + 1 - i%2);
        switch(type) {
            default:
            case RHOMBI: red_tris[iCurrentBuffer].push_back(Tri(0,0,scale*cos(angle1),scale*sin(angle1),
                             scale*cos(angle2),scale*sin(angle2))); break;
            case DARTS_AND_KITES: red_tris[iCurrentBuffer].push_back(Tri(scale*cos(angle1),scale*sin(angle1),0,0,
                                      scale*cos(angle2),scale*sin(angle2))); break;
        }
    }

    // subdivide
    double px,py,qx,qy,rx,ry;
    for(int i=0;i<n_subdivisions;i++)
    {
        int iTargetBuffer = 1-iCurrentBuffer;
        red_tris[iTargetBuffer].clear();
        blue_tris[iTargetBuffer].clear();
        // subdivide the red triangles
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
        // subdivide the blue triangles
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

void MeshGenerators::GetRandomDelaunay2D(int n_points,vtkUnstructuredGrid *mesh,int n_chems)
{
    // make a 2D mesh by delaunay triangulation on a point cloud
    float side = sqrt((float)n_points); // spread enough for <pixel> access
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(n_points);
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    for(vtkIdType i=0;i<(vtkIdType)n_points;i++)
    {
        pts->SetPoint(i,vtkMath::Random()*side,vtkMath::Random()*side,0);
        cells->InsertNextCell(1);
        cells->InsertCellPoint(i);
    }
    vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints(pts);
    poly->SetPolys(cells);
    vtkSmartPointer<vtkDelaunay2D> del = vtkSmartPointer<vtkDelaunay2D>::New();
    del->SetInput(poly);
    del->Update();
    mesh->SetPoints(del->GetOutput()->GetPoints());
    mesh->SetCells(VTK_POLYGON,del->GetOutput()->GetPolys());

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

void MeshGenerators::GetRandomVoronoi2D(int n_points,vtkUnstructuredGrid *mesh,int n_chems)
{
    // make a 2D mesh of voronoi cells from a point cloud

    vtkSmartPointer<vtkPolyData> old_poly = vtkSmartPointer<vtkPolyData>::New();
    double side = sqrt((double)n_points); // spread enough for <pixel> access
    // first make a delaunay triangular mesh
    {
        vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
        pts->SetNumberOfPoints(n_points);
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        for(vtkIdType i=0;i<(vtkIdType)n_points;i++)
        {
            pts->SetPoint(i,vtkMath::Random()*side,vtkMath::Random()*side,0);
            cells->InsertNextCell(1);
            cells->InsertCellPoint(i);
        }
        old_poly->SetPoints(pts);
        old_poly->SetPolys(cells);
        vtkSmartPointer<vtkDelaunay2D> del = vtkSmartPointer<vtkDelaunay2D>::New();
        del->SetInput(old_poly);
        del->Update();
        old_poly->DeepCopy(del->GetOutput());
        old_poly->BuildLinks();
    }

    // then make polygons from the circumcenters of the neighboring triangles of each vertex

    // points: the circumcenter of each tri
    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(old_poly->GetNumberOfCells());
    double center[3]={0,0,0},p1[3],p2[3],p3[3];
    vtkSmartPointer<vtkGenericCell> cell = vtkSmartPointer<vtkGenericCell>::New();
    for(vtkIdType i=0;i<old_poly->GetNumberOfCells();i++)
    {
        old_poly->GetCell(i,cell);
        vtkIdType pt1 = cell->GetPointId(0);
        vtkIdType pt2 = cell->GetPointId(1);
        vtkIdType pt3 = cell->GetPointId(2);
        old_poly->GetPoint(pt1,p1);
        old_poly->GetPoint(pt2,p2);
        old_poly->GetPoint(pt3,p3);
        vtkTriangle::Circumcircle(p1,p2,p3,center);
        pts->SetPoint(i,center);
    }
    // polys: join the circumcenters of each neighboring tri of each point (if >2)
    vtkSmartPointer<vtkCellArray> new_cells = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkIdList> cell_ids = vtkSmartPointer<vtkIdList>::New();
    for(vtkIdType i=0;i<old_poly->GetNumberOfPoints();i++)
    {
        old_poly->GetPointCells(i,cell_ids);
        if(cell_ids->GetNumberOfIds()<=2) continue;
        // collect the points
        vector<vtkIdType> pt_ids;
        {
            const int N_POINTS = cell_ids->GetNumberOfIds();
            int iCurrentCell = 0;
            vector<bool> seen(N_POINTS,false);
            pt_ids.push_back(cell_ids->GetId(iCurrentCell));
            seen[0]=true;
            vtkSmartPointer<vtkIdList> cell_pts = vtkSmartPointer<vtkIdList>::New();
            vtkSmartPointer<vtkIdList> cell_pts2 = vtkSmartPointer<vtkIdList>::New();
            for(vtkIdType j=1;j<N_POINTS;j++)
            {
                // find a cell in the list that is a neighbor of iCurrentCell and not yet seen
                for(vtkIdType k=0;k<N_POINTS;k++)
                {
                    if(seen[k]) continue;
                    old_poly->GetCellPoints(cell_ids->GetId(iCurrentCell),cell_pts);
                    old_poly->GetCellPoints(cell_ids->GetId(k),cell_pts2);
                    cell_pts->IntersectWith(cell_pts2);
                    if(cell_pts->GetNumberOfIds()==2) // (input mesh is only triangles)
                    {
                        pt_ids.push_back(cell_ids->GetId(k));
                        seen[k] = true;
                        iCurrentCell = k;
                        break;
                    }
                }
            }
        }
        // check if all the points are within the original area (don't want the external stretched ones)
        bool is_ok = true;
        for(vector<vtkIdType>::const_iterator it=pt_ids.begin();it!=pt_ids.end();it++)
        {
            double p[3];
            pts->GetPoint(*it,p);
            if(p[0]<0 || p[0]>side || p[1]<0 || p[1]>side)
            {
                is_ok = false;
                break;
            }
        }
        if(!is_ok) continue;
        // add the cell to the mesh
        new_cells->InsertNextCell((vtkIdType)pt_ids.size(),&pt_ids[0]);
    }

    mesh->SetPoints(pts);
    mesh->SetCells(VTK_POLYGON,new_cells);

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
