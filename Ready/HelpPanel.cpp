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
#include "HelpPanel.hpp"
#include "frame.hpp"
#include "IDs.hpp"

// wxWidgets:
#include <wx/filename.h>        // for wxFileName
#include <wx/html/htmlwin.h>    // for wxHtmlWindow

BEGIN_EVENT_TABLE(HelpPanel, wxPanel)
    EVT_BUTTON (ID::BackButton,     HelpPanel::OnBackButton)
    EVT_BUTTON (ID::ForwardButton,  HelpPanel::OnForwardButton)
    EVT_BUTTON (ID::ContentsButton, HelpPanel::OnContentsButton)
END_EVENT_TABLE()

const wxString helphome = _("Help/index.html");    // contents page

// define a child window for displaying HTML info
class HtmlView : public wxHtmlWindow
{
    public:

        HtmlView(wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxHW_SCROLLBAR_AUTO)
            : wxHtmlWindow(parent, id, pos, size, style) {}

        void OnLinkClicked(const wxHtmlLinkInfo& link)
        {
            wxString url = link.GetHref();
            if ( url.StartsWith(_T("http://")) || url.StartsWith(_T("mailto://")) ) {
                wxLaunchDefaultBrowser(url);
            } else {
                // assume it's a link to a local target or another help file
                HelpPanel* panel = (HelpPanel*)GetParent();
                panel->ShowHelp(url);
            }
            // AKT TODO!!! look for special link prefixes like "prefs:"
        }
};

HelpPanel::HelpPanel(MyFrame* parent, wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    html = new HtmlView(this, wxID_ANY);
    
    // AKT TODO!!! html->SetFontSizes(helpfontsize);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    // add buttons at top of html window
    backbutt = new wxButton(this, ID::BackButton, _("<"), wxDefaultPosition, wxSize(40,wxDefaultCoord));
    forwbutt = new wxButton(this, ID::ForwardButton, _(">"), wxDefaultPosition, wxSize(40,wxDefaultCoord));
    contbutt = new wxButton(this, ID::ContentsButton, _("Contents"));

    #ifdef __WXMAC__
        // nicer to use smaller buttons -- TODO: also do for Win/Linux???
        backbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        forwbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        contbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    #endif

    hbox->Add(backbutt, 0, wxALL | wxALIGN_LEFT, 10);
    hbox->Add(forwbutt, 0, wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);
    hbox->Add(contbutt, 0, wxALL | wxALIGN_LEFT, 10);
    hbox->AddStretchSpacer(1);
    vbox->Add(hbox, 0, wxALL | wxEXPAND | wxALIGN_TOP, 0);
    vbox->Add(html, 1, wxEXPAND | wxALIGN_TOP, 0);
    SetSizer(vbox);

    ShowHelp(helphome);
}

void HelpPanel::ShowHelp(const wxString& filepath)
{
    if (filepath == SHOW_KEYBOARD_SHORTCUTS) {
        /* AKT TODO!!!
        // build HTML string to display current keyboard shortcuts
        wxString contents = GetShortcutTable();
        
        // write contents to file and call LoadPage so that back/forwards buttons work
        wxString htmlfile = tempdir + SHOW_KEYBOARD_SHORTCUTS;
        wxFile outfile(htmlfile, wxFile::write);
        if (outfile.IsOpened()) {
            outfile.Write(contents);
            outfile.Close();
            html->LoadPage(htmlfile);
        } else {
            Warning(_("Could not create file:\n") + htmlfile);
            // might as well show contents
            html->SetPage(contents);
        }
        */
        html->SetPage(_T("<html><body><p>Not yet implemented!!!</p></body></html>"));

    } else if ( filepath.StartsWith(_("Help/")) ) {
        // safer to prepend location of app
        wxString fullpath = /* AKT TODO!!! readydir + */ filepath;
        html->LoadPage(fullpath);
    
    } else {
        // assume full path or local link
        html->LoadPage(filepath);
    }
    
    UpdateHelpButtons();
}

void HelpPanel::OnBackButton(wxCommandEvent& WXUNUSED(event))
{
    if ( html->HistoryBack() ) UpdateHelpButtons();
}

void HelpPanel::OnForwardButton(wxCommandEvent& WXUNUSED(event))
{
    if ( html->HistoryForward() ) UpdateHelpButtons();
}

void HelpPanel::OnContentsButton(wxCommandEvent& WXUNUSED(event))
{
    ShowHelp(helphome);
}

void HelpPanel::UpdateHelpButtons()
{
    backbutt->Enable( html->HistoryCanBack() );
    forwbutt->Enable( html->HistoryCanForward() );
    // check for title used in helphome
    contbutt->Enable( html->GetOpenedPageTitle() != _("Ready Help: Contents") );
      
    html->SetFocus();       // for keyboard shortcuts
}
