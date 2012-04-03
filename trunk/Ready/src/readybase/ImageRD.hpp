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

#ifndef __IMAGERD__
#define __IMAGERD__

// local:
#include "AbstractRD.hpp"

// VTK:
class vtkImageData;
class vtkImageWrapPad;

/// abstract base class for image-based reaction-diffusion systems
class ImageRD : public AbstractRD
{
    public:

        ImageRD();
        virtual ~ImageRD();

        virtual void Update(int n_steps);

        virtual void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        // e.g. 2 for 2D systems, 3 for 3D
        int GetDimensionality() const;

        int GetX() const;
        int GetY() const;
        int GetZ() const;

        vtkSmartPointer<vtkImageData> GetImage() const;
        vtkImageData* GetImage(int iChemical) const;
        virtual void CopyFromImage(vtkImageData* im);

        // only some implementations (OpenCL_FullKernel) can have their block size edited
        virtual bool HasEditableBlockSize() const { return false; }
        virtual int GetBlockSizeX() const { return 1; } // e.g. block size may be 4x1x1 for kernels that use float4 (like OpenCL_Formula)
        virtual int GetBlockSizeY() const { return 1; }
        virtual int GetBlockSizeZ() const { return 1; }
        virtual void SetBlockSizeX(int n) {}
        virtual void SetBlockSizeY(int n) {}
        virtual void SetBlockSizeZ(int n) {}

        virtual void GenerateInitialPattern();
        virtual void BlankImage();

        // use to change the dimensions or the number of chemicals
        virtual void AllocateImages(int x,int y,int z,int nc);

        // kludgy workaround for the GenerateCubesFromLabels approach not being fully pipelined (see vtk_pipeline.cpp)
        void SetImageWrapPadFilter(vtkImageWrapPad *p) { this->image_wrap_pad_filter = p; }

    protected:

        std::vector<vtkImageData*> images; // one for each chemical

    protected:

        void DeallocateImages();

        static vtkImageData* AllocateVTKImage(int x,int y,int z);

    private:

        // kludgy workaround for the GenerateCubesFromLabels approach not being fully pipelined (see vtk_pipeline.cpp)
        vtkImageWrapPad *image_wrap_pad_filter;
        void UpdateImageWrapPadFilter();

    private: // deliberately not implemented, to prevent use

        ImageRD(ImageRD&);
        ImageRD& operator=(ImageRD&);
};

#endif
