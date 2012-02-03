#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/propgrid/propgrid.h>

class MyFrame : public wxFrame
{
    public:

        MyFrame(const wxString& title);
};

class MyApp : public wxApp
{
    public:

        virtual bool OnInit();
};

DECLARE_APP(MyApp)

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    MyFrame *frame = new MyFrame(_("wxTestbed"));
    SetTopWindow(frame);
    frame->Show();

    return true;
}

MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
{
    wxPropertyGrid *pg = new wxPropertyGrid(this,wxID_ANY);
    pg->Append(new wxStringProperty(_("string:"),wxPG_LABEL,_("blah")));
    pg->Append(new wxLongStringProperty(_("long string:"),wxPG_LABEL,_("blah blah blah")));
    
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(pg,wxSizerFlags(1).Expand());
    this->SetSizer(sizer);
}
