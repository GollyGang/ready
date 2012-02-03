// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/numdlg.h>

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include "propgrid.h"

// -----------------------------------------------------------------------
// Menu IDs
// -----------------------------------------------------------------------

enum
{
    PGID = 1,
    TCID,
    ID_ABOUT,
    ID_QUIT,
    ID_APPENDPROP,
    ID_APPENDCAT,
    ID_INSERTPROP,
    ID_INSERTCAT,
    ID_ENABLE,
    ID_SETREADONLY,
    ID_HIDE,
    ID_DELETE,
    ID_DELETER,
    ID_DELETEALL,
    ID_UNSPECIFY,
    ID_ITERATE1,
    ID_ITERATE2,
    ID_ITERATE3,
    ID_ITERATE4,
    ID_CLEARMODIF,
    ID_FREEZE,
    ID_DUMPLIST,
    ID_COLOURSCHEME1,
    ID_COLOURSCHEME2,
    ID_COLOURSCHEME3,
    ID_CATCOLOURS,
    ID_SETBGCOLOUR,
    ID_SETBGCOLOURRECUR,
    ID_STATICLAYOUT,
    ID_POPULATE1,
    ID_POPULATE2,
    ID_COLLAPSE,
    ID_COLLAPSEALL,
    ID_GETVALUES,
    ID_SETVALUES,
    ID_SETVALUES2,
    ID_RUNTESTFULL,
    ID_RUNTESTPARTIAL,
    ID_FITCOLUMNS,
    ID_CHANGEFLAGSITEMS,
    ID_TESTINSERTCHOICE,
    ID_TESTDELETECHOICE,
    ID_INSERTPAGE,
    ID_REMOVEPAGE,
    ID_SETSPINCTRLEDITOR,
    ID_SETPROPERTYVALUE,
    ID_TESTREPLACE,
    ID_SETCOLUMNS,
    ID_TESTXRC,
    ID_ENABLECOMMONVALUES,
    ID_SELECTSTYLE,
    ID_SAVESTATE,
    ID_RESTORESTATE,
    ID_RUNMINIMAL,
    ID_ENABLELABELEDITING,
    ID_VETOCOLDRAG,
    ID_SHOWHEADER,
    ID_ONEXTENDEDKEYNAV
};

// -----------------------------------------------------------------------

void FormMain::OnResize( wxSizeEvent& event )
{
    if ( !m_pPropGridManager )
    {
        // this check is here so the frame layout can be tested
        // without creating propertygrid
        event.Skip();
        return;
    }

    // Update size properties
    int w, h;
    GetSize(&w,&h);

    wxPGProperty* id;
    wxPGProperty* p;

    // Must check if properties exist (as they may be deleted).

    // Using m_pPropGridManager, we can scan all pages automatically.
    p = m_pPropGridManager->GetPropertyByName( wxT("Width") );
    if ( p && !p->IsValueUnspecified() )
        m_pPropGridManager->SetPropertyValue( p, w );

    p = m_pPropGridManager->GetPropertyByName( wxT("Height") );
    if ( p && !p->IsValueUnspecified() )
        m_pPropGridManager->SetPropertyValue( p, h );

    id = m_pPropGridManager->GetPropertyByName ( wxT("Size") );
    if ( id )
        m_pPropGridManager->SetPropertyValue( id, WXVARIANT(wxSize(w,h)) );

    // Should always call event.Skip() in frame's SizeEvent handler
    event.Skip();
}

void FormMain::OnLabelTextChange( wxCommandEvent& WXUNUSED(event) )
{
// Uncomment following to allow property label modify in real-time
//    wxPGProperty& p = m_pPropGridManager->GetGrid()->GetSelection();
//    if ( !p.IsOk() ) return;
//    m_pPropGridManager->SetPropertyLabel( p, m_tcPropLabel->DoGetValue() );
}

// -----------------------------------------------------------------------

static const wxChar* _fs_windowstyle_labels[] = {
    wxT("wxSIMPLE_BORDER"),
    wxT("wxDOUBLE_BORDER"),
    wxT("wxSUNKEN_BORDER"),
    wxT("wxRAISED_BORDER"),
    wxT("wxNO_BORDER"),
    wxT("wxTRANSPARENT_WINDOW"),
    wxT("wxTAB_TRAVERSAL"),
    wxT("wxWANTS_CHARS"),
#if wxNO_FULL_REPAINT_ON_RESIZE
    wxT("wxNO_FULL_REPAINT_ON_RESIZE"),
#endif
    wxT("wxVSCROLL"),
    wxT("wxALWAYS_SHOW_SB"),
    wxT("wxCLIP_CHILDREN"),
#if wxFULL_REPAINT_ON_RESIZE
    wxT("wxFULL_REPAINT_ON_RESIZE"),
#endif
    (const wxChar*) NULL // terminator is always needed
};

static const long _fs_windowstyle_values[] = {
    wxSIMPLE_BORDER,
    wxDOUBLE_BORDER,
    wxSUNKEN_BORDER,
    wxRAISED_BORDER,
    wxNO_BORDER,
    wxTRANSPARENT_WINDOW,
    wxTAB_TRAVERSAL,
    wxWANTS_CHARS,
#if wxNO_FULL_REPAINT_ON_RESIZE
    wxNO_FULL_REPAINT_ON_RESIZE,
#endif
    wxVSCROLL,
    wxALWAYS_SHOW_SB,
    wxCLIP_CHILDREN,
#if wxFULL_REPAINT_ON_RESIZE
    wxFULL_REPAINT_ON_RESIZE
#endif
};

static const wxChar* _fs_framestyle_labels[] = {
    wxT("wxCAPTION"),
    wxT("wxMINIMIZE"),
    wxT("wxMAXIMIZE"),
    wxT("wxCLOSE_BOX"),
    wxT("wxSTAY_ON_TOP"),
    wxT("wxSYSTEM_MENU"),
    wxT("wxRESIZE_BORDER"),
    wxT("wxFRAME_TOOL_WINDOW"),
    wxT("wxFRAME_NO_TASKBAR"),
    wxT("wxFRAME_FLOAT_ON_PARENT"),
    wxT("wxFRAME_SHAPED"),
    (const wxChar*) NULL
};

static const long _fs_framestyle_values[] = {
    wxCAPTION,
    wxMINIMIZE,
    wxMAXIMIZE,
    wxCLOSE_BOX,
    wxSTAY_ON_TOP,
    wxSYSTEM_MENU,
    wxRESIZE_BORDER,
    wxFRAME_TOOL_WINDOW,
    wxFRAME_NO_TASKBAR,
    wxFRAME_FLOAT_ON_PARENT,
    wxFRAME_SHAPED
};

