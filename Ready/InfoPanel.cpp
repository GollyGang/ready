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
#include "app.hpp"              // for wxGetApp
#include "frame.hpp"
#include "IDs.hpp"
#include "prefs.hpp"            // for readydir, etc
#include "wxutils.hpp"          // for Warning, CopyTextToClipboard

// readybase:
#include "BaseRD.hpp"

// wxWidgets:
#include <wx/filename.h>        // for wxFileName
#include <wx/html/htmlwin.h>    // for wxHtmlWindow

// STL:
#include <string>
using namespace std;

#if defined(__WXMAC__) && wxCHECK_VERSION(2,9,0)
    // wxMOD_CONTROL has been changed to mean Command key down
    #define wxMOD_CONTROL wxMOD_RAW_CONTROL
    #define ControlDown RawControlDown
#endif

const wxString change_prefix = wxT("change: ");         // must end with space
const wxString parameter_prefix = wxT("parameter ");    // ditto

// labels in 1st column
const wxString rule_name_label = wxT("Rule name");
const wxString description_label = wxT("Description");
const wxString num_chemicals_label = wxT("# of chemicals");
const wxString formula_label = wxT("Formula");
const wxString dimensions_label = wxT("Dimensions");
const wxString block_size_label = wxT("Block size");

// -----------------------------------------------------------------------------

// define a child window for displaying HTML info
class HtmlInfo : public wxHtmlWindow
{
    public:

        HtmlInfo(wxWindow* parent, MyFrame* myframe, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxHW_SCROLLBAR_AUTO)
            : wxHtmlWindow(parent, id, pos, size, style)
        {
            frame = myframe;
            panel = (InfoPanel*)parent;
            editlink = false;
            linkrect = wxRect(0,0,0,0);
        }

        virtual void OnLinkClicked(const wxHtmlLinkInfo& link);
        virtual void OnCellMouseHover(wxHtmlCell* cell, wxCoord x, wxCoord y);
        
        void ClearStatus();  // clear pane's status line
        
        void SetFontSizes(int size);
        void ChangeFontSizes(int size);

        wxRect linkrect;     // rect for cell containing link

    private:

        void OnSize(wxSizeEvent& event);
        void OnMouseMotion(wxMouseEvent& event);
        void OnMouseLeave(wxMouseEvent& event);
        void OnMouseDown(wxMouseEvent& event);
        void OnHtmlCellClicked(wxHtmlCellEvent& event);
        
        MyFrame* frame;
        InfoPanel* panel;
        
