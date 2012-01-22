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

// AKT TODO!!!
BEGIN_EVENT_TABLE(HelpPanel, wxPanel)
    // !!!
END_EVENT_TABLE()

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
            // http://wiki.wxwidgets.org/Calling_The_Default_Browser_In_WxHtmlWindow
            if( link.GetHref().StartsWith(_T("http://")) ||
                link.GetHref().StartsWith(_T("mailto://")) )
                wxLaunchDefaultBrowser(link.GetHref());
            else
                wxHtmlWindow::OnLinkClicked(link);
        }
};

HelpPanel::HelpPanel(MyFrame* parent, wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    this->html = new HtmlView(this, wxID_ANY);

    sizer->Add(this->html, wxSizerFlags(1).Expand());
    this->SetSizer(sizer);

    // AKT TODO!!! display quickstart.html file from Help folder
    this->html->SetPage(_("<html><body>"
        "<h3>Quick start guide</h3>"
        "<h5>1. Overview</h5>"
        "<p>Ready is a program for exploring <a href=\"http://en.wikipedia.org/wiki/Reaction-diffusion_system\">reaction-diffusion</a> systems.</p>"
        "<p>Click on the files in the Patterns Pane to see some different systems."
        "<p>The <a href=\"http://en.wikipedia.org/wiki/OpenCL\">OpenCL</a> demos will only work if you've got OpenCL installed. Either install the latest drivers for your graphics card, "
        "or install one of the SDKs from <a href=\"http://developer.amd.com/appsdk\">AMD</a> or <a href=\"http://software.intel.com/en-us/articles/vcsource-tools-opencl-sdk/\">Intel</a> "
        "that will work with your CPU. Use the commands at the bottom of the Action menu to examine the OpenCL devices available."
        "<h5>2. Interacting with the rendered scene</h5>"
        "<p>From the Action menu: Start or Stop (F5) the system running, or take small Steps (F4)."
        "<p><b>left mouse:</b> rotates the camera around the focal point, as if the scene is a trackball"
        "<p><b>right mouse, or shift+ctrl+left mouse:</b> move up and down to zoom in and out"
        "<p><b>scroll wheel:</b> zoom in and out"
        "<p><b>shift+left mouse:</b> pan"
        "<p><b>ctrl+left mouse:</b> roll"
        "<p><b>'r':</b> reset the view to make everything visible"
        "<p><b>'w':</b> switch to wireframe view"
        "<p><b>'s':</b> switch to surface view</ul>"
        "<h5>3. Working with the windows</h5>"
        "<p>The Patterns Pane, Help Pane and Rule Pane can be shown or hidden by using the commands in the View menu. By dragging the panes by their title bar you can dock them into the "
        "Ready frame in different positions or float them as separate windows."
        "<h5>4. More help</h5>"
        "<p>Send an email to <a href=\"mailto://reaction-diffusion@googlegroups.com\">reaction-diffusion@googlegroups.com</a> if you have any problems, or want to get involved."
        "<p>See the text files in the installation folder for more information."
        "</body></html>"));
}
