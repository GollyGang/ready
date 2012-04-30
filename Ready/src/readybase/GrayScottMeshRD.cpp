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
    float timestep = this->GetParameterValueByName("timestep");
    float D_a = this->GetParameterValueByName("D_a");
    float D_b = this->GetParameterValueByName("D_b");
    float k = this->GetParameterValueByName("k");
    float F = this->GetParameterValueByName("F");

    vtkFloatArray *source_a,*source_b;
    vtkFloatArray *target_a,*target_b;
    float dda,ddb,aval,bval,da,db;
    int n_neighbors;
    TNeighbor neighbor;

    for(int iStep=0;iStep<n_steps;iStep++)
    {
        if(iStep%2)
        {
            source_a = vtkFloatArray::SafeDownCast( this->buffer->GetCellData()->GetArray(GetChemicalName(0).c_str()) );
            source_b = vtkFloatArray::SafeDownCast( this->buffer->GetCellData()->GetArray(GetChemicalName(1).c_str()) );
            target_a = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(0).c_str()) );
            target_b = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(1).c_str()) );
        }
        else
        {
            source_a = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(0).c_str()) );
            source_b = vtkFloatArray::SafeDownCast( this->mesh->GetCellData()->GetArray(GetChemicalName(1).c_str()) );
            target_a = vtkFloatArray::SafeDownCast( this->buffer->GetCellData()->GetArray(GetChemicalName(0).c_str()) );
            target_b = vtkFloatArray::SafeDownCast( this->buffer->GetCellData()->GetArray(GetChemicalName(1).c_str()) );
        }
        for(vtkIdType iCell=0;iCell<(int)this->cell_neighbors.size();iCell++)
        {
            // compute the laplacian
            aval = source_a->GetValue(iCell);
            bval = source_b->GetValue(iCell);
            dda = 0.0f;
            ddb = 0.0f;
            n_neighbors = this->cell_neighbors[iCell].size();
            for(vtkIdType iNeighbor=0;iNeighbor<n_neighbors;iNeighbor++)
            {
                neighbor = this->cell_neighbors[iCell][iNeighbor];
                dda += source_a->GetValue(neighbor.iNeighbor) * neighbor.diffusion_coefficient;
                ddb += source_b->GetValue(neighbor.iNeighbor) * neighbor.diffusion_coefficient;
            }
            dda -= aval;
            ddb -= bval;
            dda *= 4.0f; // scale the Laplacian to be more similar to the 2D square grid version, so the same parameters work
            ddb *= 4.0f;
            // Gray-Scott update step:
            da = D_a * dda - aval*bval*bval + F*(1-aval);
            db = D_b * ddb + aval*bval*bval - (F+k)*bval;
            // apply the step:
            target_a->SetValue(iCell,aval + timestep*da );
            target_b->SetValue(iCell,bval + timestep*db );
        }
    }
    if(n_steps%2)
        this->mesh->DeepCopy(this->buffer);
}

// ---------------------------------------------------------------------