// -----------------------------------------------------------------------

void FormMain::OnTestXRC(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxT("Sorrt, not yet implemented"));
}

void FormMain::OnEnableCommonValues(wxCommandEvent& WXUNUSED(event))
{
    wxPGProperty* prop = m_pPropGridManager->GetSelection();
    if ( prop )
        prop->EnableCommonValue();
    else
        wxMessageBox(wxT("First select a property"));
}

void FormMain::PopulateWithExamples ()
{
    wxPropertyGridManager* pgman = m_pPropGridManager;
    wxPropertyGridPage* pg = pgman->GetPage(wxT("Examples"));
    wxPGProperty* pid;
    wxPGProperty* prop;

    // A string property that can be edited in a separate editor dialog.
    pg->Append( new wxLongStringProperty( wxT("LongStringProperty"), wxT("LongStringProp"),
        wxT("This is much longer string than the first one. Edit it by clicking the button.") ) );

}


//
// Handle events of the third page here.
class wxMyPropertyGridPage : public wxPropertyGridPage
{
public:

    // Return false here to indicate unhandled events should be
    // propagated to manager's parent, as normal.
    virtual bool IsHandlingAllEvents() const { return false; }

protected:

    virtual wxPGProperty* DoInsert( wxPGProperty* parent,
                                    int index,
                                    wxPGProperty* property )
    {
        return wxPropertyGridPage::DoInsert(parent,index,property);
    }

    void OnPropertySelect( wxPropertyGridEvent& event );
    void OnPropertyChanging( wxPropertyGridEvent& event );
    void OnPropertyChange( wxPropertyGridEvent& event );
    void OnPageChange( wxPropertyGridEvent& event );

private:
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxMyPropertyGridPage, wxPropertyGridPage)
    EVT_PG_SELECTED( wxID_ANY, wxMyPropertyGridPage::OnPropertySelect )
    EVT_PG_CHANGING( wxID_ANY, wxMyPropertyGridPage::OnPropertyChanging )
    EVT_PG_CHANGED( wxID_ANY, wxMyPropertyGridPage::OnPropertyChange )
    EVT_PG_PAGE_CHANGED( wxID_ANY, wxMyPropertyGridPage::OnPageChange )
END_EVENT_TABLE()


void wxMyPropertyGridPage::OnPropertySelect( wxPropertyGridEvent& WXUNUSED(event) )
{
    wxLogDebug(wxT("wxMyPropertyGridPage::OnPropertySelect()"));
}

void wxMyPropertyGridPage::OnPropertyChange( wxPropertyGridEvent& event )
{
    wxPGProperty* p = event.GetProperty();
    wxLogVerbose(wxT("wxMyPropertyGridPage::OnPropertyChange('%s', to value '%s')"),
               p->GetName().c_str(),
               p->GetDisplayedString().c_str());
}

void wxMyPropertyGridPage::OnPropertyChanging( wxPropertyGridEvent& event )
{
    wxPGProperty* p = event.GetProperty();
    wxLogVerbose(wxT("wxMyPropertyGridPage::OnPropertyChanging('%s', to value '%s')"),
               p->GetName().c_str(),
               event.GetValue().GetString().c_str());
}

void wxMyPropertyGridPage::OnPageChange( wxPropertyGridEvent& WXUNUSED(event) )
{
    wxLogDebug(wxT("wxMyPropertyGridPage::OnPageChange()"));
}


class wxPGKeyHandler : public wxEvtHandler
{
public:

    void OnKeyEvent( wxKeyEvent& event )
    {
        wxMessageBox(wxString::Format(wxT("%i"),event.GetKeyCode()));
        event.Skip();
    }
private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxPGKeyHandler,wxEvtHandler)
    EVT_KEY_DOWN( wxPGKeyHandler::OnKeyEvent )
END_EVENT_TABLE()


// -----------------------------------------------------------------------

void FormMain::InitPanel()
{
    if ( m_panel )
        m_panel->Destroy();

    wxWindow* panel = new wxPanel(this, wxID_ANY,
                                  wxPoint(0, 0), wxSize(400, 400),
                                  wxTAB_TRAVERSAL);
    m_panel = panel;

    // Column
    wxBoxSizer* topSizer = new wxBoxSizer ( wxVERTICAL );

    m_topSizer = topSizer;
}

void FormMain::FinalizePanel( bool wasCreated )
{
    // Button for tab traversal testing
    m_topSizer->Add( new wxButton(m_panel, wxID_ANY,
                     wxS("Should be able to move here with Tab")),
                     0, wxEXPAND );

    m_panel->SetSizer( m_topSizer );
    m_topSizer->SetSizeHints( m_panel );

    wxBoxSizer* panelSizer = new wxBoxSizer( wxHORIZONTAL );
    panelSizer->Add( m_panel, 1, wxEXPAND|wxFIXED_MINSIZE );

    SetSizer( panelSizer );
    panelSizer->SetSizeHints( this );

    if ( wasCreated )
        FinalizeFramePosition();
}

void FormMain::PopulateGrid()
{
    wxPropertyGridManager* pgman = m_pPropGridManager;


    wxPropertyGridPage* myPage = new wxMyPropertyGridPage();
    myPage->Append( new wxIntProperty ( wxT("IntProperty"), wxPG_LABEL, 12345678 ) );

    // Use wxMyPropertyGridPage (see above) to test the
    // custom wxPropertyGridPage feature.
    pgman->AddPage(wxT("Examples"),wxNullBitmap,myPage);

    PopulateWithExamples();
}