        bool editlink;       // open clicked file in editor?

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(HtmlInfo, wxHtmlWindow)
    EVT_SIZE          (HtmlInfo::OnSize)
    EVT_MOTION        (HtmlInfo::OnMouseMotion)
    EVT_ENTER_WINDOW  (HtmlInfo::OnMouseMotion)
    EVT_LEAVE_WINDOW  (HtmlInfo::OnMouseLeave)
    EVT_LEFT_DOWN     (HtmlInfo::OnMouseDown)
    EVT_RIGHT_DOWN    (HtmlInfo::OnMouseDown)
    EVT_HTML_CELL_CLICKED (wxID_ANY, HtmlInfo::OnHtmlCellClicked)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

void HtmlInfo::OnLinkClicked(const wxHtmlLinkInfo& link)
{
    wxString url = link.GetHref();
    if ( url.StartsWith(wxT("http:")) || url.StartsWith(wxT("mailto:")) ) {
    // pass http/mailto URL to user's preferred browser/emailer
    #if defined(__WXMAC__)
        // wxLaunchDefaultBrowser doesn't work on Mac with IE (get msg in console.log)
        // but it's easier just to use the Mac OS X open command
        if ( wxExecute(wxT("open ") + url, wxEXEC_ASYNC) == -1 )
            Warning(_("Could not open URL!"));
    #elif defined(__WXGTK__)
        // wxLaunchDefaultBrowser is not reliable on Linux/GTK so we call gnome-open
        if ( wxExecute(wxT("gnome-open ") + url, wxEXEC_ASYNC) == -1 )
            Warning(_("Could not open URL!"));
    #else
        if ( !wxLaunchDefaultBrowser(url) )
            Warning(_("Could not launch browser!"));
    #endif

    } else if ( url.StartsWith(change_prefix) ) {
        panel->ChangeInfo( url.AfterFirst(' ') );
        // best to reset focus after dialog closes
        SetFocus();

    } else if ( url.StartsWith(wxT("prefs:")) ) {
        // user clicked on link to Preferences dialog
        frame->ShowPrefsDialog( url.AfterFirst(':') );

    } else if ( url.StartsWith(wxT("edit:")) ) {
        // open clicked file in user's preferred text editor
        wxString path = url.AfterFirst(':');
        #ifdef __WXMSW__
            path.Replace(wxT("/"), wxT("\\"));
        #endif
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;
        frame->EditFile(path);


    } else if ( url.StartsWith(wxT("open:")) ) {
        // open clicked file
        wxString path = url.AfterFirst(':');
        #ifdef __WXMSW__
            path.Replace(wxT("/"), wxT("\\"));
        #endif
        wxFileName fname(path);
        if (!fname.IsAbsolute()) path = readydir + path;
        if (editlink) {
            frame->EditFile(path);
        } else {
            frame->Raise();
            frame->OpenFile(path);
        }

    } else {
        // assume it's a link to a local target
        LoadPage(url);
    }
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnHtmlCellClicked(wxHtmlCellEvent& event)
{
    wxHtmlCell* cell = event.GetCell();
    int x = event.GetPoint().x;
    int y = event.GetPoint().y;
    
    wxHtmlLinkInfo* link = cell->GetLink(x,y);
    if (link) {
        // set linkrect to avoid bug in wxHTML if click was in outer edge of link
        // (bug is in htmlwin.cpp in wxHtmlWindowMouseHelper::HandleIdle;
        // OnCellMouseHover needs to be called if cell != m_tmpLastCell)
        wxPoint pt = ScreenToClient( wxGetMousePosition() );
        linkrect = wxRect(pt.x-x, pt.y-y, cell->GetWidth(), cell->GetHeight());
    }

    event.Skip();   // call OnLinkClicked
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnCellMouseHover(wxHtmlCell* cell, wxCoord x, wxCoord y)
{
    wxHtmlLinkInfo* link = cell->GetLink(x,y);
    if (link) {
        wxString href = link->GetHref();
        href.Replace(wxT("&"), wxT("&&"));
        panel->SetStatus(href);
        wxPoint pt = ScreenToClient( wxGetMousePosition() );
        linkrect = wxRect(pt.x-x, pt.y-y, cell->GetWidth(), cell->GetHeight());
    } else {
        ClearStatus();
    }
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnMouseMotion(wxMouseEvent& event)
{
    if (!linkrect.IsEmpty()) {
        int x = event.GetX();
        int y = event.GetY();
        if (!linkrect.Contains(x,y)) ClearStatus();
    }
    event.Skip();
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnMouseLeave(wxMouseEvent& event)
{
    ClearStatus();
    event.Skip();
}

// -----------------------------------------------------------------------------

void HtmlInfo::ClearStatus()
{
    panel->SetStatus(wxEmptyString);
    linkrect = wxRect(0,0,0,0);
}

// -----------------------------------------------------------------------------

void HtmlInfo::OnMouseDown(wxMouseEvent& event)
{
    // set flag so ctrl/right-clicked file can be opened in editor
    editlink = event.ControlDown() || event.RightDown();
    event.Skip();
}

// -----------------------------------------------------------------------------

// avoid scroll position being reset to top when wxHtmlWindow is resized
void HtmlInfo::OnSize(wxSizeEvent& event)
{
    int x, y;
    GetViewStart(&x, &y);            // save current position

    wxHtmlWindow::OnSize(event);

    wxString currpage = GetOpenedPage();
    if ( !currpage.IsEmpty() ) {
        LoadPage(currpage);         // reload page
        Scroll(x, y);               // scroll to old position
    }
    
    // prevent wxHtmlWindow::OnSize being called again
    event.Skip(false);
}

// -----------------------------------------------------------------------------

void HtmlInfo::SetFontSizes(int size)
{
    // set font sizes for <FONT SIZE=-2> to <FONT SIZE=+4>
    int f_sizes[7];
    f_sizes[0] = int(size * 0.6);
    f_sizes[1] = int(size * 0.8);
    f_sizes[2] = size;
    f_sizes[3] = int(size * 1.2);
    f_sizes[4] = int(size * 1.4);
    f_sizes[5] = int(size * 1.6);
    f_sizes[6] = int(size * 1.8);
    #ifdef __WXOSX_COCOA__
        SetFonts(wxT("Lucida Grande"), wxT("Monaco"), f_sizes);
    #else
        SetFonts(wxEmptyString, wxEmptyString, f_sizes);
    #endif
}

// -----------------------------------------------------------------------------

void HtmlInfo::ChangeFontSizes(int size)
{
    // changing font sizes resets pos to top, so save and restore pos
    int x, y;
    GetViewStart(&x, &y);
    SetFontSizes(size);
    Scroll(x, y);
    panel->UpdateButtons();
}

// -----------------------------------------------------------------------------

// define a panel that contains a HtmlInfo window

BEGIN_EVENT_TABLE(InfoPanel, wxPanel)
    EVT_BUTTON (ID::SmallerButton,  InfoPanel::OnSmallerButton)
    EVT_BUTTON (ID::BiggerButton,   InfoPanel::OnBiggerButton)
END_EVENT_TABLE()

InfoPanel::InfoPanel(MyFrame* parent, wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    html = new HtmlInfo(this, frame, wxID_ANY);
    
    html->SetBorders(0);
    html->SetFontSizes(infofontsize);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    // add buttons at top of html window
    smallerbutt = new wxButton(this, ID::SmallerButton, _("-"), wxDefaultPosition, wxSize(30,wxDefaultCoord));
    biggerbutt = new wxButton(this, ID::BiggerButton, _("+"), wxDefaultPosition, wxSize(30,wxDefaultCoord));

    #ifdef __WXMAC__
        // nicer to use smaller buttons -- also do for Win/Linux???
        smallerbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        biggerbutt->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    #endif

    hbox->Add(smallerbutt, 0, wxLEFT | wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);
    hbox->Add(biggerbutt, 0, wxLEFT | wxTOP | wxBOTTOM | wxALIGN_LEFT, 10);

    status = new wxStaticText(this, wxID_STATIC, wxEmptyString);
    #ifdef __WXMAC__
        status->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    #endif
    wxBoxSizer* statbox = new wxBoxSizer(wxHORIZONTAL);
    statbox->Add(status);
    hbox->Add(statbox, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 10);

    hbox->AddStretchSpacer(1);
    vbox->Add(hbox, 0, wxALL | wxEXPAND | wxALIGN_TOP, 0);
    vbox->Add(html, 1, wxEXPAND | wxALIGN_TOP, 0);
    SetSizer(vbox);
    
    UpdateButtons();

    // install event handlers to detect keyboard shortcuts when html window has focus
    html->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MyFrame::OnKeyDown), NULL, frame);
    html->Connect(wxEVT_CHAR, wxKeyEventHandler(MyFrame::OnChar), NULL, frame);
}

static int rownum;  // for alternating row background colors

// -----------------------------------------------------------------------------

static wxString FormatFloat(float f)
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

// -----------------------------------------------------------------------------

void InfoPanel::Update(const BaseRD* const system)
{
    // build HTML string to display current parameters
    wxString contents;
    
    contents += _("<html><body><table border=0 cellspacing=0 cellpadding=4 width=\"100%\">");

    rownum = 0;
    wxString s(system->GetRuleName().c_str(),wxConvUTF8);
    s.Replace(wxT("\n"), wxT("<br>"));
    contents += AppendRow(rule_name_label, s);
    s = wxString(system->GetDescription().c_str(),wxConvUTF8);
    s.Replace(wxT("\n"), wxT("<br>"));
    contents += AppendRow(description_label, s);

    contents += AppendRow(num_chemicals_label, wxString::Format(wxT("%d"),system->GetNumberOfChemicals()),
                          false, system->HasEditableNumberOfChemicals());
    
    for(int iParam=0;iParam<(int)system->GetNumberOfParameters();iParam++)
    {
        contents += AppendRow(system->GetParameterName(iParam),
                              FormatFloat(system->GetParameterValue(iParam)), true);
    }

    wxString formula = system->GetFormula();
    // escape HTML reserved characters
    formula.Replace(wxT("&"), wxT("&amp;")); // (the order of these is important)
    formula.Replace(wxT("<"), wxT("&lt;"));
    formula.Replace(wxT(">"), wxT("&gt;"));
    // deal with line endings
    formula.Replace(wxT("\r\n"), wxT("<br>"));
    formula.Replace(wxT("\n\r"), wxT("<br>"));
    formula.Replace(wxT("\r"), wxT("<br>"));
    formula.Replace(wxT("\n"), wxT("<br>"));
    // convert whitespace to &nbsp; so we can use the <code> block
    formula.Replace(wxT("  "), wxT("&nbsp;&nbsp;")); 
    // (This is a bit of a hack. We only want to keep the leading whitespace on each line, and since &ensp; is not supported we
    //  have to use &nbsp; but this prevents wrapping. By only replacing *double* spaces we cover most usages and it's good enough for now.)
    formula = _("<code>") + formula + _("</code>"); // (would prefer the <pre> block here but it adds a leading newline, and also prevents wrapping)
    contents += AppendRow(formula_label, formula, false, system->HasEditableFormula());

    contents += AppendRow(dimensions_label, wxString::Format(wxT("XYZ = %d; %d; %d"),
                                            system->GetX(),system->GetY(),system->GetZ()));

    contents += AppendRow(block_size_label, wxString::Format(wxT("XYZ = %d; %d; %d"),
                                            system->GetBlockSizeX(),system->GetBlockSizeY(),system->GetBlockSizeZ()),
                                            false, system->HasEditableBlockSize());

    contents += _("</table></body></html>");
    
    html->SetPage(contents);
}

// -----------------------------------------------------------------------------

wxString InfoPanel::AppendRow(const wxString& label, const wxString& value,
                              bool is_parameter, bool is_editable)
{
    wxString result;
    if (rownum & 1)
        result += _("<tr bgcolor=\"#F0F0F0\">");
    else
        result += _("<tr>");
    rownum++;

    result += _("<td width=3></td><td valign=top width=\"22%\"><b>");
    result += label;
    result += _("</b></td><td valign=top>");
    result += value;
    result += _("</td>");

    if (is_editable) {
        result += _("<td valign=top align=right><a href=\"");
        result += change_prefix;
        if (is_parameter) result += parameter_prefix;
        result += label;
        result += _("\">edit</a></td>");
    } else {
        result += _("<td></td>");
    }
    result += _("<td width=3></td>");
    
    
    result += _("</tr>");
    return result;
}

// =============================================================================

// define a modal dialog for editing a parameter name and/or value

class ParameterDialog : public wxDialog
{
    public:
        ParameterDialog(wxWindow* parent, bool can_edit_name,
                        const wxString& inname, float inval,
                        const wxPoint& pos, const wxSize& size);
    
        #ifdef __WXOSX__
            ~ParameterDialog() { delete onetimer; }
            void OnOneTimer(wxTimerEvent& event);
        #endif
    
        void OnChar(wxKeyEvent& event);
    
        virtual bool TransferDataFromWindow();  // called when user hits OK
    
        wxString GetName() { return name; }
        float GetValue() { return value; }
    
    private:
        wxTextCtrl* namebox;    // text box for entering name
        wxTextCtrl* valuebox;   // text box for entering value
        wxString name;          // the given name
        float value;            // the given value
    
        #ifdef __WXOSX__
            wxTimer* onetimer;  // one shot timer (see OnOneTimer)
            DECLARE_EVENT_TABLE()
        #endif
};

// -----------------------------------------------------------------------------

#ifdef __WXOSX__

BEGIN_EVENT_TABLE(ParameterDialog, wxDialog)
    EVT_TIMER (wxID_ANY, ParameterDialog::OnOneTimer)
END_EVENT_TABLE()

void ParameterDialog::OnOneTimer(wxTimerEvent& WXUNUSED(event))
{
    if (namebox) {
        namebox->SetFocus();
        valuebox->SetFocus();
        valuebox->SetSelection(-1,-1);
    }
}

#endif

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

ParameterDialog::ParameterDialog(wxWindow* parent, bool can_edit_name,
                                 const wxString& inname, float inval,
                                 const wxPoint& pos, const wxSize& size)
{
    Create(parent, wxID_ANY, _("Change parameter"), pos, size);
    
    // create the controls
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vbox);
    
    wxString prompt = can_edit_name ? _("Enter a new name and/or a new value:")
                                    : _("Enter a new value:");
    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, prompt);

    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->AddStretchSpacer(1);

    if (can_edit_name) {
        namebox = new wxTextCtrl(this, wxID_ANY, inname);
        hbox->Add(namebox, 0, wxGROW | wxLEFT | wxRIGHT, 10);
    } else {
        namebox = NULL;
        hbox->Add(new wxStaticText(this, wxID_STATIC, inname),
                                   0, wxGROW | wxLEFT | wxRIGHT, 10);
    }

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT("=")), 0, wxLEFT | wxRIGHT, 0);
    
    valuebox = new wxTextCtrl(this, wxID_ANY, FormatFloat(inval));
    hbox->Add(valuebox, 0, wxGROW | wxLEFT | wxRIGHT, 10);
    hbox->AddStretchSpacer(1);
    
    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);
    
    // position the controls
    wxBoxSizer* buttbox = new wxBoxSizer(wxHORIZONTAL);
    buttbox->Add(stdbutts, 1, wxGROW | wxALIGN_CENTER_VERTICAL | wxRIGHT, STDHGAP);
    wxSize minsize = buttbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        buttbox->SetMinSize(minsize);
    }

    vbox->AddSpacer(12);
    vbox->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(hbox, 0, wxALL | wxEXPAND | wxALIGN_TOP, 0);
    vbox->AddSpacer(12);
    vbox->Add(buttbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    
    if (pos == wxDefaultPosition) Centre();
    if (size != wxDefaultSize) SetSize(size);

    #ifdef __WXOSX__
        // due to wxOSX bug we have to set focus after dialog creation (see OnOneTimer)
        onetimer = new wxTimer(this, wxID_ANY);
        if (onetimer) onetimer->Start(10, wxTIMER_ONE_SHOT);
    #else
        // select value (must do this last on Windows)
        valuebox->SetFocus();
        valuebox->SetSelection(-1,-1);
    #endif

    // install event handler to detect illegal chars when entering value
    valuebox->Connect(wxEVT_CHAR, wxKeyEventHandler(ParameterDialog::OnChar), NULL, this);
}

