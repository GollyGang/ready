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
#include <vtkDelaunay3D.h>
#include <vtkCellArray.h>

// STL:
#include <vector>
using namespace std;

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

void MeshGenerators::GetTetrahedralMesh(int n_points,vtkUnstructuredGrid *mesh,int n_chems)
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

void MeshGenerators::GetTorus(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
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

void MeshGenerators::GetTriangularMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
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

void MeshGenerators::GetRhombilleTiling(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems)
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

void MeshGenerators::GetPenroseTiling(int n_subdivisions,int type,vtkUnstructuredGrid* mesh,int n_chems)
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
