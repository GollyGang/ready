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
#include "app.hpp"
#include "frame.hpp"

// STL:
#include <stdexcept>
using namespace std;

// stdlib:
#include <stdlib.h>
#include <time.h>

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution and also implements the
// accessor function wxGetApp() which will return the reference of the correct
// type (ie. MyApp and not wxApp).
IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    srand((unsigned int)time(NULL));

    currframe = new MyFrame(_("Ready"));
    SetTopWindow(currframe);
    currframe->Show();

    return true;
}

#ifdef __WXMAC__
// open a .vti file that was double-clicked or dropped onto app icon
void MyApp::MacOpenFile(const wxString& fullPath)
{
    currframe->Raise();
    currframe->OpenFile(fullPath);
}
#endif
