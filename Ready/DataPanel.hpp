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
class BaseRD;
class MyFrame;
class HtmlInfo;

// this panel allows the user to change the parameters of an RD system
// (it doesn't change the BaseRD itself though, MyFrame does that)
class DataPanel : public wxPanel
{
    public:

        DataPanel(MyFrame* parent, wxWindowID id);

        // update the displayed info to reflect the state of the RD system
        void Update(const BaseRD* const system);
        
        // update buttons at top of panel
        void UpdateButtons();
        
        // display link info in status line
        void SetStatus(const wxString& text) { status->SetLabel(text); }
        
        bool HtmlHasFocus();    // html window has keyboard focus?
        void SelectAllText();   // select all text in html window
        void CopySelection();   // copy selected text to clipboard
        
        // return false if key event should be passed to default handler
        bool DoKey(int key, int mods);
        
    private:

        MyFrame* frame;         // link to parent frame

        HtmlInfo* html;         // child window for rendering HTML data

        wxButton* smallerbutt;  // smaller text
        wxButton* biggerbutt;   // bigger text
   
        wxStaticText* status;   // for link info
        
        // event handlers
        void OnSmallerButton(wxCommandEvent& event);
        void OnBiggerButton(wxCommandEvent& event);
        
        wxString AppendRow(const wxString& label, const wxString& value);
        wxString FormatFloat(const float& f);

        DECLARE_EVENT_TABLE()
};
