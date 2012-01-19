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

// wxWidgets:
#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/propgrid/propgrid.h>

// local:
class MyFrame;
class BaseRD;

// STL:
#include <vector>

// our rule panel has controls to allow the user to change the parameters of an RD system
// (it doesn't change the BaseRD itself though, MyFrame does that)
class RulePanel : public wxPanel
{
    public:

        RulePanel(MyFrame* parent,wxWindowID id);

        // update the controls to the state of the RD system
        void Update(const BaseRD* const system);

    private:

        void OnPropertyGridChanged(wxPropertyGridEvent& event);

    private:

        MyFrame *frame; // keep a link so that we can alert the parent frame when user makes a change

        wxPropertyGrid *pgrid;
        std::vector<wxPGProperty*> parameter_properties;
        wxPGProperty *rule_name_property;

        DECLARE_EVENT_TABLE()
};
