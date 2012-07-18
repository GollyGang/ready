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

#ifndef __MESHRD__
#define __MESHRD__

// local:
#include "AbstractRD.hpp"

// VTK:
#include <vtkType.h>
class vtkUnstructuredGrid;

/// Base class for mesh-based systems.
class MeshRD : public AbstractRD
{
    public:

        MeshRD();
        virtual ~MeshRD();

        virtual void Update(int n_steps);

        virtual float GetX() const;
        virtual float GetY() const;
        virtual float GetZ() const;

        virtual void SetNumberOfChemicals(int n);

        virtual bool HasEditableFormula() const { return true; }

        virtual std::string GetRuleType() const { return "formula"; } // TODO

        virtual std::string GetFileExtension() const { return MeshRD::GetFileExtensionStatic(); }
        static std::string GetFileExtensionStatic() { return "vtu"; }
        
        virtual int GetNumberOfCells() const;

        virtual void SaveFile(const char* filename,const Properties& render_settings) const;

        virtual void GenerateInitialPattern();
        virtual void BlankImage();
        virtual void CopyFromMesh(vtkUnstructuredGrid* mesh2);
        virtual void SaveStartingPattern();
        virtual void RestoreStartingPattern();

        virtual void InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings);

        virtual void GetAsMesh(vtkPolyData *out,const Properties& render_settings) const;
        virtual void GetAs2DImage(vtkImageData *out,const Properties& render_settings) const;

        virtual int GetArenaDimensionality() const;

        virtual float GetValue(int iChemical,int cellID) const;
        virtual void SetValue(int iChemical,int cellID,float val);

    protected:

        /// work out which cells are neighbors of each other
        void ComputeCellNeighbors();

        /// advance the RD system by n timesteps
        virtual void InternalUpdate(int n_steps) =0;

    protected:

        vtkUnstructuredGrid* mesh;             ///< the cell data contains a named array for each chemical ('a', 'b', etc.)
        vtkUnstructuredGrid* starting_pattern; ///< we save the starting pattern, to allow the user to reset

        int max_neighbors;
        int *cell_neighbor_indices;   ///< index of each neighbor of a cell
        float *cell_neighbor_weights; ///< diffusion coefficient between each cell and a neighbor

    private: // deliberately not implemented, to prevent use

        MeshRD(MeshRD&);
        MeshRD& operator=(MeshRD&);
};

#endif
