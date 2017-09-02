/////////////////////////////////////////////////////////////////////////////
// Name:        samples/docview/mainframe.h
// Purpose:     DB Handler
// Author:      Igor Korot
// Created:     17 DEC 2015
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SAMPLES_MAINFRAME_H_
#define _WX_SAMPLES_MAINFRAME_H_

class MainFrame : public wxDocMDIParentFrame
{
public:
    MainFrame(wxDocManager *manager);
    ~MainFrame();
protected:
    void Connect();
private:
    void InitToolBar(wxToolBar* toolBar);
    void InitMenuBar(int id);
    void DatabaseMenu();
    void QueryMenu();
    void TableMenu();
    void OnConfigureODBC(wxCommandEvent &event);
    void OnDatabaseProfile(wxCommandEvent &event);
    void OnTable(wxCommandEvent &event);
    void OnDatabase(wxCommandEvent &event);
    void OnQuery(wxCommandEvent &event);
    void OnSize(wxSizeEvent &event);
    Database *m_db;
    wxDynamicLibrary *m_lib;
    wxMenu *m_menuFile;
    wxDocManager *m_manager;
#if defined __WXMSW__ || defined __WXGTK__
    wxToolBar *m_tb;
#endif
    std::map<wxString, wxDynamicLibrary *> m_painters;
    wxDECLARE_EVENT_TABLE();
};

#endif