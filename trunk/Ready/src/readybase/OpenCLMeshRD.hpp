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
#include "MeshRD.hpp"
#include "OpenCL_MixIn.hpp"

/// Base class for mesh implementations that use OpenCL.
class OpenCLMeshRD : public MeshRD, public OpenCL_MixIn
{
    public:

        virtual std::string GetRuleType() const { return "formula"; }

        virtual bool HasEditableFormula() const { return true; }

        virtual void TestFormula(std::string s);

        // we override the parameter access functions because changing the parameters requires rewriting the kernel
        virtual void AddParameter(const std::string& name,float val);
        virtual void DeleteParameter(int iParam);
        virtual void DeleteAllParameters();
        virtual void SetParameterName(int iParam,const std::string& s);
        virtual void SetParameterValue(int iParam,float val);
};
