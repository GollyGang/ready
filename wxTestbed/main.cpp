// wxWidgets:
#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/aui/aui.h>

class MyFrame : public wxFrame
{
    public:

        MyFrame(const wxString& title);
        ~MyFrame();

    private:

        wxAuiManager aui_mgr;
};

class MyApp : public wxApp
{
    public:

        virtual bool OnInit();
};

DECLARE_APP(MyApp)   // so other files can use wxGetApp

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    SetAppName(_("wxTestbed"));

    MyFrame *frame = new MyFrame(_("wxTestbed"));
    SetTopWindow(frame);
    frame->Show();

    return true;
}

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
{
    this->aui_mgr.SetManagedWindow(this);

    this->aui_mgr.AddPane(new wxTextCtrl(this,wxID_ANY),
              wxAuiPaneInfo()
              .Name("pane1")
              .Caption(_("Pane 1"))
              .BestSize(500,300)
              .CenterPane()
              );

    this->aui_mgr.AddPane(new wxTextCtrl(this,wxID_ANY),
              wxAuiPaneInfo()
              .Name("pane2")
              .Caption(_("Pane 2"))
              .BestSize(500,300)
              );

    this->aui_mgr.AddPane(new wxTextCtrl(this,wxID_ANY),
              wxAuiPaneInfo()
              .Name("pane3")
              .Caption(_("Pane 3"))
              .BestSize(500,300)
              );

    this->aui_mgr.Update();
}

MyFrame::~MyFrame()
{
    this->aui_mgr.UnInit();
}
