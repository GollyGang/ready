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
#include "GrayScottMeshRD.hpp"
#include "utils.hpp"

// VTK:
#include <vtkFloatArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellData.h>

// ---------------------------------------------------------------------

GrayScottMeshRD::GrayScottMeshRD()
{
    this->rule_name = "Gray-Scott";
    this->n_chemicals = 2;
}

// ---------------------------------------------------------------------

void GrayScottMeshRD::InternalUpdate(int n_steps)
{
    // TODO for now, a hard-coded heat equation
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
