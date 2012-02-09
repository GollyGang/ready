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
#include "OpenCL_RD.hpp"

// n-dimensional (1D,2D,3D) OpenCL RD implementations with n chemicals, specified as
// a short formula involving delta_a, laplacian_a, etc. implemented with Euler integration,
// a basic finite difference stencil and float4 blocks for speed
class OpenCL_Formula : public OpenCL_RD
{
    public:

        OpenCL_Formula();

        virtual void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        virtual int GetBlockSizeX() const { return 4; } // we use float4 in a 4x1x1 block

        virtual std::string AssembleKernelSourceFromFormula(std::string formula) const;
};
