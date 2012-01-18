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

// local:
class MyFrame;
class BaseRD;

// our rule panel has controls to allow the user to change the parameters of an RD system
// (it doesn't talk to the BaseRD itself though, MyFrame does that)
class RulePanel : public wxPanel
{
    public:

        RulePanel(MyFrame* parent,wxWindowID id);

        void Update(BaseRD *system);

    private:

        void OnSetRuleName(wxCommandEvent& event);
        void OnUpdateSetRuleName(wxUpdateUIEvent& event);

    private:

        MyFrame *frame;
        wxTextCtrl *rule_name_ctrl,*formula_ctrl;

        DECLARE_EVENT_TABLE()
};