// -----------------------------------------------------------------------------

void ParameterDialog::OnChar(wxKeyEvent& event)
{
    int key = event.GetKeyCode();
    if ( key >= ' ' && key <= '~' ) {
        if ( (key >= '0' && key <= '9') || key == '.' ) {
            // allow digits and decimal pt
            event.Skip();
        } else {
            // disallow any other displayable ascii char
            wxBell();
        }
    } else {
        event.Skip();
    }
}

// -----------------------------------------------------------------------------

bool ParameterDialog::TransferDataFromWindow()
{
    if (namebox) name = namebox->GetValue();
    wxString valstr = valuebox->GetValue();
    double dbl;
    if (valstr.ToDouble(&dbl) && dbl >= 0.0 && dbl <= 1.0) {
        value = (float)dbl;
        return true;
    } else {
        Warning(_("The value must be a number from 0.0 to 1.0."));
        return false;
    }
}

// =============================================================================

void InfoPanel::ChangeParameter(const wxString& parameter)
{
    BaseRD* sys = frame->GetCurrentRDSystem();
    int iParam = 0;
    while (iParam < (int)sys->GetNumberOfParameters()) {
        if (parameter == sys->GetParameterName(iParam)) break;
        iParam++;
    }
    if (iParam == (int)sys->GetNumberOfParameters()) {
        Warning(_("Bug in ChangeParameter! Unknown parameter: ") + parameter);
        return;
    }
    
    wxString newname;
    float newval;
    float oldval = sys->GetParameterValue(iParam);
    bool can_edit_name = sys->HasEditableFormula();

    // position dialog box to left of linkrect
    wxPoint pos = ClientToScreen( wxPoint(html->linkrect.x, html->linkrect.y) );
    int dlgwd = 300;
    pos.x -= dlgwd + 20;

    ParameterDialog dialog(frame, can_edit_name, parameter, oldval, pos, wxSize(dlgwd,-1));
    
    if ( dialog.ShowModal() == wxID_OK )
    {
        newname = can_edit_name ? dialog.GetName() : parameter;
        newval = dialog.GetValue();
        if (newname != parameter) frame->SetParameterName(iParam, string(newname.mb_str()));
        if (newval != oldval) frame->SetParameter(iParam, newval);
    }
}

