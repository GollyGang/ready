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

// wxWidgets:
#include <wx/spinctrl.h>    // for wxSpinCtrl
#include <wx/clipbrd.h>     // for wxTheClipboard

// -----------------------------------------------------------------------------

// need platform-specific gap after OK/Cancel buttons
#ifdef __WXMAC__
    const int STDHGAP = 0;
#elif defined(__WXMSW__)
    const int STDHGAP = 6;
#else
    const int STDHGAP = 10;
#endif

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

// =============================================================================

// define a modal dialog for getting a string

class StringDialog : public wxDialog
{
public:
    StringDialog(wxWindow* parent, const wxString& title,
                     const wxString& prompt, const wxString& instring);

    virtual bool TransferDataFromWindow();     // called when user hits OK

    wxString GetValue() { return result; }

private:
    wxTextCtrl* textbox;       // text box for entering the string
    wxString result;           // the resulting string
};

// -----------------------------------------------------------------------------

StringDialog::StringDialog(wxWindow* parent, const wxString& title,
                           const wxString& prompt, const wxString& instring)
{
    Create(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize);

    // create the controls
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    textbox = new wxTextCtrl(this, wxID_ANY, instring);

    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, prompt);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);
    
    // position the controls
    wxBoxSizer* stdhbox = new wxBoxSizer(wxHORIZONTAL);
    stdhbox->Add(stdbutts, 1, wxGROW | wxALIGN_CENTER_VERTICAL | wxRIGHT, STDHGAP);
    wxSize minsize = stdhbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        stdhbox->SetMinSize(minsize);
    }

    topSizer->AddSpacer(12);
    topSizer->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(10);
    topSizer->Add(textbox, 0, wxGROW | wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(12);
    topSizer->Add(stdhbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

    // select initial string (must do this last on Windows)
    textbox->SetFocus();
    textbox->SetSelection(-1,-1);
}

// -----------------------------------------------------------------------------

bool StringDialog::TransferDataFromWindow()
{
    result = textbox->GetValue();
    return true;
}

// -----------------------------------------------------------------------------

bool GetString(const wxString& title, const wxString& prompt,
               const wxString& instring, wxString& outstring)
{
    StringDialog dialog(wxGetApp().GetTopWindow(), title, prompt, instring);
    if ( dialog.ShowModal() == wxID_OK ) {
        outstring = dialog.GetValue();
        return true;
    } else {
        // user hit Cancel button
        return false;
    }
}

// =============================================================================

// define a modal dialog for getting an integer

class IntegerDialog : public wxDialog
{
public:
    IntegerDialog(wxWindow* parent,
                  const wxString& title,
                  const wxString& prompt,
                  int inval, int minval, int maxval);

    #ifdef __WXOSX__
        ~IntegerDialog() { delete onetimer; }
        void OnOneTimer(wxTimerEvent& event);
    #endif
    
    virtual bool TransferDataFromWindow();     // called when user hits OK

    #ifdef __WXMAC__
        void OnSpinCtrlChar(wxKeyEvent& event);
    #endif

    int GetValue() { return result; }

private:
    enum {
        ID_SPIN_CTRL = wxID_HIGHEST + 1
    };
    wxSpinCtrl* spinctrl;    // for entering the integer
    int minint;              // minimum value
    int maxint;              // maximum value
    int result;              // the resulting integer

    #ifdef __WXOSX__
        wxTimer* onetimer;   // one shot timer (see OnOneTimer)
        DECLARE_EVENT_TABLE()
    #endif
};

// -----------------------------------------------------------------------------

#ifdef __WXOSX__

BEGIN_EVENT_TABLE(IntegerDialog, wxDialog)
    EVT_TIMER (wxID_ANY, IntegerDialog::OnOneTimer)
END_EVENT_TABLE()

void IntegerDialog::OnOneTimer(wxTimerEvent& WXUNUSED(event))
{
    wxSpinCtrl* s1 = (wxSpinCtrl*) FindWindowById(ID_SPIN_CTRL);
    wxSpinCtrl* s2 = (wxSpinCtrl*) FindWindowById(ID_SPIN_CTRL+1);
    if (s1 && s2) {
        // first need to change focus to hidden control
        s2->SetFocus();
        s1->SetFocus();
        s1->SetSelection(-1,-1);
    }
}

#endif

// -----------------------------------------------------------------------------

#ifdef __WXMAC__

// override key event handler for wxSpinCtrl to allow key checking
class MySpinCtrl : public wxSpinCtrl
{
public:
    MySpinCtrl(wxWindow* parent, wxWindowID id) : wxSpinCtrl(parent, id)
    {
        // create a dynamic event handler for the underlying wxTextCtrl
        wxTextCtrl* textctrl = GetText();
        if (textctrl) {
            textctrl->Connect(wxID_ANY, wxEVT_CHAR,
                              wxKeyEventHandler(IntegerDialog::OnSpinCtrlChar));
        }
    }
};

void IntegerDialog::OnSpinCtrlChar(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    
    if (event.CmdDown()) {
        // allow handling of cmd-x/v/etc
        event.Skip();

    } else if ( key == WXK_TAB ) {
        wxSpinCtrl* sc = (wxSpinCtrl*) FindWindowById(ID_SPIN_CTRL);
        if ( sc ) {
            sc->SetFocus();
            sc->SetSelection(-1,-1);
        }

    } else if ( key >= ' ' && key <= '~' ) {
        if ( (key >= '0' && key <= '9') || key == '+' || key == '-' ) {
            // allow digits and + or -
            event.Skip();
        } else {
            // disallow any other displayable ascii char
            Beep();
        }

    } else {
        event.Skip();
    }
}

#else

#define MySpinCtrl wxSpinCtrl

#endif // __WXMAC__

// -----------------------------------------------------------------------------

IntegerDialog::IntegerDialog(wxWindow* parent,
                             const wxString& title,
                             const wxString& prompt,
                             int inval, int minval, int maxval)
{
    Create(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize);

    minint = minval;
    maxint = maxval;

    // create the controls
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    spinctrl = new MySpinCtrl(this, ID_SPIN_CTRL);
    spinctrl->SetRange(minval, maxval);
    spinctrl->SetValue(inval);

    #ifdef __WXOSX__
        // create hidden spin ctrl so we can SetFocus below
        wxSpinCtrl* hidden = new wxSpinCtrl(this, ID_SPIN_CTRL+1, wxEmptyString,
                                            wxPoint(-1000,-1000), wxDefaultSize);
        hidden->SetValue(666);
    #endif
    
    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, prompt);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);
    
    // position the controls
    wxBoxSizer* stdhbox = new wxBoxSizer(wxHORIZONTAL);
    stdhbox->Add(stdbutts, 1, wxGROW | wxALIGN_CENTER_VERTICAL | wxRIGHT, STDHGAP);
    wxSize minsize = stdhbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        stdhbox->SetMinSize(minsize);
    }

    topSizer->AddSpacer(12);
    topSizer->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(10);
    topSizer->Add(spinctrl, 0, wxGROW | wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(12);
    topSizer->Add(stdhbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();

    #ifdef __WXOSX__
        // due to wxOSX bug we have to set focus after dialog creation
        onetimer = new wxTimer(this, wxID_ANY);
        if (onetimer) onetimer->Start(10, wxTIMER_ONE_SHOT);
    #else
        // select initial value (must do this last on Windows)
        spinctrl->SetFocus();
        spinctrl->SetSelection(-1,-1);
    #endif
}

// -----------------------------------------------------------------------------

bool IntegerDialog::TransferDataFromWindow()
{
#if defined(__WXMSW__) || defined(__WXGTK__)
    // spinctrl->GetValue() always returns a value within range even if
    // the text ctrl doesn't contain a valid number -- yuk!
    result = spinctrl->GetValue();
    if (result < minint || result > maxint)
#else
    // GetTextValue returns FALSE if text ctrl doesn't contain a valid number
    // or the number is out of range, but it's not available in wxMSW or wxGTK
    if ( !spinctrl->GetTextValue(&result) || result < minint || result > maxint )
#endif
    {
        wxString msg;
        msg.Printf(_("Value must be from %d to %d."), minint, maxint);
        Warning(msg);
        spinctrl->SetFocus();
        spinctrl->SetSelection(-1,-1);
        return false;
    } else {
        return true;
    }
}

// -----------------------------------------------------------------------------

bool GetInteger(const wxString& title, const wxString& prompt,
                int inval, int minval, int maxval, int* outval)
{
    IntegerDialog dialog(wxGetApp().GetTopWindow(), title, prompt,
                         inval, minval, maxval);
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

MonospaceMessageBox::MonospaceMessageBox(const wxString& message, const wxString& title, const wxArtID& icon) 
     : wxDialog(NULL,wxID_ANY,title,wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    wxFont font(10,wxTELETYPE,wxFONTSTYLE_NORMAL,wxNORMAL,false,_T("Monospace"),wxFONTENCODING_DEFAULT);
    wxTextCtrl *text = new wxTextCtrl(this,wxID_ANY,message,wxDefaultPosition,wxDefaultSize,
        wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    text->SetFont(font);
    text->SetMinSize(wxSize(800,200));
    text->SetBackgroundColour(this->GetBackgroundColour());
    text->SetForegroundColour(*wxBLACK);
    this->SetIcon(wxArtProvider::GetIcon(icon));
    vbox->Add(text,wxSizerFlags(1).Expand().DoubleBorder());
    vbox->Add(this->CreateButtonSizer(wxOK),wxSizerFlags(0).Expand().DoubleBorder());
    this->SetSizerAndFit(vbox);
    this->ShowModal();
}
