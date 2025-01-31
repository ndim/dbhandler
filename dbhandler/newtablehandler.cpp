#include <string>
#include <algorithm>
#if _MSC_VER >= 1900 || !(defined __WXMSW__)
#include <mutex>
#endif
#include "wx/docmdi.h"
#include "wx/fswatcher.h"
#include "wx/dynlib.h"
#include "wx/thread.h"
#include "wx/msgdlg.h"
#include "database.h"
#include "newtablehandler.h"

#if _MSC_VER >= 1900 || !(defined __WXMSW__)
std::mutex Database::Impl::my_mutex;
#endif

NewTableHandler::NewTableHandler(MainFrame *frame, Database *db)
{
    m_db = db;
    pCs = frame;
}

NewTableHandler::~NewTableHandler(void)
{
#if defined _DEBUG
    wxLogDebug( "Starting thread destructor...\n\r" );
#endif
#if defined __WXMSW__ && _MSC_VER < 1900
    wxCriticalSectionLocker enter( pCs->m_threadCS );
#else
    std::lock_guard<std::mutex>( m_db->GetTableVector().my_mutex );
#endif
    pCs->m_handler = NULL;
#if defined _DEBUG
    wxLogDebug( "Thread deleted\n\r" );
#endif
}

wxThread::ExitCode NewTableHandler::Entry()
{
    std::vector<std::wstring> errorMsg;
    int res;
    while( !TestDestroy() )
    {
        {
#if defined __WXMSW__ && _MSC_VER < 1900
            wxCriticalSectionLocker( pCs->m_threadCS );
#else
            std::lock_guard<std::mutex> locker( m_db->GetTableVector().my_mutex );
#endif
            res = m_db->NewTableCreation( errorMsg );
            if( res )
                Delete();
        }
        Sleep( 5000 );
    }
    for( std::vector<std::wstring>::iterator it = errorMsg.begin(); it < errorMsg.end(); ++it )
        wxMessageBox( (*it) );
    return (wxThread::ExitCode) 0;
}
