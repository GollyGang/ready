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
class BaseOverlay;

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

        // e.g. 2 for 2D systems, 3 for 3D
        int GetDimensionality() const;

        int GetX() const;
        int GetY() const;
        int GetZ() const;
        int GetNumberOfChemicals() const { return this->n_chemicals; }
        void SetNumberOfChemicals(int n) { this->n_chemicals = n; this->is_modified = true; }

        // advance the RD system by n timesteps
        virtual void Update(int n_steps)=0;

        float GetTimestep() const;
        virtual void SetTimestep(float t);

        // how many timesteps have we advanced since being initialized?
        int GetTimestepsTaken() const;
        void SetTimestepsTaken(int t) { this->timesteps_taken=t; }

        vtkSmartPointer<vtkImageData> GetImage() const;
        vtkImageData* GetImage(int iChemical) const;
        virtual void CopyFromImage(vtkImageData* im);

        virtual bool HasEditableFormula() const =0;
        std::string GetFormula() const;
        virtual void TestFormula(std::string program_string) {}
        virtual void SetFormula(std::string s);

        virtual void InitWithBlobInCenter() =0;

        virtual void Allocate(int x,int y,int z,int nc);

        std::string GetRuleName() const;
        std::string GetRuleDescription() const;
        std::string GetPatternDescription() const;
        void SetRuleName(std::string s);
        void SetRuleDescription(std::string s);
        void SetPatternDescription(std::string s);
        int GetNumberOfParameters() const;
        std::string GetParameterName(int iParam) const;
        float GetParameterValue(int iParam) const;
        virtual void AddParameter(std::string name,float val);
        virtual void DeleteParameter(int iParam);
        virtual void DeleteAllParameters();
        virtual void SetParameterName(int iParam,std::string s);
        virtual void SetParameterValue(int iParam,float val);

        bool IsModified() const;
        void SetModified(bool m);
        std::string GetFilename() const;
        void SetFilename(std::string s);

        std::vector<BaseOverlay*>& GetInitialPatternGenerator() { return this->initial_pattern_generator; }
        virtual void GenerateInitialPattern();

    protected:

        std::string rule_name,rule_description,pattern_description;

        int n_chemicals;

        std::vector<vtkImageData*> images; // one for each chemical

        std::vector<BaseOverlay*> initial_pattern_generator;

        std::vector<std::pair<std::string,float> > parameters;

        float timestep;
        int timesteps_taken;

        std::string formula;
        bool need_reload_formula;

        std::string filename;
        bool is_modified;

    protected:

        void Deallocate();

        static vtkImageData* AllocateVTKImage(int x,int y,int z);
        static float* vtk_at(float* origin,int x,int y,int z,int X,int Y);

    private: // deliberately not implemented, to prevent use

        BaseRD(BaseRD&);
        BaseRD& operator=(BaseRD&);
};

#endif