// -----------------------------------------------------------------------------

void InfoPanel::ChangeRuleName()
{
    wxString oldname(frame->GetCurrentRDSystem()->GetRuleName().c_str(),wxConvUTF8);
    wxString newname;
    
    // position dialog box to left of linkrect
    wxPoint pos = ClientToScreen( wxPoint(html->linkrect.x, html->linkrect.y) );
    int dlgwd = 300;
    pos.x -= dlgwd + 20;

    if ( GetString(_("Change rule name"), _("Enter the new rule name:"),
                   oldname, newname, pos, wxSize(dlgwd,wxDefaultCoord)) )
    {
        if (newname != oldname) frame->SetRuleName(string(newname.mb_str()));
    }
}

// =============================================================================

// define a modal dialog for editing multi-line text
// (essentially wxTextEntryDialog but with wxRESIZE_BORDER style)

class MultiLineDialog : public wxDialog
{
    public:
        MultiLineDialog(wxWindow* parent,
                        const wxString& caption,
                        const wxString& message,
                        const wxString& value);
    
        wxString GetValue() const { return m_value; }
        void OnOK(wxCommandEvent& event);
    
    private:
        wxTextCtrl* m_textctrl;
        wxString m_value;
    
        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MultiLineDialog, wxDialog)
    EVT_BUTTON (wxID_OK, MultiLineDialog::OnOK)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------

