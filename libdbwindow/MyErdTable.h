#ifndef _MYERDTABLE_H
#define _MYERDTABLE_H

class MyErdTable : public wxSFRoundRectShape
{
public:
    XS_DECLARE_CLONABLE_CLASS(MyErdTable);
    MyErdTable();
    MyErdTable(DatabaseTable *table, ViewType type);
    virtual ~MyErdTable();
    void UpdateTable();
    void SetTableComment(const wxString &comment);
    const DatabaseTable *GetTable();
    wxSFTextShape *GetLabel();
    GridTableShape *GetFieldGrid();
    std::wstring &GetTableName();
    void DisplayTypes(bool display) { m_displayTypes = display; m_pGrid->ShowDataTypes( display ); if( display ) m_columns++; else m_columns--;  }
    void DisplayComments(bool display) { m_displayComments = display; m_pGrid->ShowComments( display ); if( display ) { m_columns++; m_headerColumns++; } else { m_columns--; m_headerColumns--; } }
protected:
    void ClearGrid();
    void ClearConnections();
    void DrawDetail(wxDC &dc);
    void AddColumn(TableField *field, int id, Constraint::constraintType type);
    void SetCommonProps(wxSFShapeBase* shape);
//    virtual void DrawHighlighted(wxDC &dc);
    virtual void DrawHover(wxDC &dc);
    virtual void DrawNormal(wxDC &dc);
    virtual void DrawSelected(wxDC& dc);
private:
    void MarkSerializableDataMembers();
    ViewType m_type;
    HeaderGrid *m_header;
    wxSFTextShape *m_pLabel;
    CommentTableShape *m_comment;
    GridTableShape* m_pGrid;
    DatabaseTable *m_table;
    bool m_displayTypes, m_displayComments;
    int m_columns, m_headerColumns;
};

#endif