void FormMain::CreateGrid( int style, int extraStyle )
{
    //
    // This function (re)creates the property grid in our sample
    //

    if ( style == -1 )
        style = // default style
                wxPG_BOLD_MODIFIED |
                wxPG_SPLITTER_AUTO_CENTER |
                wxPG_AUTO_SORT |
                //wxPG_HIDE_MARGIN|wxPG_STATIC_SPLITTER |
                //wxPG_TOOLTIPS |
                //wxPG_HIDE_CATEGORIES |
                //wxPG_LIMITED_EDITING |
                wxPG_TOOLBAR |
                wxPG_DESCRIPTION;

    if ( extraStyle == -1 )
        // default extra style
        extraStyle = wxPG_EX_MODE_BUTTONS |
                     wxPG_EX_MULTIPLE_SELECTION;
                //| wxPG_EX_AUTO_UNSPECIFIED_VALUES
                //| wxPG_EX_GREY_LABEL_WHEN_DISABLED
                //| wxPG_EX_NATIVE_DOUBLE_BUFFERING
                //| wxPG_EX_HELP_AS_TOOLTIPS

    bool wasCreated = m_panel ? false : true;

    InitPanel();

    //
    // This shows how to combine two static choice descriptors
    m_combinedFlags.Add( _fs_windowstyle_labels, _fs_windowstyle_values );
    m_combinedFlags.Add( _fs_framestyle_labels, _fs_framestyle_values );

    wxPropertyGridManager* pgman = m_pPropGridManager =
        new wxPropertyGridManager(m_panel,
                                  // Don't change this into wxID_ANY in the sample, or the
                                  // event handling will obviously be broken.
                                  PGID, /*wxID_ANY*/
                                  wxDefaultPosition,
                                  wxSize(100, 100), // FIXME: wxDefaultSize gives assertion in propgrid.
                                                    // But calling SetInitialSize in manager changes the code
                                                    // order to the grid gets created immediately, before SetExtraStyle
                                                    // is called.
                                  style );

    m_propGrid = pgman->GetGrid();

    pgman->SetExtraStyle(extraStyle);

    // This is the default validation failure behaviour
    m_pPropGridManager->SetValidationFailureBehavior( wxPG_VFB_MARK_CELL |
                                                      wxPG_VFB_SHOW_MESSAGEBOX );

    m_pPropGridManager->GetGrid()->SetVerticalSpacing( 2 );

    //
    // Set somewhat different unspecified value appearance
    wxPGCell cell;
    cell.SetText("Unspecified");
    cell.SetFgCol(*wxLIGHT_GREY);
    m_propGrid->SetUnspecifiedValueAppearance(cell);

    PopulateGrid();

    // Change some attributes in all properties
    //pgman->SetPropertyAttributeAll(wxPG_BOOL_USE_DOUBLE_CLICK_CYCLING,true);
    //pgman->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX,true);

    //m_pPropGridManager->SetSplitterLeft(true);
    //m_pPropGridManager->SetSplitterPosition(137);

    /*
    // This would setup event handling without event table entries
    Connect(m_pPropGridManager->GetId(), wxEVT_PG_SELECTED,
            wxPropertyGridEventHandler(FormMain::OnPropertyGridSelect) );
    Connect(m_pPropGridManager->GetId(), wxEVT_PG_CHANGED,
            wxPropertyGridEventHandler(FormMain::OnPropertyGridChange) );
    */

    m_topSizer->Add( m_pPropGridManager, 1, wxEXPAND );

    FinalizePanel(wasCreated);
}

// -----------------------------------------------------------------------

