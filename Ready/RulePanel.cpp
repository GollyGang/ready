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
    // controls
    EVT_BUTTON(ID::SetRuleName,RulePanel::OnSetRuleName)
    EVT_UPDATE_UI(ID::SetRuleName,RulePanel::OnUpdateSetRuleName)
    EVT_SCROLL(RulePanel::OnScroll)
END_EVENT_TABLE()

RulePanel::RulePanel(MyFrame* parent,wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    // add the rule name
    sizer->Add(new wxStaticText(this,wxID_ANY,_("Rule name:")));
    {
        wxBoxSizer *h_sizer = new wxBoxSizer(wxHORIZONTAL);
        this->rule_name_ctrl = new wxTextCtrl(this,wxID_ANY);
        h_sizer->Add(this->rule_name_ctrl,wxSizerFlags(1).Expand());
        h_sizer->Add(new wxButton(this,ID::SetRuleName,_("set")));
        sizer->Add(h_sizer,wxSizerFlags().Expand());
    }

    // add the parameters
    const int MAX_PARAMS=20;
    for(int i=0;i<MAX_PARAMS;i++)
    {
        wxStaticBoxSizer *sbox = new wxStaticBoxSizer(wxHORIZONTAL,this);
        this->parameter_names.push_back(sbox);

        wxSlider *slider = new wxSlider(this,wxID_ANY,0,0,1000);
        this->parameter_sliders.push_back(slider);
        sbox->Add(slider,wxSizerFlags(1).Expand());

        wxButton *button = new wxButton(this,wxID_ANY,_("..."),wxDefaultPosition,wxDefaultSize,wxBU_EXACTFIT);
        this->parameter_buttons.push_back(button);
        sbox->Add(button,wxSizerFlags().Expand());

        sizer->Add(sbox,wxSizerFlags().Expand());
    }

    // add a compile button
    //sizer->Add(new wxButton(this->rule_panel,ID::ReplaceProgram,_("Compile")),wxSizerFlags(0).Align(wxALIGN_RIGHT));
    // TODO: kernel-editing temporarily disabled, can edit file instead for now

    // add the formula box
    sizer->Add(new wxStaticText(this,wxID_ANY,_("Formula:")));
    this->formula_ctrl = new wxTextCtrl(this,wxID_ANY,
                        _T(""),
                        wxDefaultPosition,wxDefaultSize,
                        wxTE_MULTILINE | wxTE_RICH2 | wxTE_DONTWRAP | wxTE_PROCESS_TAB );
    // TODO: provide UI for changing font size (ditto in Help pane)
    #ifdef __WXMAC__
        // need bigger font size on Mac
        this->formula_ctrl->SetFont(wxFont(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));
    #else
        this->formula_ctrl->SetFont(wxFont(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));
    #endif
    sizer->Add(this->formula_ctrl,wxSizerFlags(1).Expand());

    this->SetSizer(sizer);
}

void RulePanel::Update(const BaseRD* const system)
{
    // set the rule name
    this->rule_name_ctrl->SetValue(wxString(system->GetRuleName().c_str(),wxConvUTF8));
    // update the parameters controls
    this->parameter_ranges.resize(this->parameter_names.size());
    for(int i=0;i<(int)this->parameter_names.size();i++)
    {
        bool active = i<system->GetNumberOfParameters();
        this->parameter_names[i]->Show(active);
        this->parameter_buttons[i]->Show(active);
        this->parameter_sliders[i]->Show(active);
        if(active)
        {
            ostringstream label;
            label << system->GetParameterName(i);
            label << " = " << system->GetParameterValue(i);
            this->parameter_names[i]->GetStaticBox()->SetLabel(wxString(label.str().c_str(),wxConvUTF8));
            this->parameter_sliders[i]->SetValue(1000*(system->GetParameterValue(i)-system->GetParameterMin(i))/(system->GetParameterMax(i)-system->GetParameterMin(i)));
            this->parameter_ranges[i] = make_pair(system->GetParameterMin(i),system->GetParameterMax(i));
        }
    }
    // enable or disable the formula text box
    if(system->HasEditableFormula())
    {
        this->formula_ctrl->SetValue(wxString(system->GetFormula().c_str(),wxConvUTF8));
        this->formula_ctrl->Enable(true);
    }
    else
    {
        this->formula_ctrl->SetValue(_T("(this implementation has no editable formula)"));
        this->formula_ctrl->Enable(false);
    }
    this->Layout();
}

void RulePanel::OnSetRuleName(wxCommandEvent& event)
{
    this->frame->SetRuleName(string(this->rule_name_ctrl->GetValue().mb_str()));
    this->rule_name_ctrl->SetModified(false);
}

void RulePanel::OnUpdateSetRuleName(wxUpdateUIEvent& event)
{
    event.Enable(this->rule_name_ctrl->IsModified());
}

void RulePanel::OnScroll(wxScrollEvent& event)
{
    for(int i=0;i<(int)this->parameter_sliders.size();i++)
    {
        wxSlider *slider = this->parameter_sliders[i];
        if(event.GetEventObject()==slider)
        {
            float val = (slider->GetValue()-slider->GetMin()) / float(slider->GetMax()-slider->GetMin());
            val = this->parameter_ranges[i].first + val*(this->parameter_ranges[i].second-this->parameter_ranges[i].first);
            this->frame->SetParameter(i,val);
        }
    }
}
