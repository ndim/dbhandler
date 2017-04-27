#ifdef __GNUC__
#pragma implementation "dialogs.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <map>
#include <vector>
#include <string>
#include "wx/docmdi.h"
#include "wx/docview.h"
#include "wx/cmdproc.h"
#include "wx/notebook.h"
#include "wx/grid.h"
#include "wxsf/ShapeCanvas.h"
#include "wxsf/RoundRectShape.h"
#include "wxsf/FlexGridShape.h"
#include "database.h"
#include "constraintsign.h"
#include "table.h"
#include "GridTableShape.h"
#include "HeaderGrid.h"
#include "MyErdTable.h"
#include "FieldShape.h"
#include "fieldwindow.h"
#include "wherehavingpage.h"
#include "syntaxproppage.h"
#include "databasecanvas.h"
#include "databasedoc.h"
#include "databaseview.h"
#include "databasetemplate.h"

#ifdef __WXMSW__
WXDLLIMPEXP_BASE void wxSetInstance( HINSTANCE hInst );

BOOL APIENTRY DllMain( HANDLE hModule, DWORD fdwReason, LPVOID lpReserved)
{
    lpReserved = lpReserved;
    hModule = hModule;
#ifndef WXUSINGDLL
    int argc = 0;
    char **argv = NULL;
#endif
    switch( fdwReason )
    {
    case DLL_PROCESS_ATTACH:
#ifdef WXUSINGDLL
        wxInitialize();
#else
        wxSetInstance( (HINSTANCE) hModule );
        wxEntryStart( argc, argv );
        if( !wxTheApp || !wxTheApp->CallOnInit() )
        {
            wxEntryCleanup();
            if( wxTheApp )
                wxTheApp->OnExit();
            return FALSE;
        }
#endif
        break;
    case DLL_PROCESS_DETACH:
#ifdef WXUSINGDLL
        wxUninitialize();
#else
        if( wxTheApp )
            wxTheApp->OnExit();
        wxEntryCleanup();
#endif
        break;
    }
    return TRUE;
}
#endif

class MyDllApp : public wxApp
{
public:
    bool OnInit()
    {
        return true;
    }

    int OnExit()
    {
        return 0;
    }
};

IMPLEMENT_APP_NO_MAIN(MyDllApp);

extern "C" WXEXPORT void CreateDatabaseWindow(wxWindow *parent, wxDocManager *docManager, Database *db, ViewType type)
{
    DatabaseTemplate *docTemplate;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    docTemplate = (DatabaseTemplate *) docManager->FindTemplate( CLASSINFO( DrawingDocument ) );
    if( !docTemplate )
    {
        if( type == DatabaseView )
            docTemplate = new DatabaseTemplate( docManager, "Drawing", "*.drw", "", "drw", "Drawing Doc", "Drawing View", CLASSINFO( DrawingDocument ), CLASSINFO( DrawingView ) );
        else
            docTemplate = new DatabaseTemplate( docManager, "Drawing", "*.qry", "", "qry", "Drawing Doc", "Drawing View", CLASSINFO( DrawingDocument ), CLASSINFO( DrawingView ) );
    }
    docTemplate->CreateDatabaseDocument( "*.drw", type, wxDOC_NEW | wxDOC_SILENT );
    dynamic_cast<DrawingDocument *>( docManager->GetCurrentDocument() )->SetDatabase( db );
}

/*
extern "C" WXEXPORT void ODBCSetup(wxWindow *pParent)
{
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( pParent );
#endif
    CODBCConfigure dlg( pParent, wxID_ANY, _T( "Configure ODBC" ) );
    dlg.ShowModal();
}
*/