FormMain::FormMain(const wxString& title, const wxPoint& pos, const wxSize& size) :
           wxFrame((wxFrame *)NULL, -1, title, pos, size,
               (wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCAPTION|
                wxTAB_TRAVERSAL|wxCLOSE_BOX|wxNO_FULL_REPAINT_ON_RESIZE) )
{
    m_propGrid = NULL;
    m_panel = NULL;

#ifdef __WXMAC__
    // we need this in order to allow the about menu relocation, since ABOUT is
    // not the default id of the about menu
    wxApp::s_macAboutMenuItemId = ID_ABOUT;
#endif

#if wxUSE_IMAGE
    // This is here to really test the wxImageFileProperty.
    wxInitAllImageHandlers();
#endif

    // Register all editors (SpinCtrl etc.)
    m_pPropGridManager->RegisterAdditionalEditors();

    CreateGrid( // style
                wxPG_BOLD_MODIFIED |
                wxPG_SPLITTER_AUTO_CENTER |
                wxPG_AUTO_SORT |
                //wxPG_HIDE_MARGIN|wxPG_STATIC_SPLITTER |
                //wxPG_TOOLTIPS |
                //wxPG_HIDE_CATEGORIES |
                //wxPG_LIMITED_EDITING |
                wxPG_TOOLBAR |
                wxPG_DESCRIPTION,
                // extra style
                wxPG_EX_MODE_BUTTONS |
                wxPG_EX_MULTIPLE_SELECTION
                //| wxPG_EX_AUTO_UNSPECIFIED_VALUES
                //| wxPG_EX_GREY_LABEL_WHEN_DISABLED
                //| wxPG_EX_NATIVE_DOUBLE_BUFFERING
                //| wxPG_EX_HELP_AS_TOOLTIPS
              );

    //
    // Create menubar
    wxMenu *menuFile = new wxMenu(wxEmptyString, wxMENU_TEAROFF);
    wxMenu *menuTry = new wxMenu;
    wxMenu *menuTools1 = new wxMenu;
    wxMenu *menuTools2 = new wxMenu;
    wxMenu *menuHelp = new wxMenu;

    menuHelp->Append(ID_ABOUT, wxT("&About..."), wxT("Show about dialog") );

    menuTools1->Append(ID_APPENDPROP, wxT("Append New Property") );
    menuTools1->Append(ID_APPENDCAT, wxT("Append New Category\tCtrl-S") );
    menuTools1->AppendSeparator();
    menuTools1->Append(ID_INSERTPROP, wxT("Insert New Property\tCtrl-Q") );
    menuTools1->Append(ID_INSERTCAT, wxT("Insert New Category\tCtrl-W") );
    menuTools1->AppendSeparator();
    menuTools1->Append(ID_DELETE, wxT("Delete Selected") );
    menuTools1->Append(ID_DELETER, wxT("Delete Random") );
    menuTools1->Append(ID_DELETEALL, wxT("Delete All") );
    menuTools1->AppendSeparator();
    menuTools1->Append(ID_SETBGCOLOUR, wxT("Set Bg Colour") );
    menuTools1->Append(ID_SETBGCOLOURRECUR, wxT("Set Bg Colour (Recursively)") );
    menuTools1->Append(ID_UNSPECIFY, "Set Value to Unspecified");
    menuTools1->AppendSeparator();
    m_itemEnable = menuTools1->Append(ID_ENABLE, wxT("Enable"),
        wxT("Toggles item's enabled state.") );
    m_itemEnable->Enable( FALSE );
    menuTools1->Append(ID_HIDE, "Hide", "Hides a property" );
    menuTools1->Append(ID_SETREADONLY, "Set as Read-Only",
                       "Set property as read-only" );

    menuTools2->Append(ID_ITERATE1, wxT("Iterate Over Properties") );
    menuTools2->Append(ID_ITERATE2, wxT("Iterate Over Visible Items") );
    menuTools2->Append(ID_ITERATE3, wxT("Reverse Iterate Over Properties") );
    menuTools2->Append(ID_ITERATE4, wxT("Iterate Over Categories") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_ONEXTENDEDKEYNAV, "Extend Keyboard Navigation",
                       "This will set Enter to navigate to next property, "
                       "and allows arrow keys to navigate even when in "
                       "editor control.");
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_SETPROPERTYVALUE, wxT("Set Property Value") );
    menuTools2->Append(ID_CLEARMODIF, wxT("Clear Modified Status"), wxT("Clears wxPG_MODIFIED flag from all properties.") );
    menuTools2->AppendSeparator();
    m_itemFreeze = menuTools2->AppendCheckItem(ID_FREEZE, wxT("Freeze"),
        wxT("Disables painting, auto-sorting, etc.") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_DUMPLIST, wxT("Display Values as wxVariant List"), wxT("Tests GetAllValues method and wxVariant conversion.") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_GETVALUES, wxT("Get Property Values"), wxT("Stores all property values.") );
    menuTools2->Append(ID_SETVALUES, wxT("Set Property Values"), wxT("Reverts property values to those last stored.") );
    menuTools2->Append(ID_SETVALUES2, wxT("Set Property Values 2"), wxT("Adds property values that should not initially be as items (so new items are created).") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_SAVESTATE, wxT("Save Editable State") );
    menuTools2->Append(ID_RESTORESTATE, wxT("Restore Editable State") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_ENABLECOMMONVALUES, wxT("Enable Common Value"),
        wxT("Enable values that are common to all properties, for selected property."));
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_COLLAPSE, wxT("Collapse Selected") );
    menuTools2->Append(ID_COLLAPSEALL, wxT("Collapse All") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_INSERTPAGE, wxT("Add Page") );
    menuTools2->Append(ID_REMOVEPAGE, wxT("Remove Page") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_FITCOLUMNS, wxT("Fit Columns") );
    m_itemVetoDragging =
        menuTools2->AppendCheckItem(ID_VETOCOLDRAG,
                                    "Veto Column Dragging");
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_CHANGEFLAGSITEMS, wxT("Change Children of FlagsProp") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_TESTINSERTCHOICE, wxT("Test InsertPropertyChoice") );
    menuTools2->Append(ID_TESTDELETECHOICE, wxT("Test DeletePropertyChoice") );
    menuTools2->AppendSeparator();
    menuTools2->Append(ID_SETSPINCTRLEDITOR, wxT("Use SpinCtrl Editor") );
    menuTools2->Append(ID_TESTREPLACE, wxT("Test ReplaceProperty") );

    menuTry->Append(ID_SELECTSTYLE, wxT("Set Window Style"),
        wxT("Select window style flags used by the grid."));
    menuTry->Append(ID_ENABLELABELEDITING, "Enable label editing",
        "This calls wxPropertyGrid::MakeColumnEditable(0)");
    menuTry->AppendCheckItem(ID_SHOWHEADER,
        "Enable header",
        "This calls wxPropertyGridManager::ShowHeader()");
    menuTry->AppendSeparator();
    menuTry->AppendRadioItem( ID_COLOURSCHEME1, wxT("Standard Colour Scheme") );
    menuTry->AppendRadioItem( ID_COLOURSCHEME2, wxT("White Colour Scheme") );
    menuTry->AppendRadioItem( ID_COLOURSCHEME3, wxT(".NET Colour Scheme") );
    menuTry->AppendSeparator();
    m_itemCatColours = menuTry->AppendCheckItem(ID_CATCOLOURS, wxT("Category Specific Colours"),
        wxT("Switches between category-specific cell colours and default scheme (actually done using SetPropertyTextColour and SetPropertyBackgroundColour).") );
    menuTry->AppendSeparator();
    menuTry->AppendCheckItem(ID_STATICLAYOUT, wxT("Static Layout"),
        wxT("Switches between user-modifiedable and static layouts.") );
    menuTry->Append(ID_SETCOLUMNS, wxT("Set Number of Columns") );
    menuTry->AppendSeparator();
    menuTry->Append(ID_TESTXRC, wxT("Display XRC sample") );
    menuTry->AppendSeparator();
    menuTry->Append(ID_RUNTESTFULL, wxT("Run Tests (full)") );
    menuTry->Append(ID_RUNTESTPARTIAL, wxT("Run Tests (fast)") );

    menuFile->Append(ID_RUNMINIMAL, wxT("Run Minimal Sample") );
    menuFile->AppendSeparator();
    menuFile->Append(ID_QUIT, wxT("E&xit\tAlt-X"), wxT("Quit this program") );

    // Now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(menuFile, wxT("&File") );
    menuBar->Append(menuTry, wxT("&Try These!") );
    menuBar->Append(menuTools1, wxT("&Basic") );
    menuBar->Append(menuTools2, wxT("&Advanced") );
    menuBar->Append(menuHelp, wxT("&Help") );

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

#if wxUSE_STATUSBAR
    // create a status bar
    CreateStatusBar(1);
    SetStatusText(wxEmptyString);
#endif // wxUSE_STATUSBAR

    FinalizeFramePosition();

#if wxUSE_LOGWINDOW
    // Create log window
    m_logWindow = new wxLogWindow(this, "Log Messages", false);
    m_logWindow->GetFrame()->Move(GetPosition().x + GetSize().x + 10,
                                  GetPosition().y);
    m_logWindow->Show();
#endif
}

void FormMain::FinalizeFramePosition()
{
    wxSize frameSize((wxSystemSettings::GetMetric(wxSYS_SCREEN_X)/10)*4,
                     (wxSystemSettings::GetMetric(wxSYS_SCREEN_Y)/10)*8);

    if ( frameSize.x > 500 )
        frameSize.x = 500;

    SetSize(frameSize);

    Centre();
}

//
// Normally, wxPropertyGrid does not check whether item with identical
// label already exists. However, since in this sample we use labels for
// identifying properties, we have to be sure not to generate identical
// labels.
//
void GenerateUniquePropertyLabel( wxPropertyGridManager* pg, wxString& baselabel )
{
    int count = -1;
    wxString newlabel;

    if ( pg->GetPropertyByLabel( baselabel ) )
    {
        for (;;)
        {
            count++;
            newlabel.Printf(wxT("%s%i"),baselabel.c_str(),count);
            if ( !pg->GetPropertyByLabel( newlabel ) ) break;
        }
    }

    if ( count >= 0 )
    {
        baselabel = newlabel;
    }
}

// -----------------------------------------------------------------------

