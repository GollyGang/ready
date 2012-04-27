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
#include "OpenCLMeshRD.hpp"

/// An RD system that uses an OpenCL formula snippet.
class FormulaOpenCLMeshRD : public OpenCLMeshRD
{
    public:

        FormulaOpenCLMeshRD();

        virtual void InitializeFromXML(vtkXMLDataElement* rd,bool& warn_to_update);
        virtual vtkSmartPointer<vtkXMLDataElement> GetAsXML() const;

        virtual std::string GetRuleType() const { return "formula"; }

        virtual int GetBlockSizeX() const { return 4; } // we use float4 in a 4x1x1 block

        virtual std::string AssembleKernelSourceFromFormula(std::string formula) const;

        // we override the parameter access functions because changing the parameters requires rewriting the kernel
        virtual void AddParameter(const std::string& name,float val);
        virtual void DeleteParameter(int iParam);
        virtual void DeleteAllParameters();
        virtual void SetParameterName(int iParam,const std::string& s);
        virtual void SetParameterValue(int iParam,float val);
};
