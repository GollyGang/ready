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

        virtual void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

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

        virtual void InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings);
        virtual void SaveStartingPattern();
        virtual void RestoreStartingPattern();

        virtual void GetAsMesh(vtkPolyData *out,const Properties& render_settings) const;

        virtual int GetArenaDimensionality() const;

    protected:

        /// work out which cells are neighbors of each other
        void ComputeCellNeighbors();

        /// advance the RD system by n timesteps
        virtual void InternalUpdate(int n_steps);

    protected:

        vtkUnstructuredGrid* mesh;             ///< the cell data contains a named array for each chemical ('a', 'b', etc.)
        vtkUnstructuredGrid* buffer;           ///< temporary storage used during computation
        vtkUnstructuredGrid* starting_pattern; ///< we save the starting pattern, to allow the user to reset

        std::vector<std::vector<vtkIdType> > cell_neighbors; ///< the connectivity between cells; for each cell, what cells are its neighbors?

    private: // deliberately not implemented, to prevent use

        MeshRD(MeshRD&);
        MeshRD& operator=(MeshRD&);
};
