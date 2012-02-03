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

// displays some information about the current pattern
class InfoPanel : public wxPanel
{
    public:

        InfoPanel(MyFrame* parent,wxWindowID id);

        // update the text in description_ctrl
        void Update(const BaseRD* const system);

        bool TextHasFocus();    // description_ctrl has keyboard focus?
        
        // return false if key event should be passed to default handler
        bool DoKey(int key, int mods);
        
    private:
    
        void OnLinkClicked(wxTextUrlEvent& event);
        
    private:

        MyFrame *frame; // link to parent frame

        wxTextCtrl *description_ctrl;

        DECLARE_EVENT_TABLE()
};