void FormMain::OnInsertPropClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString propLabel;

    if ( !m_pPropGridManager->GetGrid()->GetRoot()->GetChildCount() )
    {
        wxMessageBox(wxT("No items to relate - first add some with Append."));
        return;
    }

    wxPGProperty* id = m_pPropGridManager->GetGrid()->GetSelection();
    if ( !id )
    {
        wxMessageBox(wxT("First select a property - new one will be inserted right before that."));
        return;
    }
    if ( propLabel.Len() < 1 ) propLabel = wxT("Property");

    GenerateUniquePropertyLabel( m_pPropGridManager, propLabel );

    m_pPropGridManager->Insert( m_pPropGridManager->GetPropertyParent(id),
                                id->GetIndexInParent(),
                                new wxStringProperty(propLabel) );

}

// -----------------------------------------------------------------------

void FormMain::OnAppendPropClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString propLabel;

    if ( propLabel.Len() < 1 ) propLabel = wxT("Property");

    GenerateUniquePropertyLabel( m_pPropGridManager, propLabel );

    m_pPropGridManager->Append( new wxStringProperty(propLabel) );

    m_pPropGridManager->Refresh();
}

// -----------------------------------------------------------------------

void FormMain::OnClearClick( wxCommandEvent& WXUNUSED(event) )
{
    m_pPropGridManager->GetGrid()->Clear();
}

// -----------------------------------------------------------------------

void FormMain::OnAppendCatClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString propLabel;

    if ( propLabel.Len() < 1 ) propLabel = wxT("Category");

    GenerateUniquePropertyLabel( m_pPropGridManager, propLabel );

    m_pPropGridManager->Append( new wxPropertyCategory (propLabel) );

    m_pPropGridManager->Refresh();

}

// -----------------------------------------------------------------------

void FormMain::OnInsertCatClick( wxCommandEvent& WXUNUSED(event) )
{
    wxString propLabel;

    if ( !m_pPropGridManager->GetGrid()->GetRoot()->GetChildCount() )
    {
        wxMessageBox(wxT("No items to relate - first add some with Append."));
        return;
    }

    wxPGProperty* id = m_pPropGridManager->GetGrid()->GetSelection();
    if ( !id )
    {
        wxMessageBox(wxT("First select a property - new one will be inserted right before that."));
        return;
    }

    if ( propLabel.Len() < 1 ) propLabel = wxT("Category");

    GenerateUniquePropertyLabel( m_pPropGridManager, propLabel );

    m_pPropGridManager->Insert( m_pPropGridManager->GetPropertyParent(id),
                                id->GetIndexInParent(),
                                new wxPropertyCategory (propLabel) );
}

// -----------------------------------------------------------------------

void FormMain::OnDelPropClick( wxCommandEvent& WXUNUSED(event) )
{
    wxPGProperty* id = m_pPropGridManager->GetGrid()->GetSelection();
    if ( !id )
    {
        wxMessageBox(wxT("First select a property."));
        return;
    }

    m_pPropGridManager->DeleteProperty( id );
}

// -----------------------------------------------------------------------

void FormMain::OnDelPropRClick( wxCommandEvent& WXUNUSED(event) )
{
    // Delete random property
    wxPGProperty* p = m_pPropGridManager->GetGrid()->GetRoot();

    for (;;)
    {
        if ( !p->IsCategory() )
        {
            m_pPropGridManager->DeleteProperty( p );
            break;
        }

        if ( !p->GetChildCount() )
            break;

        int n = rand() % ((int)p->GetChildCount());

        p = p->Item(n);
    }
}

// -----------------------------------------------------------------------

void FormMain::OnContextMenu( wxContextMenuEvent& event )
{
    wxLogDebug(wxT("FormMain::OnContextMenu(%i,%i)"),
        event.GetPosition().x,event.GetPosition().y);

    wxUnusedVar(event);

    //event.Skip();
}

// -----------------------------------------------------------------------

int IterateMessage( wxPGProperty* prop )
{
    wxString s;

    s.Printf( wxT("\"%s\" class = %s, valuetype = %s"), prop->GetLabel().c_str(),
        prop->GetClassInfo()->GetClassName(), prop->GetValueType().c_str() );

    return wxMessageBox( s, wxT("Iterating... (press CANCEL to end)"), wxOK|wxCANCEL );
}

// -----------------------------------------------------------------------

void FormMain::OnIterate1Click( wxCommandEvent& WXUNUSED(event) )
{
    wxPropertyGridIterator it;

    for ( it = m_pPropGridManager->GetCurrentPage()->
            GetIterator();
          !it.AtEnd();
          it++ )
    {
        wxPGProperty* p = *it;
        int res = IterateMessage( p );
        if ( res == wxCANCEL ) break;
    }
}

// -----------------------------------------------------------------------

void FormMain::OnIterate2Click( wxCommandEvent& WXUNUSED(event) )
{
    wxPropertyGridIterator it;

    for ( it = m_pPropGridManager->GetCurrentPage()->
            GetIterator( wxPG_ITERATE_VISIBLE );
          !it.AtEnd();
          it++ )
    {
        wxPGProperty* p = *it;

        int res = IterateMessage( p );
        if ( res == wxCANCEL ) break;
    }
}

// -----------------------------------------------------------------------

void FormMain::OnIterate3Click( wxCommandEvent& WXUNUSED(event) )
{
    // iterate over items in reverse order
    wxPropertyGridIterator it;

    for ( it = m_pPropGridManager->GetCurrentPage()->
                GetIterator( wxPG_ITERATE_DEFAULT, wxBOTTOM );
          !it.AtEnd();
          it-- )
    {
        wxPGProperty* p = *it;

        int res = IterateMessage( p );
        if ( res == wxCANCEL ) break;
    }
}

// -----------------------------------------------------------------------

void FormMain::OnIterate4Click( wxCommandEvent& WXUNUSED(event) )
{
    wxPropertyGridIterator it;

    for ( it = m_pPropGridManager->GetCurrentPage()->
            GetIterator( wxPG_ITERATE_CATEGORIES );
          !it.AtEnd();
          it++ )
    {
        wxPGProperty* p = *it;

        int res = IterateMessage( p );
        if ( res == wxCANCEL ) break;
    }
}

// -----------------------------------------------------------------------

void FormMain::OnExtendedKeyNav( wxCommandEvent& WXUNUSED(event) )
{
    // Use AddActionTrigger() and DedicateKey() to set up Enter,
    // Up, and Down keys for navigating between properties.
    wxPropertyGrid* propGrid = m_pPropGridManager->GetGrid();

    propGrid->AddActionTrigger(wxPG_ACTION_NEXT_PROPERTY,
                               WXK_RETURN);
    propGrid->DedicateKey(WXK_RETURN);

    // Up and Down keys are alredy associated with navigation,
    // but we must also prevent them from being eaten by
    // editor controls.
    propGrid->DedicateKey(WXK_UP);
    propGrid->DedicateKey(WXK_DOWN);
}

