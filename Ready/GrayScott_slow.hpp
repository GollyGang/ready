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
#include "BaseRD.hpp"

// a basic RD system, provided as a placeholder
// N.B. Work in progress. Most implementations will use a generic 'algorithm' instead (e.g. 'RD_OpenCL_2D')
class GrayScott_slow : public BaseRD
{
    public:

        GrayScott_slow();
        ~GrayScott_slow();

        virtual void Allocate(int x,int y,int z,int nc);
        virtual void InitWithBlobInCenter();

        virtual void Update(int n_steps);

        virtual bool HasEditableFormula() const { return false; }

    protected:

        std::vector<vtkImageData*> buffer_images;

    protected:

        void DeleteBuffers();
};