MultiLineDialog::MultiLineDialog(wxWindow *parent,
                                 const wxString& caption,
                                 const wxString& message,
                                 const wxString& value)
     : wxDialog(GetParentForModalDialog(parent, wxOK | wxCANCEL),
                wxID_ANY, caption, wxDefaultPosition, wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxBeginBusyCursor();

    wxBoxSizer* topsizer = new wxBoxSizer( wxVERTICAL );

    wxSizerFlags flagsBorder2;
    flagsBorder2.DoubleBorder();

    topsizer->Add(CreateTextSizer(message), flagsBorder2);

    m_textctrl = new wxTextCtrl(this, wxID_ANY, value,
                                wxDefaultPosition, wxSize(100,50), wxTE_MULTILINE);

    topsizer->Add(m_textctrl, wxSizerFlags(1).Expand().TripleBorder(wxLEFT | wxRIGHT));

    wxSizer* buttonSizer = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    topsizer->Add(buttonSizer, wxSizerFlags(flagsBorder2).Expand());

    SetAutoLayout(true);
    SetSizer(topsizer);

    topsizer->SetSizeHints(this);
    topsizer->Fit(this);

    m_textctrl->SetFocus();
    m_textctrl->SetSelection(0,0);      // probably nicer not to select all text

    wxEndBusyCursor();
}

