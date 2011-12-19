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
#include "BaseRD.hpp"

// a basic RD system, provided as a placeholder
// N.B. Work in progress. Most implementations will use a generic 'algorithm' instead (e.g. 'RD_OpenCL_2D')
class GrayScott_slow : public BaseRD
{
    public:

        GrayScott_slow();

        void Allocate(int x,int y);
        float GetF() { return this->f; }
        float GetK() { return this->k; }
        void SetF(float new_f) { this->f = new_f; }
        void SetK(float new_k) { this->k = new_k; }
        void InitWithBlobInCenter();

        void Update(int n_steps);

        bool HasEditableProgram() const { return false; }

    protected:

        float f,k,r_a,r_b;
};
