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
#include "RulePanel.hpp"
#include "BaseRD.hpp"
#include "frame.hpp"
#include "IDs.hpp"

// STL:
#include <string>
#include <sstream>
using namespace std;

BEGIN_EVENT_TABLE(RulePanel, wxPanel)
    EVT_PG_CHANGED(wxID_ANY,RulePanel::OnPropertyGridChanged)
END_EVENT_TABLE()

RulePanel::RulePanel(MyFrame* parent,wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    this->pgrid = new wxPropertyGrid(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,
        wxPG_SPLITTER_AUTO_CENTER );

    sizer->Add(this->pgrid,wxSizerFlags(1).Expand());
    this->SetSizer(sizer);
}

void RulePanel::Update(const BaseRD* const system)
{
    // remake the whole property grid (we don't know what changed)
    this->pgrid->Freeze();
    this->pgrid->Clear();

    this->rule_name_property = this->pgrid->Append(new wxStringProperty( _("Rule name"),wxPG_LABEL,system->GetRuleName()));
    this->rule_description_property = this->pgrid->Append(new wxLongStringProperty( _("Rule description"),wxPG_LABEL,system->GetRuleDescription()));
    this->pattern_description_property = this->pgrid->Append(new wxLongStringProperty( _("Pattern description"),wxPG_LABEL,system->GetPatternDescription()));

    this->parameter_properties.resize(system->GetNumberOfParameters());
    for(int iParam=0;iParam<(int)system->GetNumberOfParameters();iParam++)
    {
        wxPGProperty *cat = this->pgrid->Append(new wxFloatProperty(system->GetParameterName(iParam),wxPG_LABEL,system->GetParameterValue(iParam)));
        this->parameter_properties[iParam] = cat;
        this->pgrid->AppendIn(cat,new wxFloatProperty(_("Min"),wxPG_LABEL,system->GetParameterMin(iParam)));
        this->pgrid->AppendIn(cat,new wxFloatProperty(_("Max"),wxPG_LABEL,system->GetParameterMax(iParam)));
        this->pgrid->AppendIn(cat,new wxStringProperty(_("Name"),wxPG_LABEL,system->GetParameterName(iParam)));
    }

    if(system->HasEditableFormula())
        this->pgrid->Append(new wxLongStringProperty(_("Formula"),wxPG_LABEL,system->GetFormula()));

    this->pgrid->Append(new wxFileProperty("Filename", wxPG_LABEL, wxEmptyString));

    this->pgrid->CollapseAll();
    this->pgrid->Thaw();
}

void RulePanel::OnPropertyGridChanged(wxPropertyGridEvent& event)
{
    wxPGProperty *property = event.GetProperty();
    if(!property) return;

    // was it a parameter that changed?
    for(int iParam=0;iParam<(int)this->parameter_properties.size();iParam++)
    {
        if(property != this->parameter_properties[iParam]) continue;
        this->frame->SetParameter(iParam,(wxAny(property->GetValue())).As<float>());
    }

    if(property == this->rule_name_property) 
        this->frame->SetRuleName(string((wxAny(property->GetValue())).As<wxString>().mb_str()));
    if(property == this->rule_description_property) 
        this->frame->SetRuleDescription(string((wxAny(property->GetValue())).As<wxString>().mb_str()));
    if(property == this->pattern_description_property) 
        this->frame->SetPatternDescription(string((wxAny(property->GetValue())).As<wxString>().mb_str()));
}
