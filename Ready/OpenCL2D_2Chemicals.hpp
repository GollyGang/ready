/*  Copyright 2011, The Ready Bunch

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

// base class for 2D OpenCL RD implementations with 2 chemicals, where the user can specify the kernel program
class OpenCL2D_2Chemicals : public OpenCL_RD
{
    public:

        OpenCL2D_2Chemicals();

        void Allocate(int x,int y);
        void InitWithBlobInCenter();

        void Update(int n_steps);
};
