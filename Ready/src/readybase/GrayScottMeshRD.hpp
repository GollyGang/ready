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

/// Base class for all the inbuilt mesh implementations.
// TODO: put in its own file (when there is more than one derived class)
class InbuiltMeshRD : public MeshRD
{
    public:

        virtual std::string GetRuleType() const { return "inbuilt"; }

        virtual bool HasEditableFormula() const { return false; }
        virtual bool HasEditableNumberOfChemicals() const { return false; }
};

/// A non-OpenCL mesh implementation, just as an example.
class GrayScottMeshRD : public InbuiltMeshRD
{
    public:

        GrayScottMeshRD();

        /// For imported meshes we need a way to get something going on the surface so the user can get started.
        void CreateGrayScottStartingConditions();

    protected:

        virtual void InternalUpdate(int n_steps);
};
