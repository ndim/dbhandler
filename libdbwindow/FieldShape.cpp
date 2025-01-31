#include "wxsf/TextShape.h"
#include "wxsf/FlexGridShape.h"
#include "database.h"
#include "GridTableShape.h"
#include "FieldShape.h"

XS_IMPLEMENT_CLONABLE_CLASS(FieldShape,wxSFTextShape);

FieldShape::FieldShape()
{
/*    m_name = wxEmptyString;
    m_type = wxEmptyString;
    m_size = 0;
    m_decimal = 0;
    m_isNotNull = false;
    m_isPK = false;
    m_isAutoInc = false;
    m_backColour = wxColour( 210, 225, 245 );
    XS_SERIALIZE( m_name, wxT( "m_name" ) );
    XS_SERIALIZE( m_type, wxT( "m_type" ) );
    XS_SERIALIZE_LONG( m_size, wxT( "m_size" ) );
    XS_SERIALIZE_LONG( m_decimal, wxT( "m_decimal" ) );
    XS_SERIALIZE( m_isPK, wxT( "m_isPK" ) );
    XS_SERIALIZE( m_isNotNull, wxT( "m_notNull" ) );
    XS_SERIALIZE( m_isAutoInc, wxT( "m_isAutoInc" ) );
    XS_SERIALIZE( m_backColour, wxT( "m_backColour" ) );*/
}

FieldShape::FieldShape(const FieldShape &shape)
{
    m_field = shape.m_field;
    m_parentRect = shape.m_parentRect;
    m_backColour = shape.m_backColour;
    RemoveStyle( sfsSHOW_HANDLES );
}

FieldShape::~FieldShape(void)
{
}

void FieldShape::DrawNormal(wxDC &dc)
{
    wxRect rect = this->GetBoundingBox();
    wxSFShapeBase *parentShape = GetParentShape()->GetParentShape();
    wxRect rectParent = parentShape->GetBoundingBox();
    m_parentRect.x = rectParent.x;
    m_parentRect.width = rectParent.width;
    wxString line;
    int i = 0;
    dc.SetTextForeground( m_TextColor );
    if( this->m_fSelected )
    {
        m_backColour = wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT );
        dc.SetBrush( m_backColour );
    }
    else
    {
        m_backColour = wxColour( 210, 225, 245 );
        dc.SetBrush( m_Fill );
        dc.SetPen( m_Border );
        dc.SetBackgroundMode( wxTRANSPARENT );
    }
    dc.DrawRectangle( m_parentRect.x, rect.y, m_parentRect.width, rect.height );
    dc.SetFont( m_Font );
    wxRealPoint pos = GetAbsolutePosition();
    // draw all text lines
    wxStringTokenizer tokens( m_sText, wxT("\n\r"), wxTOKEN_RET_EMPTY );
    while( tokens.HasMoreTokens() )
    {
        line = tokens.GetNextToken();
        dc.DrawText( line, (int)pos.x, (int)pos.y + i * 12 );
        i++;
    }
    dc.SetPen( wxNullPen );
    dc.SetFont( wxNullFont );
    dc.SetBrush( wxNullBrush );
}

void FieldShape::SetParentRect(const wxRect &rect)
{
    m_parentRect = rect;
}

void FieldShape::SetField(TableField *field)
{
    m_field = field;
}

TableField *FieldShape::GetField()
{
    return m_field;
}

bool FieldShape::Contains(const wxPoint& pos)
{
    bool result = false;
    wxSFShapeBase *parent = GetParentShape();
    wxRect rectParent = parent->GetBoundingBox();
    wxRect rect = GetBoundingBox();
    if( pos.y >= rect.GetTop() && pos.y < rect.GetBottom() && pos.x >= rectParent.GetLeft() && pos.x <= rectParent.GetRight() )
        result = true;
    return result;
}

void FieldShape::DrawSelected(wxDC& dc)
{
    DrawNormal( dc );
}

void FieldShape::Select(bool state)
{
    m_fSelected = state;
}

wxRect FieldShape::GetBoundingBox()
{
    wxRect rect = wxSFRectShape::GetBoundingBox();
    wxRect parentRect = this->GetParentShape()->GetParentShape()->GetBoundingBox();
    rect.SetLeft( parentRect.GetLeft() );
    return rect;
}
