#ifdef __GNUC__
#pragma implementation "dialogs.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if defined __WXMSW__ && defined __MEMORYLEAKS__
#include <vld.h>
#endif

#include <map>
#include <vector>
#include "wx/wizard.h"
#include "wx/filepicker.h"
#include "wx/dynlib.h"
#include "wx/listctrl.h"
#include "wx/notebook.h"
#include "wx/spinctrl.h"
#include "wx/bmpcbox.h"
#include "wx/grid.h"
#ifdef __WXGTK__
#include "gtk/gtk.h"
#include "wx/nativewin.h"
#endif
#ifdef __WXOSX_COCOA__
#include "wx/nativewin.h"
#endif
#ifndef __WXMSW__
#include "odbccredentials.h"
#endif
#include "database.h"
#include "wxsf/ShapeCanvas.h"
#include "wx/fontenum.h"
//#include "fontpropertypagebase.h"
#include "databasetype.h"
#include "propertypagebase.h"
#include "tablegeneral.h"
#include "odbcconfigure.h"
#include "selecttables.h"
#include "fieldwindow.h"
#include "createindex.h"
#include "fontpropertypagebase.h"
#include "fieldgeneral.h"
#include "fieldheader.h"
#include "foreignkey.h"
#include "getobjectname.h"
#include "jointype.h"
#include "properties.h"
#include "addcolumnsdialog.h"
#include "bitmappanel.h"
#include "newquery.h"
#include "quickselect.h"
#include "retrievalarguments.h"

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

extern "C" WXEXPORT void ODBCSetup(wxWindow *pParent)
{
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( pParent );
#endif
    CODBCConfigure dlg( pParent, wxID_ANY, _( "Configure ODBC" ) );
    dlg.ShowModal();
}

extern "C" WXEXPORT int DatabaseProfile(wxWindow *parent, const wxString &title, wxString &name, wxString &dbEngine, wxString &connectedUser, bool ask, const std::vector<std::wstring> &dsn, wxString &password)
{
    int res;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    DatabaseType dlg( parent, title, name, dbEngine, dsn );
    bool result = dlg.RunWizard( dlg.GetFirstPage() );
    if( result )
    {
        wxTextCtrl *userId = dlg.GetUserControl();
        if( userId )
        {
            connectedUser = userId->GetValue();
        }
        else
            connectedUser = "";
        dlg.GetDatabaseEngine( dbEngine );
        name = dlg.GetDatabaseName();
        ask = dlg.GetODBCConnectionParam();
        if( name.empty() )
            name = dlg.GetConnectString();
        res = wxID_OK;
    }
	else
        res = wxID_CANCEL;
    return res;
}

extern "C" WXEXPORT int SelectTablesForView(wxWindow *parent, Database *db, std::vector<wxString> &tableNames, std::vector<std::wstring> &names, bool isTableView, const int type)
{
    int res;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    SelectTables dlg( parent, wxID_ANY, "", db, names, isTableView, type );
    if( isTableView )
        dlg.Center();
	res = dlg.ShowModal();
    if( res != wxID_CANCEL )
        dlg.GetSelectedTableNames( tableNames );
    return res;
}

extern "C" WXEXPORT int CreateIndexForDatabase(wxWindow *parent, DatabaseTable *table, Database *db, wxString &command, wxString &indexName)
{
    int res;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    CreateIndex dlg( parent, wxID_ANY, "", table, table->GetSchemaName(), db );
    dlg.Center();
    res = dlg.ShowModal();
    if( res != wxID_CANCEL )
    {
        command = dlg.GetCommand();
        indexName = dlg.GetIndexNameCtrl()->GetValue();
    }
    return res;
}

extern "C" WXEXPORT int CreatePropertiesDialog(wxWindow *parent, Database *db, int type, void *object, wxString &command, bool logOnly, const wxString &tableName, const wxString &schemaName, const wxString &ownerName, wxCriticalSection &cs)
{
    wxString title;
    int res = 0;
    if( type == 0 )
    {
        DatabaseTable *table = static_cast<DatabaseTable *>( object );
        title = _( "Table " );
        title += table->GetSchemaName() + L"." + table->GetTableName();
    }
    if( type == 1 )
    {
        title = _( "Column " );
        title += tableName + ".";
        title += static_cast<Field *>( object )->GetFieldName();
    }
    PropertiesDialog dlg( parent, wxID_ANY, title, db, type, object, tableName, schemaName, ownerName, cs );
	dlg.Center();
    res = dlg.ShowModal();
    if( res != wxID_CANCEL )
    {
        command = dlg.GetCommand();
        logOnly = dlg.IsLogOnly();
    }
    return res;
}