// -----------------------------------------------------------------------------

void MultiLineDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    m_value = m_textctrl->GetValue();
    EndModal(wxID_OK);
}

// =============================================================================

void InfoPanel::ChangeDescription()
{
    wxString oldtext(frame->GetCurrentRDSystem()->GetDescription().c_str(),wxConvUTF8);
    wxString newtext;

    MultiLineDialog dialog(frame, _("Change description"), _("Enter the new description:"), oldtext);
    
    // position dialog box to left of linkrect
    wxPoint pos = ClientToScreen( wxPoint(html->linkrect.x, html->linkrect.y) );
    dialog.SetSize(pos.x - textdlgwd - 20, pos.y, textdlgwd, textdlght);

    if (dialog.ShowModal() == wxID_OK)
    {
        newtext = dialog.GetValue();
        if (newtext != oldtext) frame->SetDescription(string(newtext.mb_str()));
    }

    textdlgwd = dialog.GetRect().width;
    textdlght = dialog.GetRect().height;
}

// -----------------------------------------------------------------------------

void InfoPanel::ChangeFormula()
{
    wxString oldcode(frame->GetCurrentRDSystem()->GetFormula().c_str(),wxConvUTF8);
    wxString newcode;

    MultiLineDialog dialog(frame, _("Change formula"), _("Enter the new formula:"), oldcode);
    
    // position dialog box to left of linkrect
    wxPoint pos = ClientToScreen( wxPoint(html->linkrect.x, html->linkrect.y) );
    dialog.SetSize(pos.x - textdlgwd - 20, pos.y, textdlgwd, textdlght);

    if (dialog.ShowModal() == wxID_OK)
    {
        newcode = dialog.GetValue();
        if (newcode != oldcode) frame->SetFormula(string(newcode.mb_str()));
    }

    textdlgwd = dialog.GetRect().width;
    textdlght = dialog.GetRect().height;
}

