/////////////////////////////////////////////////////////////////////////////
// Name:        samples/docview/docview.cpp
// Purpose:     DB Handler
// Author:      Igor Korot
// Created:     17 DEC 2015
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"
    #include "wx/stockitem.h"
#endif

#include "database.h"
#include "tablegeneral.h"

TableGeneralProperty::TableGeneralProperty(wxWindow *parent, DatabaseTable *table) : wxPanel( parent )
{
    m_table = table;
    m_isModified = false;
    m_label1 = new wxStaticText( this, wxID_ANY, _( "Owner" ) );
    m_label2 = new wxStaticText( this, wxID_ANY, _( "Table" ) );
    m_label3 = new wxStaticText( this, wxID_ANY, _( "&Table comment:" ) );
    m_owner = new wxTextCtrl( this, wxID_ANY );
    m_tableName = new wxTextCtrl( this, wxID_ANY );
    m_comment = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_PROCESS_ENTER );
#ifdef __WXMSW__
    m_comment->SetMaxLength( 215 );
#endif
    m_log = new wxCheckBox( this, wxID_ANY, _( "Log Only" ) );
    set_properties();
    do_layout();
    wxButton *ok = dynamic_cast<wxButton *>( FindWindowById( wxID_OK ) );
    if( ok )
        ok->SetDefault();
    m_comment->Bind( wxEVT_CHAR, &TableGeneralProperty::OnCommentKeyEntered, this );
    m_comment->Bind( wxEVT_TEXT, &TableGeneralProperty::OnEditComment, this );
}

TableGeneralProperty::~TableGeneralProperty()
{
}

void TableGeneralProperty::set_properties()
{
    m_owner->Enable( false );
    m_owner->SetValue( m_table->GetSchemaName() );
    m_tableName->Enable( false );
    m_tableName->SetValue( m_table->GetTableName() );
    m_comment->SetValue( m_table->GetComment() );
}

void TableGeneralProperty::do_layout()
{
    wxBoxSizer *sizer1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer *sizer2 = new wxBoxSizer( wxVERTICAL );
    wxFlexGridSizer *sizer3 = new wxFlexGridSizer( 2, 2, 5, 10 );
    sizer1->Add( 5, 5, 0, wxEXPAND, 0 );
    sizer2->Add( 5, 5, 0, wxEXPAND, 0 );
    sizer3->Add( m_label1, 0, wxEXPAND, 0 );
    sizer3->Add( m_owner, 1, wxEXPAND, 0 );
    sizer3->Add( m_label2, 0, wxEXPAND, 0 );
    sizer3->Add( m_tableName, 1, wxEXPAND, 0 );
    sizer2->Add( sizer3, 1, wxEXPAND, 0 );
    sizer2->Add( m_label3, 0, wxEXPAND, 0 );
    sizer2->Add( 5, 5, 0, wxEXPAND, 0 );
    sizer2->Add( m_comment, 1, wxEXPAND, 0 );
    sizer2->Add( 5, 5, 0, wxEXPAND, 0 );
    sizer2->Add( m_log, 0, wxEXPAND, 0 );
    sizer2->Add( 5, 5, 0, wxEXPAND, 0 );
    sizer2->Add( 5, 5, 0, wxEXPAND, 0 );
    sizer1->Add( sizer2, 0, wxEXPAND, 0 );
    sizer1->Add( 5, 5, 0, wxEXPAND, 0 );
    SetSizer( sizer1 );
}

void TableGeneralProperty::OnCommentKeyEntered(wxKeyEvent &event)
{
    if( event.GetKeyCode() != WXK_RETURN )
    {
        dynamic_cast<wxButton *>( GetParent()->GetParent()->FindWindowById( wxID_APPLY ) )->Enable( true );
        event.Skip();
    }
}

bool TableGeneralProperty::IsModified()
{
    return m_isModified;
}

void TableGeneralProperty::OnEditComment(wxCommandEvent &event)
{
#if __WXGTK__
    int length = m_comment->GetValue().length();
    if( length < 5 )
        event.Skip();
#endif
    m_isModified = true;
}

const std::wstring &TableGeneralProperty::GetComment()
{
    return m_comment->GetValue().ToStdWstring();
}

bool TableGeneralProperty::IsLogOnly()
{
    return m_log->GetValue();
}