// -----------------------------------------------------------------------

void FormMain::OnFitColumnsClick( wxCommandEvent& WXUNUSED(event) )
{
    wxPropertyGridPage* page = m_pPropGridManager->GetCurrentPage();

    // Remove auto-centering
    m_pPropGridManager->SetWindowStyle( m_pPropGridManager->GetWindowStyle() & ~wxPG_SPLITTER_AUTO_CENTER);

    // Grow manager size just prior fit - otherwise
    // column information may be lost.
    wxSize oldGridSize = m_pPropGridManager->GetGrid()->GetClientSize();
    wxSize oldFullSize = GetSize();
    SetSize(1000, oldFullSize.y);

    wxSize newSz = page->FitColumns();

    int dx = oldFullSize.x - oldGridSize.x;
    int dy = oldFullSize.y - oldGridSize.y;

    newSz.x += dx;
    newSz.y += dy;

    SetSize(newSz);
}

// -----------------------------------------------------------------------

void FormMain::OnChangeFlagsPropItemsClick( wxCommandEvent& WXUNUSED(event) )
{
    wxPGProperty* p = m_pPropGridManager->GetPropertyByName(wxT("Window Styles"));

    wxPGChoices newChoices;

    newChoices.Add(wxT("Fast"),0x1);
    newChoices.Add(wxT("Powerful"),0x2);
    newChoices.Add(wxT("Safe"),0x4);
    newChoices.Add(wxT("Sleek"),0x8);

    p->SetChoices(newChoices);
}

// -----------------------------------------------------------------------

void FormMain::OnEnableDisable( wxCommandEvent& )
{
    wxPGProperty* id = m_pPropGridManager->GetGrid()->GetSelection();
    if ( !id )
    {
        wxMessageBox(wxT("First select a property."));
        return;
    }

    if ( m_pPropGridManager->IsPropertyEnabled( id ) )
    {
        m_pPropGridManager->DisableProperty ( id );
        m_itemEnable->SetItemLabel( wxT("Enable") );
    }
    else
    {
        m_pPropGridManager->EnableProperty ( id );
        m_itemEnable->SetItemLabel( wxT("Disable") );
    }
}

// -----------------------------------------------------------------------

void FormMain::OnSetReadOnly( wxCommandEvent& WXUNUSED(event) )
{
    wxPGProperty* p = m_pPropGridManager->GetGrid()->GetSelection();
    if ( !p )
    {
        wxMessageBox(wxT("First select a property."));
        return;
    }
    m_pPropGridManager->SetPropertyReadOnly(p);
}

// -----------------------------------------------------------------------

void FormMain::OnHide( wxCommandEvent& WXUNUSED(event) )
{
    wxPGProperty* id = m_pPropGridManager->GetGrid()->GetSelection();
    if ( !id )
    {
        wxMessageBox(wxT("First select a property."));
        return;
    }

    m_pPropGridManager->HideProperty( id, true );
}

// -----------------------------------------------------------------------

#include "wx/colordlg.h"

void
FormMain::OnSetBackgroundColour( wxCommandEvent& event )
{
    wxPropertyGrid* pg = m_pPropGridManager->GetGrid();
    wxPGProperty* prop = pg->GetSelection();
    if ( !prop )
    {
        wxMessageBox(wxT("First select a property."));
        return;
    }

    wxColour col = ::wxGetColourFromUser(this, *wxWHITE, "Choose colour");

    if ( col.IsOk() )
    {
        bool recursively = (event.GetId()==ID_SETBGCOLOURRECUR) ? true : false;
        pg->SetPropertyBackgroundColour(prop, col, recursively);
    }
}

// -----------------------------------------------------------------------

void FormMain::OnInsertPage( wxCommandEvent& WXUNUSED(event) )
{
    m_pPropGridManager->AddPage(wxT("New Page"));
}

// -----------------------------------------------------------------------

void FormMain::OnRemovePage( wxCommandEvent& WXUNUSED(event) )
{
    m_pPropGridManager->RemovePage(m_pPropGridManager->GetSelectedPage());
}

// -----------------------------------------------------------------------

void FormMain::OnSaveState( wxCommandEvent& WXUNUSED(event) )
{
    m_savedState = m_pPropGridManager->SaveEditableState();
    wxLogDebug(wxT("Saved editable state string: \"%s\""), m_savedState.c_str());
}

// -----------------------------------------------------------------------

void FormMain::OnRestoreState( wxCommandEvent& WXUNUSED(event) )
{
    m_pPropGridManager->RestoreEditableState(m_savedState);
}

// -----------------------------------------------------------------------

void FormMain::OnTestReplaceClick( wxCommandEvent& WXUNUSED(event) )
{
    wxPGProperty* pgId = m_pPropGridManager->GetSelection();
    if ( pgId )
    {
        wxPGChoices choices;
        choices.Add(wxT("Flag 0"),0x0001);
        choices.Add(wxT("Flag 1"),0x0002);
        choices.Add(wxT("Flag 2"),0x0004);
        choices.Add(wxT("Flag 3"),0x0008);
        wxPGProperty* newId = m_pPropGridManager->ReplaceProperty( pgId,
            new wxFlagsProperty(wxT("ReplaceFlagsProperty"), wxPG_LABEL, choices, 0x0003) );
        m_pPropGridManager->SetPropertyAttribute( newId,
                                              wxPG_BOOL_USE_CHECKBOX,
                                              true,
                                              wxPG_RECURSE );
    }
    else
        wxMessageBox(wxT("First select a property"));
}

// -----------------------------------------------------------------------

void FormMain::OnClearModifyStatusClick( wxCommandEvent& WXUNUSED(event) )
{
    m_pPropGridManager->ClearModifiedStatus();
}

// -----------------------------------------------------------------------

// Freeze check-box checked?
void FormMain::OnFreezeClick( wxCommandEvent& event )
{
    if ( !m_pPropGridManager ) return;

    if ( event.IsChecked() )
    {
        if ( !m_pPropGridManager->IsFrozen() )
        {
            m_pPropGridManager->Freeze();
        }
    }
    else
    {
        if ( m_pPropGridManager->IsFrozen() )
        {
            m_pPropGridManager->Thaw();
            m_pPropGridManager->Refresh();
        }
    }
}

// -----------------------------------------------------------------------

void FormMain::OnEnableLabelEditing( wxCommandEvent& WXUNUSED(event) )
{
    m_propGrid->MakeColumnEditable(0);
}

// -----------------------------------------------------------------------