// -----------------------------------------------------------------------------

void InfoPanel::ChangeNumChemicals()
{
    Warning(_("TODO!!!"));
}

// -----------------------------------------------------------------------------

void InfoPanel::ChangeDimensions()
{
    Warning(_("TODO!!!"));
}

// -----------------------------------------------------------------------------

void InfoPanel::ChangeBlockSize()
{
    Warning(_("TODO!!!"));
}

// -----------------------------------------------------------------------------

void InfoPanel::ChangeInfo(const wxString& label)
{
    if ( label.StartsWith(parameter_prefix) ) {
        ChangeParameter(label.AfterFirst(' '));

    } else if ( label == rule_name_label ) {
        ChangeRuleName();

    } else if ( label == description_label ) {
        ChangeDescription();

    } else if ( label == num_chemicals_label ) {
        ChangeNumChemicals();

    } else if ( label == formula_label ) {
        ChangeFormula();

    } else if ( label == dimensions_label ) {
        ChangeDimensions();

    } else if ( label == block_size_label ) {
        ChangeBlockSize();

    } else {
        Warning(_("Bug in ChangeInfo! Unexpected label: ") + label);
    }
}

// -----------------------------------------------------------------------------

void InfoPanel::OnSmallerButton(wxCommandEvent& WXUNUSED(event))
{
    if ( infofontsize > minfontsize ) {
        infofontsize--;
        html->ChangeFontSizes(infofontsize);
    }
}

// -----------------------------------------------------------------------------

void InfoPanel::OnBiggerButton(wxCommandEvent& WXUNUSED(event))
{
    if ( infofontsize < maxfontsize ) {
        infofontsize++;
        html->ChangeFontSizes(infofontsize);
    }
}

// -----------------------------------------------------------------------------

void InfoPanel::UpdateButtons()
{
    smallerbutt->Enable( infofontsize > minfontsize );
    biggerbutt->Enable( infofontsize < maxfontsize );
    
    html->ClearStatus();
    html->SetFocus();       // for keyboard shortcuts
}

// -----------------------------------------------------------------------------

bool InfoPanel::HtmlHasFocus()
{
    return html->HasFocus();
}

// -----------------------------------------------------------------------------

void InfoPanel::SelectAllText()
{
    html->SelectAll();
}

// -----------------------------------------------------------------------------

void InfoPanel::CopySelection()
{
    wxString text = html->SelectionToText();
    if (text.length() > 0) {
        CopyTextToClipboard(text);
    }
}

// -----------------------------------------------------------------------------

bool InfoPanel::DoKey(int key, int mods)
{
    // first look for keys that should be passed to the default handler
    if ( mods == wxMOD_NONE ) {
        if ( key == WXK_UP || key == WXK_DOWN || key == WXK_LEFT || key == WXK_RIGHT ||
             key == WXK_PAGEUP || key == WXK_PAGEDOWN ) {
            // let default handler see arrow keys and page up/down keys (for scroll bars)
            return false;
        }
    }
    
    // now do keyboard shortcuts specific to this pane
    if ( mods == wxMOD_NONE ) {
        if ( key == '+' || key == '=' || key == WXK_ADD ) {
            if ( infofontsize < maxfontsize ) {
                infofontsize++;
                html->ChangeFontSizes(infofontsize);
            }
            return true;
        }
        if ( key == '-' || key == WXK_SUBTRACT ) {
            if ( infofontsize > minfontsize ) {
                infofontsize--;
                html->ChangeFontSizes(infofontsize);
            }
            return true;
        }
    }
    
    // finally do other keyboard shortcuts
    frame->ProcessKey(key, mods);
    return true;
}
