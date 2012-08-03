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
#include "dialogs.hpp"
#include "RecordingDialog.hpp"
#include "prefs.hpp"

// STL:
using namespace std;

// -----------------------------------------------------------------------------------------------

RecordingDialog::RecordingDialog(wxWindow *parent,bool is_2D_data_available,bool default_is_2D_data) 
    : wxDialog(parent,wxID_ANY,_("Recording settings"),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    // create the controls
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vbox);
    
    wxStaticText* source_label = new wxStaticText(this, wxID_STATIC, _("Image source:"));
    this->source_combo = new wxComboBox(this,wxID_ANY);
    this->source_combo->AppendString(_("current view"));
    if(is_2D_data_available)
        this->source_combo->AppendString(_("2D data"));
    this->source_combo->SetSelection(default_is_2D_data?1:0);

    wxStaticText* folder_label = new wxStaticText(this, wxID_STATIC, _("Save frames here: (will overwrite)"));
    wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
    {
        this->folder_edit = new wxTextCtrl(this,wxID_ANY);
        this->folder_edit->SetValue(recordingdir); // from prefs
        this->folder_edit->SetMinSize(wxSize(400,-1));
        hbox1->Add(this->folder_edit,1, wxEXPAND | wxRIGHT,10);
        wxButton *change_folder_button = new wxButton(this,wxID_ANY,_("..."));
        change_folder_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED,&RecordingDialog::OnChangeFolder,this);
        hbox1->Add(change_folder_button,0,wxLEFT | wxRIGHT,0);
    }

    wxStaticText* filenames_label = new wxStaticText(this, wxID_STATIC, _("Filenames:"));
    wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
    {
        this->filename_prefix_edit = new wxTextCtrl(this,wxID_ANY,_("frame_"));
        hbox2->Add(this->filename_prefix_edit,1,wxRIGHT,10);
        hbox2->Add(new wxStaticText(this, wxID_STATIC, _("000000")),0, wxRIGHT | wxALIGN_BOTTOM,10);
        this->extension_combo = new wxComboBox(this,wxID_ANY);
        this->extension_combo->AppendString(_(".png"));
        this->extension_combo->AppendString(_(".jpg"));
        this->extension_combo->SetSelection(0);
        hbox2->Add(this->extension_combo,0,wxLEFT | wxRIGHT,0);
    }
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
    vbox->Add(source_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->Add(this->source_combo, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(folder_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(filenames_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(12);
    vbox->Add(buttbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}

// -----------------------------------------------------------------------------------------------

bool RecordingDialog::TransferDataFromWindow()
{
    this->record_data_image = (this->source_combo->GetSelection()==1);
    this->recording_extension = string(this->extension_combo->GetValue().mb_str());
    recordingdir = this->folder_edit->GetValue(); // save folder in prefs
    this->recording_prefix = string(this->folder_edit->GetValue().mb_str()) + "/" + string(this->filename_prefix_edit->GetValue().mb_str());
    // TODO: save other settings in prefs too, if wanted
    return true;
}

// -----------------------------------------------------------------------------------------------

void RecordingDialog::OnChangeFolder(wxCommandEvent& event)
{
    wxDirDialog dlg(NULL, "Choose target folder", this->folder_edit->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->folder_edit->SetValue(dlg.GetPath());
}

// -----------------------------------------------------------------------------------------------
