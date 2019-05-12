#ifndef __DATABASEVIEW__H
#define __DATABASEVIEW__H

#if !defined CONSTRAINT_H && !defined __GRIDTABLESHAPE_H__
enum ViewType
{
    DatabaseView,
    QueryView
};
#endif

// The view using MyCanvas to show its contents
class DrawingView : public wxView
{
public:
    DrawingView() : wxView(), m_canvas(NULL) {}
    ~DrawingView();
    void UpdateQueryFromSignChange(const QueryConstraint *type);
//    std::vector<Table> &GetTablesForView(Database *db);
    wxFrame *GetLogWindow() const;
    wxTextCtrl *GetTextLogger() const;
    void GetTablesForView(Database *db, bool init);
    void SetViewType(ViewType type);
    ViewType GetViewType();
    WhereHavingPage *GetWherePage();
    WhereHavingPage *GetHavingPage();
    void AddFieldToQuery(const FieldShape &field, bool isAdding, const std::wstring &tableName, bool quickSelect);
    void HideShowSQLBox(bool show);
    void SetSynchronisationObject(wxCriticalSection &cs);
    virtual bool OnCreate(wxDocument *doc, long flags) wxOVERRIDE;
    virtual void OnDraw(wxDC *dc) wxOVERRIDE;
    virtual void OnUpdate(wxView *sender, wxObject *hint = NULL) wxOVERRIDE;
    virtual bool OnClose(bool deleteWindow = true) wxOVERRIDE;
#if defined __WXMSW__ || defined __WXGTK__
    virtual void OnActivateView(bool activate, wxView *activeView, wxView *deactiveView);
#endif
    void OnViewSelectedTables(wxCommandEvent &event);
    void OnNewIndex(wxCommandEvent &event);
    void OnFieldDefinition(wxCommandEvent &event);
    void OnFieldProperties(wxCommandEvent &event);
    void OnSetProperties(wxCommandEvent &event);
    void OnCloseLogWindow(wxCloseEvent &event);
    void OnForeignKey(wxCommandEvent &event);
    void OnLogUpdateUI(wxUpdateUIEvent &event);
    void OnStartLog(wxCommandEvent &event);
    void OnStopLog(wxCommandEvent &event);
    void OnSaveLog(wxCommandEvent &event);
    void OnClearLog(wxCommandEvent &event);
    void OnAlterTable(wxCommandEvent &event);
    void OnCreateDatabase(wxCommandEvent &event);
    void OnSelectAllFields(wxCommandEvent &event);
    void OnSQLNotebookPageChanged(wxBookCtrlEvent &event);
    void OnDistinct(wxCommandEvent &event);
    void OnQueryChange(wxCommandEvent &event);
    void OnRetrievalArguments(wxCommandEvent &event);
/*#if defined __WXMSW__ || defined __WXGTK__
    virtual void OnActivateView(bool activate, wxView *activeView, wxView *deactiveView);
#endif*/
    DrawingDocument* GetDocument();
protected:
    void AddDeleteFields(MyErdTable *table, bool isAdd, const std::wstring &tableName);
    void CreateViewToolBar();
private:
    bool m_isActive;
    wxToolBar *m_tb, *m_styleBar;
    wxTextCtrl *m_fieldText;
    wxComboBox *m_fontName, *m_fontSize;
    wxFrame *m_log;
    wxTextCtrl *m_text;
    DatabaseCanvas *m_canvas;
    bool m_isCreated;
    ViewType m_type;
    FieldWindow *m_fields;
    wxDocMDIChildFrame *m_frame;
    wxNotebook *m_queryBook;
    wxBoxSizer *sizer, *mainSizer;
    WhereHavingPage *m_page2, *m_page4;
    SyntaxPropPage *m_page6;
    wxCriticalSection *pcs;
    int m_source, m_presentation;
    std::vector<Field *> m_queryFields;
    std::vector<wxString> m_selectTableName;
//    std::vector<wxString> m_selectFields;
    std::vector<QueryArguments> m_arguments;
    DesignCanvas *m_designCanvas;
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_DYNAMIC_CLASS(DrawingView);
};

wxDECLARE_EVENT(wxEVT_SET_TABLE_PROPERTY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_SET_FIELD_PROPERTY, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_CHANGE_QUERY, wxCommandEvent);

#define wxID_DATABASEWINDOW 2

#endif