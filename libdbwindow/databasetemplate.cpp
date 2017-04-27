#ifdef __GNUC__
#pragma implementation "dialogs.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/docview.h"
#include "wx/docmdi.h"
#include "wx/notebook.h"
#include "wx/grid.h"
#include "wxsf/TextShape.h"
#include "wxsf/ShapeCanvas.h"
#include "wxsf/RoundRectShape.h"
#include "wxsf/FlexGridShape.h"
#include "constraintsign.h"
#include "constraint.h"
#include "GridTableShape.h"
#include "HeaderGrid.h"
#include "database.h"
#include "MyErdTable.h"
#include "FieldShape.h"
#include "fieldwindow.h"
#include "wherehavingpage.h"
#include "syntaxproppage.h"
#include "databasecanvas.h"
#include "databasedoc.h"
#include "databaseview.h"
#include "databasetemplate.h"

DatabaseTemplate::DatabaseTemplate(wxDocManager *manager, const wxString &descr, const wxString &filter, const wxString &dir, const wxString &ext, const wxString &docTypeName, const wxString &viewTypeName, wxClassInfo *docClassInfo, wxClassInfo *viewClassInfo, long flags)
  : wxDocTemplate(manager, descr, filter, dir, ext, docTypeName, viewTypeName, docClassInfo, viewClassInfo)
{
}

DrawingView *DatabaseTemplate::CreateDatabaseView(wxDocument *doc, ViewType type, long flags)
{
    wxScopedPtr<DrawingView> view( (DrawingView *) DoCreateView() );
    if( !view )
        return NULL;
    view->SetViewType( type );
    view->SetDocument( doc );
    if( !view->OnCreate( doc, flags ) )
        return NULL;
    return view.release();
}

bool DatabaseTemplate::CreateDatabaseDocument(const wxString &path, ViewType type, long flags)
{
    DrawingDocument * const doc = (DrawingDocument *) DoCreateDocument();
    wxTRY
    {
        doc->SetFilename( path );
        doc->SetDocumentTemplate( this );
        GetDocumentManager()->AddDocument( doc );
        doc->SetCommandProcessor( doc->OnCreateCommandProcessor() );
        if( CreateDatabaseView( doc, type, flags ) )
            return true;
        if( GetDocumentManager()->GetDocuments().Member( doc ) )
            doc->DeleteAllViews();
        return false;
    }
    wxCATCH_ALL
    (
        if( GetDocumentManager()->GetDocuments().Member( doc ) )
            doc->DeleteAllViews();
        throw;
    )
}