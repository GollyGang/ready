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

#ifndef __BASERD__
#define __BASERD__

// local:
class Overlay;

// STL:
#include <string>
#include <vector>

// VTK:
#include <vtkSmartPointer.h>
class vtkImageData;
class vtkXMLDataElement;

// abstract base classes for all reaction-diffusion systems
class BaseRD 
{
    public:

        BaseRD();
        virtual ~BaseRD();

        virtual void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        // e.g. 2 for 2D systems, 3 for 3D
        int GetDimensionality() const;

        int GetX() const;
        int GetY() const;
        int GetZ() const;

        // inbuilt implementations cannot have their number_of_chemicals edited
        virtual bool HasEditableNumberOfChemicals() const { return true; }
        int GetNumberOfChemicals() const { return this->n_chemicals; }

        // advance the RD system by n timesteps
        virtual void Update(int n_steps)=0;

        // how many timesteps have we advanced since being initialized?
        int GetTimestepsTaken() const;
        void SetTimestepsTaken(int t) { this->timesteps_taken=t; }

        vtkSmartPointer<vtkImageData> GetImage() const;
        vtkImageData* GetImage(int iChemical) const;
        virtual void CopyFromImage(vtkImageData* im);

        // inbuilt implementations cannot have their formula edited
        virtual bool HasEditableFormula() const =0;
        std::string GetFormula() const;
        virtual void TestFormula(std::string program_string) {}
        virtual void SetFormula(std::string s);

        // only some implementations (OpenCL_FullKernel) can have their block size edited
        virtual bool HasEditableBlockSize() const { return false; }
        virtual int GetBlockSizeX() const { return 1; } // e.g. block size may be 4x1x1 for kernels that use float4 (like OpenCL_Formula)
        virtual int GetBlockSizeY() const { return 1; }
        virtual int GetBlockSizeZ() const { return 1; }
        virtual void SetBlockSizeX(int n) {}
        virtual void SetBlockSizeY(int n) {}
        virtual void SetBlockSizeZ(int n) {}

        // use to change the dimensions or the number of chemicals
        virtual void Allocate(int x,int y,int z,int nc);

        std::string GetRuleName() const;
        void SetRuleName(std::string s);

        std::string GetDescription() const;
        void SetDescription(std::string s);

        // every implementation has parameters that can be edited and changed 
        // (will cause errors if they don't match the inbuilt names, the formula or the kernel)
        int GetNumberOfParameters() const;
        std::string GetParameterName(int iParam) const;
        float GetParameterValue(int iParam) const;
        float GetParameterValueByName(const std::string& name) const;
        bool IsParameter(const std::string& name) const;
        virtual void AddParameter(const std::string& name,float val);
        virtual void DeleteParameter(int iParam);
        virtual void DeleteAllParameters();
        virtual void SetParameterName(int iParam,const std::string& s);
        virtual void SetParameterValue(int iParam,float val);

        // should the user be asked if they want to save?
        bool IsModified() const;
        void SetModified(bool m);

        std::string GetFilename() const;
        void SetFilename(const std::string& s);

        void AddInitialPatternGeneratorOverlay(Overlay* overlay);
        virtual void GenerateInitialPattern();
        virtual void BlankImage();
        void ClearInitialPatternGenerator();
        int GetNumberOfInitialPatternGeneratorOverlays() const { return (int)this->initial_pattern_generator.size(); }
        Overlay* GetInitialPatternGeneratorOverlay(int i) const { return this->initial_pattern_generator[i]; }

    protected:

        std::string rule_name, description;

        int n_chemicals; // (could use images.size() but TestFormula can be called before Allocate)

        std::vector<vtkImageData*> images; // one for each chemical

        std::vector<Overlay*> initial_pattern_generator;

        std::vector<std::pair<std::string,float> > parameters;

        int timesteps_taken;

        std::string formula;
        bool need_reload_formula;

        std::string filename;
        bool is_modified;

    protected:

        void Deallocate();

        static vtkImageData* AllocateVTKImage(int x,int y,int z);

    private: // deliberately not implemented, to prevent use

        BaseRD(BaseRD&);
        BaseRD& operator=(BaseRD&);
};

#endif
