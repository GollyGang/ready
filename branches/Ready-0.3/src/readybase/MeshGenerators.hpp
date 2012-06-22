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

// VTK:
class vtkUnstructuredGrid;

/// Methods for generating meshes from scratch.
namespace MeshGenerators 
{
    /// Subdivides an icosahedron to get a sphere covered with triangles.
    void GetGeodesicSphere(int n_subdivisions,vtkUnstructuredGrid* mesh,int n_chems);

    /// Applies the Delaunay algorith to scattered points to get a mesh of tetrahedra.
    void GetTetrahedralMesh(int n_points,vtkUnstructuredGrid* mesh,int n_chems);

    /// Subdivides a torus with quadrilaterals.
    void GetTorus(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems);

    /// Makes a planar mesh of triangles.
    void GetTriangularMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems);

    /// Makes a planar mesh of hexagons.
    void GetHexagonalMesh(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems);

    /// Makes a planar mesh using the rhombille tiling.
    void GetRhombilleTiling(int nx,int ny,vtkUnstructuredGrid* mesh,int n_chems);

    /// Make a planar Penrose tiling, using either rhombi (type=0) or darts and kites (type=1).
    void GetPenroseTiling(int n_subdivisions,int type,vtkUnstructuredGrid* mesh,int n_chems);
}
