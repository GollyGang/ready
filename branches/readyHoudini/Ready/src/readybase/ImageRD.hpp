/*  Copyright 2011, 2012, 2013 The Ready Bunch

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
class vtkAssignAttribute;
class vtkRearrangeFields;

/// Base class for image-based systems.
class ImageRD : public AbstractRD
{
    public:

        ImageRD();
        virtual ~ImageRD();

        virtual void SaveFile(const char* filename,const Properties& render_settings,
            bool generate_initial_pattern_when_loading) const;

        virtual void Update(int n_steps);

        virtual bool HasEditableDimensions() const { return true; }
        virtual float GetX() const;
        virtual float GetY() const;
        virtual float GetZ() const;
        virtual void SetDimensions(int x,int y,int z);
        virtual void SetDimensionsAndNumberOfChemicals(int x,int y,int z,int nc);
        
        virtual int GetNumberOfCells() const;

        virtual void SetNumberOfChemicals(int n);

        virtual void GenerateInitialPattern();
        virtual void BlankImage();
        void GetImage(vtkImageData* im) const;
        virtual void CopyFromImage(vtkImageData* im);
        virtual void SaveStartingPattern();
        virtual void RestoreStartingPattern();

        virtual void InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings);

        virtual std::string GetFileExtension() const { return ImageRD::GetFileExtensionStatic(); }
        static std::string GetFileExtensionStatic() { return "vti"; }

        virtual void GetAsMesh(vtkPolyData *out,const Properties& render_settings) const;
        virtual void GetAs2DImage(vtkImageData *out,const Properties& render_settings) const;
        virtual bool Is2DImageAvailable() const { return true; }

        virtual float GetValue(float x,float y,float z,const Properties& render_settings);
        virtual void SetValue(float x,float y,float z,float val,const Properties& render_settings);
        virtual void SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings);
		virtual void GetFromOpenCLBuffers( float* dest, int chemical_id );
		
    protected:

        std::vector<vtkImageData*> images; ///< one for each chemical

        // we save the starting pattern, to allow the user to reset
        vtkImageData *starting_pattern;

        double xgap,ygap;           /// spatial separation for rendering multiple chemicals
        double image_top1D;        /// topmost location of the 1D image strips
        double image_ratio1D;     /// proportions of the 1D image strips

    protected:

        vtkImageData* GetImage(int iChemical) const;

        /// use to change the dimensions or the number of chemicals
        virtual void AllocateImages(int x,int y,int z,int nc);

        void DeallocateImages();

        static vtkImageData* AllocateVTKImage(int x,int y,int z);

        virtual int GetArenaDimensionality() const;

        virtual void FlipPaintAction(PaintAction& cca);

        // some saved handles into the pipeline, for manual updated to workaround a named arrays problem
        vtkAssignAttribute *assign_attribute_filter;
        vtkRearrangeFields *rearrange_fields_filter;

    private:

        void InitializeVTKPipeline_1D(vtkRenderer* pRenderer,const Properties& render_settings);
        void InitializeVTKPipeline_2D(vtkRenderer* pRenderer,const Properties& render_settings);
        void InitializeVTKPipeline_3D(vtkRenderer* pRenderer,const Properties& render_settings);
        void AddPhasePlot(vtkRenderer* pRenderer,float scaling,float low,float high,float posX,float posY,float posZ,
                            int iChemX,int iChemY,int iChemZ);

    private: // deliberately not implemented, to prevent use

        ImageRD(ImageRD&);
        ImageRD& operator=(ImageRD&);
};

#endif
