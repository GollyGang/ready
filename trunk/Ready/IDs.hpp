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

// IDs for the controls and the menu commands
namespace ID { enum {
    
    // we can use IDs higher than this for our own purposes
    Dummy = wxID_HIGHEST+1,
    
    // file menu
    Screenshot,
    
    // view menu
    PatternsPane,
    RulePane,
    CanvasPane,
    HelpPane,
    RestoreDefaultPerspective,
    ChangeActiveChemical,
    
    // action menu
    Step,
    RunStop,
    Reset,
    InitWithBlobInCenter,
    SelectOpenCLDevice,
    OpenCLDiagnostics,
    
    // help menu
    HelpQuick,
    HelpTips,
    HelpKeyboard,
    HelpMouse,
    HelpFile,
    HelpEdit,
    HelpView,
    HelpAction,
    HelpHelp,
    HelpRefs,
    HelpFormats,
    HelpProblems,
    HelpChanges,
    HelpCredits,
    
    // PatternsPanel:
    PatternsTree,
    
    // HelpPanel:
    BackButton,
    ForwardButton,
    ContentsButton,

    // toolbars:
    FileToolbar,
    ActionToolbar,

}; };
