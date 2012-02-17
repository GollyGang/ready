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

// local
#include "wxutils.hpp"
#include "app.hpp"          // for wxGetApp
#include "prefs.hpp"        // for allowbeep
#include "dialogs.hpp"

// wxWidgets:
#include <wx/clipbrd.h>     // for wxTheClipboard

// -----------------------------------------------------------------------------

void Note(const wxString& msg)
{
    wxString title = wxGetApp().GetAppName() + _(" note:");
    #ifdef __WXMAC__
        wxSetCursor(*wxSTANDARD_CURSOR);
    #endif
    wxMessageBox(msg, title, wxOK | wxICON_INFORMATION, wxGetActiveWindow());
}

// -----------------------------------------------------------------------------

void Warning(const wxString& msg)
{
    Beep();
    wxString title = wxGetApp().GetAppName() + _(" warning:");
    #ifdef __WXMAC__
        wxSetCursor(*wxSTANDARD_CURSOR);
    #endif
    wxMessageBox(msg, title, wxOK | wxICON_EXCLAMATION, wxGetActiveWindow());
}

// -----------------------------------------------------------------------------

void Fatal(const wxString& msg)
{
    Beep();
    wxString title = wxGetApp().GetAppName() + _(" error:");
    #ifdef __WXMAC__
        wxSetCursor(*wxSTANDARD_CURSOR);
    #endif
    wxMessageBox(msg, title, wxOK | wxICON_ERROR, wxGetActiveWindow());

    exit(1);     // safer than calling wxExit()
}

// -----------------------------------------------------------------------------

void Beep()
{
    if (allowbeep) wxBell();
}

// -----------------------------------------------------------------------------

bool GetString(const wxString& title, const wxString& prompt,
               const wxString& instring, wxString& outstring,
               const wxPoint& pos, const wxSize& size)
{
    StringDialog dialog(wxGetApp().GetTopWindow(), title, prompt,
                        instring, pos, size);
    if ( dialog.ShowModal() == wxID_OK ) {
        outstring = dialog.GetValue();
        return true;
    } else {
        // user hit Cancel button
        return false;
    }
}

// -----------------------------------------------------------------------------

bool GetInteger(const wxString& title, const wxString& prompt,
                int inval, int minval, int maxval, int* outval,
                const wxPoint& pos, const wxSize& size)
{
    IntegerDialog dialog(wxGetApp().GetTopWindow(), title, prompt,
                         inval, minval, maxval, pos, size);
    if ( dialog.ShowModal() == wxID_OK ) {
        *outval = dialog.GetValue();
        return true;
    } else {
        // user hit Cancel button
        return false;
    }
}

// =============================================================================

bool IsHTMLFile(const wxString& filename)
{
    wxString ext = filename.AfterLast('.');
    // if filename has no extension then ext == filename
    if (ext == filename) return false;
    return ( ext.IsSameAs(wxT("htm"),false) ||
             ext.IsSameAs(wxT("html"),false) );
}

// -----------------------------------------------------------------------------

bool IsTextFile(const wxString& filename)
{
    if (!IsHTMLFile(filename)) {
        // if non-html file name contains "readme" then assume it's a text file
        wxString name = filename.AfterLast(wxFILE_SEP_PATH).MakeLower();
        if (name.Contains(wxT("readme"))) return true;
    }
    wxString ext = filename.AfterLast('.');
    // if filename has no extension then ext == filename
    if (ext == filename) return false;
    return ( ext.IsSameAs(wxT("txt"),false) ||
             ext.IsSameAs(wxT("doc"),false) );
}

// -----------------------------------------------------------------------------

int SaveChanges(const wxString& query, const wxString& msg)
{
    #ifdef __WXMAC__
        // create a standard looking Mac dialog
        wxMessageDialog dialog(wxGetActiveWindow(), msg, query,
                               wxCENTER | wxNO_DEFAULT | wxYES_NO | wxCANCEL |
                               wxICON_INFORMATION);
        
        // change button order to what Mac users expect to see
        dialog.SetYesNoCancelLabels("Cancel", "Save", "Don't Save");
       
        switch ( dialog.ShowModal() )
        {
            case wxID_YES:    return wxCANCEL;  // Cancel
            case wxID_NO:     return wxYES;     // Save
            case wxID_CANCEL: return wxNO;      // Don't Save
            default:          return wxCANCEL;  // should never happen
        }
    #else
        // Windows/Linux
        return wxMessageBox(msg, query, wxICON_QUESTION | wxYES_NO | wxCANCEL,
                            wxGetActiveWindow());
    #endif
}

// -----------------------------------------------------------------------------

bool ClipboardHasText()
{
    bool hastext = false;
    #ifdef __WXGTK__
        // avoid re-entrancy bug in wxGTK 2.9.x
        if (wxTheClipboard->IsOpened()) return false;
    #endif
    if (wxTheClipboard->Open()) {
        hastext = wxTheClipboard->IsSupported(wxDF_TEXT);
        wxTheClipboard->Close();
    }
    return hastext;
}

// -----------------------------------------------------------------------------

bool CopyTextToClipboard(const wxString& text)
{
    bool result = true;
    if (wxTheClipboard->Open()) {
        if ( !wxTheClipboard->SetData(new wxTextDataObject(text)) ) {
            Warning(_("Could not copy text to clipboard!"));
            result = false;
        }
        wxTheClipboard->Close();
    } else {
        Warning(_("Could not open clipboard!"));
        result = false;
    }
    return result;
}

// -----------------------------------------------------------------------------

wxString FormatFloat(float f)
{
    wxString result = wxString::Format(wxT("%f"),f);
    // strip any trailing zeros
    while (result.GetChar(result.Length()-1) == wxChar('0')) {
        result.Truncate(result.Length()-1);
    }
    // strip any trailing '.'
    if (result.GetChar(result.Length()-1) == wxChar('.')) {
        result.Truncate(result.Length()-1);
    }
    return result;
}
