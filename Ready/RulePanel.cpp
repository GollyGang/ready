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
    EVT_TEXT(wxID_ANY,RulePanel::OnTextChanged)
END_EVENT_TABLE()

RulePanel::RulePanel(MyFrame* parent,wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    wxBookCtrl *tabs_ctrl = new wxBookCtrl(this,wxID_ANY);
    {
        wxPanel *panel = new wxPanel(tabs_ctrl);
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        panel->SetSizer(sizer);
        this->description_ctrl = new wxTextCtrl(panel,wxID_ANY,wxEmptyString,wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE);
        sizer->Add(this->description_ctrl,wxSizerFlags(1).Expand());
        tabs_ctrl->AddPage(panel,_("Overview"));
    }
    {
        wxPanel *panel = new wxPanel(tabs_ctrl);
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        panel->SetSizer(sizer);
        this->pgrid = new wxPropertyGrid(panel,wxID_ANY,wxDefaultPosition,wxDefaultSize);
        sizer->Add(this->pgrid,wxSizerFlags(1).Expand());
        tabs_ctrl->AddPage(panel,_("Details"));
    }

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(tabs_ctrl,wxSizerFlags(1).Expand());
    this->SetSizer(sizer);

    // install event handler to detect keyboard shortcuts when rule panel has focus
    this->pgrid->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MyFrame::OnKeyDown), NULL, frame);
    this->pgrid->Connect(wxEVT_CHAR, wxKeyEventHandler(MyFrame::OnChar), NULL, frame);
}

void RulePanel::Update(const BaseRD* const system)
{
    class LongStringProperty : public wxLongStringProperty
    {
        public:
            LongStringProperty(const wxString& label,const wxString& name,const wxString& value) 
                : wxLongStringProperty(label,name,value)
            {
                this->m_flags |= wxPG_PROP_NO_ESCAPE;
            }
    };

    // remake the whole property grid (we don't know what changed)
    this->pgrid->Freeze();
    this->pgrid->Clear();

    this->rule_name_property = this->pgrid->Append(new wxStringProperty( _("Rule name"),wxPG_LABEL,system->GetRuleName()));
    //this->rule_description_property = this->pgrid->Append(new LongStringProperty( _("Rule description"),wxPG_LABEL,system->GetRuleDescription()));
    this->timestep_property = this->pgrid->Append(new wxFloatProperty(_("Timestep"),wxPG_LABEL,system->GetTimestep()));

    this->parameter_value_properties.resize(system->GetNumberOfParameters(),NULL);
    this->parameter_name_properties.resize(system->GetNumberOfParameters(),NULL);
    for(int iParam=0;iParam<(int)system->GetNumberOfParameters();iParam++)
    {
        wxPGProperty *prop = this->pgrid->Append(new wxFloatProperty(system->GetParameterName(iParam),wxPG_LABEL,system->GetParameterValue(iParam)));
        this->parameter_value_properties[iParam] = prop;
        if(system->HasEditableFormula()) // (don't allow name editing for inbuilt rules)
            this->parameter_name_properties[iParam] = this->pgrid->AppendIn(prop,new wxStringProperty(_("Name"),wxPG_LABEL,system->GetParameterName(iParam)));
    }

    if(system->HasEditableFormula())
        this->formula_property = this->pgrid->Append(new LongStringProperty(_("Formula"),wxPG_LABEL,system->GetFormula()));
    else
        this->formula_property = NULL;

    //this->pattern_description_property = this->pgrid->Append(new LongStringProperty( _("Pattern description"),wxPG_LABEL,system->GetPatternDescription()));
    this->description_ctrl->ChangeValue(system->GetPatternDescription());

    this->dimensions_property = this->pgrid->Append(new wxStringProperty("Dimensions", wxPG_LABEL,_T("<composed>")));
    this->pgrid->AppendIn(this->dimensions_property,new wxIntProperty("X",wxPG_LABEL,system->GetX()));
    this->pgrid->AppendIn(this->dimensions_property,new wxIntProperty("Y",wxPG_LABEL,system->GetY()));
    this->pgrid->AppendIn(this->dimensions_property,new wxIntProperty("Z",wxPG_LABEL,system->GetZ()));

    this->pgrid->SetVerticalSpacing(3); // increase row height

    this->pgrid->CollapseAll();
    this->pgrid->SetCellBackgroundColour(wxColour(240,235,235));
    this->pgrid->SetLineColour(*wxWHITE);
    this->pgrid->Thaw();

    // TODO: remember collapsed/expanded state, and splitter position
}

void RulePanel::OnPropertyGridChanged(wxPropertyGridEvent& event)
{
    wxPGProperty *property = event.GetProperty();
    if(!property) return;

    // was it a parameter that changed?
    for(int iParam=0;iParam<(int)this->parameter_value_properties.size();iParam++)
    {
        if(property == this->parameter_value_properties[iParam])
            this->frame->SetParameter(iParam,(wxAny(property->GetValue())).As<float>());
        if(property == this->parameter_name_properties[iParam])
        {
            wxString new_name = (wxAny(property->GetValue())).As<wxString>();
            this->parameter_value_properties[iParam]->SetLabel(new_name);
            this->frame->SetParameterName(iParam,string(new_name.mb_str()));
        }
    }

    if(property == this->rule_name_property) 
        this->frame->SetRuleName(string((wxAny(property->GetValue())).As<wxString>().mb_str()));
    if(property == this->rule_description_property) 
        this->frame->SetRuleDescription(string((wxAny(property->GetValue())).As<wxString>().mb_str()));
    if(property == this->pattern_description_property) 
        this->frame->SetPatternDescription(string((wxAny(property->GetValue())).As<wxString>().mb_str()));
    if(property == this->timestep_property)
        this->frame->SetTimestep((wxAny(property->GetValue())).As<float>());
    if(property == this->formula_property) 
        this->frame->SetFormula(string((wxAny(property->GetValue())).As<wxString>().mb_str()));
    if(property == this->dimensions_property)
        wxMessageBox(_T("TODO!!!"));
}

bool RulePanel::GridHasFocus()
{
    return this->pgrid->HasFocus();
}

bool RulePanel::DoKey(int key, int mods)
{
    // first look for keys that should be passed to the default handler
    if ( mods == wxMOD_NONE ) {
        if ( key == WXK_UP || key == WXK_DOWN || key == WXK_LEFT || key == WXK_RIGHT ||
             key == WXK_TAB ) {
            // let default handler see tab and arrow keys (to select rows, etc)
            return false;
        }
    }
    
    // finally do other keyboard shortcuts
    frame->ProcessKey(key, mods);
    return true;
}

void RulePanel::OnTextChanged(wxCommandEvent& event)
{
    if(event.GetEventObject()==this->description_ctrl)
        this->frame->SetPatternDescription(string(this->description_ctrl->GetValue().mb_str()));
}