extern "C" WXEXPORT int CreateForeignKey(wxWindow *parent, wxString &keyName, DatabaseTable *table, std::vector<std::wstring> &foreignKeyFields, std::vector<std::wstring> &refKeyFields, std::wstring &refTableName, int &deleteProp, int &updateProp, Database *db, bool &logOnly, bool isView, std::vector<FKField *> &newFK, int &match)
{
    int res;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    wxString refTblName = wxString( refTableName );
    ForeignKeyDialog dlg( parent, wxID_ANY, _( "" ), table, db, keyName, foreignKeyFields, refTblName, isView, match );
    dlg.Center();
    res = dlg.ShowModal();
    if( res != wxID_CANCEL || dlg.IsForeignKeyEdited() )
    {
        logOnly = dlg.IsLogOnlyI();
        keyName = dlg.GetKeyNameCtrl()->GetValue();
        if( !isView )
        {
            foreignKeyFields = dlg.GetForeignKeyFields();
            refKeyFields = dlg.GetPrimaryKeyFields();
            refTableName = dlg.GetReferencedTable();
            deleteProp = dlg.GetDeleteParam();
            updateProp = dlg.GetUpdateParam();
            match = dlg.GetMatchingOptions();
		}
        newFK = dlg.GetForeignKeyVector();
    }
    return res;
}

extern "C" WXEXPORT int ChooseObject(wxWindow *parent, int objectId)
{
    int res;
    wxString title;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    switch( objectId )
    {
    case 1:
        title = _( "Query" );
        break;
    default:
        break;
    }
    GetObjectName dlg( parent, wxID_ANY, title, objectId );
    dlg.Center();
    res = dlg.ShowModal();
    return res;
}

extern "C" WXEXPORT int NewQueryDlg(wxWindow *parent, int &source, int &presentation)
{
    int res;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    NewQuery dlg( parent, _( "New Query" ) );
    dlg.Center();
    res = dlg.ShowModal();
    if( res == wxID_OK )
    {
        source = dlg.GetSource();
        presentation = dlg.GetPresentation();
    }
    return res;
}

extern "C" WXEXPORT int QuickSelectDlg(wxWindow *parent, const Database *db, std::vector<wxString> &tableName, std::vector<wxString> &fields)
{
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    QuickSelect dlg( parent, db );
    dlg.Center();
    int res = dlg.ShowModal();
    if( res == wxID_OK )
    {
        tableName.push_back( dlg.GetQueryTable()->GetString( 0 ) );
        fields = dlg.GetQueryFields();
    }
    return res;
}

extern "C" WXEXPORT int SelectJoinType(wxWindow *parent, const wxString &origTable, const wxString &refTable, const wxString &origField, const wxString &refField, int &type)
{
    int res;
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    JointType dlg( parent, wxID_ANY, _( "Join" ), origTable, refTable, origField, refField, type );
    res = dlg.ShowModal();
    if( res == wxID_OK )
        type = dlg.GetTypeCtrl()->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
    return res;
}

extern "C" WXEXPORT int AddColumnToQuery(wxWindow *parent, int type, const std::vector<std::wstring> &fields, wxString &selection, const wxString &dbType, const wxString &dbSubtype)
{
    int res;
    AddColumnsDialog dlg( parent, type, fields, dbType, dbSubtype );
    res = dlg.ShowModal();
    if( res == wxID_OK )
    {
        wxListBox *flds = dlg.GetFieldsControl();
        selection = flds->GetString( flds->GetSelection() );
    }
    else
        selection = wxEmptyString;
    return res;
}

extern "C" WXEXPORT int GetODBCCredentails(wxWindow *parent, const wxString &dsn, wxString &userID, wxString &password)
{
    int res = wxID_OK;
#ifndef __WXMSW__
    ODBCCredentials dlg( parent, wxID_ANY, L"", dsn, userID, password );
    res = dlg.ShowModal();
    if( res == wxID_OK )
    {
        userID = dlg.GetUserIDControl().GetValue();
        password = dlg.GetPasswordControl().GetValue();
    }
#endif
    return res;
}

extern "C" WXEXPORT int GetQueryArguments(wxWindow *parent, std::vector<QueryArguments> &arguments, const wxString &dbType, const wxString &subType)
{
#ifdef __WXMSW__
    wxTheApp->SetTopWindow( parent );
#endif
    RetrievalArguments dlg( parent, arguments, dbType, subType );
    dlg.Center();
    int result = dlg.ShowModal();
    return result;
}
