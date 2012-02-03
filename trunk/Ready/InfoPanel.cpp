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
#include "InfoPanel.hpp"
#include "BaseRD.hpp"
#include "frame.hpp"
#include "IDs.hpp"

// STL:
#include <sstream>
using namespace std;

BEGIN_EVENT_TABLE(InfoPanel, wxPanel)
    EVT_TEXT_URL(wxID_ANY,InfoPanel::OnLinkClicked)
END_EVENT_TABLE()

InfoPanel::InfoPanel(MyFrame* parent,wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    this->description_ctrl = new wxTextCtrl(this,wxID_ANY,wxEmptyString,wxDefaultPosition,wxDefaultSize,
        wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH2|wxTE_AUTO_URL);

    this->description_ctrl->SetBackgroundColour(*wxLIGHT_GREY);    
    this->description_ctrl->SetForegroundColour(*wxBLACK);    
    
    sizer->Add(this->description_ctrl,wxSizerFlags(1).Expand());

    this->SetSizer(sizer);
}

void InfoPanel::Update(const BaseRD* const system)
{
    ostringstream oss;
    oss << system->GetRuleName() << "\n\n";
    if(!system->GetRuleDescription().empty())
        oss << system->GetRuleDescription() << "\n\n";
    if(!system->GetPatternDescription().empty())
        oss << system->GetPatternDescription();
    this->description_ctrl->ChangeValue(wxString(oss.str().c_str(),wxConvUTF8));
}

void InfoPanel::OnLinkClicked(wxTextUrlEvent& event)
{
    if(event.GetMouseEvent().LeftUp())
    {
        wxLaunchDefaultBrowser(this->description_ctrl->GetValue().Mid(
            event.GetURLStart(),event.GetURLEnd() - event.GetURLStart()));
    }
    event.Skip();
}
