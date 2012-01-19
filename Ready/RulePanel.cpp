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
    //
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
    this->pgrid->Clear();

    this->pgrid->Append(new wxStringProperty( _T("Rule name"),wxPG_LABEL,system->GetRuleName()) );
    this->pgrid->Append(new wxLongStringProperty( _T("Rule description"),wxPG_LABEL,system->GetRuleDescription()) );

    for(int iParam=0;iParam<(int)system->GetNumberOfParameters();iParam++)
    {
        wxPGProperty *cat = this->pgrid->Append(new wxFloatProperty(system->GetParameterName(iParam),wxPG_LABEL,system->GetParameterValue(iParam)));
        this->pgrid->AppendIn(cat,new wxFloatProperty(_("min"),wxPG_LABEL,system->GetParameterMin(iParam)));
        this->pgrid->AppendIn(cat,new wxFloatProperty(_("max"),wxPG_LABEL,system->GetParameterMax(iParam)));
        this->pgrid->AppendIn(cat,new wxStringProperty(_("name"),wxPG_LABEL,system->GetParameterName(iParam)));
    }

    if(system->HasEditableFormula())
        this->pgrid->Append(new wxLongStringProperty(_("formula"),wxPG_LABEL,system->GetFormula()));

    this->pgrid->CollapseAll();
}