void FormMain::OnShowHeader( wxCommandEvent& event )
{
    m_pPropGridManager->ShowHeader(event.IsChecked());
    m_pPropGridManager->SetColumnTitle(2, _("Units"));
}

// -----------------------------------------------------------------------

void FormMain::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxString msg;
    msg.Printf( wxT("wxPropertyGrid Sample")
#if wxUSE_UNICODE
  #if defined(wxUSE_UNICODE_UTF8) && wxUSE_UNICODE_UTF8
                wxT(" <utf-8>")
  #else
                wxT(" <unicode>")
  #endif
#else
                wxT(" <ansi>")
#endif
#ifdef __WXDEBUG__
                wxT(" <debug>")
#else
                wxT(" <release>")
#endif
                wxT("\n\n")
                wxT("Programmed by %s\n\n")
                wxT("Using %s\n\n"),
            wxT("Jaakko Salli"), wxVERSION_STRING
            );

    wxMessageBox(msg, wxT("About"), wxOK | wxICON_INFORMATION, this);
}

// -----------------------------------------------------------------------

void FormMain::OnColourScheme( wxCommandEvent& event )
{
    int id = event.GetId();
    if ( id == ID_COLOURSCHEME1 )
    {
        m_pPropGridManager->GetGrid()->ResetColours();
    }
    else if ( id == ID_COLOURSCHEME2 )
    {
        // white
        wxColour my_grey_1(212,208,200);
        wxColour my_grey_3(113,111,100);
        m_pPropGridManager->Freeze();
        m_pPropGridManager->GetGrid()->SetMarginColour( *wxWHITE );
        m_pPropGridManager->GetGrid()->SetCaptionBackgroundColour( *wxWHITE );
        m_pPropGridManager->GetGrid()->SetCellBackgroundColour( *wxWHITE );
        m_pPropGridManager->GetGrid()->SetCellTextColour( my_grey_3 );
        m_pPropGridManager->GetGrid()->SetLineColour( my_grey_1 ); //wxColour(160,160,160)
        m_pPropGridManager->Thaw();
    }
    else if ( id == ID_COLOURSCHEME3 )
    {
        // .NET
        wxColour my_grey_1(212,208,200);
        wxColour my_grey_2(236,233,216);
        m_pPropGridManager->Freeze();
        m_pPropGridManager->GetGrid()->SetMarginColour( my_grey_1 );
        m_pPropGridManager->GetGrid()->SetCaptionBackgroundColour( my_grey_1 );
        m_pPropGridManager->GetGrid()->SetLineColour( my_grey_1 );
        m_pPropGridManager->Thaw();
    }
}

// -----------------------------------------------------------------------

void FormMain::OnCatColours( wxCommandEvent& event )
{
    wxPropertyGrid* pg = m_pPropGridManager->GetGrid();
    m_pPropGridManager->Freeze();

    if ( event.IsChecked() )
    {
        // Set custom colours.
        pg->SetPropertyTextColour( wxT("Appearance"), wxColour(255,0,0), false );
        pg->SetPropertyBackgroundColour( wxT("Appearance"), wxColour(255,255,183) );
        pg->SetPropertyTextColour( wxT("Appearance"), wxColour(255,0,183) );
        pg->SetPropertyTextColour( wxT("PositionCategory"), wxColour(0,255,0), false );
        pg->SetPropertyBackgroundColour( wxT("PositionCategory"), wxColour(255,226,190) );
        pg->SetPropertyTextColour( wxT("PositionCategory"), wxColour(255,0,190) );
        pg->SetPropertyTextColour( wxT("Environment"), wxColour(0,0,255), false );
        pg->SetPropertyBackgroundColour( wxT("Environment"), wxColour(208,240,175) );
        pg->SetPropertyTextColour( wxT("Environment"), wxColour(255,255,255) );
        pg->SetPropertyBackgroundColour( wxT("More Examples"), wxColour(172,237,255) );
        pg->SetPropertyTextColour( wxT("More Examples"), wxColour(172,0,255) );
    }
    else
    {
        // Revert to original.
        pg->SetPropertyColoursToDefault( wxT("Appearance") );
        pg->SetPropertyColoursToDefault( wxT("PositionCategory") );
        pg->SetPropertyColoursToDefault( wxT("Environment") );
        pg->SetPropertyColoursToDefault( wxT("More Examples") );
    }
    m_pPropGridManager->Thaw();
    m_pPropGridManager->Refresh();
}

// -----------------------------------------------------------------------

