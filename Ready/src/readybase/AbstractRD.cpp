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
#include "AbstractRD.hpp"
#include "overlays.hpp"

// STL:
using namespace std;

void AbstractRD::ClearInitialPatternGenerator()
{
    for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
        delete this->initial_pattern_generator[iOverlay];
    this->initial_pattern_generator.clear();
}

void AbstractRD::AddInitialPatternGeneratorOverlay(Overlay* overlay)
{
    this->initial_pattern_generator.push_back(overlay);
}

int AbstractRD::GetTimestepsTaken() const
{
    return this->timesteps_taken;
}

void AbstractRD::SetFormula(string s)
{
    if(s != this->formula)
        this->need_reload_formula = true;
    this->formula = s;
    this->is_modified = true;
}

string AbstractRD::GetFormula() const
{
    return this->formula;
}

std::string AbstractRD::GetRuleName() const
{
    return this->rule_name;
}

std::string AbstractRD::GetDescription() const
{
    return this->description;
}

void AbstractRD::SetRuleName(std::string s)
{
    this->rule_name = s;
    this->is_modified = true;
}

void AbstractRD::SetDescription(std::string s)
{
    this->description = s;
    this->is_modified = true;
}

int AbstractRD::GetNumberOfParameters() const
{
    return (int)this->parameters.size();
}

std::string AbstractRD::GetParameterName(int iParam) const
{
    return this->parameters[iParam].first;
}

float AbstractRD::GetParameterValue(int iParam) const
{
    return this->parameters[iParam].second;
}

float AbstractRD::GetParameterValueByName(const std::string& name) const
{
    for(int iParam=0;iParam<(int)this->parameters.size();iParam++)
        if(this->parameters[iParam].first == name)
            return this->parameters[iParam].second;
    throw runtime_error("ImageRD::GetParameterValueByName : parameter name not found: "+name);
}

void AbstractRD::AddParameter(const std::string& name,float val)
{
    this->parameters.push_back(make_pair(name,val));
    this->is_modified = true;
}

void AbstractRD::DeleteParameter(int iParam)
{
    this->parameters.erase(this->parameters.begin()+iParam);
    this->is_modified = true;
}

void AbstractRD::DeleteAllParameters()
{
    this->parameters.clear();
    this->is_modified = true;
}

void AbstractRD::SetParameterName(int iParam,const string& s)
{
    this->parameters[iParam].first = s;
    this->is_modified = true;
}

void AbstractRD::SetParameterValue(int iParam,float val)
{
    this->parameters[iParam].second = val;
    this->is_modified = true;
}

bool AbstractRD::IsParameter(const string& name) const
{
    for(int i=0;i<(int)this->parameters.size();i++)
        if(this->parameters[i].first == name)
            return true;
    return false;
}

bool AbstractRD::IsModified() const
{
    return this->is_modified;
}

void AbstractRD::SetModified(bool m)
{
    this->is_modified = m;
}

std::string AbstractRD::GetFilename() const
{
    return this->filename;
}

void AbstractRD::SetFilename(const string& s)
{
    this->filename = s;
}
