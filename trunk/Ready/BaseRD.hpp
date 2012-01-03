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

#ifndef __BASERD__
#define __BASERD__

// STL:
#include <string>

class vtkImageData;

// abstract base classes for all reaction-diffusion systems, 2D and 3D
class BaseRD 
{
    public:

        BaseRD();
        virtual ~BaseRD();

        // e.g. 2 for 2D systems, 3 for 3D
        int GetDimensionality() const;

        int GetNumberOfChemicals() const;

        // advance the RD system by n timesteps
        virtual void Update(int n_steps)=0;

        float GetTimestep() const;

        // how many timesteps have we advanced since being initialized?
        int GetTimestepsTaken() const;

        vtkImageData* GetImage() const;

        virtual bool HasEditableProgram() const =0;
        void SetProgram(std::string s);
        std::string GetProgram() const;
        virtual void TestProgram(std::string program_string) const {}

        virtual void InitWithBlobInCenter() =0;

    protected:

        vtkImageData *image;

        float timestep;
        int timesteps_taken;

        std::string program_string;
        bool need_reload_program;

    protected:

        void AllocateImage(int x,int y,int z,int nc);

        static vtkImageData* AllocateVTKImage(int x,int y,int z,int nc);
        static float* vtk_at(float* origin,int x,int y,int z,int iC,int X,int Y,int NC);
};

#endif
