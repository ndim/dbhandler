// -*- C++ -*-
//
// generated by wxGlade 0.7.2 (standalone edition) on Sat Jan 04 11:51:25 2020
//
// Example for compiling a single file project under Linux using g++:
//  g++ MyApp.cpp $(wx-config --libs) $(wx-config --cxxflags) -o MyApp
//
// Example for compiling a multi file project under Linux using g++:
//  g++ main.cpp $(wx-config --libs) $(wx-config --cxxflags) -o MyApp Dialog1.cpp Frame1.cpp
//

#ifndef GROUPBYPAGE_H
#define GROUPBYPAGE_H

#define ADDFIELD 0
#define REMOVEFIELD 1
#define CHANGEFIELD 2

#define ASCENDING 0
#define DESCENDING 1

struct Positions
{
    long position, originalPosition;
};

struct FieldSorter
{
    wxString m_name;
    bool m_isAscending;
    long m_originalPosition;
    FieldSorter(wxString name, bool isAscending, long original_position) : m_name(name), m_isAscending(isAscending), m_originalPosition(original_position) {};
    FieldSorter &operator=(const FieldSorter &sorter)
    {
        if( m_name == sorter.m_name )
            return *this;
        else
        {
            m_name = sorter.m_name;
            m_isAscending = sorter.m_isAscending;
            m_originalPosition = sorter.m_originalPosition;
            return *this;
        }
    }
};

class MyListCtrl : public wxListCtrl
{
public:
    MyListCtrl (wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style) : wxListCtrl (parent, id, pos, size, style)
    {
    }
#ifdef __WXMSW__
    virtual bool MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result) wxOVERRIDE;
#endif
protected:
    virtual wxSize DoGetBestClientSize() const wxOVERRIDE;
};

class SortColumnRenderer
    : public wxDataViewCustomRenderer
{
public:
    static wxString GetDefaultType() { return wxS("bool"); }
    explicit SortColumnRenderer(wxCheckBoxState state = wxCHK_CHECKED, wxDataViewCellMode mode = wxDATAVIEW_CELL_ACTIVATABLE, int align = wxDVR_DEFAULT_ALIGNMENT);
#ifdef __WXMSW__
    virtual wxString GetAccessibleDescription() const wxOVERRIDE;
#endif
    virtual bool SetValue(const wxVariant& value) wxOVERRIDE;
    virtual bool GetValue(wxVariant& value) const wxOVERRIDE;
    virtual wxSize GetSize() const wxOVERRIDE;
    virtual bool Render(wxRect cell, wxDC* dc, int state) wxOVERRIDE;
    virtual bool ActivateCell (const wxRect& cell, wxDataViewModel *model, const wxDataViewItem & item, unsigned int col, const wxMouseEvent *mouseEvent) wxOVERRIDE;
private:
    wxSize GetCheckSize () const { return wxRendererNative::Get().GetCheckBoxSize( GetView() ); }

    enum
    {
        MARGIN_CHECK_ICON = 3
    };

    bool m_allow3rdStateForUser;
    bool m_toggle;
    wxDECLARE_DYNAMIC_CLASS_NO_COPY(SortColumnRenderer);
};

class WXEXPORT SortGroupByPage : public wxPanel
{
public:
    SortGroupByPage(wxWindow *parent, bool isSortPage);
    ~SortGroupByPage();
    wxListCtrl *GetSourceList();
    wxListCtrl *GetDestList();
    wxDataViewListCtrl *GetSortSourceList();
    wxDataViewListCtrl *GetSourceDestList();
    void AddRemoveSortingField(bool isAdding, const wxString &field);
    void RemoveTable(const wxString tbl);
    void AddQuickSelectSortingFields(const std::vector<FieldSorter> &allSorted, const std::vector<FieldSorter> &querySorted);
protected:
    void set_properties();
    void do_layout();
    void FinishDragging(const wxPoint &pt);
    void OnBeginDrag(wxListEvent &event);
    void OnItemSelected(wxListEvent &event);
    void OnItemFocused(wxListEvent &event);
    void OnLeftUp(wxMouseEvent &event);
    void OnMouseMove(wxMouseEvent &event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent &event);
    void OnSortBeginDrag(wxDataViewEvent &event);
    void OnSortDrop(wxDataViewEvent &event);
    void OnSortDropPossible(wxDataViewEvent &event);
    void OnSortSelectionChanged(wxDataViewEvent &event);
    void OnSortListStartEditing(wxDataViewEvent &event);
private:
    MyListCtrl *m_source, *m_dest, *m_dragSource, *m_dragDest;
    wxDataViewListCtrl *m_sortSource, *m_sortDest, *m_sortDragSource, *m_sortDragDest;
    wxStaticText *m_label;
    wxString m_item;
    long m_itemPos, m_sourcePos;
    bool m_isDragging, m_isSorting;
};

#endif // GROUPBYPAGE_H