#define ADD_FLAG(FLAG) \
        chs.Add(wxT(#FLAG)); \
        vls.Add(FLAG); \
        if ( (flags & FLAG) == FLAG ) sel.Add(ind); \
        ind++;

void FormMain::OnSelectStyle( wxCommandEvent& WXUNUSED(event) )
{
    int style = 0;
    int extraStyle = 0;

    {
        wxArrayString chs;
        wxArrayInt vls;
        wxArrayInt sel;
        unsigned int ind = 0;
        int flags = m_pPropGridManager->GetWindowStyle();
        ADD_FLAG(wxPG_HIDE_CATEGORIES)
        ADD_FLAG(wxPG_AUTO_SORT)
        ADD_FLAG(wxPG_BOLD_MODIFIED)
        ADD_FLAG(wxPG_SPLITTER_AUTO_CENTER)
        ADD_FLAG(wxPG_TOOLTIPS)
        ADD_FLAG(wxPG_STATIC_SPLITTER)
        ADD_FLAG(wxPG_HIDE_MARGIN)
        ADD_FLAG(wxPG_LIMITED_EDITING)
        ADD_FLAG(wxPG_TOOLBAR)
        ADD_FLAG(wxPG_DESCRIPTION)
        ADD_FLAG(wxPG_NO_INTERNAL_BORDER)
        wxMultiChoiceDialog dlg( this, wxT("Select window styles to use"),
                                 wxT("wxPropertyGrid Window Style"), chs );
        dlg.SetSelections(sel);
        if ( dlg.ShowModal() == wxID_CANCEL )
            return;

        flags = 0;
        sel = dlg.GetSelections();
        for ( ind = 0; ind < sel.size(); ind++ )
            flags |= vls[sel[ind]];

        style = flags;
    }

    {
        wxArrayString chs;
        wxArrayInt vls;
        wxArrayInt sel;
        unsigned int ind = 0;
        int flags = m_pPropGridManager->GetExtraStyle();
        ADD_FLAG(wxPG_EX_INIT_NOCAT)
        ADD_FLAG(wxPG_EX_NO_FLAT_TOOLBAR)
        ADD_FLAG(wxPG_EX_MODE_BUTTONS)
        ADD_FLAG(wxPG_EX_HELP_AS_TOOLTIPS)
        ADD_FLAG(wxPG_EX_NATIVE_DOUBLE_BUFFERING)
        ADD_FLAG(wxPG_EX_AUTO_UNSPECIFIED_VALUES)
        ADD_FLAG(wxPG_EX_WRITEONLY_BUILTIN_ATTRIBUTES)
        ADD_FLAG(wxPG_EX_HIDE_PAGE_BUTTONS)
        ADD_FLAG(wxPG_EX_MULTIPLE_SELECTION)
        ADD_FLAG(wxPG_EX_ENABLE_TLP_TRACKING)
        ADD_FLAG(wxPG_EX_NO_TOOLBAR_DIVIDER)
        ADD_FLAG(wxPG_EX_TOOLBAR_SEPARATOR)
        wxMultiChoiceDialog dlg( this, wxT("Select extra window styles to use"),
                                 wxT("wxPropertyGrid Extra Style"), chs );
        dlg.SetSelections(sel);
        if ( dlg.ShowModal() == wxID_CANCEL )
            return;

        flags = 0;
        sel = dlg.GetSelections();
        for ( ind = 0; ind < sel.size(); ind++ )
            flags |= vls[sel[ind]];

        extraStyle = flags;
    }

    CreateGrid( style, extraStyle );

    FinalizeFramePosition();
}

// -----------------------------------------------------------------------

void FormMain::OnSetColumns( wxCommandEvent& WXUNUSED(event) )
{
    long colCount = ::wxGetNumberFromUser(wxT("Enter number of columns (2-20)."),wxT("Columns:"),
                                          wxT("Change Columns"),m_pPropGridManager->GetColumnCount(),
                                          2,20);

    if ( colCount >= 2 )
    {
        m_pPropGridManager->SetColumnCount(colCount);
    }
}

// -----------------------------------------------------------------------

void FormMain::OnSetPropertyValue( wxCommandEvent& WXUNUSED(event) )
{
    wxPropertyGrid* pg = m_pPropGridManager->GetGrid();
    wxPGProperty* selected = pg->GetSelection();

    if ( selected )
    {
        wxString value = ::wxGetTextFromUser( wxT("Enter new value:") );
        pg->SetPropertyValue( selected, value );
    }
}

// -----------------------------------------------------------------------

void FormMain::OnInsertChoice( wxCommandEvent& WXUNUSED(event) )
{
    wxPropertyGrid* pg = m_pPropGridManager->GetGrid();

    wxPGProperty* selected = pg->GetSelection();
    const wxPGChoices& choices = selected->GetChoices();

    // Insert new choice to the center of list

    if ( choices.IsOk() )
    {
        int pos = choices.GetCount() / 2;
        selected->InsertChoice(wxT("New Choice"), pos);
    }
    else
    {
        ::wxMessageBox(wxT("First select a property with some choices."));
    }
}

// -----------------------------------------------------------------------

void FormMain::OnDeleteChoice( wxCommandEvent& WXUNUSED(event) )
{
    wxPropertyGrid* pg = m_pPropGridManager->GetGrid();

    wxPGProperty* selected = pg->GetSelection();
    const wxPGChoices& choices = selected->GetChoices();

    // Deletes choice from the center of list

    if ( choices.IsOk() )
    {
        int pos = choices.GetCount() / 2;
        selected->DeleteChoice(pos);
    }
    else
    {
        ::wxMessageBox(wxT("First select a property with some choices."));
    }
}

// -----------------------------------------------------------------------

#include <wx/colordlg.h>

void FormMain::OnMisc ( wxCommandEvent& event )
{
    int id = event.GetId();
    if ( id == ID_STATICLAYOUT )
    {
        long wsf = m_pPropGridManager->GetWindowStyleFlag();
        if ( event.IsChecked() ) m_pPropGridManager->SetWindowStyleFlag( wsf|wxPG_STATIC_LAYOUT );
        else m_pPropGridManager->SetWindowStyleFlag( wsf&~(wxPG_STATIC_LAYOUT) );
    }
    else if ( id == ID_COLLAPSEALL )
    {
        wxPGVIterator it;
        wxPropertyGrid* pg = m_pPropGridManager->GetGrid();

        for ( it = pg->GetVIterator( wxPG_ITERATE_ALL ); !it.AtEnd(); it.Next() )
            it.GetProperty()->SetExpanded( false );

        pg->RefreshGrid();
    }
    else if ( id == ID_GETVALUES )
    {
        m_storedValues = m_pPropGridManager->GetGrid()->GetPropertyValues(wxT("Test"),
                                                                      m_pPropGridManager->GetGrid()->GetRoot(),
                                                                      wxPG_KEEP_STRUCTURE|wxPG_INC_ATTRIBUTES);
    }
    else if ( id == ID_SETVALUES )
    {
        if ( m_storedValues.GetType() == wxT("list") )
        {
            m_pPropGridManager->GetGrid()->SetPropertyValues(m_storedValues);
        }
        else
            wxMessageBox(wxT("First use Get Property Values."));
    }
    else if ( id == ID_SETVALUES2 )
    {
        wxVariant list;
        list.NullList();
        list.Append( wxVariant((long)1234,wxT("VariantLong")) );
        list.Append( wxVariant((bool)TRUE,wxT("VariantBool")) );
        list.Append( wxVariant(wxT("Test Text"),wxT("VariantString")) );
        m_pPropGridManager->GetGrid()->SetPropertyValues(list);
    }
    else if ( id == ID_COLLAPSE )
    {
        // Collapses selected.
        wxPGProperty* id = m_pPropGridManager->GetSelection();
        if ( id )
        {
            m_pPropGridManager->Collapse(id);
        }
    }
    else if ( id == ID_UNSPECIFY )
    {
        wxPGProperty* prop = m_pPropGridManager->GetSelection();
        if ( prop )
        {
            m_pPropGridManager->SetPropertyValueUnspecified(prop);
            prop->RefreshEditor();
        }
    }
}

// -----------------------------------------------------------------------

void FormMain::OnPopulateClick( wxCommandEvent& event )
{
}

// -----------------------------------------------------------------------


void FormMain::OnRunMinimalClick( wxCommandEvent& WXUNUSED(event) )
{
}

// -----------------------------------------------------------------------

FormMain::~FormMain()
{
}

// -----------------------------------------------------------------------

IMPLEMENT_APP(cxApplication)

bool cxApplication::OnInit()
{
    FormMain* frame = Form1 = new FormMain( wxT("wxPropertyGrid Sample"), wxPoint(0,0), wxSize(300,500) );
    frame->Show(true);

    return true;
}

// -----------------------------------------------------------------------
