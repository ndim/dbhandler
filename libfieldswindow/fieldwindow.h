#pragma once
class WXEXPORT FieldWindow : public wxSFShapeCanvas
{
public:
    FieldWindow(wxWindow *parent, int type, const wxPoint &pos = wxDefaultPosition, int width = -1);
    virtual ~FieldWindow();
    void AddField(const wxString &fieldName);
    void RemoveField(const std::vector<std::wstring> &names);
    virtual void OnLeftDown(wxMouseEvent &event);
    void Clear();
private:
    wxPoint m_startPoint;
    wxSFDiagramManager m_manager;
};

