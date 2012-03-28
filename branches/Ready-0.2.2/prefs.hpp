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

#ifndef _PREFS_H_
#define _PREFS_H_

// Routines for getting, saving and changing user preferences:

void GetPrefs();
// Read preferences from the ReadyPrefs file.

void SavePrefs();
// Write preferences to the ReadyPrefs file.

bool ChangePrefs(const wxString& page);
// Open a modal dialog so user can change various preferences.
// Returns true if the user hits OK (so client can call SavePrefs).

void ChooseTextEditor(wxWindow* parent, wxString& result);
// Let user select their preferred text editor.  The result is the
// path to the application or empty if the user cancels the dialog.

// Various constants:

const int minfontsize = 8;       // minimum value of infofontsize/helpfontsize
const int maxfontsize = 30;      // maximum value of infofontsize/helpfontsize

// Global directory paths:

extern wxString readydir;        // directory containing app
extern wxString datadir;         // directory for user-specific data
extern wxString tempdir;         // directory for temporary data
extern wxString patterndir;      // directory for supplied patterns

// Global preference data:

extern int mainx;                // main window's location
extern int mainy;
extern int mainwd;               // main window's size
extern int mainht;
extern bool maximize;            // maximize main window?
extern wxString auilayout;       // wxAUI layout info
extern int opencl_platform;      // current OpenCL platform
extern int opencl_device;        // current OpenCL device
extern int debuglevel;           // for displaying debug info if > 0
extern int infofontsize;         // font size in info pane
extern int helpfontsize;         // font size in help pane
extern int textdlgwd;            // width of multi-line text dialog
extern int textdlght;            // height of multi-line text dialog
extern bool showtips;            // show button tips?
extern bool allowbeep;           // okay to play beep sound?
extern bool askonnew;            // ask to save changes before creating new pattern?
extern bool askonload;           // ask to save changes before loading pattern file?
extern bool askonquit;           // ask to save changes before quitting app?
extern wxString opensavedir;     // directory for Open/Save Pattern dialogs
extern wxString screenshotdir;   // directory for Save Screenshot dialog
extern wxString userdir;         // directory for user's patterns
extern wxString texteditor;      // path of user's preferred text editor
extern wxMenu* patternSubMenu;   // submenu of recent pattern files
extern int numpatterns;          // current number of recent pattern files
extern int maxpatterns;          // maximum number of recent pattern files

// Keyboard shortcuts:

// define the actions that can be invoked by various key combinations
typedef enum
{
    DO_NOTHING = 0,              // null action must be zero
    DO_OPENFILE,                 // open a chosen pattern/html file
                                 // the rest are in (mostly) alphabetical order:
    DO_ABOUT,                    // about Ready
    DO_ADDPATTS,                 // add my patterns...
    DO_ADDPARAM,                 // add parameter...
    DO_DELPARAM,                 // delete parameter...
    DO_CHEMICAL,                 // change active chemical...
    DO_CHANGESPEED,              // change running speed...
    DO_CLEAR,                    // clear selection
    DO_COPY,                     // copy selection
    DO_CUT,                      // cut selection
    DO_FIT,                      // fit pattern
    DO_FULLSCREEN,               // full screen
    DO_GENPATT,                  // generate pattern
    DO_NEWPATT,                  // new pattern
    DO_OPENPATT,                 // open pattern...
    DO_PASTE,                    // paste
    DO_PREFS,                    // preferences...
    DO_QUIT,                     // quit Ready
    DO_RESET,                    // reset
    DO_RESTORE,                  // restore default layout
    DO_RUNSTOP,                  // run/stop
    DO_FASTER,                   // run faster
    DO_SLOWER,                   // run slower
    DO_SAVE,                     // save pattern...
    DO_SCREENSHOT,               // save screenshot...
    DO_SELALL,                   // select all
    DO_DEVICE,                   // select OpenCL device...
    DO_OPENCL,                   // show OpenCL diagnostics...
    DO_PATTERNS,                 // show patterns pane
    DO_INFO,                     // show info pane
    DO_HELP,                     // show help pane
    DO_STEP1,                    // step by 1
    DO_STEPN,                    // step by N
    DO_WIREFRAME,                // wireframe
    MAX_ACTIONS
} action_id;

typedef struct {
    action_id id;                // one of the above
    wxString file;               // non-empty if id is DO_OPENFILE
} action_info;

action_info FindAction(int key, int modifiers);
// return the action info for the given key and modifier set

wxString GetAccelerator(action_id action);
// return a string, possibly empty, containing the menu item
// accelerator(s) for the given action

void RemoveAccelerator(wxMenuBar* mbar, int item, action_id action);
// remove any accelerator from given menu item

void SetAccelerator(wxMenuBar* mbar, int item, action_id action);
// update accelerator for given menu item using given action

wxString GetShortcutTable();
// return HTML data to display current keyboard shortcuts

#endif
