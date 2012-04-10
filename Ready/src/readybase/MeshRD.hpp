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
class vtkPolyData;

/// Base class for mesh-based systems.
class MeshRD : public AbstractRD
{
    public:

        MeshRD();
        virtual ~MeshRD();

        virtual void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        virtual void Update(int n_steps);

        virtual void SetNumberOfChemicals(int n);

        virtual bool HasEditableFormula() const { return true; }

        virtual std::string GetRuleType() const { return "formula"; } // TODO
        virtual std::string GetFileExtension() const { return "vtp"; }

        virtual void SaveFile(const char* filename,const Properties& render_settings) const;

        virtual void GenerateInitialPattern();
        virtual void BlankImage();
        virtual void CopyFromMesh(vtkPolyData* pd);

        virtual void InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings);
        virtual void SaveStartingPattern();
        virtual void RestoreStartingPattern();

        virtual float SampleAt(int x,int y,int z,int ic);

    protected:

        /// advance the RD system by n timesteps
        virtual void InternalUpdate(int n_steps);

    protected:

        vtkPolyData* mesh;

        /// we save the starting pattern, to allow the user to reset
        vtkPolyData *starting_pattern;

    private: // deliberately not implemented, to prevent use

        MeshRD(MeshRD&);
        MeshRD& operator=(MeshRD&);
};