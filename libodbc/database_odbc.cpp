#ifdef WIN32
#include <windows.h>
#endif
#ifndef WIN32
#include <sstream>
#endif

#if defined __WXMSW__ && defined __MEMORYLEAKS__
#include <vld.h>
#endif

#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#ifdef _IODBCUNIX_FRAMEWORK
#include "iODBC/sql.h"
#include "iODBC/sqlext.h"
#include "iODBCinst/odbcinst.h"
#else
#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#endif
#include "string.h"
#include "database.h"
#include "database_odbc.h"

ODBCDatabase::ODBCDatabase() : Database()
{
    m_env = 0;
    m_hdbc = 0;
    m_hstmt = 0;
    pimpl = NULL;
    odbc_pimpl = NULL;
    m_oneStatement = false;
    m_connectString = NULL;
    m_isConnected = false;
    connectToDatabase = false;
}

ODBCDatabase::~ODBCDatabase()
{
    RETCODE ret;
    std::vector<std::wstring> errorMsg;
    delete m_connectString;
    m_connectString = 0;
    delete pimpl;
    pimpl = NULL;
    delete odbc_pimpl;
    odbc_pimpl = NULL;
    if( m_hdbc != 0 )
    {
        ret = SQLFreeHandle( SQL_HANDLE_DBC, m_hdbc );
        if( ret != SQL_SUCCESS )
            GetErrorMessage( errorMsg, 2 );
        m_hdbc = 0;
        if( m_env != 0 )
        {
            ret = SQLFreeHandle( SQL_HANDLE_ENV, m_env );
            if( ret != SQL_SUCCESS )
                GetErrorMessage( errorMsg, 0 );
            m_env = 0;
        }
    }
}

bool ODBCDatabase::GetDriverList(std::map<std::wstring, std::vector<std::wstring> > &driversDSN, std::vector<std::wstring> &errMsg)
{
    bool result = true;
    std::wstring s1, s2, error, dsn_result;
    SQLWCHAR driverDesc[1024], driverAttributes[256], dsn[SQL_MAX_DSN_LENGTH+1], dsnDescr[255];
    SQLSMALLINT attr_ret;
    SWORD pcbDSN, pcbDesc;
    SQLSMALLINT descriptionLength;
    SQLUSMALLINT direction = SQL_FETCH_FIRST, direct = SQL_FETCH_FIRST;
    RETCODE ret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HENV, &m_env ), ret1, ret2;
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        result = false;
        GetErrorMessage( errMsg, 0, m_env );
    }
    else
    {
        ret = SQLSetEnvAttr( m_env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0 );
        if( ret != SQL_SUCCESS )
        {
            result = false;
            GetErrorMessage( errMsg, 0, m_env );
        }
        else
        {
            while( ( ret1 = SQLDrivers( m_env, direction, driverDesc, sizeof( driverDesc ) - 1, &descriptionLength, driverAttributes, sizeof( driverAttributes ), &attr_ret ) ) == SQL_SUCCESS )
            {
                str_to_uc_cpy( s1, driverDesc );
                while( ( ret2 = SQLDataSources( m_env, direct, dsn, SQL_MAX_DSN_LENGTH, &pcbDSN, dsnDescr, 254, &pcbDesc ) ) == SQL_SUCCESS )
                {
                    str_to_uc_cpy( s2, dsnDescr );
                    if( s1 == s2 )
                    {
                        str_to_uc_cpy( dsn_result, dsn  );
                        driversDSN[s1].push_back( dsn_result );
                        dsn_result = L"";
                    }
                    direct = SQL_FETCH_NEXT;
                    s2 = L"";
                }
                if( ret2 != SQL_SUCCESS && ret2 != SQL_NO_DATA )
                {
                    result = false;
                    GetErrorMessage( errMsg, 0, m_env );
                    driversDSN.clear();
                    break;
                }
                else
                {
                    direction = SQL_FETCH_FIRST;
                    direct = SQL_FETCH_FIRST;
                }
                if( driversDSN.find( s1 ) == driversDSN.end() )
                    driversDSN[s1].push_back( L"" );
                if( ret2 == SQL_NO_DATA )
                    ret2 = SQL_SUCCESS;
                direction = SQL_FETCH_NEXT;
                s1 = L"";
            }
            if( ret1 != SQL_SUCCESS && ret1 != SQL_NO_DATA )
            {
                result = false;
                driversDSN.clear();
                GetErrorMessage( errMsg, 0, m_env );
            }
        }
    }
    return result;
}

bool ODBCDatabase::GetDSNList(std::vector<std::wstring> &dsns, std::vector<std::wstring> &errorMsg)
{
    bool result = false;
    SQLWCHAR dsn[SQL_MAX_DSN_LENGTH+1], dsnDescr[255];
    SQLSMALLINT direct = SQL_FETCH_FIRST;
    SWORD pcbDSN, pcbDesc;
    RETCODE ret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HENV, &m_env );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 0, m_env );
        result = true;
    }
    else
    {
        ret = SQLSetEnvAttr( m_env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0 );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 0, m_env );
            result = true;
        }
        else
        {
            ret = SQLDataSources( m_env, direct, dsn, SQL_MAX_DSN_LENGTH, &pcbDSN, dsnDescr, 254, &pcbDesc );
            while( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
            {
                std::wstring s1;
                str_to_uc_cpy( s1, dsn );
                dsns.push_back( s1 );
                direct = SQL_FETCH_NEXT;
                ret = SQLDataSources( m_env, direct, dsn, SQL_MAX_DSN_LENGTH, &pcbDSN, dsnDescr, 254, &pcbDesc );
            }
            if( ret != SQL_SUCCESS && ret != SQL_NO_DATA )
            {
                GetErrorMessage( errorMsg, 0, m_env );
                result = true;
            }
        }
    }
    ret = SQLFreeHandle( SQL_HANDLE_ENV, m_env );
    if( ret != SQL_SUCCESS && ret != SQL_NO_DATA )
    {
        GetErrorMessage( errorMsg, 0, m_env );
        result = true;
    }
    return result;
}

bool ODBCDatabase::AddDsn(SQLHWND hwnd, const std::wstring &driver, std::vector<std::wstring> &errorMsg)
{
    bool result = true;
    std::wstring error;
    SQLWCHAR buf[256];
    SQLWCHAR temp1[512];
    memset( temp1, 0, 512 );
    memset( buf, 0, 256 );
    uc_to_str_cpy( temp1, driver );
    BOOL ret = SQLConfigDataSource( hwnd, ODBC_ADD_DSN, temp1, buf );
    if( !ret )
    {
        GetDSNErrorMessage( errorMsg );
        if( !errorMsg.empty() )
            result = false;
    }
    return result;
}

bool ODBCDatabase::EditDsn(SQLHWND hwnd, const std::wstring &driver, const std::wstring &dsn, std::vector<std::wstring> &errorMsg)
{
    bool result = true;
    SQLWCHAR temp1[1024];
    SQLWCHAR temp2[1024];
    memset( temp1, 0, 1024 );
    memset( temp2, 0, 1024 );
    uc_to_str_cpy( temp1, driver );
    uc_to_str_cpy( temp2, L"DSN=" );
    uc_to_str_cpy( temp2, dsn );
    BOOL ret= SQLConfigDataSource( hwnd, ODBC_CONFIG_DSN, temp1, temp2 );
    if( !ret )
    {
        GetDSNErrorMessage( errorMsg );
        if( !errorMsg.empty() )
            result = false;
    }
    return result;
}

bool ODBCDatabase::RemoveDsn(const std::wstring &driver, const std::wstring &dsn, std::vector<std::wstring> &errorMsg)
{
    bool result = true;
    SQLWCHAR temp1[1024];
    SQLWCHAR temp2[1024];
    memset( temp1, 0, 1024 );
    memset( temp2, 0, 1024 );
    uc_to_str_cpy( temp1, driver );
    uc_to_str_cpy( temp2, L"DSN=" );
    uc_to_str_cpy( temp2, dsn );
    BOOL ret= SQLConfigDataSource( NULL, ODBC_REMOVE_DSN, temp1, temp2 );
    if( !ret )
    {
        GetDSNErrorMessage( errorMsg );
        if( !errorMsg.empty() )
            result = false;
    }
    return result;
}

void ODBCDatabase::SetWindowHandle(SQLHWND handle)
{
    m_handle = handle;
}

void ODBCDatabase::AskForConnectionParameter(bool ask)
{
    m_ask = ask;
}

int ODBCDatabase::GetErrorMessage(std::vector<std::wstring> &errorMsg, int type, SQLHSTMT stmt)
{
    RETCODE ret;
    SQLWCHAR sqlstate[20], msg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER native_error;
    SQLSMALLINT i = 1, msglen, option = SQL_HANDLE_STMT;
    SQLHANDLE handle = m_hstmt;
    switch( type )
    {
        case 0:
            option = SQL_HANDLE_ENV;
            handle = m_env;
            break;
        case 1:
            option = SQL_HANDLE_STMT;
            handle = stmt == 0 ? m_hstmt : stmt;
            break;
        case 2:
            option = SQL_HANDLE_DBC;
            handle = stmt == 0 ? m_hdbc : stmt;
            break;
    }
    while( ( ret = SQLGetDiagRec( option, handle, i, sqlstate, &native_error, msg, sizeof( msg ), &msglen ) ) == SQL_SUCCESS )
    {
        std::wstring strMsg;
        i++;
        str_to_uc_cpy( strMsg, msg );
        errorMsg.push_back( strMsg );
    }
    return 0;
}

int ODBCDatabase::GetDSNErrorMessage(std::vector<std::wstring> &errorMsg)
{
    WORD i = 1, cbErrorMsgMax = SQL_MAX_MESSAGE_LENGTH, pcbErrorMsg;
    SQLWCHAR errorMessage[512];
    DWORD pfErrorCode;
    RETCODE ret;
    while( ( ret = SQLInstallerError( i, &pfErrorCode, errorMessage, cbErrorMsgMax, &pcbErrorMsg ) ) == SQL_SUCCESS )
    {
        if( pfErrorCode != ODBC_ERROR_USER_CANCELED )
        {
            std::wstring strMsg;
            i++;
            str_to_uc_cpy( strMsg, errorMessage );
            errorMsg.push_back( strMsg );
        }
        else break;
    }
    return 0;
}

void ODBCDatabase::uc_to_str_cpy(SQLWCHAR *dest, const std::wstring &src)
{
    const wchar_t *temp = src.c_str();
    while( *dest )
    {
        *dest++;
    }
    while( *temp )
    {
        *dest = *temp;
        *dest++;
        *temp++;
    }
    *dest++ = 0;
    *dest = 0;
}

void ODBCDatabase::str_to_uc_cpy(std::wstring &dest, const SQLWCHAR *src)
{
    while( *src )
    {
        dest += *src++;
    }
}

bool ODBCDatabase::equal(SQLWCHAR *dest, SQLWCHAR *src)
{
    bool eq = true;
    while( *dest && eq )
        if( *src++ != *dest++ )
            eq = false;
    return eq;
}

void ODBCDatabase::copy_uc_to_uc(SQLWCHAR *dest, SQLWCHAR *src)
{
    while( *dest )
        *dest++;
    while( *src )
        *dest++ = *src++;
    *dest = 0;
}

int ODBCDatabase::GetSQLStringSize(SQLWCHAR *str)
{
    int ret = 1;
    while( *str++ )
        ret++;
    return ret;
}

int ODBCDatabase::GetDriverForDSN(SQLWCHAR *dsn, SQLWCHAR *driver, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    bool found = false;
    RETCODE ret1;
    SQLUSMALLINT direct = SQL_FETCH_FIRST;
    SWORD pcbDSN, pcbDesc;
    SQLWCHAR dsnDescr[255], dsnRead[SQL_MAX_DSN_LENGTH + 1];
    ret1 = SQLDataSources( m_env, direct, dsnRead, SQL_MAX_DSN_LENGTH, &pcbDSN, dsnDescr, 254, &pcbDesc );
    while( ( ret1 == SQL_SUCCESS || ret1 == SQL_SUCCESS_WITH_INFO ) && !found )
    {
        if( equal( dsn, dsnRead ) )
            found = true;
        direct = SQL_FETCH_NEXT;
        ret1 = SQLDataSources( m_env, direct, dsnRead, SQL_MAX_DSN_LENGTH, &pcbDSN, dsnDescr, 254, &pcbDesc );
    }
    if( ret1 != SQL_SUCCESS && ret1 != SQL_NO_DATA )
    {
        GetErrorMessage( errorMsg, 0 );
        result = 1;
    }
    else
    {
        if( found )
            copy_uc_to_uc( driver, dsnDescr );
    }
    return result;
}

int ODBCDatabase::CreateDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    SQLWCHAR *query = NULL;
    RETCODE ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
    {
        query = new SQLWCHAR[name.length() + 2];
        memset( query, '\0', name.length() + 2 );
        uc_to_str_cpy( query, name );
        ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2 );
            result = 1;
        }
        else
        {
            if( m_hstmt )
            {
                ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 2 );
                    result = 1;
                }
                else
                    m_hstmt = 0;
            }
        }
    }
    else
    {
        GetErrorMessage( errorMsg, 2 );
        result = 1;
    }
    delete query;
    query = NULL;
    return result;
}

int ODBCDatabase::DropDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    SQLWCHAR *query = NULL;
    RETCODE ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
    {
        query = new SQLWCHAR[name.length() + 2];
        memset( query, '\0', name.length() + 2 );
        uc_to_str_cpy( query, name );
        ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2 );
            result = 1;
        }
        else
        {
            if( m_hstmt )
            {
                ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 2 );
                    result = 1;
                }
                else
                    m_hstmt = 0;
            }
        }
    }
    else
    {
        GetErrorMessage( errorMsg, 2 );
        result = 1;
    }
    delete query;
    query = NULL;
    return result;
}

int ODBCDatabase::Connect(const std::wstring &selectedDSN, std::vector<std::wstring> &dbList, std::vector<std::wstring> &errorMsg)
{
    int result = 0, bufferSize = 1024;
    std::vector<SQLWCHAR *> errorMessage;
    SQLWCHAR connectStrIn[sizeof(SQLWCHAR) * 255], driver[1024], dsn[1024], dbType[1024], *query = NULL, dbName[1024];
    SQLSMALLINT OutConnStrLen;
    SQLRETURN ret;
    SQLUSMALLINT options;
    SQLWCHAR *user = NULL, *password = NULL;
    std::wstring connectingDSN, connectingUser = L"", connectingPassword = L"";
    if( !pimpl )
    {
        pimpl = new Impl;
        pimpl->m_type = L"ODBC";
    }
    if( !odbc_pimpl )
        odbc_pimpl = new ODBCImpl;
    int pos = selectedDSN.find( L';' );
    if( pos == std::wstring::npos )
        connectingDSN = selectedDSN;
    else
    {
        connectingDSN = selectedDSN.substr( 0, pos );
        std::wstring temp = selectedDSN.substr( pos + 1 );
        pos = temp.find( L';' );
        connectingUser = temp.substr( 0, pos );
        connectingPassword = temp.substr( pos + 1 );
        user = new SQLWCHAR[connectingUser.length() + 2];
        password = new SQLWCHAR[connectingPassword.length() + 2];
        memset( user, '\0', connectingUser.length() + 2 );
        memset( password, '\0', connectingPassword.length() + 2 );
        uc_to_str_cpy( user, connectingUser );
        uc_to_str_cpy( password, connectingPassword );
    }
    m_connectString = new SQLWCHAR[sizeof(SQLWCHAR) * 1024];
    memset( dsn, 0, sizeof( dsn ) );
    memset( connectStrIn, 0, sizeof( connectStrIn ) );
    memset( driver, 0, sizeof( driver ) );
    uc_to_str_cpy( dsn, connectingDSN.c_str() );
    ret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HENV, &m_env );
    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
    {
        ret = SQLSetEnvAttr( m_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER );
        if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
        {
            ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &m_hdbc );
            if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
            {
                if( !GetDriverForDSN( dsn, driver, errorMsg ) )
                {
                    SQLWCHAR *temp = new SQLWCHAR[13];
                    memset( temp, '\0', 13 );
                    uc_to_str_cpy( temp, L"PostgreSQL " );
                    uc_to_str_cpy( connectStrIn, L"DSN=" );
                    uc_to_str_cpy( connectStrIn, selectedDSN.c_str() );
                    uc_to_str_cpy( connectStrIn, L";Driver=" );
                    copy_uc_to_uc( connectStrIn, driver );
                    if( equal( temp, driver ) )
                        uc_to_str_cpy( connectStrIn, L";UseServerSidePrepare=1" );
                    delete temp;
                    temp = NULL;
                    if( user && password )
                    {
                        uc_to_str_cpy( connectStrIn, L";UID=" );
                        copy_uc_to_uc( connectStrIn, user );
                        uc_to_str_cpy( connectStrIn, L";PWD=" );
                        copy_uc_to_uc( connectStrIn, password );
                    }
                    ret = SQLSetConnectAttr( m_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0 );
                    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                    {
#ifdef _WIN32
                        options = m_ask ? SQL_DRIVER_COMPLETE_REQUIRED : SQL_DRIVER_COMPLETE;
#else
                        options = SQL_DRIVER_NOPROMPT;
#endif
                        ret = SQLDriverConnect( m_hdbc, m_handle, connectStrIn, SQL_NTS, m_connectString, 1024, &OutConnStrLen, options );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                        {
                            GetErrorMessage( errorMsg, 2 );
                            result = 1;
                        }
                        else
                        {
                            str_to_uc_cpy( pimpl->m_connectString, m_connectString );
                            ret = SQLGetInfo( m_hdbc, SQL_DBMS_NAME, dbType, (SQLSMALLINT) bufferSize, (SQLSMALLINT *) &bufferSize );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 2 );
                                result = 1;
                            }
                            else
                            {
                                str_to_uc_cpy( pimpl->m_subtype, dbType );
                                bufferSize = 1024;
                                ret = SQLGetInfo( m_hdbc, SQL_DATABASE_NAME, dbName, (SQLSMALLINT) bufferSize, (SQLSMALLINT *) &bufferSize );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 2 );
                                    result = 1;
                                }
                                else
                                {
                                    str_to_uc_cpy( pimpl->m_dbName, dbName );
                                    if( !pimpl->m_dbName.empty() )
                                        connectToDatabase = true;
                                    if( GetServerVersion( errorMsg ) )
                                    {
                                        result = 1;
                                    }
                                    else
                                    {
/**************************************/
                                        SQLWCHAR *query;
                                        std::wstring query8, query9, query10, query11;
                                        SQLRETURN retcode = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
                                        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 2, m_hdbc );
                                            result = 1;
                                        }
                                        else
                                        {
                                            if( pimpl->m_subtype == L"Microsoft SQL Server" )
                                            {
                                                std::wstring query8 = L"IF EXISTS(SELECT * FROM sys.databases WHERE name = \'" +  pimpl->m_dbName + L"\' AND is_broker_enabled = 0) ALTER DATABASE " + pimpl->m_dbName + L" SET ENABLE_BROKER";
                                                std::wstring query9 = L"IF NOT EXISTS(SELECT * FROM sys.service_queues WHERE name = \'EventNotificationQueue\') CREATE QUEUE dbo.EventNotificationQueue";
                                                std::wstring query10 = L"IF NOT EXISTS(SELECT * FROM sys.services WHERE name = \'//" + pimpl->m_dbName + L"/EventNotificationService\') CREATE SERVICE [//" + pimpl->m_dbName +L"/EventNotificationService] ON QUEUE dbo.EventNotificationQueue([http://schemas.microsoft.com/SQL/Notifications/PostEventNotification])";
                                                std::wstring query11 = L"IF NOT EXISTS(SELECT * FROM sys.event_notifications WHERE name = \'SchemaChangeEventsTable\') CREATE EVENT NOTIFICATION SchemaChangeEventsTable ON DATABASE FOR DDL_TABLE_EVENTS TO SERVICE \'//" + pimpl->m_dbName + L"/EventNotificationService\' , \'current database\'";
                                                std::wstring query12 = L"IF NOT EXISTS(SELECT * FROM sys.event_notifications WHERE name = \'SchemaChangeEventsIndex\') CREATE EVENT NOTIFICATION SchemaChangeEventsIndex ON DATABASE FOR DDL_INDEX_EVENTS TO SERVICE \'//" + pimpl->m_dbName + L"/EventNotificationService\' , \'current database\'";
                                                std::wstring query13 = L"IF NOT EXISTS(SELECT * FROM sys.event_notifications WHERE name = \'SchemaChangeEventsView\') CREATE EVENT NOTIFICATION SchemaChangeEventsView ON DATABASE FOR DDL_VIEW_EVENTS TO SERVICE \'//" + pimpl->m_dbName + L"/EventNotificationService\' , \'current database\'";
                                                query = new SQLWCHAR[query8.size() + 2];
                                                memset( query, '\0', query8.size() + 2 );
                                                uc_to_str_cpy( query, query8 );
                                                ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                                delete query;
                                                query = NULL;
                                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                {
                                                    GetErrorMessage( errorMsg, 1 );
                                                    result = 1;
                                                }
                                                else
                                                {
                                                    query = new SQLWCHAR[query9.size() + 2];
                                                    memset( query, '\0', query9.size() + 2 );
                                                    uc_to_str_cpy( query, query9 );
                                                    ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                                    delete query;
                                                    query = NULL;
                                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                    {
                                                        GetErrorMessage( errorMsg, 1 );
                                                        result = 1;
                                                    }
                                                    else
                                                    {
                                                        query = new SQLWCHAR[query10.size() + 2];
                                                        memset( query, '\0', query10.size() + 2 );
                                                        uc_to_str_cpy( query, query10 );
                                                        ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                                        delete query;
                                                        query = NULL;
                                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                        {
                                                            GetErrorMessage( errorMsg, 1 );
                                                            result = 1;
                                                        }
                                                        else
                                                        {
                                                            query = new SQLWCHAR[query11.size() + 2];
                                                            memset( query, '\0', query11.size() + 2 );
                                                            uc_to_str_cpy( query, query11 );
                                                            ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                                            delete query;
                                                            query = NULL;
                                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                            {
                                                                GetErrorMessage( errorMsg, 1 );
                                                                result = 1;
                                                            }
                                                            else
                                                            {
                                                                query = new SQLWCHAR[query12.size() + 2];
                                                                memset( query, '\0', query12.size() + 2 );
                                                                uc_to_str_cpy( query, query12 );
                                                                ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                                                delete query;
                                                                query = NULL;
                                                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                                {
                                                                    GetErrorMessage( errorMsg, 1 );
                                                                    result = 1;
                                                                }
                                                                else
                                                                {
                                                                    query = new SQLWCHAR[query13.size() + 2];
                                                                    memset( query, '\0', query13.size() + 2 );
                                                                    uc_to_str_cpy( query, query13 );
                                                                    ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                                                    delete query;
                                                                    query = NULL;
                                                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                                    {
                                                                        GetErrorMessage( errorMsg, 1 );
                                                                        result = 1;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if( pimpl->m_subtype == L"PostgreSQL" )
                                    {
                                        if( pimpl->m_versionMajor >= 9 && pimpl->m_versionMinor >= 3 )
                                        {
                                            std::wstring query8 = L"IF NOT EXIST(SELECT 1 FROM pg_proc AS proc, pg_namespace AS ns WHERE proc.pronamespace = ns.oid AND ns.nspname = \'public\' AND proname = \'watch_schema_changes\') CREATE FUNCTION watch_schema_changes() RETURNS event_trigger LANGUAGE plpgsql AS $$ BEGIN NOTIFY tg_tag; END; $$;";
                                            std::wstring query9 = L"CREATE EVENT TRIGGER schema_change_notify ON ddl_command_end WHEN TAG IN(\'CREATE TABLE\', \'ALTER TABLE\', \'DROP TABLE\', \'CREATE INDEX\', \'DROP INDEX\') EXECUTE PROCEDURE watch_schema_changes();";
                                        }
                                    }
                                    ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 2 );
                                        result = 1;
                                    }
/*****************************************/
                                    else
                                    {
                                        if( pimpl->m_subtype != L"Sybase" && pimpl->m_subtype != L"ASE" )
                                        {
                                            ret = SQLSetConnectAttr( m_hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) FALSE, 0 );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 2 );
                                                result = 1;
                                                ret = SQLEndTran( SQL_HANDLE_DBC, m_hdbc, SQL_ROLLBACK );
                                            }
                                        }
                                        if( !result )
                                        {
                                            if( !connectToDatabase )
                                            {
                                                if( ServerConnect( dbList, errorMsg ) )
                                                {
                                                    result = 1;
                                                }
                                            }
                                            else
                                            {
                                                if( CreateSystemObjectsAndGetDatabaseInfo( errorMsg ) )
                                                {
                                                    result = 1;
                                                    ret = SQLEndTran( SQL_HANDLE_DBC, m_hdbc, SQL_ROLLBACK );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                GetErrorMessage( errorMsg, 2 );
                result = 1;
            }
        }
        else
        {
            GetErrorMessage( errorMsg, 2 );
            result = 1;
        }
    }
    else
    {
        GetErrorMessage( errorMsg, 0 );
        result = 1;
    }
    if( result )
    {
        for( std::vector<SQLWCHAR *>::iterator it = errorMessage.begin(); it != errorMessage.end(); it++ )
        {
            std::wstring strMsg;
            str_to_uc_cpy( strMsg, (*it) );
            errorMsg.push_back( strMsg );
        }
    }
    else
        m_isConnected = true;
    delete query;
    query = NULL;
    return result;
}

int ODBCDatabase::CreateSystemObjectsAndGetDatabaseInfo(std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    std::wstring query1, query2, query3, query4, query5, query6, query7, query8, query9;
    SQLWCHAR *query = NULL;
    if( pimpl->m_subtype == L"Microsoft SQL Server" ) // MS SQL SERVER
    {
        query1 = L"IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='abcatcol' AND xtype='U') CREATE TABLE \"abcatcol\"(abc_tnam varchar(129) NOT NULL, abc_tid integer, abc_ownr varchar(129) NOT NULL, abc_cnam varchar(129) NOT NULL, abc_cid smallint, abc_labl varchar(254), abc_lpos smallint, abc_hdr varchar(254), abc_hpos smallint, abc_itfy smallint, abc_mask varchar(31), abc_case smallint, abc_hght smallint, abc_wdth smallint, abc_ptrn varchar(31), abc_bmap char(1), abc_init varchar(254), abc_cmnt varchar(254), abc_edit varchar(31), abc_tag varchar(254) PRIMARY KEY( abc_tnam, abc_ownr, abc_cnam ));";
        query2 = L"IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='abcatedt' AND xtype='U') CREATE TABLE \"abcatedt\"(abe_name varchar(30) NOT NULL, abe_edit varchar(254), abe_type smallint, abe_cntr integer, abe_seqn smallint NOT NULL, abe_flag integer, abe_work varchar(32) PRIMARY KEY( abe_name, abe_seqn ));";
        query3 = L"IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='abcatfmt' AND xtype='U') CREATE TABLE \"abcatfmt\"(abf_name varchar(30) NOT NULL, abf_frmt varchar(254), abf_type smallint, abf_cntr integer PRIMARY KEY( abf_name ));";
        query4 = L"IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='abcattbl' AND xtype='U') CREATE TABLE \"abcattbl\"(abt_tnam char(129) NOT NULL, abt_tid integer, abt_ownr char(129) NOT NULL, abd_fhgt smallint, abd_fwgt smallint, abd_fitl char(1), abd_funl integer, abd_fstr integer, abd_fchr smallint, abd_fptc smallint, abd_ffce char(18), abh_fhgt smallint, abh_fwgt smallint, abh_fitl char(1), abh_funl integer, abh_fstr integer, abh_fchr smallint, abh_fptc smallint, abh_ffce char(18), abl_fhgt smallint, abl_fwgt smallint, abl_fitl char(1), abl_funl integer, abl_fstr integer, abl_fchr smallint, abl_fptc smallint, abl_ffce char(18), abt_cmnt char(254) PRIMARY KEY( abt_tnam, abt_ownr ));";
        query5 = L"IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='abcatvld' AND xtype='U') CREATE TABLE \"abcatvld\"(abv_name varchar(30) NOT NULL, abv_vald varchar(254), abv_type smallint, abv_cntr integer, abv_msg varchar(254) PRIMARY KEY( abv_name ));";
        query6 = L"IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name='abcattbl_tnam_ownr' AND object_id = OBJECT_ID('abcattbl')) CREATE INDEX \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"abt_tnam\" ASC, \"abt_ownr\" ASC);";
        query7 = L"IF NOT EXISTS (SELECT * FROM sys.indexes WHERE name='abcatcol_tnam_ownr_cnam' AND object_id = OBJECT_ID('abcatcol')) CREATE INDEX \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"abc_tnam\" ASC, \"abc_ownr\" ASC, \"abc_cnam\" ASC);";
    }
    if( pimpl->m_subtype == L"MySQL" || pimpl->m_subtype == L"PostgreSQL" )
    {
        query1 = L"CREATE TABLE IF NOT EXISTS abcatcol(abc_tnam char(129) NOT NULL, abc_tid integer, abc_ownr char(129) NOT NULL, abc_cnam char(129) NOT NULL, abc_cid smallint, abc_labl char(254), abc_lpos smallint, abc_hdr char(254), abc_hpos smallint, abc_itfy smallint, abc_mask char(31), abc_case smallint, abc_hght smallint, abc_wdth smallint, abc_ptrn char(31), abc_bmap char(1), abc_init char(254), abc_cmnt char(254), abc_edit char(31), abc_tag char(254), PRIMARY KEY( abc_tnam, abc_ownr, abc_cnam ));";
        query2 = L"CREATE TABLE IF NOT EXISTS abcatedt(abe_name char(30) NOT NULL, abe_edit char(254), abe_type smallint, abe_cntr integer, abe_seqn smallint NOT NULL, abe_flag integer, abe_work char(32), PRIMARY KEY( abe_name, abe_seqn ));";
        query3 = L"CREATE TABLE IF NOT EXISTS abcatfmt(abf_name char(30) NOT NULL, abf_frmt char(254), abf_type smallint, abf_cntr integer, PRIMARY KEY( abf_name ));";
        query4 = L"CREATE TABLE IF NOT EXISTS abcattbl(abt_tnam char(129) NOT NULL, abt_tid integer, abt_ownr char(129) NOT NULL, abd_fhgt smallint, abd_fwgt smallint, abd_fitl char(1), abd_funl integer, abd_fstr integer, abd_fchr smallint, abd_fptc smallint, abd_ffce char(18), abh_fhgt smallint, abh_fwgt smallint, abh_fitl char(1), abh_funl integer, abh_fstr integer, abh_fchr smallint, abh_fptc smallint, abh_ffce char(18), abl_fhgt smallint, abl_fwgt smallint, abl_fitl char(1), abl_funl integer, abl_fstr integer, abl_fchr smallint, abl_fptc smallint, abl_ffce char(18), abt_cmnt char(254), PRIMARY KEY( abt_tnam, abt_ownr ));";
        query5 = L"CREATE TABLE IF NOT EXISTS abcatvld(abv_name char(30) NOT NULL, abv_vald char(254), abv_type smallint, abv_cntr integer, abv_msg char(254), PRIMARY KEY( abv_name ));";
        if( pimpl->m_subtype == L"MySQL" )
        {
            query6 = L"SELECT( IF( ( SELECT 1 FROM information_schema.statistics WHERE index_name=\'abcattbl_tnam_ownr\' AND table_name=\'abcattbl\' ) > 0, \"SELECT 0\", \"CREATE INDEX abcattbl_tnam_ownr ON abcattbl(abt_tnam ASC, abt_ownr ASC)\"));";
            query7 = L"SELECT( IF( ( SELECT 1 FROM information_schema.statistics WHERE index_name=\'abcatcol_tnam_ownr_cnam\' AND table_name=\'abcatcol\' ) > 0, \"SELECT 0\", \"CREATE INDEX abcatcol_tnam_ownr_cnam ON abcatcoll(abc_tnam ASC, abc_ownr ASC, abc_cnam ASC)\"));";
        }
        else
        {
            if( pimpl->m_versionMajor >= 9 && pimpl->m_versionMinor >= 5 )
            {
                query6 = L"CREATE INDEX IF NOT EXISTS \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"abt_tnam\" ASC, \"abt_ownr\" ASC);";
                query7 = L"CREATE INDEX IF NOT EXISTS \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"abc_tnam\" ASC, \"abc_ownr\" ASC, \"abc_cnam\" ASC);";
            }
            else
            {
                query6 = L"DO $$ BEGIN IF NOT EXISTS( SELECT 1 FROM pg_class c, pg_namespace n WHERE n.oid = c.relnamespace AND c.relname = \'abcattbl_tnam_ownr\' AND n.nspname = \'public\' ) THEN CREATE INDEX  \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"abt_tnam\" ASC, \"abt_ownr\" ASC); END IF; END;";
                query7 = L"DO $$ BEGIN IF NOT EXISTS( SELECT 1 FROM pg_class c, pg_namespace n WHERE n.oid = c.relnamespace AND c.relname = \'abcatcol_tnam_ownr_cnam\' AND n.nspname = \'public\' ) THEN CREATE INDEX \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"abc_tnam\" ASC, \"abc_ownr\" ASC, \"abc_cnam\" ASC); END IF; END;";
            }
        }
    }
    if( pimpl->m_subtype == L"Sybase" || pimpl->m_subtype == L"ASE" )
    {
        query1 = L"IF NOT EXISTS(SELECT 1 FROM sysobjects WHERE name = 'abcatcol' AND type = 'U') CREATE TABLE abcatcol(abc_tnam char(129) NOT NULL, abc_tid integer, abc_ownr char(129) NOT NULL, abc_cnam char(129) NOT NULL, abc_cid smallint, abc_labl char(254), abc_lpos smallint, abc_hdr char(254), abc_hpos smallint, abc_itfy smallint, abc_mask char(31), abc_case smallint, abc_hght smallint, abc_wdth smallint, abc_ptrn char(31), abc_bmap char(1), abc_init char(254), abc_cmnt char(254), abc_edit char(31), abc_tag char(254), PRIMARY KEY( abc_tnam, abc_ownr, abc_cnam ))";
        query2 = L"IF NOT EXISTS(SELECT 1 FROM sysobjects WHERE name = 'abcatedt' AND type = 'U') CREATE TABLE abcatedt(abe_name char(30) NOT NULL, abe_edit char(254), abe_type smallint, abe_cntr integer, abe_seqn smallint NOT NULL, abe_flag integer, abe_work char(32), PRIMARY KEY( abe_name, abe_seqn ))";
        query3 = L"IF NOT EXISTS(SELECT 1 FROM sysobjects WHERE name = 'abcatfmt' AND type = 'U') CREATE TABLE abcatfmt(abf_name char(30) NOT NULL, abf_frmt char(254), abf_type smallint, abf_cntr integer, PRIMARY KEY( abf_name ))";
        query4 = L"IF NOT EXISTS(SELECT 1 FROM sysobjects WHERE name = 'abcattbl' AND type = 'U') CREATE TABLE abcattbl(abt_tnam char(129) NOT NULL, abt_tid integer, abt_ownr char(129) NOT NULL, abd_fhgt smallint, abd_fwgt smallint, abd_fitl char(1), abd_funl char(1), abd_fchr smallint, abd_fptc smallint, abd_ffce char(18), abh_fhgt smallint, abh_fwgt smallint, abh_fitl char(1), abh_funl char(1), abh_fchr smallint, abh_fptc smallint, abh_ffce char(18), abl_fhgt smallint, abl_fwgt smallint, abl_fitl char(1), abl_funl char(1), abl_fchr smallint, abl_fptc smallint, abl_ffce char(18), abt_cmnt char(254), PRIMARY KEY( abt_tnam, abt_ownr ))";
        query5 = L"IF NOT EXISTS(SELECT 1 FROM sysobjects WHERE name = 'abcatvld' AND type = 'U') CREATE TABLE abcatvld(abv_name char(30) NOT NULL, abv_vald char(254), abv_type smallint, abv_cntr integer, abv_msg char(254), PRIMARY KEY( abv_name ))";
        query6 = L"IF NOT EXISTS(SELECT o.name, i.name FROM tempdb..sysobjects o, tempdb..sysindexes i WHERE o.id = i.id AND o.name='abcattbl' AND i.name='abcattbl_tnam_ownr') CREATE INDEX \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"abt_tnam\" ASC, \"abt_ownr\" ASC)";
        query7 = L"IF NOT EXISTS(SELECT o.name, i.name FROM tempdb..sysobjects o, tempdb..sysindexes i WHERE o.id = i.id AND o.name='abcatcol' AND i.name='abcatcol_tnam_ownr_cnam') CREATE INDEX \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"abc_tnam\" ASC, \"abc_ownr\" ASC, \"abc_cnam\" ASC)";
    }
    if( pimpl->m_subtype == L"Oracle" )
    {
        query1 = L"IF( SELECT count(*) FROM all_tables WHERE table_name = 'abcatcol' ) <= 0 CREATE TABLE abcatcol(abc_tnam char(129) NOT NULL, abc_tid integer, abc_ownr char(129) NOT NULL, abc_cnam char(129) NOT NULL, abc_cid smallint, abc_labl char(254), abc_lpos smallint, abc_hdr char(254), abc_hpos smallint, abc_itfy smallint, abc_mask char(31), abc_case smallint, abc_hght smallint, abc_wdth smallint, abc_ptrn char(31), abc_bmap char(1), abc_init char(254), abc_cmnt char(254), abc_edit char(31), abc_tag char(254), PRIMARY KEY( abc_tnam, abc_ownr, abc_cnam ));";
        query2 = L"IF( SELECT count(*) FROM all_tables WHERE table_name = 'abcatedt' ) <= 0 CREATE TABLE abcatedt(abe_name char(30) NOT NULL, abe_edit char(254), abe_type smallint, abe_cntr integer, abe_seqn smallint NOT NULL, abe_flag integer, abe_work char(32), PRIMARY KEY( abe_name, abe_seqn ));";
        query3 = L"IF( SELECT count(*) FROM all_tables WHERE table_name = 'abcatfmt' ) <= 0 CREATE TABLE abcatfmt(abf_name char(30) NOT NULL, abf_frmt char(254), abf_type smallint, abf_cntr integer, PRIMARY KEY( abf_name ));";
        query4 = L"IF( SELECT count(*) FROM all_tables WHERE table_name = 'abcattbl' ) <= 0 CREATE TABLE abcattbl(abt_tnam char(129) NOT NULL, abt_tid integer, abt_ownr char(129) NOT NULL, abd_fhgt smallint, abd_fwgt smallint, abd_fitl char(1), abd_funl integer, abd_fstr integer, abd_fchr smallint, abd_fptc smallint, abd_ffce char(18), abh_fhgt smallint, abh_fwgt smallint, abh_fitl char(1), abh_funl integer, abh_fstr integer, abh_fchr smallint, abh_fptc smallint, abh_ffce char(18), abl_fhgt smallint, abl_fwgt smallint, abl_fitl char(1), abl_funl integer, abl_fstr integer, abl_fchr smallint, abl_fptc smallint, abl_ffce char(18), abt_cmnt char(254), PRIMARY KEY( abl_tnam, abl_ownr ));";
        query5 = L"IF( SELECT count(*) FROM all_tables WHERE table_name = 'abcatvld' ) <= 0 CREATE TABLE abcatvld(abv_name char(30) NOT NULL, abv_vald char(254), abv_type smallint, abv_cntr integer, abv_msg char(254), PRIMARY KEY( abv_name ));";
        query6 = L"IF( SELECT count(*) FROM user_indexes WHERE index_name = 'abcattbl_tnam_ownr' ) <= 0 CREATE INDEX \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"tnam\", \"ownr\");";
        query7 = L"IF( SELECT count(*) FROM user_indexes WHERE index_name = 'abcatcol_tnam_ownr_cnam' ) <= 0 CREATE INDEX \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"tnam\", \"ownr\", \"cnam\");";
    }
    RETCODE ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
    {
        query = new SQLWCHAR[query1.length() + 2];
        memset( query, '\0', query1.size() + 2 );
        uc_to_str_cpy( query, query1 );
        ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
        delete query;
        query = NULL;
        if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
        {
            query = new SQLWCHAR[query2.length() + 2];
            memset( query, '\0', query2.size() + 2 );
            uc_to_str_cpy( query, query2 );
            ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
            delete query;
            query = NULL;
            if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
            {
                query = new SQLWCHAR[query3.length() + 2];
                memset( query, '\0', query3.size() + 2 );
                uc_to_str_cpy( query, query3 );
                ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                delete query;
                query = NULL;
                if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                {
                    query = new SQLWCHAR[query4.length() + 2];
                    memset( query, '\0', query4.size() + 2 );
                    uc_to_str_cpy( query, query4 );
                    ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                    delete query;
                    query = NULL;
                    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                    {
                        query = new SQLWCHAR[query5.length() + 2];
                        memset( query, '\0', query5.size() + 2 );
                        uc_to_str_cpy( query, query5 );
                        ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                        delete query;
                        query = NULL;
                        if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                        {
                            if( ( pimpl->m_subtype == L"PostgreSQL" && pimpl->m_versionMajor >= 9 && pimpl->m_versionMinor >= 5 ) || pimpl->m_subtype != L"PostgreSQL" )
                            {
                                query = new SQLWCHAR[query6.length() + 2];
                                memset( query, '\0', query6.size() + 2 );
                                uc_to_str_cpy( query, query6 );
                                ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                delete query;
                                query = NULL;
                                if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                                {
                                    query = new SQLWCHAR[query7.length() + 2];
                                    memset( query, '\0', query7.size() + 2 );
                                    uc_to_str_cpy( query, query7 );
                                    ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                                    delete query;
                                    query = NULL;
                                    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                                    {
                                        ret = SQLEndTran( SQL_HANDLE_DBC, m_hdbc, SQL_COMMIT );
                                    }
                                }
                            }
                            else
                            {
                                if( CreateIndexesOnPostgreConnection( errorMsg ) )
                                {
                                    errorMsg.push_back( L"" );
                                    result = 1;
                                }
                                else
                                    ret = SQLEndTran( SQL_HANDLE_DBC, m_hdbc, SQL_COMMIT );
                            }
                        }
                    }
                }
            }
        }
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1 );
            ret = SQLEndTran( SQL_HANDLE_DBC, m_hdbc, SQL_ROLLBACK );
            result = 1;
        }
        if( m_hstmt )
        {
            ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2 );
                result = 1;
            }
            m_hstmt = 0;
        }
        if( !result )
        {
            ret = SQLSetConnectAttr( m_hdbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) TRUE, 0 );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2 );
                result = 1;
            }
            else
                result = GetTableListFromDb( errorMsg );
        }
    }
    return result;
}

int ODBCDatabase::ServerConnect(std::vector<std::wstring> &UNUSED(dbList), std::vector<std::wstring> &errorMsg)
{
    std::wstring query;
    if( pimpl->m_subtype == L"Microsoft SQL Server" )
        query = L"SELECT name AS Database FROM master.dbo.sysdatabases;";
    if( pimpl->m_subtype == L"Sybase" || pimpl->m_subtype == L"ASE" )
        query = L"SELECT name FROM sp_helpdb";
    if( pimpl->m_subtype == L"MySQL"  )
        query = L"SELECT schema_name AS Database FROM information_schema.schemata;";
    if( pimpl->m_subtype == L"PostgreSQL" )
        query = L"SELECT datname AS Database FROM pg_database;";
    return 0;
}

int ODBCDatabase::Disconnect(std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    RETCODE ret;
    if( m_hstmt != 0 )
    {
        ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
        if( ret != SQL_SUCCESS )
        {
            GetErrorMessage( errorMsg, 2 );
            result = 1;
        }
        else
            m_hstmt = 0;
    }
    if( m_hdbc != 0 )
    {
        if( m_isConnected )
        {
            ret = SQLDisconnect( m_hdbc );
            if( ret != SQL_SUCCESS )
            {
                GetErrorMessage( errorMsg, 2 );
                result = 1;
            }
        }
        ret = SQLFreeHandle( SQL_HANDLE_DBC, m_hdbc );
        if( ret != SQL_SUCCESS )
        {
            GetErrorMessage( errorMsg, 2 );
            result = 1;
        }
        else
            m_hdbc = 0;
    }
    if( m_env != 0 )
    {
        ret = SQLFreeHandle( SQL_HANDLE_ENV, m_env );
        if( ret != SQL_SUCCESS )
        {
            GetErrorMessage( errorMsg, 0 );
            result = 1;
        }
        else
            m_env = 0;
    }
    for( std::map<std::wstring, std::vector<DatabaseTable *> >::iterator it = pimpl->m_tables.begin(); it != pimpl->m_tables.end(); it++ )
    {
        std::vector<DatabaseTable *> tableVec = (*it).second;
        for( std::vector<DatabaseTable *>::iterator it1 = tableVec.begin(); it1 < tableVec.end(); it1++ )
        {
            delete (*it1);
            (*it1) = NULL;
        }
    }
    delete pimpl;
    pimpl = NULL;
    return result;
}

int ODBCDatabase::GetTableListFromDb(std::vector<std::wstring> &errorMsg)
{
    RETCODE ret;
    std::wstring query4;
    int result = 0, bufferSize = 1024;
    std::vector<Field *> fields;
    std::wstring owner;
    std::wstring fieldName, fieldType, defaultValue, primaryKey, fkSchema, fkName, fkTable, schema, table, origSchema, origTable, origCol, refSchema, refTable, refCol, cat;
    std::vector<std::wstring> pk_fields, fk_fieldNames;
    std::vector<std::wstring> indexes;
    std::map<int,std::vector<FKField *> > foreign_keys;
    SQLWCHAR *catalogName = NULL, *schemaName = NULL, *tableName = NULL, *szSchemaName = NULL, *szTableName = NULL;
    SQLWCHAR userName[1024];
    SQLSMALLINT numCols = 0;
    SQLTablesDataBinding *catalog = (SQLTablesDataBinding *) malloc( 5 * sizeof( SQLTablesDataBinding ) );
    ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 1 );
        result = 1;
    }
    else
    {
        for( int i = 0; i < 5; i++ )
        {
            catalog[i].TargetType = SQL_C_WCHAR;
            catalog[i].BufferLength = ( bufferSize + 1 );
            catalog[i].TargetValuePtr = malloc( sizeof( unsigned char ) * catalog[i].BufferLength );
            ret = SQLBindCol( m_hstmt, (SQLUSMALLINT) i + 1, catalog[i].TargetType, catalog[i].TargetValuePtr, catalog[i].BufferLength, &( catalog[i].StrLen_or_Ind ) );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1 );
                result = 1;
                break;
            }
        }
        if( !result )
        {
            ret = SQLTables( m_hstmt, NULL, 0, NULL, 0, NULL, 0, NULL, 0 );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1 );
                result = 1;
            }
            else
            {
                for( ret = SQLFetch( m_hstmt ); ( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) && ret != SQL_NO_DATA; ret = SQLFetch( m_hstmt ) )
                {
                    if( catalog[0].StrLen_or_Ind != SQL_NULL_DATA )
                        catalogName = (SQLWCHAR *) catalog[0].TargetValuePtr;
                    if( catalog[1].StrLen_or_Ind != SQL_NULL_DATA )
                        schemaName = (SQLWCHAR *) catalog[1].TargetValuePtr;
                    if( catalog[2].StrLen_or_Ind != SQL_NULL_DATA )
                        tableName = (SQLWCHAR *) catalog[2].TargetValuePtr;
                    cat = L"";
                    schema = L"";
                    table = L"";
                    str_to_uc_cpy( cat, catalogName );
                    str_to_uc_cpy( schema, schemaName );
                    str_to_uc_cpy( table, tableName );
                    if( schema == L"" && cat != L"" )
                    {
                        schema = cat;
                        copy_uc_to_uc( schemaName, catalogName );
                    }
                    result = AddDropTable( cat, schema, table, true, errorMsg );
                    if( result )
                        break;
                }
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                {
                    fields.erase( fields.begin(), fields.end() );
                    foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                    fields.clear();
                    pk_fields.clear();
                    pk_fields.clear();
                    fk_fieldNames.clear();
                    indexes.clear();
                }
                else if( errorMsg.size() == 0 )
                {
                    bufferSize = 1024;
                    ret = SQLGetInfo( m_hdbc, SQL_USER_NAME, userName, (SQLSMALLINT) bufferSize, (SQLSMALLINT *) &bufferSize );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 2 );
                        result = 1;
                    }
                    else
                    {
                        str_to_uc_cpy( pimpl->m_connectedUser, userName );
                    }
                }
            }
        }
    }
    for( int i = 0; i < 5; i++ )
    {
        free( catalog[i].TargetValuePtr );
    }
    delete szTableName;
    szTableName = NULL;
    delete szSchemaName;
    szSchemaName = NULL;
    free( catalog );
    ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 2 );
        result = 1;
    }
    else
        m_hstmt = 0;
    return result;
}

int ODBCDatabase::CreateIndex(const std::wstring &command, const std::wstring &index_name, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg)
{
    SQLRETURN ret;
    int result = 0;
    std::wstring temp = L"BEGIN TRANSACTION";
    SQLWCHAR *query = NULL;
    query = new SQLWCHAR[temp.length() + 2];
    memset( query, '\0', temp.length() + 2 );
    uc_to_str_cpy( query, temp );
    ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
    {
        ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
        if( ret != SQL_SUCCESS || ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
        else
        {
            delete query;
            query = NULL;
            query = new SQLWCHAR[command.length() + 2];
            memset( query, '\0', command.length() + 2 );
            uc_to_str_cpy( query, command );
            bool exists = IsIndexExists( index_name, schemaName, tableName, errorMsg );
            if( exists )
            {
                std::wstring temp = L"Index ";
                temp += index_name;
                temp += L" already exists.";
                errorMsg.push_back( temp );
                result = 1;
            }
            else if( !errorMsg.empty() )
                result = 1;
            else
            {
                ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                    result = 1;
                }
            }
        }
        if( result == 1 )
            uc_to_str_cpy( query, L"ROLLBACK" );
        else
            uc_to_str_cpy( query, L"COMMIT" );
        ret = SQLExecDirect( m_hstmt, query, SQL_NTS );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
        ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
        else
            m_hstmt = 0;
    }
    delete query;
    query = NULL;
    return result;
}

bool ODBCDatabase::IsIndexExists(const std::wstring &indexName, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg)
{
    SQLRETURN ret;
    SQLWCHAR *query = NULL;
    bool exists = false;
    std::wstring query1;
    SQLWCHAR *index_name = NULL, *table_name = NULL, *schema_name = NULL;
    SQLLEN cbIndexName = SQL_NTS, cbTableName = SQL_NTS, cbSchemaName = SQL_NTS;
    if( pimpl->m_subtype == L"Microsoft SQL Server" )
        query1 = L"SELECT 1 FROM sys.indexes WHERE name = ? AND object_id = OBJECT_ID( ? ) );";
    if( pimpl->m_subtype == L"MySQL" )
        query1 = L"SELECT 1 FROM information_schema.statistics WHERE index_name = ? AND table_name = ? AND schema_name = ?;";
    if( pimpl->m_subtype == L"PostgreSQL" )
        query1 = L"SELECT 1 FROM pg_indexes WHERE indexname = $1 AND tablename = $2 AND schemaname = $3;";
    if( pimpl->m_subtype == L"Sybase" || pimpl->m_subtype == L"ASE" )
        query1 = L"SELECT 1 FROM sysindexes ind, sysobjects obj WHERE obj.id = ind.id AND ind.name = ? AND obj.name = ?";
    index_name = new SQLWCHAR[indexName.length() + 2];
    memset( index_name, '\0', indexName.length() + 2 );
    uc_to_str_cpy( index_name, indexName );
    if( pimpl->m_subtype == L"MySQL" || pimpl->m_subtype == L"PostgreSQL" )
    {
        table_name = new SQLWCHAR[tableName.length() + 2];
        schema_name = new SQLWCHAR[schemaName.length() + 2];
        memset( table_name, '\0', tableName.length() + 2 );
        memset( schema_name, '\0', schemaName.length() + 2 );
        uc_to_str_cpy( schema_name, schemaName );
        uc_to_str_cpy( table_name, tableName );
    }
    else
    {
        table_name = new SQLWCHAR[tableName.length() + schemaName.length() + 2];
        memset( table_name, '\0', tableName.length() + schemaName.length() + 2 );
        uc_to_str_cpy( table_name, schemaName );
        uc_to_str_cpy( table_name, L"." );
        uc_to_str_cpy( table_name, tableName );
    }
    ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
    {
        query = new SQLWCHAR[query1.length() + 2];
        memset( query, '\0', query1.size() + 2 );
        uc_to_str_cpy( query, query1 );
        ret = SQLBindParameter( m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, indexName.length(), 0, index_name, 0, &cbIndexName );
        if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
        {
            ret = SQLBindParameter( m_hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, tableName.length(), 0, table_name, 0, &cbTableName );
            if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
            {
                if( pimpl->m_subtype == L"MySQL" || pimpl->m_subtype == L"PostgreSQL" )
                    ret = SQLBindParameter( m_hstmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, schemaName.length(), 0, schema_name, 0, &cbSchemaName );
                if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                {
                    ret = SQLPrepare( m_hstmt, query, SQL_NTS );
                    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                    {
                        ret = SQLExecute( m_hstmt );
                        if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                        {
                            ret = SQLFetch( m_hstmt );
                            if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                            {
                                exists = true;
                            }
                            else if( ret != SQL_NO_DATA )
                            {
                                GetErrorMessage( errorMsg, 1, m_hstmt );
                            }
                        }
                        else
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                        }
                    }
                    else
                    {
                        GetErrorMessage( errorMsg, 1, m_hstmt );
                    }
                }
                else
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                }
            }
            else
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
            }
        }
        else
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
        }
    }
    else
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
    }
    ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
    }
    else
        m_hstmt = 0;
    delete index_name;
    index_name = NULL;
    delete table_name;
    table_name = NULL;
    delete schema_name;
    schema_name = NULL;
    delete query;
    query = NULL;
    return exists;
}

int ODBCDatabase::GetTableProperties(DatabaseTable *table, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    SQLSMALLINT OutConnStrLen;
    SQLHDBC hdbc_tableProp = 0;
    SQLHSTMT stmt_tableProp = 0;
    SQLWCHAR *qry = NULL, *table_name = NULL, *owner_name = NULL;
    unsigned short dataFontSize, dataFontWeight = 0, dataFontUnderline, dataFontStriken, headingFontSize, headingFontWeight, headingFontUnderline, headingFontStriken, labelFontSize, labelFontWeight, labelFontUnderline, labelFontStriken;
    unsigned short dataFontCharacterSet, headingFontCharacterSet, labelFontCharacterSet, dataFontPixelSize, headingFontPixelSize, labelFontPixelSize;
    SQLWCHAR dataFontItalic[2], headingFontItalic[2], labelFontItalic[2], dataFontName[20], headingFontName[20], labelFontName[20];
    SQLWCHAR comments[225];
    SQLLEN cbDataFontSize = 0, cbDataFontWeight = 0, cbDataFontItalic = SQL_NTS, cbDataFontUnderline = SQL_NTS, cbDataFontStriken = SQL_NTS, cbDataFontName = 0, cbHeadingFontSize = 0, cbHeadingFontWeight = 0;
    SQLLEN cbTableName = SQL_NTS, cbHeadingFontItalic = 0,  cbHeadingFontUnderline = 0, cbHeadingFontStriken = 0, cbHeadingFontName = 0, cbComment;
    SQLLEN cbLabelFontSize = 0, cbLabelFontWeight = 0, cbLabelFontItalic = 0, cbLabelFontUnderline = 0, cbLabelFontStriken = 0, cbLabelFontName = 0;
    SQLLEN cbDataFontCharacterSet = 0, cbHeadingFontCharacterSet = 0, cbLabelFontCharacterSet = 0, cbDataFontPixelSize = 0, cbHeadingFontPixelSize = 0, cbLabelFontPixelSize = 0;
    std::wstring query = L"SELECT rtrim(abt_tnam), abt_tid, rtrim(abt_ownr), abd_fhgt, abd_fwgt, abd_fitl, abd_funl, abd_fstr, abd_fchr, abd_fptc, rtrim(abd_ffce), abh_fhgt, abh_fwgt, abh_fitl, abh_funl, abh_fstr, abh_fchr, abh_fptc, rtrim(abh_ffce), abl_fhgt, abl_fwgt, abl_fitl, abl_funl, abl_fstr, abl_fchr, abl_fptc, rtrim(abl_ffce), rtrim(abt_cmnt) FROM abcattbl WHERE rtrim(\"abt_tnam\") = ? AND rtrim(\"abt_ownr\") = ?;";
    std::wstring tableName = table->GetTableName(), schemaName = table->GetSchemaName(), ownerName = table->GetTableOwner();
    std::wstring t = schemaName + L".";
    t += tableName;
    int tableNameLen = (int) t.length(), ownerNameLen = (int) ownerName.length();
    SQLLEN cbOwnerName = ownerNameLen == 0 ? SQL_NULL_DATA : SQL_NTS;
    qry = new SQLWCHAR[query.length() + 2], table_name = new SQLWCHAR[tableNameLen + 2], owner_name = new SQLWCHAR[ownerNameLen + 2];
    memset( owner_name, '\0', ownerNameLen + 2 );
    memset( table_name, '\0', tableNameLen + 2 );
    uc_to_str_cpy( owner_name, ownerName );
    uc_to_str_cpy( table_name, t );
    memset( qry, '\0', query.size() + 2 );
    uc_to_str_cpy( qry, query );
    SQLRETURN ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc_tableProp );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 0, m_env );
        result = 1;
    }
    else
    {
        ret = SQLDriverConnect( hdbc_tableProp, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2, m_env );
            result = 1;
        }
        else
        {
            ret = SQLAllocHandle( SQL_HANDLE_STMT, hdbc_tableProp, &stmt_tableProp );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2, m_hdbc );
                result = 1;
            }
            else
            {
                ret = SQLPrepare( stmt_tableProp, qry, SQL_NTS );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, stmt_tableProp );
                    result = 1;
                }
                else
                {
                    SQLSMALLINT DataType, DecimalDigits, Nullable;
                    SQLULEN ParamSize;
                    ret = SQLDescribeParam( stmt_tableProp, 1, &DataType, &ParamSize, &DecimalDigits, &Nullable);
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 1, stmt_tableProp );
                        SQLFreeHandle( SQL_HANDLE_STMT, stmt_tableProp );
                        SQLDisconnect( hdbc_tableProp );
                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_tableProp );
                        hdbc_tableProp = 0;
                        stmt_tableProp = 0;
                        result = 1;
                    }
                    else
                    {
                        ret = SQLBindParameter( stmt_tableProp, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, DataType, ParamSize, DecimalDigits, table_name, 0, &cbTableName );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, stmt_tableProp );
                            SQLFreeHandle( SQL_HANDLE_STMT, stmt_tableProp );
                            SQLDisconnect( hdbc_tableProp );
                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_tableProp );
                            hdbc_tableProp = 0;
                            stmt_tableProp = 0;
                            result = 1;
                        }
                        else
                        {
                            ret = SQLDescribeParam( stmt_tableProp, 2, &DataType, &ParamSize, &DecimalDigits, &Nullable);
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                SQLFreeHandle( SQL_HANDLE_STMT, stmt_tableProp );
                                SQLDisconnect( hdbc_tableProp );
                                SQLFreeHandle( SQL_HANDLE_DBC, hdbc_tableProp );
                                hdbc_tableProp = 0;
                                stmt_tableProp = 0;
                                result = 1;
                            }
                            else
                            {
                                ret = SQLBindParameter( stmt_tableProp, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, DataType, ParamSize, DecimalDigits, owner_name, 0, &cbOwnerName );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                    result = 1;
                                }
                                else
                                {
                                    ret = SQLExecute( stmt_tableProp );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                        result = 1;
                                    }
                                    else
                                    {
                                        ret = SQLBindCol( stmt_tableProp, 4, SQL_C_SSHORT, &dataFontSize, 0, &cbDataFontSize );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                            result = 1;
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 5, SQL_C_SSHORT, &dataFontWeight, 0, &cbDataFontWeight );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 6, SQL_C_CHAR, &dataFontItalic, 2, &cbDataFontItalic );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 7, SQL_C_SSHORT, &dataFontUnderline, 2, &cbDataFontUnderline );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 8, SQL_C_SSHORT, &dataFontStriken, 2, &cbDataFontStriken );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 9, SQL_C_SSHORT, &dataFontCharacterSet, 0, &cbDataFontCharacterSet );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 10, SQL_C_SSHORT, &dataFontPixelSize, 0, &cbDataFontPixelSize );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 11, SQL_C_WCHAR, &dataFontName, 44, &cbDataFontName );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 12, SQL_C_SSHORT, &headingFontSize, 0, &cbHeadingFontSize );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 13, SQL_C_SSHORT, &headingFontWeight, 0, &cbHeadingFontWeight );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 14, SQL_C_CHAR, &headingFontItalic, 2, &cbHeadingFontItalic );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 15, SQL_C_SSHORT, &headingFontUnderline, 2, &cbHeadingFontUnderline );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 16, SQL_C_SSHORT, &headingFontStriken, 2, &cbHeadingFontStriken );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 17, SQL_C_SSHORT, &headingFontCharacterSet, 0, &cbHeadingFontCharacterSet );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 18, SQL_C_SSHORT, &headingFontPixelSize, 0, &cbHeadingFontPixelSize );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 19, SQL_C_WCHAR, &headingFontName, 44, &cbHeadingFontName );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 20, SQL_C_SSHORT, &labelFontSize, 0, &cbLabelFontSize );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 21, SQL_C_SSHORT, &labelFontWeight, 0, &cbLabelFontWeight );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 22, SQL_C_CHAR, &labelFontItalic, 3, &cbLabelFontItalic );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 23, SQL_C_SSHORT, &labelFontUnderline, 3, &cbLabelFontUnderline );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 24, SQL_C_SSHORT, &labelFontStriken, 3, &cbLabelFontStriken );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 25, SQL_C_SSHORT, &labelFontCharacterSet, 0, &cbLabelFontCharacterSet );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 26, SQL_C_SSHORT, &labelFontPixelSize, 0, &cbLabelFontPixelSize );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 27, SQL_C_WCHAR, &labelFontName, 44, &cbLabelFontName );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLBindCol( stmt_tableProp, 28, SQL_C_WCHAR, &comments, 225, &cbComment );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                        if( !result )
                                        {
                                            ret = SQLFetch( stmt_tableProp );
                                            if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                                            {
                                                std::wstring name;
                                                table->SetDataFontSize( dataFontSize );
                                                table->SetDataFontWeight( dataFontWeight );
                                                table->SetDataFontItalic( dataFontItalic[0] == 'Y' );
                                                table->SetDataFontUnderline( dataFontUnderline == 1 );
                                                table->SetDataFontStrikethrough( dataFontStriken == 1 );
                                                table->SetDataFontCharacterSet( dataFontCharacterSet );
                                                table->SetDataFontPixelSize( dataFontPixelSize );
                                                str_to_uc_cpy( name, dataFontName );
                                                table->SetDataFontName( name );
                                                name = L"";
                                                table->SetHeadingFontSize( headingFontSize );
                                                table->SetHeadingFontWeight( headingFontWeight );
                                                table->SetHeadingFontItalic( headingFontItalic[0] == 'Y' );
                                                table->SetHeadingFontUnderline( headingFontUnderline == 1 );
                                                table->SetHeadingFontStrikethrough( headingFontStriken == 1 ? true : false );
                                                table->SetHeadingFontCharacterSet( headingFontCharacterSet );
                                                table->SetHeadingFontPixelSize( headingFontPixelSize );
                                                str_to_uc_cpy( name, headingFontName );
                                                table->SetHeadingFontName( name );
                                                name = L"";
                                                table->SetLabelFontSize( labelFontSize );
                                                table->SetLabelFontWeight( labelFontWeight );
                                                table->SetLabelFontItalic( labelFontItalic[0] == 'Y' );
                                                table->SetLabelFontUnderline( labelFontUnderline == 1 );
                                                table->SetLabelFontStrikethrough( labelFontStriken == 1 );
                                                table->SetLabelFontCharacterSet( labelFontCharacterSet );
                                                table->SetLabelFontPixelSize( labelFontPixelSize );
                                                str_to_uc_cpy( name, labelFontName );
                                                table->SetLabelFontName( name );
                                                name = L"";
                                                str_to_uc_cpy( name, comments );
                                                table->SetComment( name );
                                                name = L"";
                                            }
                                            else if( ret != SQL_NO_DATA )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                            ret = SQLFreeHandle( SQL_HANDLE_STMT, stmt_tableProp );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                            else
                                                stmt_tableProp = 0;
                                            ret = SQLDisconnect( hdbc_tableProp );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                            ret = SQLFreeHandle( SQL_HANDLE_DBC, hdbc_tableProp );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_tableProp );
                                                result = 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    delete qry;
    qry = NULL;
    delete table_name;
    table_name = NULL;
    delete owner_name;
    owner_name = NULL;
    return 0;
}

int ODBCDatabase::SetTableProperties(const DatabaseTable *table, const TableProperties &properties, bool isLog, std::wstring &command, std::vector<std::wstring> &errorMsg)
{
    SQLRETURN ret;
    int result = 0;
    bool exist;
    std::wostringstream istr;
    std::wstring query = L"BEGIN TRANSACTION";
    SQLWCHAR *qry = new SQLWCHAR[query.length() + 2];
    memset( qry, '\0', query.length() + 2 );
    uc_to_str_cpy( qry, query );
    RETCODE rets = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( rets == SQL_SUCCESS || rets == SQL_SUCCESS_WITH_INFO )
    {
        ret = SQLExecDirect( m_hstmt, qry, SQL_NTS );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
        else
        {
            rets = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
            if( rets != SQL_SUCCESS && rets != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
                result = 1;
            }
            else
            {
                std::wstring tableName = const_cast<DatabaseTable *>( table )->GetTableName();
                std::wstring schemaName = const_cast<DatabaseTable *>( table )->GetSchemaName();
                std::wstring comment = const_cast<DatabaseTable *>( table )->GetComment();
                std::wstring tableOwner = const_cast<DatabaseTable *>( table )->GetTableOwner();
                unsigned long tableId = const_cast<DatabaseTable *>( table )->GetTableId();
                delete qry;
                qry = NULL;
                exist = IsTablePropertiesExist( table, errorMsg );
                if( errorMsg.size() != 0 )
                    result = 1;
                else
                {
                    if( exist )
                    {
                        command = L"UPDATE \"abcattbl\" SET \"abt_tnam\" = \'";
                        command += schemaName;
                        command += L".";
                        command += tableName;
                        command += L"\', \"abt_tid\" = ";
                        istr << tableId;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abt_ownr\" = \'";
                        command += tableOwner;
                        command += L"\',  \"abd_fhgt\" = ";
                        istr << properties.m_dataFontSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abd_fwgt\" = ";
                        istr << properties.m_isDataFontBold;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abd_fitl\" = \'";
                        command += properties.m_isDataFontItalic ? L"Y" : L"N";
                        command += L"\', \"abd_funl\" = ";
                        istr << ( properties.m_isDataFontUnderlined ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abd_fstr\" = ";
                        istr << ( properties.m_isDataFontStriken ? 1: 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abd_fchr\" = ";
                        istr << properties.m_dataFontEncoding;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abd_fptc\" = ";
                        istr << properties.m_dataFontPixelSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abd_ffce\" = \'";
                        command += properties.m_dataFontName;
                        command += L"\',  \"abh_fhgt\" = ";
                        istr << properties.m_headingFontSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abh_fwgt\" = ";
                        istr << properties.m_isHeadingFontBold;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abh_fitl\" = \'";
                        command += properties.m_isHeadingFontItalic ? L"Y" : L"N";
                        command += L"\', \"abh_funl\" = ";
                        istr << ( properties.m_isHeadingFontUnderlined ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abh_fstr\" = ";
                        istr << ( properties.m_isHeadingFontStriken ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abh_fchr\" = ";
                        istr << properties.m_headingFontEncoding;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abh_fptc\" = ";
                        istr << properties.m_headingFontPixelSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abh_ffce\" = \'";
                        command += properties.m_headingFontName;
                        command += L"\',  \"abl_fhgt\" = ";
                        istr << properties.m_labelFontSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abl_fwgt\" = ";
                        istr << properties.m_isLabelFontBold;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abl_fitl\" = \'";
                        command += properties.m_isLabelFontItalic ? L"Y" : L"N";
                        command += L"\', \"abl_funl\" = ";
                        istr << ( properties.m_isLabelFontUnderlined ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        comment += L", \"abl_fstr\" = ";
                        istr << ( properties.m_isLabelFontStrioken ? 1 : 0 );
                        comment += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abl_fchr\" = ";
                        istr << properties.m_labelFontEncoding;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abl_fptc\" = ";
                        istr << properties.m_labelFontPixelSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \"abl_ffce\" = \'";
                        command += properties.m_labelFontName;
                        command += L"\', \"abt_cmnt\" = \'";
                        command += properties.m_comment;
                        command += L"\' WHERE \"abt_tnam\" = \'";
                        command += schemaName;
                        command += L".";
                        command += tableName;
                        command += L"\' AND \"abt_tid\" = ";
                        istr << tableId;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L" AND \"abt_ownr\" = \'";
                        command += pimpl->m_connectedUser;
                        command += L"\';";
                    }
                    else
                    {
                        command = L"INSERT INTO \"abcattbl\" VALUES( \'";
                        command += schemaName;
                        command += L".";
                        command += tableName;
                        command += L"\', ";
                        istr << tableId;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \'";
                        command += tableOwner;
                        command += L"\', ";
                        istr << properties.m_dataFontSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_isDataFontBold;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \'";
                        command += properties.m_isDataFontItalic ? L"Y" : L"N";
                        command += L"\', ";
                        istr << ( properties.m_isDataFontUnderlined ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << ( properties.m_isDataFontStriken ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_dataFontEncoding;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_dataFontPixelSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \'";
                        command += properties.m_dataFontName;
                        command += L"\', ";
                        istr << properties.m_headingFontSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_isHeadingFontBold;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \'";
                        command += properties.m_isHeadingFontItalic ? L"Y" : L"N";
                        command += L"\', ";
                        istr << ( properties.m_isHeadingFontUnderlined ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << ( properties.m_isHeadingFontStriken ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_headingFontEncoding;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_headingFontPixelSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \'";
                        command += properties.m_headingFontName;
                        command += L"\', ";
                        istr << properties.m_labelFontSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_isLabelFontBold;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \'";
                        command += properties.m_isLabelFontItalic ? L"Y" : L"N";
                        command += L"\', ";
                        istr << ( properties.m_isLabelFontUnderlined ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << ( properties.m_isLabelFontStrioken ? 1 : 0 );
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_labelFontEncoding;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", ";
                        istr << properties.m_labelFontPixelSize;
                        command += istr.str();
                        istr.clear();
                        istr.str( L"" );
                        command += L", \'";
                        command += properties.m_labelFontName;
                        command += L"\', \'";
                        command += properties.m_comment;
                        command += L"\' );";
                    }
                    if( !isLog )
                    {
                        qry = new SQLWCHAR[command.length() + 2];
                        memset( qry, '\0', command.length() + 2 );
                        uc_to_str_cpy( qry, command );
                        ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                            result = 1;
                        }
                        else
                        {
                            ret = SQLExecDirect( m_hstmt, qry, SQL_NTS );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, m_hstmt );
                                result = 1;
                            }
                        }
                        delete qry;
                        qry = NULL;
                    }
                }
            }
        }
    }
    else
    {
        GetErrorMessage( errorMsg, 2 );
        result = 1;
    }
    delete qry;
    qry = NULL;
    if( result == 1 )
        query = L"ROLLBACK";
    else
        query = L"COMMIT";
    qry = new SQLWCHAR[query.length() + 2];
    memset( qry, '\0', query.length() + 2 );
    uc_to_str_cpy( qry, query );
    ret = SQLExecDirect( m_hstmt, qry, SQL_NTS );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
        result = 1;
    }
    ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
        result = 1;
    }
    else
        m_hstmt = 0;
    delete qry;
    qry = NULL;
    return result;
}

bool ODBCDatabase::IsTablePropertiesExist(const DatabaseTable *table, std::vector<std::wstring> &errorMsg)
{
    bool result = false;
    SQLLEN cbTableName = SQL_NTS, cbSchemaName = SQL_NTS;
    std::wstring query = L"SELECT 1 FROM abcattbl WHERE abt_tnam = ? AND abt_ownr = ?;";
    std::wstring tname = const_cast<DatabaseTable *>( table )->GetSchemaName() + L".";
    tname += const_cast<DatabaseTable *>( table )->GetTableName();
    std::wstring ownerName = const_cast<DatabaseTable *>( table )->GetTableOwner();
    SQLWCHAR *qry = new SQLWCHAR[query.length() + 2], *table_name = new SQLWCHAR[tname.length() + 2], *owner_name = new SQLWCHAR[ownerName.length() + 2];
    memset( owner_name, '\0', ownerName.length() + 2 );
    memset( table_name, '\0', tname.length() + 2 );
    uc_to_str_cpy( owner_name, ownerName );
    uc_to_str_cpy( table_name, tname );
    memset( qry, '\0', query.size() + 2 );
    uc_to_str_cpy( qry, query );
    SQLRETURN ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
    }
    else
    {
        ret = SQLBindParameter( m_hstmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, tname.length(), 0, table_name, 0, &cbTableName );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
        }
        else
        {
            ret = SQLBindParameter( m_hstmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, ownerName.length(), 0, owner_name, 0, &cbSchemaName );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
            }
            else
            {
                ret = SQLPrepare( m_hstmt, qry, SQL_NTS );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                }
                else
                {
                    ret = SQLExecute( m_hstmt );
                    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                    {
                        ret = SQLFetch( m_hstmt );
                        if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
                            result = true;
                        else if( ret != SQL_NO_DATA )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                        }
                    }
                    else if( ret != SQL_NO_DATA )
                    {
                        GetErrorMessage( errorMsg, 1, m_hstmt );
                    }
                }
            }
        }
    }
    delete qry;
    qry = NULL;
    delete table_name;
    table_name = NULL;
    delete owner_name;
    owner_name = NULL;
    ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
    }
    m_hstmt = 0;
    return result;
}

int ODBCDatabase::GetFieldProperties(const SQLWCHAR *tableName, const SQLWCHAR *schemaName, const SQLWCHAR *ownerName, const SQLWCHAR *fieldName, Field *field, std::vector<std::wstring> &errorMsg)
{
    SQLHDBC hdbc_fieldProp;
    SQLHSTMT stmt_fieldProp;
    int result = 0;
    SQLWCHAR *commentField;
    SQLWCHAR *qry = NULL;
    SQLLEN cbSchemaName = SQL_NTS, cbTableName = SQL_NTS, cbFieldName = SQL_NTS, cbDataFontItalic;
    SQLSMALLINT OutConnStrLen;
    std::wstring query = L"SELECT * FROM \"abcatcol\" WHERE \"abc_tnam\" = ? AND \"abc_ownr\" = ? AND \"abc_cnam\" = ?;";
    qry = new SQLWCHAR[query.length() + 2];
    memset( qry, '\0', query.length() + 2 );
    uc_to_str_cpy( qry, query );
    SQLRETURN ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc_fieldProp );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 0, m_env );
        result = 1;
    }
    else
    {
        ret = SQLDriverConnect( hdbc_fieldProp, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2, m_env );
            result = 1;
        }
        else
        {
            ret = SQLAllocHandle( SQL_HANDLE_STMT, hdbc_fieldProp, &stmt_fieldProp );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2, m_hdbc );
                result = 1;
            }
            else
            {
                SQLSMALLINT dataType, decimalDigits, nullable;
                SQLULEN paramSize;
                ret = SQLDescribeParam( stmt_fieldProp, 1, &dataType, &paramSize, &decimalDigits, &nullable );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                    result = 1;
                }
                else
                {
                    ret = SQLBindParameter( stmt_fieldProp, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, paramSize, decimalDigits, &tableName, 0, &cbTableName );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                        result = 1;
                    }
                    else
                    {
                        ret = SQLDescribeParam( stmt_fieldProp, 2, &dataType, &paramSize, &decimalDigits, &nullable );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                            result = 1;
                        }
                        else
                        {
                            ret = SQLBindParameter( stmt_fieldProp, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, paramSize, decimalDigits, &ownerName, 0, &cbSchemaName );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                result = 1;
                            }
                            else
                            {
                                ret = SQLDescribeParam( stmt_fieldProp, 1, &dataType, &paramSize, &decimalDigits, &nullable );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                    result = 1;
                                }
                                else
                                {
                                    ret = SQLBindParameter( stmt_fieldProp, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, paramSize, decimalDigits, &fieldName, 0, &cbFieldName );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                        result = 1;
                                    }
                                    else
                                    {
                                        ret = SQLPrepare( stmt_fieldProp, qry, SQL_NTS );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                            result = 1;
                                        }
                                        else
                                        {
                                            ret = SQLExecute( stmt_fieldProp );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                                result = 1;
                                            }
                                            else
                                            {
                                                ret = SQLBindCol( stmt_fieldProp, 18, SQL_C_WCHAR, &commentField, 3, &cbDataFontItalic );
                                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                {
                                                    GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                                    result = 1;
                                                }
                                                else
                                                {
                                                    ret = SQLFetch( stmt_fieldProp );
                                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                                                    {
                                                        GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                                        result = 1;
                                                    }
                                                    else if( ret != SQL_NO_DATA )
                                                    {
                                                        std::wstring comment;
                                                        str_to_uc_cpy( comment, commentField );
                                                        field->SetComment( comment );
                                                    }
                                                    ret = SQLFreeHandle( SQL_HANDLE_STMT, stmt_fieldProp );
                                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                    {
                                                        GetErrorMessage( errorMsg, 1, stmt_fieldProp );
                                                        result = 1;
                                                    }
                                                    else
                                                        stmt_fieldProp = 0;
                                                    ret = SQLDisconnect( hdbc_fieldProp );
                                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                    {
                                                        GetErrorMessage( errorMsg, 1, hdbc_fieldProp );
                                                        result = 1;
                                                    }
                                                    else
                                                    {
                                                        ret = SQLFreeHandle( SQL_HANDLE_DBC, hdbc_fieldProp );
                                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                        {
                                                            GetErrorMessage( errorMsg, 1, hdbc_fieldProp );
                                                            result = 1;
                                                        }
                                                        else
                                                            hdbc_fieldProp = 0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    delete qry;
    qry = NULL;
    return result;
}

int ODBCDatabase::ApplyForeignKey(std::wstring &command, const std::wstring &keyName, DatabaseTable &tableName, const std::vector<std::wstring> &foreignKeyFields, const std::wstring &refTableName, const std::vector<std::wstring> &refKeyFields, int deleteProp, int updateProp, bool logOnly, std::vector<FKField *> &newFK, bool isNew, int, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    std::wstring query = L"ALTER TABLE ";
    query += tableName.GetSchemaName() + L"." + tableName.GetTableName() + L" ";
    query += L"ADD CONSTRAINT " + keyName + L" ";
    query += L"FOREIGN KEY(";
    std::vector<std::wstring> origFields, refFields;
    for( std::vector<FKField *>::const_iterator it1 = newFK.begin(); it1 < newFK.end(); it1++ )
    {
        query += (*it1)->GetOriginalFieldName();
        if( it1 == newFK.end() - 1 )
            query += L") ";
        else
            query += L",";
        origFields.push_back( (*it1)->GetOriginalFieldName() );
    }
    if( newFK.size() > 0 )
        query += L"REFERENCES " + newFK.at ( 0 )->GetReferencedTableName() + L"(";
    for( std::vector<FKField *>::const_iterator it1 = newFK.begin(); it1 < newFK.end(); it1++ )
    {
        query += (*it1)->GetReferencedFieldName();
        if( it1 == newFK.end() - 1 )
            query += L") ";
        else
            query += L",";
        refFields.push_back( (*it1)->GetReferencedFieldName() );
    }
    query += L"ON DELETE ";
    FK_ONUPDATE updProp = NO_ACTION_UPDATE;
    FK_ONDELETE delProp = NO_ACTION_DELETE;
    switch( deleteProp )
    {
    case 0:
        query += L"NO ACTION ";
        delProp = NO_ACTION_DELETE;
        break;
    case 1:
        if( pimpl->m_subtype == L"Microsoft SQL Server" )
        {
            query += L"NO ACTION ";
            delProp = NO_ACTION_DELETE;
        }
        else
        {
            query += L"RESTRICT ";
            delProp = RESTRICT_DELETE;
        }
        break;
    case 2:
        query += L"CASCADE ";
        delProp = CASCADE_DELETE;
        break;
    case 3:
        query += L"SET NULL ";
        delProp = SET_NULL_DELETE;
        break;
    case 4:
        query += L"SET DEFAULT ";
        delProp = SET_DEFAULT_DELETE;
        break;
    }
    query += L"ON UPDATE ";
    switch( updateProp )
    {
    case 0:
        query += L"NO ACTION";
        updProp = NO_ACTION_UPDATE;
        break;
    case 1:
        if( pimpl->m_subtype == L"Microsoft SQL Server" )
        {
            query += L"NO ACTION ";
            updProp = NO_ACTION_UPDATE;
        }
        else
        {
            query += L"RESTRICT ";
            updProp = RESTRICT_UPDATE;
        }
        query += L"RESTRICT";
        updProp = RESTRICT_UPDATE;
        break;
    case 2:
        query += L"CASCADE";
        updProp = CASCADE_UPDATE;
        break;
    case 3:
        query += L"SET NULL";
        updProp = SET_NULL_UPDATE;
        break;
    case 4:
        query += L"SET DEFAULT";
        updProp = SET_DEFAULT_UPDATE;
        break;
    }
    if( !isNew )
        result = DropForeignKey( command, keyName, tableName, foreignKeyFields, refTableName, refKeyFields, deleteProp, updateProp, logOnly, newFK, errorMsg );
    if( !result )
    {
        if( !logOnly && !newFK.empty() )
        {
            SQLRETURN ret;
            SQLWCHAR *qry = NULL;
            ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2, m_hstmt );
                result = 1;
            }
            else
            {
                qry = new SQLWCHAR[query.length() + 2];
                memset( qry, '\0', query.size() + 2 );
                uc_to_str_cpy( qry, query );
                ret = SQLExecDirect( m_hstmt, qry, SQL_NTS );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                    result = 1;
                }
                else
                {
                    std::map<int, std::vector<FKField *> > &fKeys = tableName.GetForeignKeyVector();
                    int size = fKeys.size();
                    for( unsigned int i = 0; i < foreignKeyFields.size(); i++ )
                        fKeys[size].push_back( new FKField( i, keyName, L"", tableName.GetTableName(), foreignKeyFields.at( i ), L"", refTableName, refKeyFields.at( i ), origFields, refFields, updProp, delProp ) );
                }
                ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 2, m_hstmt );
                    result = 1;
                }
                else
                    m_hstmt = 0;
            }
            delete qry;
            qry = NULL;
        }
        else if( logOnly )
        {
            command = query;
        }
    }
    return result;
}

int ODBCDatabase::DeleteTable(const std::wstring &tableName, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    SQLWCHAR *qry = NULL;
    std::wstring query = L"DROP TABLE ";
    query += tableName;
    SQLRETURN ret;
    ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 2, m_hstmt );
        result = 1;
    }
    else
    {
        qry = new SQLWCHAR[query.length() + 2];
        memset( qry, '\0', query.length() + 2 );
        uc_to_str_cpy( qry, query );
        ret = SQLExecDirect( m_hstmt, qry, SQL_NTS );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2, m_hstmt );
            result = 1;
        }
        ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2, m_hstmt );
            result = 1;
        }
        else
            m_hstmt = 0;
    }
    delete qry;
    qry = NULL;
    return result;
}

int ODBCDatabase::SetFieldProperties(const std::wstring &command, std::vector<std::wstring> &errorMsg)
{
    int res = 0;
    return res;
}

int ODBCDatabase::GetTableId(DatabaseTable *table, std::vector<std::wstring> &errorMsg)
{
    SQLHSTMT stmt = 0;
    SQLHDBC hdbc;
    SQLLEN cbName, cbTableName = SQL_NTS, cbSchemaName = SQL_NTS;
    long id;
    int result = 0;
    std::wstring query;
    SQLWCHAR *qry = NULL, *tname = NULL, *sname = NULL;;
    if( pimpl->m_subtype == L"Microsoft SQL Server" || pimpl->m_subtype == L"Sybase" || pimpl->m_subtype == L"ASE" )
        query = L"SELECT OBJECT_ID(?);";
    if( pimpl->m_subtype == L"PostgreSQL" )
        query = L"SELECT c.oid FROM pg_class c, pg_namespace nc WHERE nc.oid = c.relnamespace AND c.relname = ? AND nc.nspname = ?;";
    if( pimpl->m_subtype == L"MySQL" )
        query = L"SELECT CASE WHEN t.engine = 'InnoDB' THEN (SELECT st.table_id FROM information_schema.INNODB_SYS_TABLES st WHERE CONCAT(t.table_schema,'/', t.table_name) = st.name) ELSE (SELECT 0) END AS id FROM information_schema.tables t WHERE t.table_name = ? AND t.table_schema = ?;";
    if( pimpl->m_subtype == L"Oracle" )
        query = L"SELECT object_id FROM all_objects WHERE object_name = ? AND subobject_name = ?";
    std::wstring tableName = const_cast<DatabaseTable *>( table )->GetTableName();
    std::wstring schemaName = const_cast<DatabaseTable *>( table )->GetSchemaName();
    qry = new SQLWCHAR[query.length() + 2];
    if( pimpl->m_subtype == L"Microsoft SQL Server" || pimpl->m_subtype == L"Sybase" )
    {
        tname = new SQLWCHAR[tableName.length() + schemaName.length() + 3];
        memset( tname, '\0', tableName.length() + schemaName.length() + 3 );
        uc_to_str_cpy( tname, schemaName );
        uc_to_str_cpy( tname, L"." );
        uc_to_str_cpy( tname, tableName );
    }
    else
    {
        tname = new SQLWCHAR[tableName.length() + 2];
        sname = new SQLWCHAR[schemaName.length() + 2];
        memset( tname, '\0', tableName.length() + 2 );
        memset( sname, '\0', schemaName.length() + 2);
        uc_to_str_cpy( sname, schemaName );
        uc_to_str_cpy( tname, tableName );
    }
    memset( qry, '\0', query.length() + 2 );
    uc_to_str_cpy( qry, query );
    SQLRETURN retcode = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc );
    if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 0 );
        result = 1;
    }
    else
    {
        SQLSMALLINT OutConnStrLen;
        retcode = SQLDriverConnect( hdbc, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2, hdbc );
            result = 1;
        }
        else
        {
            retcode = SQLAllocHandle( SQL_HANDLE_STMT, hdbc, &stmt );
            if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2, hdbc );
                result = 1;
            }
            else
            {
                retcode = SQLBindParameter( stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, tableName.length() + schemaName.length() + 3, 0, tname, 0, &cbTableName );
                if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 2, hdbc );
                    result = 1;
                }
                else
                {
                    if( pimpl->m_subtype != L"Microsoft SQL Server" && pimpl->m_subtype != L"Sybase" )
                    {
                        retcode = SQLBindParameter( stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, schemaName.length(), 0, sname, 0, &cbSchemaName );
                        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, stmt );
                            result = 1;
                        }
                    }
                    if( !result )
                    {
                        retcode = SQLPrepare( stmt, qry, SQL_NTS );
                        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, stmt );
                            result = 1;
                        }
                        else
                        {
                            retcode = SQLExecute( stmt );
                            if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt );
                                result = 1;
                            }
                            else
                            {
                                retcode = SQLFetch( stmt );
                                if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO && retcode != SQL_NO_DATA )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt );
                                    result = 1;
                                }
                                else if( retcode != SQL_NO_DATA )
                                {
                                    retcode = SQLGetData( stmt, 1, SQL_C_SLONG, &id, 0, &cbName );
                                    if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                                    {
                                        table->SetTableId( id );
                                    }
                                    else
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt );
                                        result = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    delete qry;
    qry = NULL;
    delete tname;
    tname = NULL;
    delete sname;
    sname = NULL;
    if( stmt )
    {
        retcode = SQLFreeHandle( SQL_HANDLE_STMT, stmt );
        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, stmt );
            result = 1;
        }
        else
        {
            stmt = 0;
            retcode = SQLDisconnect( hdbc );
            if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2, hdbc );
                result = 1;
            }
            else
            {
                retcode = SQLFreeHandle( SQL_HANDLE_DBC, hdbc );
                if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 2, hdbc );
                    result = 1;
                }
                else
                    hdbc = 0;
            }
        }
    }
    return result;
}

int ODBCDatabase::GetTableOwner(const std::wstring &schemaName, const std::wstring &tableName, std::wstring &tableOwner, std::vector<std::wstring> &errorMsg)
{
    SQLHSTMT stmt = 0;
    SQLHDBC hdbc = 0;
    int result = 0;
    SQLLEN cbTableName = SQL_NTS, cbSchemaName = SQL_NTS;
    SQLWCHAR *table_name = NULL, *schema_name = NULL, *qry = NULL;
    SQLWCHAR *owner = NULL;
    std::wstring query;
    if( pimpl->m_subtype == L"Microsoft SQL Server" )
        query = L"SELECT cast(su.name AS nvarchar(128)) FROM sysobjects so, sysusers su, sys.tables t, sys.schemas s WHERE so.uid = su.uid AND t.object_id = so.id AND t.schema_id = s.schema_id AND s.name = ? AND so.name = ?;";
    if( pimpl->m_subtype == L"PostgreSQL" )
        query = L"SELECT u.usename FROM pg_class c, pg_user u, pg_namespace n WHERE n.oid = c.relnamespace AND u.usesysid = c.relowner AND n.nspname = ? AND relname = ?";
    if( pimpl->m_subtype == L"Sybase" || pimpl->m_subtype == L"ASE" )
        query = L"SELECT su.name FROM sysobjects so, sysusers su WHERE su.uid = so.uid AND so.name = ?";
    if( pimpl->m_subtype == L"MySQL" )
    {
        odbc_pimpl->m_currentTableOwner = L"";
        return result;
    }
    SQLRETURN retcode = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc );
    if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 0 );
        result = 1;
    }
    else
    {
        SQLSMALLINT OutConnStrLen;
        retcode = SQLDriverConnect( hdbc, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2, hdbc );
            result = 1;
        }
        else
        {
            retcode = SQLAllocHandle( SQL_HANDLE_STMT, hdbc, &stmt );
            if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2, hdbc );
                result = 1;
            }
            else
            {
                table_name = new SQLWCHAR[tableName.length() + 2];
                schema_name = new SQLWCHAR[schemaName.length() + 2];
                qry = new SQLWCHAR[query.length() + 2];
                memset( qry, '\0', query.size() + 2 );
                memset( table_name, '\0', tableName.length() + 2 );
                memset( schema_name, '\0', schemaName.length() + 2 );
                uc_to_str_cpy( qry, query );
                uc_to_str_cpy( table_name, tableName );
                uc_to_str_cpy( schema_name, schemaName );
                retcode = SQLPrepare( stmt, qry, SQL_NTS );
                if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                {
                    retcode = SQLBindParameter( stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, schemaName.length(), 0, schema_name, 0, &cbSchemaName );
                    if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                    {
                        retcode = SQLBindParameter( stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, tableName.length(), 0, table_name, 0, &cbTableName );
                        if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                        {
                            retcode = SQLExecute( stmt );
                            if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                            {
                                SQLSMALLINT nameBufLength, dataTypePtr, decimalDigitsPtr, isNullable;
                                SQLULEN columnSizePtr;
                                SQLLEN cbTableOwner;
                                retcode = SQLDescribeCol( stmt, 1, NULL, 0, &nameBufLength, &dataTypePtr, &columnSizePtr, &decimalDigitsPtr, &isNullable );
                                if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                                {
                                    owner = new SQLWCHAR[columnSizePtr + 1];
                                    retcode = SQLBindCol( stmt, 1, SQL_C_WCHAR, owner, columnSizePtr, &cbTableOwner );
                                    if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                                    {
                                        retcode = SQLFetch( stmt );
                                        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO && retcode != SQL_NO_DATA )
                                        {
                                            GetErrorMessage( errorMsg, 1, stmt );
                                            result = 1;
                                        }
                                        if( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
                                            str_to_uc_cpy( tableOwner, owner );
                                    }
                                    else
                                    {
                                        if( pimpl->m_subtype == L"Microsoft SQL Server" )
                                        {
                                            tableOwner = L"dbo";
                                            result = 0;
                                        }
                                        else
                                        {
                                            GetErrorMessage( errorMsg, 1, stmt );
                                            result = 1;
                                        }
                                    }
                                }
                                else if( retcode != SQL_NO_DATA )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt );
                                    result = 1;
                                }
                            }
                            else if( retcode != SQL_NO_DATA )
                            {
                                GetErrorMessage( errorMsg, 1, stmt );
                                result = 1;
                            }
                        }
                        else if( retcode != SQL_NO_DATA )
                        {
                            GetErrorMessage( errorMsg, 1, stmt );
                            result = 1;
                        }
                    }
                    else if( retcode != SQL_NO_DATA )
                    {
                        GetErrorMessage( errorMsg, 1, stmt );
                        result = 1;
                    }
                }
                else if( retcode != SQL_NO_DATA )
                {
                    GetErrorMessage( errorMsg, 1, stmt );
                    result = 1;
                }
            }
        }
    }
    if( stmt )
    {
        retcode = SQLFreeHandle( SQL_HANDLE_STMT, stmt );
        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, stmt );
            result = 1;
        }
        else
        {
            stmt = 0;
            retcode = SQLDisconnect( hdbc );
            if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, stmt );
                result = 1;
            }
            else
            {
                retcode = SQLFreeHandle( SQL_HANDLE_DBC, hdbc );
                if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, stmt );
                    result = 1;
                }
                else
                    hdbc = 0;
            }
        }
    }
    delete qry;
    qry = NULL;
    delete table_name;
    table_name = NULL;
    delete schema_name;
    schema_name = NULL;
    delete owner;
    owner = NULL;
    return result;
}

void ODBCDatabase::SetFullType(Field *field)
{
    std::wostringstream ss;
    std::wstring fieldType = field->GetFieldType();
    if( pimpl->m_subtype == L"Microsoft SQL Server" )
    {
        if( fieldType == L"bigint" || fieldType == L"bit" || fieldType == L"int" || fieldType == L"smallint" || fieldType == L"tinyint" || fieldType == L"date" || fieldType == L"datetime" || fieldType == L"smalldatetime" || fieldType == L"text" || fieldType == L"image" || fieldType == L"ntext" )
            field->SetFullType( fieldType );
        if( fieldType == L"decimal" || fieldType == L"numeric" )
        {
            std::wstring type = fieldType + L"(";
            ss << field->GetFieldSize();
            type += ss.str();
            ss.clear();
            ss.str( L"" );
            type += L",";
            ss << field->GetPrecision();
            type += ss.str();
            type += L")";
            field->SetFullType( type );
        }
        if( fieldType == L"float" || fieldType == L"real" || fieldType == L"datetime2" || fieldType == L"datetimeoffset" || fieldType == L"time" || fieldType == L"char" || fieldType == L"varchar" || fieldType == L"binary" || fieldType == L"varbinary" )
        {
            std::wstring type = fieldType + L"(";
            ss << field->GetFieldSize();
            type += ss.str();
            ss.clear();
            ss.str( L"" );
            type += L")";
        }
    }
}

int ODBCDatabase::GetServerVersion(std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    std::wstring query;
    long versionMajor, versionMinor;
    SQLLEN cbVersion = SQL_NTS;
    SQLWCHAR *qry = NULL, version[1024];
    if( pimpl->m_subtype == L"Microsoft SQL Server" ) // MS SQL SERVER
    {
        query = L"SELECT SERVERPROPERTY('productversion') AS version, COALESCE(SERVERPROPERTY('ProductMajorVersion'), PARSENAME(CAST(SERVERPROPERTY('productversion') AS varchar(20)), 4)) AS major, COALESCE(SERVERPROPERTY('ProductMinorVersion'), PARSENAME(CAST(SERVERPROPERTY('productversion') AS varchar(20)), 3)) AS minor;";
    }
    if( pimpl->m_subtype == L"MySQL" )
    {
        query = L"SELECT version(), LEFT( version(), LOCATE( '.', version() ) - 1 ) AS major, LEFT( SUBSTR( version(), LOCATE( '.', version() ) + 1 ), LOCATE( '.', SUBSTR( version(), LOCATE( '.', version() ) + 1 ) ) - 1 ) AS minor;";
    }
    if( pimpl->m_subtype == L"PostgreSQL" )
    {
        query = L"SELECT version() AS version, split_part( split_part( version(), ' ', 2 ) , '.', 1 ) AS major, split_part( split_part( version(), ' ', 2 ), '.', 2 ) AS minor;";
    }
    if( pimpl->m_subtype == L"Sybase" || pimpl->m_subtype == L"ASE" )
    {
        query = L"SELECT @@version_number AS version, @@version_as_integer / 1000 AS major";
    }
    if( pimpl->m_subtype == L"Oracle" )
    {
    }
    SQLRETURN retcode = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 2, m_hdbc );
        result = 1;
    }
    else
    {
        qry = new SQLWCHAR[query.length() + 2];
        memset( qry, '\0', query.length() + 2 );
        uc_to_str_cpy( qry, query );
        retcode = SQLExecDirect( m_hstmt, qry, SQL_NTS );
        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
        else
        {
            retcode = SQLBindCol( m_hstmt, 1, SQL_C_WCHAR, &version, 1024, &cbVersion );
            if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
                result = 1;
            }
            else
            {
                retcode = SQLBindCol( m_hstmt, 2, SQL_C_SLONG, &versionMajor, 0, 0 );
                if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                    result = 1;
                }
                else
                {
                    retcode = SQLBindCol( m_hstmt, 3, SQL_C_SLONG, &versionMinor, 0, 0 );
                    if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 1, m_hstmt );
                        result = 1;
                    }
                    else
                    {
                        retcode = SQLFetch( m_hstmt );
                        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                            result = 1;
                        }
                        else
                        {
                            str_to_uc_cpy( pimpl->m_serverVersion, version );
                            pimpl->m_versionMajor = (int) versionMajor;
                            pimpl->m_versionMinor = (int) versionMinor;
                        }
                    }
                }
            }
        }
        retcode = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
        if( retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
        m_hstmt = 0;
    }
    delete qry;
    qry = NULL;
    return result;
}

int ODBCDatabase::CreateIndexesOnPostgreConnection(std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    SQLWCHAR *qry1 = NULL, *qry2 = NULL, *qry3 = NULL, *qry4 = NULL;
    std::wstring query1 = L"SELECT 1 FROM pg_class c, pg_namespace n WHERE n.oid = c.relnamespace AND c.relname = \'abcattbl_tnam_ownr\' AND n.nspname = \'public\';";
    std::wstring query3 = L"CREATE INDEX  \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"abt_tnam\" ASC, \"abt_ownr\" ASC);";
    std::wstring query2 = L"SELECT 1 FROM pg_class c, pg_namespace n WHERE n.oid = c.relnamespace AND c.relname = \'abcatcol_tnam_ownr_cnam\' AND n.nspname = \'public\'";
    std::wstring query4 = L"CREATE INDEX \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"abc_tnam\" ASC, \"abc_ownr\" ASC, \"abc_cnam\" ASC);";
    qry1 = new SQLWCHAR[query1.length() + 2];
    qry2 = new SQLWCHAR[query2.length() + 2];
    qry3 = new SQLWCHAR[query3.length() + 2];
    qry4 = new SQLWCHAR[query4.length() + 2];
    memset( qry1, '\0', query1.length() + 2 );
    memset( qry2, '\0', query2.length() + 2 );
    memset( qry3, '\0', query3.length() + 2 );
    memset( qry4, '\0', query4.length() + 2 );
    uc_to_str_cpy( qry1, query1 );
    uc_to_str_cpy( qry2, query2 );
    uc_to_str_cpy( qry3, query3 );
    uc_to_str_cpy( qry4, query4 );
    RETCODE ret = SQLExecDirect( m_hstmt, qry1, SQL_NTS );
    if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
    {
        ret = SQLFetch( m_hstmt );
        if( ret == SQL_NO_DATA )
        {
            ret = SQLFreeStmt( m_hstmt, SQL_CLOSE );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
                result = 1;
            }
            else
            {
                ret = SQLExecDirect( m_hstmt, qry3, SQL_NTS );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                    result = 1;
                }
            }
        }
        else if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
    }
    else
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
        result = 1;
    }
    if( !result )
    {
        ret = SQLFreeStmt( m_hstmt, SQL_CLOSE );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 1, m_hstmt );
            result = 1;
        }
        else
        {
            RETCODE ret = SQLExecDirect( m_hstmt, qry2, SQL_NTS );
            if( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO )
            {
                ret = SQLFetch( m_hstmt );
                if( ret == SQL_NO_DATA )
                {
                    ret = SQLFreeStmt( m_hstmt, SQL_CLOSE );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 1, m_hstmt );
                        result = 1;
                    }
                    else
                    {
                        ret = SQLExecDirect( m_hstmt, qry4, SQL_NTS );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                            result = 1;
                        }
                    }
                }
                else if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                    result = 1;
                }
            }
            else if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
                result = 1;
            }
        }
    }
    delete qry1;
    qry1 = NULL;
    delete qry2;
    qry2 = NULL;
    delete qry3;
    qry3 = NULL;
    delete qry4;
    qry4 = NULL;
    return result;
}

int ODBCDatabase::DropForeignKey(std::wstring &command, const std::wstring &keyName, DatabaseTable &tableName, const std::vector<std::wstring> &foreignKeyFields, const std::wstring &refTableName, const std::vector<std::wstring> &refKeyFields, int deleteProp, int updateProp, bool logOnly, std::vector<FKField *> &newFK, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    std::wstring query;
    query = L"ALTER TABLE ";
    query += tableName.GetSchemaName() + L"." + tableName.GetTableName() + L" ";
    query += L"DROP CONSTRAINT " + keyName + L" ";
    if( !logOnly )
    {
        SQLRETURN ret;
        SQLWCHAR *qry = NULL;
        ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
        {
            GetErrorMessage( errorMsg, 2, m_hstmt );
            result = 1;
        }
        else
        {
            qry = new SQLWCHAR[query.length() + 2];
            memset( qry, '\0', query.size() + 2 );
            uc_to_str_cpy( qry, query );
            ret = SQLExecDirect( m_hstmt, qry, SQL_NTS );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
                result = 1;
            }
            else
            {
                bool found = false;
                std::map<int, std::vector<FKField *> > &fKeys = tableName.GetForeignKeyVector();
                for( std::map<int, std::vector<FKField *> >::iterator it = fKeys.begin(); it != fKeys.end() && !found; ++it )
                    for( std::vector<FKField *>::iterator it1 = (*it).second.begin(); it1 != (*it).second.end() && !found;  )
                    {
                        if( (*it1)->GetFKName() == keyName )
                        {
                            found = true;
                            delete (*it1);
                            (*it1) = NULL;
                            it1 = (*it).second.erase( it1 );
                        }
                        else
                            ++it1;
                    }
            }
            ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 2, m_hstmt );
                result = 1;
            }
            else
                m_hstmt = 0;
        }
        delete qry;
        qry = NULL;
    }
    else
    {
        command = query;
    }
    return result;
}

int ODBCDatabase::NewTableCreation(std::vector<std::wstring> &errorMsg)
{
    SQLHDBC hdbc;
    SQLHSTMT hstmt;
    int result = 0, ret, ops, bufferSize = 1024;
    SQLSMALLINT **columnNameLen, numCols, **columnDataType, **colummnDataDigits, **columnDataNullable;
    SQLULEN **columnDataSize;
    SQLWCHAR *columnName[3], **columnData;
    SQLLEN **columnDataLen;
    std::wstring tableName, command, operation, element, schemaName, catalogName;
    SQLWCHAR *cat = NULL, *schema = NULL, *table = NULL;
    SQLTablesDataBinding *catalog = (SQLTablesDataBinding *) malloc( 5 * sizeof( SQLTablesDataBinding ) );
    ret = SQLAllocHandle( SQL_HANDLE_STMT, m_hdbc, &m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 2, m_hstmt );
        result = 1;
    }
    else
    {
        if( pimpl->m_subtype == L"Microsoft SQL Server" )
        {
            SQLLEN messageType, sqlCommand, name;
            std::wstring sub_query1 = L"SET NOCOUNT ON; DECLARE @TargetDialogHandle UNIQUEIDENTIFIER; ";
            std::wstring sub_query2 = L"DECLARE @EventMessage XML; ";
            std::wstring sub_query3 = L"DECLARE @EventMessageTypeName sysname; ";
            std::wstring sub_query4 = L"WAITFOR( RECEIVE TOP(1) @TargetDialogHandle = conversation_handle, @EventMessage = CONVERT(XML, message_body), @EventMessageTypeName = message_type_name FROM dbo.EventNotificationQueue ), TIMEOUT 1000;";
            std::wstring sub_query5 = L"SELECT @EventMessageTypeName AS MessageTypeName, @EventMessage.value('(/EVENT_INSTANCE/TSQLCommand/CommandText)[1]','nvarchar(max)') AS TSQLCommand, @EventMessage.value('(/EVENT_INSTANCE/ObjectName)[1]', 'varchar(128)' ) as TableName";
            std::wstring query = sub_query1 + sub_query2 + sub_query3 + sub_query4 + sub_query5;
            SQLWCHAR *qry = new SQLWCHAR[query.length() + 2];
            memset( qry, '\0', query.length() + 2 );
            uc_to_str_cpy( qry, query );
            ret = SQLPrepare( m_hstmt, qry, SQL_NTS );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, m_hstmt );
                result = 1;
            }
            else
            {
                ret = SQLNumResultCols( m_hstmt, &numCols );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, m_hstmt );
                    result = 1;
                }
                else
                {
                    for( int i = 0; i < numCols; i++ )
                    {
                        columnNameLen = new SQLSMALLINT *[numCols];
                        columnDataType = new SQLSMALLINT *[numCols];
                        columnDataSize = new SQLULEN *[numCols];
                        colummnDataDigits = new SQLSMALLINT *[numCols];
                        columnDataNullable = new SQLSMALLINT *[numCols];
                        columnData = new SQLWCHAR *[numCols];
                        columnDataLen = new SQLLEN *[numCols];
                    }
                    for( int i = 0; i < numCols; i++ )
                    {
                        columnNameLen[i] = new SQLSMALLINT;
                        columnDataType[i] = new SQLSMALLINT;
                        columnDataSize[i] = new SQLULEN;
                        colummnDataDigits[i] = new SQLSMALLINT;
                        columnDataNullable[i] = new SQLSMALLINT;
                        columnName[i] = new SQLWCHAR[256];
                        columnDataLen[i] = new SQLLEN;
                        ret = SQLDescribeCol( m_hstmt, i + 1, columnName[i], 256, columnNameLen[i], columnDataType[i], columnDataSize[i], colummnDataDigits[i], columnDataNullable[i] );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                            result = 1;
                            break;
                        }
                        if( *columnDataSize[i] == 0 )
                            *columnDataSize[i] = 2048;
                        columnData[i] = new SQLWCHAR[(unsigned int) *columnDataSize[i] + 1];
                        memset( columnData[i], '\0', (unsigned int) *columnDataSize[i] + 1 );
                        switch( *columnDataType[i] )
                        {
                            case SQL_INTEGER:
                                *columnDataType[i] = SQL_C_LONG;
                                break;
                            case SQL_VARCHAR:
                            case SQL_CHAR:
                            case SQL_WVARCHAR:
                            case SQL_WCHAR:
                                *columnDataType[i] = SQL_C_WCHAR;
                                break;
                        }
                    }
                    ret = SQLExecute( m_hstmt );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 1, m_hstmt );
                        result = 1;
                    }
                    for( ret = SQLFetch( m_hstmt ); ( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) && ret != SQL_NO_DATA; ret = SQLFetch( m_hstmt ) )
                    {
                        ret = SQLGetData( m_hstmt, 1, *columnDataType[0], columnData[0], *columnDataSize[0], &messageType );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                            result = 1;
                            break;
                        }
                        ret = SQLGetData( m_hstmt, 2, *columnDataType[1], columnData[1], *columnDataSize[1], &sqlCommand );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                            result = 1;
                            break;
                        }
                        ret = SQLGetData( m_hstmt, 3, *columnDataType[2], columnData[2], *columnDataSize[2], &name );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, m_hstmt );
                            result = 1;
                            break;
                        }
                        str_to_uc_cpy( tableName, columnData[2] );
                        str_to_uc_cpy( command, columnData[1] );
                        trim( tableName );
                        trim( command );
                        int pos = command.find( L' ' );
                        operation = command.substr( 0, pos );
                        command = command.substr( pos + 1 );
                        trim( command );
                        pos = command.find( L' ' );
                        element = command.substr( 0, pos );
                        command = command.substr( pos + 1 );
                        trim( command );
                        std::transform( element.begin(), element.end(), element.begin(), towupper );
                        if( element == L"TABLE" )
                        {
                            if( operation == L"DROP" )
                            {
                                ops = 1;
                                tableName = command.substr( 0, command.length() - 1 );
                            }
                            if( operation == L"CREATE" )
                            {
                                ops = 0;
                                tableName = command.substr( 0, command.find( L'(' ) );
                            }
                            if( operation == L"ALTER" )
                            {
                                ops = 2;
                                tableName = command.substr( 0, command.find( L' ' ) );
                            }
                            tableName.erase( std::find_if( tableName.rbegin(), tableName.rend(), [](int ch) { return !isspace( ch ); } ).base(), tableName.end() );
                            pos = tableName.find( L'.' );
                            if( pos != std::wstring::npos )
                            {
                                schemaName = tableName.substr( 0, pos );
                                tableName = tableName.substr( pos + 1 );
                            }
                            else
                            {
                                schemaName = L"";
                                tableName = L"";
                            }
                            if( schemaName != L"" )
                            {
                                schema = new SQLWCHAR[schemaName.length() + 2];
                                memset( schema, '\0', schemaName.length() + 2 );
                            }
                            table = new SQLWCHAR[tableName.length() + 2];
                            memset( table, '\0', tableName.length() + 2 );
                            uc_to_str_cpy( schema, schemaName );
                            uc_to_str_cpy( table, tableName );
                            for( int i = 0; i < 5; i++ )
                            {
                                catalog[i].TargetType = SQL_C_WCHAR;
                                catalog[i].BufferLength = ( bufferSize + 1 );
                                catalog[i].TargetValuePtr = malloc( sizeof( unsigned char ) * catalog[i].BufferLength );
                                ret = SQLBindCol( m_hstmt, (SQLUSMALLINT) i + 1, catalog[i].TargetType, catalog[i].TargetValuePtr, catalog[i].BufferLength, &( catalog[i].StrLen_or_Ind ) );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1 );
                                    result = 1;
                                    break;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1 );
                                    result = 1;
                                    break;
                                }
                                else
                                {
                                    SQLSMALLINT OutConnStrLen;
                                    ret = SQLDriverConnect( hdbc, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 2, hdbc );
                                        result = 1;
                                        break;
                                    }
                                    else
                                    {
                                        ret = SQLAllocHandle( SQL_HANDLE_DBC, hdbc, &hstmt );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 1 );
                                            result = 1;
                                            break;
                                        }
                                        else
                                        {
                                            ret = SQLTables( hstmt, NULL, 0, schema, SQL_NTS, table, SQL_NTS, NULL, 0 );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 1 );
                                                result = 1;
                                                break;
                                            }
                                            else
                                            {
                                                ret = SQLFetch( m_hstmt );
                                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                                {
                                                    GetErrorMessage( errorMsg, 1 );
                                                    result = 1;
                                                    break;
                                                }
                                                else
                                                {
                                                    if( catalog[0].StrLen_or_Ind != SQL_NULL_DATA )
                                                        cat = (SQLWCHAR *) catalog[0].TargetValuePtr;
                                                    if( catalog[1].StrLen_or_Ind != SQL_NULL_DATA )
                                                        schema = (SQLWCHAR *) catalog[1].TargetValuePtr;
                                                    if( catalog[2].StrLen_or_Ind != SQL_NULL_DATA )
                                                        table = (SQLWCHAR *) catalog[2].TargetValuePtr;
                                                    catalogName = L"";
                                                    schemaName = L"";
                                                    tableName = L"";
                                                    str_to_uc_cpy( catalogName, cat );
                                                    str_to_uc_cpy( schemaName, schema );
                                                    str_to_uc_cpy( tableName, table );
                                                    if( schemaName == L"" && catalogName != L"" )
                                                    {
                                                        schema = cat;
                                                        copy_uc_to_uc( schema, cat );
                                                    }
                                                    if( ops == 0 )
                                                        AddDropTable( catalogName, schemaName, tableName, true, errorMsg );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if( command == L"VIEW" )
		    {
		    }
                    if( command == L"INDEX" )
		    {
		    }
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                    {
                        GetErrorMessage( errorMsg, 1, m_hstmt );
                        result = 1;
                    }
                }
            }
            for( int i = 0; i < numCols; i++ )
            {
                delete columnNameLen[i];
                delete columnDataType[i];
                delete columnDataSize[i];
                delete colummnDataDigits[i];
                delete columnDataNullable[i];
                delete columnData[i];
                delete columnDataLen[i];
                delete columnName[i];
            }
            delete columnNameLen;
            delete columnDataType;
            delete columnDataSize;
            delete colummnDataDigits;
            delete columnDataNullable;
            delete columnData;
            delete columnDataLen;
        }
    }
    ret = SQLFreeHandle( SQL_HANDLE_STMT, m_hstmt );
    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
    {
        GetErrorMessage( errorMsg, 1, m_hstmt );
        result = 1;
    }
    return result;
}

int ODBCDatabase::AddDropTable(const std::wstring &catalog, const std::wstring &schemaName, const std::wstring &tableName, bool tableAdded, std::vector<std::wstring> &errorMsg)
{
    SQLRETURN ret;
    SQLWCHAR *table_name = new SQLWCHAR[tableName.length() + 2], *schema_name = new SQLWCHAR[schemaName.length() + 2], *catalog_name = new SQLWCHAR[catalog.size() + 2];
    std::wstring owner;
    std::vector<std::wstring> autoinc_fields, indexes;
    SQLWCHAR *qry = NULL;
    SQLWCHAR **columnNames = NULL;
    std::wstring query4;
    int result = 0, bufferSize = 1024;
    std::vector<Field *> fields;
    std::wstring fieldName, fieldType, defaultValue, primaryKey, fkSchema, fkName, fkTable, schema, table, origSchema, origTable, origCol, refSchema, refTable, refCol, cat;
    std::vector<std::wstring> pk_fields, fk_fieldNames;
    std::map<int,std::vector<FKField *> > foreign_keys;
    SQLWCHAR *szSchemaName = NULL;
    SQLHSTMT stmt_col = 0, stmt_pk = 0, stmt_colattr = 0, stmt_fk = 0, stmt_ind = 0;
    SQLHDBC hdbc_col = 0, hdbc_pk = 0, hdbc_colattr = 0, hdbc_fk = 0, hdbc_ind = 0;
    SQLWCHAR szColumnName[256], szTypeName[256], szRemarks[256], szColumnDefault[256], szIsNullable[256], pkName[SQL_MAX_COLUMN_NAME_LEN + 1], userName[1024];
    SQLWCHAR szFkTable[SQL_MAX_COLUMN_NAME_LEN + 1], szPkCol[SQL_MAX_COLUMN_NAME_LEN + 1], szPkTable[SQL_MAX_COLUMN_NAME_LEN + 1], szPkSchema[SQL_MAX_COLUMN_NAME_LEN + 1], szFkTableSchema[SQL_MAX_SCHEMA_NAME_LEN + 1], szFkCol[SQL_MAX_COLUMN_NAME_LEN + 1], szFkName[256], szFkCatalog[SQL_MAX_CATALOG_NAME_LEN + 1];
    SQLSMALLINT updateRule, deleteRule, keySequence;
    SQLLEN cbPkCol, cbFkTable, cbFkCol, cbFkName, cbFkSchemaName, cbUpdateRule, cbDeleteRule, cbKeySequence, cbFkCatalog, fkNameLength = 256;
    SQLLEN cbColumnName, cbDataType, cbTypeName, cbColumnSize, cbBufferLength, cbDecimalDigits, cbNumPrecRadix, pkLength;
    SQLLEN cbNullable, cbRemarks, cbColumnDefault, cbSQLDataType, cbDatetimeSubtypeCode, cbCharOctetLength, cbOrdinalPosition, cbIsNullable;
    SQLSMALLINT DataType, DecimalDigits, NumPrecRadix, Nullable, SQLDataType, DatetimeSubtypeCode, numCols = 0;
    SQLINTEGER ColumnSize, BufferLength, CharOctetLength, OrdinalPosition;
    if( pimpl->m_subtype == L"PostgreSQL" )
        query4 = L"SELECT indexname FROM pg_indexes WHERE schemaname = ? AND tablename = ?";
    if( pimpl->m_subtype == L"MySQL" )
        query4 = L"SELECT index_name FROM information_schema.statistics WHERE table_schema = ? AND table_name = ?;";
    if( pimpl->m_subtype == L"Microsoft SQL Server" )
        query4 = L"SELECT i.name FROM sys.indexes i, sys.tables t WHERE i.object_id = t.object_id AND SCHEMA_NAME(t.schema_id) = ? AND t.name = ?;";
    memset( table_name, '\0', tableName.length() + 2 );
    memset( schema_name, '\0', schemaName.length() + 2 );
    memset( catalog_name, '\0', catalog.length() + 2 );
    uc_to_str_cpy( table_name, tableName );
    uc_to_str_cpy( schema_name, schemaName );
    uc_to_str_cpy( catalog_name, catalog );
    if( tableAdded )
    {
        if( GetTableOwner( schemaName, tableName, owner, errorMsg ) )
        {
            result = 1;
            ret = SQL_ERROR;
        }
        else
        {
            ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc_fk );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 0 );
                result = 1;
            }
            else
            {
                SQLSMALLINT OutConnStrLen;
                ret = SQLDriverConnect( hdbc_fk, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 2, hdbc_fk );
                    result = 1;
                }
                else
                {
                    ret = SQLAllocHandle(  SQL_HANDLE_STMT, hdbc_fk, &stmt_fk );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 2, hdbc_fk );
                        result = 1;
                    }
                    else
                    {
                        ret = SQLBindCol( stmt_fk, 2, SQL_C_WCHAR, szPkSchema, SQL_MAX_COLUMN_NAME_LEN + 1, &cbPkCol );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 1, stmt_fk );
                            result = 1;
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 3, SQL_C_WCHAR, szPkTable, SQL_MAX_COLUMN_NAME_LEN + 1, &cbPkCol );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 4, SQL_C_WCHAR, szPkCol, SQL_MAX_COLUMN_NAME_LEN + 1, &cbPkCol );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 5, SQL_C_WCHAR, szFkCatalog, SQL_MAX_CATALOG_NAME_LEN + 1, &cbFkCatalog );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 6, SQL_C_WCHAR, szFkTableSchema, SQL_MAX_TABLE_NAME_LEN + 1, &cbFkSchemaName );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 7, SQL_C_WCHAR, szFkTable, SQL_MAX_TABLE_NAME_LEN + 1, &cbFkTable );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 8, SQL_C_WCHAR, szFkCol, SQL_MAX_COLUMN_NAME_LEN + 1, &cbFkCol );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 9, SQL_C_SSHORT, &keySequence, SQL_MAX_TABLE_NAME_LEN + 1, &cbKeySequence );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 10, SQL_C_SSHORT, &updateRule, SQL_MAX_COLUMN_NAME_LEN + 1, &cbUpdateRule );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 11, SQL_C_SSHORT, &deleteRule, SQL_MAX_COLUMN_NAME_LEN + 1, &cbDeleteRule );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLBindCol( stmt_fk, 12, SQL_C_WCHAR, szFkName, fkNameLength, &cbFkName );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                        }
                        if( !result )
                        {
                            ret = SQLForeignKeys( stmt_fk, NULL, 0, NULL, 0, NULL, 0, catalog_name, SQL_NTS, schema_name, SQL_NTS, table_name, SQL_NTS );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_fk );
                                result = 1;
                            }
                            else
                            {
                                bool fkFound = false;
                                std::map<int, std::vector<std::wstring> >origFields, refFields;
                                memset( szFkName, '\0', fkNameLength );
                                for( ret = SQLFetch( stmt_fk ); ( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) && ret != SQL_NO_DATA; ret = SQLFetch( stmt_fk ) )
                                {
                                    str_to_uc_cpy( origCol, szPkCol );
                                    str_to_uc_cpy( refCol, szFkCol );
                                    origFields[keySequence].push_back( origCol );
                                    refFields[keySequence].push_back( refCol );
                                    fkFound = true;
                                    origCol = L"";
                                    refCol = L"";
                                }
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_fk );
                                    result = 1;
                                }
                                else if( fkFound )
                                {
                                    ret = SQLCloseCursor( stmt_fk );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_fk );
                                        result = 1;
                                    }
                                    else
                                    {
                                        ret = SQLForeignKeys( stmt_fk, NULL, 0, NULL, 0, NULL, 0, catalog_name, SQL_NTS, schema_name, SQL_NTS, table_name, SQL_NTS );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 1, stmt_fk );
                                            result = 1;
                                        }
                                        else
                                        {
                                            for( ret = SQLFetch( stmt_fk ); ( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) && ret != SQL_NO_DATA; ret = SQLFetch( stmt_fk ) )
                                            {
                                                FK_ONUPDATE update_constraint = NO_ACTION_UPDATE;
                                                FK_ONDELETE delete_constraint = NO_ACTION_DELETE;
                                                if( szFkName[0] == '\0' )
                                                    fkName = L"";
                                                else
                                                    str_to_uc_cpy( fkName, szFkName );
                                                str_to_uc_cpy( origSchema, schema_name );
                                                str_to_uc_cpy( origTable, table_name );
                                                str_to_uc_cpy( origCol, szPkCol );
                                                str_to_uc_cpy( refSchema, szPkSchema );
                                                str_to_uc_cpy( refTable, szPkTable );
                                                str_to_uc_cpy( refCol, szFkCol );
                                                switch( updateRule )
                                                {
                                                    case SQL_NO_ACTION:
                                                        update_constraint = NO_ACTION_UPDATE;
                                                        break;
                                                    case SQL_SET_NULL:
                                                        update_constraint = SET_NULL_UPDATE;
                                                        break;
                                                    case SQL_SET_DEFAULT:
                                                        update_constraint = SET_DEFAULT_UPDATE;
                                                        break;
                                                    case SQL_CASCADE:
                                                        update_constraint = CASCADE_UPDATE;
                                                        break;
                                                }
                                                if( pimpl->m_subtype == L"Microsoft SQL Server" && updateRule == SQL_RESTRICT )
                                                    update_constraint = NO_ACTION_UPDATE;
                                                switch( deleteRule )
                                                {
                                                    case SQL_NO_ACTION:
                                                        delete_constraint = NO_ACTION_DELETE;
                                                        break;
                                                    case SQL_SET_NULL:
                                                        delete_constraint = SET_NULL_DELETE;
                                                        break;
                                                    case SQL_SET_DEFAULT:
                                                        delete_constraint = SET_DEFAULT_DELETE;
                                                        break;
                                                    case SQL_CASCADE:
                                                        delete_constraint = CASCADE_DELETE;
                                                        break;
                                                }
                                                if( pimpl->m_subtype == L"Microsoft SQL Server" && deleteRule == SQL_RESTRICT )
                                                    delete_constraint = NO_ACTION_DELETE;
                                                                                         //id,         name,   orig_schema,  table_name,  orig_field,  ref_schema, ref_table, ref_field, update_constraint, delete_constraint
                                                foreign_keys[keySequence].push_back( new FKField( keySequence, fkName, origSchema, origTable, origCol, refSchema,  refTable,  refCol, origFields[keySequence], refFields[keySequence], update_constraint, delete_constraint ) );
                                                fk_fieldNames.push_back( origCol );
                                                primaryKey = L"";
                                                fkSchema = L"";
                                                fkTable = L"";
                                                fkName = L"";
                                                memset( szFkName, '\0', fkNameLength );
                                                memset( szFkTable, '\0', SQL_MAX_TABLE_NAME_LEN + 1 );
                                                memset( szPkTable, '\0', SQL_MAX_TABLE_NAME_LEN + 1 );
                                                origSchema = L"";
                                                origTable = L"";
                                                origCol = L"";
                                                refSchema = L"";
                                                refTable = L"";
                                                refCol = L"";
                                            }
                                        }
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                                        {
                                            GetErrorMessage( errorMsg, 1, stmt_fk );
                                            result = 1;
                                        }
                                    }
                                }
                            }
                            ret = SQLFreeHandle( SQL_HANDLE_STMT, stmt_fk );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 2, hdbc_fk );
                                result = 1;
                            }
                            else
                            {
                                stmt_fk = 0;
                                ret = SQLDisconnect( hdbc_fk );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 2, hdbc_fk );
                                    result = 1;
                                }
                                else
                                {
                                    ret = SQLFreeHandle( SQL_HANDLE_DBC, hdbc_fk );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 2, hdbc_fk );
                                        result = 1;
                                    }
                                    else
                                        hdbc_fk = 0;
                                }
                            }
                        }
                    }
                }
            }
            if( !result )
            {
                ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc_pk );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 0 );
                    result = 1;
                }
                else
                {
                    SQLSMALLINT OutConnStrLen;
                    ret = SQLDriverConnect( hdbc_pk, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 2 );
                        result = 1;
                        ret = SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                        hdbc_pk = 0;
                    }
                    else
                    {
                        ret = SQLAllocHandle(  SQL_HANDLE_STMT, hdbc_pk, &stmt_pk );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 2 );
                            result = 1;
                            SQLDisconnect( hdbc_pk );
                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                            hdbc_pk = 0;
                        }
                        else
                        {
                            ret = SQLBindCol( stmt_pk, 4, SQL_C_WCHAR, pkName, SQL_MAX_COLUMN_NAME_LEN + 1, &pkLength );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1 );
                                result = 1;
                                SQLFreeHandle( SQL_HANDLE_STMT, stmt_pk );
                                SQLDisconnect( hdbc_pk );
                                SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                                stmt_pk = 0;
                                hdbc_pk = 0;
                            }
                            else
                            {
                                ret = SQLPrimaryKeys( stmt_pk, catalog_name, SQL_NTS, schema_name, SQL_NTS, table_name, SQL_NTS );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_pk );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_pk );
                                    SQLDisconnect( hdbc_pk );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                                    stmt_pk = 0;
                                    hdbc_pk = 0;
                                }
                                else
                                {
                                    for( ret = SQLFetch( stmt_pk ); ( ret != SQL_SUCCESS || ret != SQL_SUCCESS_WITH_INFO ) && ret != SQL_NO_DATA; ret = SQLFetch( stmt_pk ) )
                                    {
                                        std::wstring pkFieldName;
                                        str_to_uc_cpy( pkFieldName, pkName );
                                        pk_fields.push_back( pkFieldName );
                                    }
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                                    {
                                        GetErrorMessage( errorMsg, 1 );
                                        result = 1;
                                        SQLFreeHandle( SQL_HANDLE_STMT, stmt_pk );
                                        SQLDisconnect( hdbc_pk );
                                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                                        stmt_pk = 0;
                                        hdbc_pk = 0;
                                    }
                                    ret = SQLFreeHandle( SQL_HANDLE_STMT, stmt_pk );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_pk );
                                        result = 1;
                                        SQLDisconnect( hdbc_pk );
                                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                                        stmt_pk = 0;
                                        hdbc_pk = 0;
                                    }
                                    else
                                    {
                                        stmt_pk = 0;
                                        ret = SQLDisconnect( hdbc_pk );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 2, hdbc_pk );
                                            result = 1;
                                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                                            hdbc_pk = 0;
                                        }
                                        else
                                        {
                                            ret = SQLFreeHandle( SQL_HANDLE_DBC, hdbc_pk );
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                            {
                                                GetErrorMessage( errorMsg, 2, hdbc_pk );
                                                result = 1;
                                            }
                                            else
                                                hdbc_pk = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if( !result )
            {
                ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc_colattr );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 0 );
                    result = 1;
                }
                else
                {
                    SQLSMALLINT OutConnStrLen;
                    ret = SQLDriverConnect( hdbc_colattr, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 2, hdbc_colattr );
                        result = 1;
                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_colattr );
                        hdbc_colattr = 0;
                    }
                    else
                    {
                        ret = SQLAllocHandle(  SQL_HANDLE_STMT, hdbc_colattr, &stmt_colattr );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 2, hdbc_colattr );
                            result = 1;
                            SQLDisconnect( hdbc_colattr );
                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_colattr );
                            hdbc_colattr = 0;
                        }
/***********************/
                        else
                        {
                            SQLWCHAR *queryTemp = new SQLWCHAR[tableName.length() + schemaName.length() + 17];
                            memset( queryTemp, '\0', tableName.length() + schemaName.length() + 17 );
                            uc_to_str_cpy( queryTemp, L"SELECT * FROM " );
                            uc_to_str_cpy( queryTemp, schemaName + L"." + tableName );
                            ret = SQLExecDirect( stmt_colattr, queryTemp, SQL_NTS );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_colattr );
                                result = 1;
                                SQLFreeHandle( SQL_HANDLE_STMT, stmt_colattr );
                                SQLDisconnect( hdbc_colattr );
                                SQLFreeHandle( SQL_HANDLE_DBC, hdbc_colattr );
                                hdbc_colattr = 0;
                                stmt_colattr = 0;
                            }
                            else
                            {
                                SQLSMALLINT bufSize = 1024, lenUsed;
                                ret = SQLNumResultCols( stmt_colattr, &numCols );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_colattr );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_colattr );
                                    SQLDisconnect( hdbc_colattr );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_colattr );
                                    hdbc_colattr = 0;
                                    stmt_colattr = 0;
                                }
                                else
                                {
                                    columnNames = new SQLWCHAR *[numCols];
                                    SQLLEN autoincrement;
                                    for( int i = 0; i < numCols; i++ )
                                    {
                                        autoincrement = 0;
                                        columnNames[i] = new SQLWCHAR[sizeof( SQLWCHAR ) * SQL_MAX_COLUMN_NAME_LEN + 1];
                                        ret = SQLColAttribute( stmt_colattr, (SQLUSMALLINT) i + 1, SQL_DESC_LABEL, columnNames[i], bufSize, &lenUsed, NULL );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 2, hdbc_colattr );
                                            result = 1;
                                            SQLFreeHandle( SQL_HANDLE_STMT, stmt_colattr );
                                            SQLDisconnect( hdbc_colattr );
                                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_colattr );
                                            hdbc_colattr = 0;
                                            stmt_colattr = 0;
                                            break;
                                        }
                                        ret = SQLColAttribute( stmt_colattr, (SQLUSMALLINT) i + 1, SQL_DESC_AUTO_UNIQUE_VALUE, NULL, 0, NULL, &autoincrement );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 2, hdbc_colattr );
                                            result = 1;
                                            SQLFreeHandle( SQL_HANDLE_STMT, stmt_colattr );
                                            SQLDisconnect( hdbc_colattr );
                                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_colattr );
                                            hdbc_colattr = 0;
                                            stmt_colattr = 0;
                                            break;
                                        }
                                        if( autoincrement == SQL_TRUE )
                                        {
                                            std::wstring autoincFieldName;
                                            str_to_uc_cpy( autoincFieldName, columnNames[i] );
                                            autoinc_fields.push_back( autoincFieldName );
                                        }
                                    }
                                }
                            }
                            delete queryTemp;
                            if( columnNames )
                            {
                                for( int i = 0; i < numCols; i++ )
                                {
                                    delete columnNames[i];
                                    columnNames[i] = NULL;
                                }
                                delete columnNames;
                                columnNames = NULL;
                            }
                        }
/***********************/
                    }
                }
            }
            if( !result )
            {
                ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc_col );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 0 );
                    result = 1;
                }
                else
                {
                    SQLSMALLINT OutConnStrLen;
                    ret = SQLDriverConnect( hdbc_col, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 2, hdbc_col );
                        result = 1;
                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                        hdbc_col = 0;
                    }
                    else
                    {
                        ret = SQLAllocHandle(  SQL_HANDLE_STMT, hdbc_col, &stmt_col );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 2, hdbc_col );
                            result = 1;
                            SQLDisconnect( hdbc_col );
                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                            hdbc_col = 0;
                        }
                        else
                        {
                            ret = SQLColumns( stmt_col, catalog_name, SQL_NTS, schema_name, SQL_NTS, table_name, SQL_NTS, NULL, 0);
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_col );
                                result = 1;
                                SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                stmt_col = 0;
                                SQLDisconnect( hdbc_col );
                                SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                hdbc_col = 0;
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 4, SQL_C_WCHAR, szColumnName, 256, &cbColumnName );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 6, SQL_C_WCHAR, szTypeName, 256, &cbTypeName );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 7, SQL_C_SLONG, &ColumnSize, 0, &cbColumnSize );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 8, SQL_C_SLONG, &BufferLength, 0, &cbBufferLength );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 9, SQL_C_SSHORT, &DecimalDigits, 0, &cbDecimalDigits );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 10, SQL_C_SSHORT, &NumPrecRadix, 0, &cbNumPrecRadix );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 11, SQL_C_SSHORT, &Nullable, 0, &cbNullable );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 12, SQL_C_WCHAR, szRemarks, 256, &cbRemarks );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 13, SQL_C_WCHAR, szColumnDefault, 256, &cbColumnDefault );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 14, SQL_C_SSHORT, &SQLDataType, 0, &cbSQLDataType );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 15, SQL_C_SSHORT, &DatetimeSubtypeCode, 0, &cbDatetimeSubtypeCode );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 16, SQL_C_SLONG, &CharOctetLength, 0, &cbCharOctetLength );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 17, SQL_C_SLONG, &OrdinalPosition, 0, &cbOrdinalPosition );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                ret = SQLBindCol( stmt_col, 18, SQL_C_WCHAR, szIsNullable, 256, &cbIsNullable );
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_col );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                                    stmt_col = 0;
                                    SQLDisconnect( hdbc_col );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                                    hdbc_col = 0;
                                }
                            }
                            if( !result )
                            {
                                int i = 0;
                                for( ret = SQLFetch( stmt_col ); ( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) && ret != SQL_NO_DATA; ret = SQLFetch( stmt_col ) )
                                {
                                    std::wstring schema_name, table_name;
                                    str_to_uc_cpy( fieldName, szColumnName );
                                    str_to_uc_cpy( fieldType, szTypeName );
                                    str_to_uc_cpy( defaultValue, szColumnDefault );
/*
									SQLLEN autoincrement;
                                    autoincrement = 0;
                                    ret = SQLColAttribute( stmt_colattr, (SQLUSMALLINT) i + 1, SQL_DESC_AUTO_UNIQUE_VALUE, NULL, 0, NULL, &autoincrement );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_colattr );
                                        result = 1;
                                        break;
                                    }
									else
*/
                                    {
                                        Field *field = new Field( fieldName, fieldType, ColumnSize, DecimalDigits, defaultValue, Nullable == 1, /*autoincrement == SQL_TRUE ? true : false*/std::find( autoinc_fields.begin(), autoinc_fields.end(), fieldName ) == autoinc_fields.end(), std::find( pk_fields.begin(), pk_fields.end(), fieldName ) != pk_fields.end(), std::find( fk_fieldNames.begin(), fk_fieldNames.end(), fieldName ) != fk_fieldNames.end() );
/*                                    if( GetFieldProperties( fieldName.c_str(), schemaName, odbc_pimpl->m_currentTableOwner, szColumnName, field, errorMsg ) )
                                    {
                                        GetErrorMessage( errorMsg, 2 );
                                        result = 1;
                                        break;
                                    }*/
                                        fields.push_back( field );
                                        fieldName = L"";
                                        fieldType = L"";
                                        defaultValue = L"";
                                        i++;
                                    }
                                }
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                                {
                                    GetErrorMessage( errorMsg, 2 );
                                    result = 1;
                                }
                            }
                        }
                    }
                }
                ret = SQLFreeHandle( SQL_HANDLE_STMT, stmt_col );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 1, stmt_col );
                    result = 1;
                }
                else
                {
                    stmt_col = 0;
                    ret = SQLDisconnect( hdbc_col );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 2, hdbc_col );
                        result = 1;
                    }
                    else
                    {
                        ret = SQLFreeHandle( SQL_HANDLE_DBC, hdbc_col );
                        if( ret != SQL_SUCCESS )
                        {
                            GetErrorMessage( errorMsg, 2, hdbc_col );
                            result = 1;
                        }
                        else
                            hdbc_col = 0;
                    }
                }
            }
            ret = SQLFreeHandle( SQL_HANDLE_STMT, stmt_colattr );
            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
            {
                GetErrorMessage( errorMsg, 1, stmt_colattr );
                result = 1;
            }
            else
            {
                stmt_colattr = 0;
                ret = SQLDisconnect( hdbc_colattr );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 2, hdbc_colattr );
                    result = 1;
                }
                else
                {
                    ret = SQLFreeHandle( SQL_HANDLE_DBC, hdbc_colattr );
                    if( ret != SQL_SUCCESS )
                    {
                        GetErrorMessage( errorMsg, 2, hdbc_colattr );
                        result = 1;
                    }
                    else
                        hdbc_colattr = 0;
                }
            }
            if( !result )
            {
                ret = SQLAllocHandle( SQL_HANDLE_DBC, m_env, &hdbc_ind );
                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                {
                    GetErrorMessage( errorMsg, 0 );
                    result = 1;
                }
                else
                {
                    SQLSMALLINT OutConnStrLen;
                    ret = SQLDriverConnect( hdbc_ind, NULL, m_connectString, SQL_NTS, NULL, 0, &OutConnStrLen, SQL_DRIVER_NOPROMPT );
                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                    {
                        GetErrorMessage( errorMsg, 2, hdbc_ind );
                        result = 1;
                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                        hdbc_ind = 0;
                    }
                    else
                    {
                        ret = SQLAllocHandle(  SQL_HANDLE_STMT, hdbc_ind, &stmt_ind );
                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                        {
                            GetErrorMessage( errorMsg, 2, hdbc_ind );
                            result = 1;
                            SQLDisconnect( hdbc_ind );
                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                            hdbc_ind = 0;
                        }
                        else
                        {
                            qry = new SQLWCHAR[query4.length() + 2];
                            memset( qry, '\0', query4.length() + 2 );
                            uc_to_str_cpy( qry, query4 );
                            ret = SQLPrepare( stmt_ind, qry, SQL_NTS );
                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                            {
                                GetErrorMessage( errorMsg, 1, stmt_ind );
                                result = 1;
                                SQLFreeHandle( SQL_HANDLE_STMT, stmt_ind );
                                SQLDisconnect( hdbc_ind );
                                SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                hdbc_ind = 0;
                                stmt_ind = 0;
                            }
                            else
                            {
                                SQLSMALLINT DataType, DecimalDigits, Nullable;
                                SQLLEN cbSchemaName = SQL_NTS, cbTableName = SQL_NTS;
                                SQLULEN ParamSize;
                                ret = SQLDescribeParam( stmt_ind, 1, &DataType, &ParamSize, &DecimalDigits, &Nullable);
                                if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                {
                                    GetErrorMessage( errorMsg, 1, stmt_ind );
                                    result = 1;
                                    SQLFreeHandle( SQL_HANDLE_STMT, stmt_ind );
                                    SQLDisconnect( hdbc_ind );
                                    SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                    hdbc_ind = 0;
                                    stmt_ind = 0;
                                }
                                else
                                {
                                    ret = SQLBindParameter( stmt_ind, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, DataType, ParamSize, DecimalDigits, schema_name, 0, &cbSchemaName );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_ind );
                                        result = 1;
                                        SQLFreeHandle( SQL_HANDLE_STMT, stmt_ind );
                                        SQLDisconnect( hdbc_ind );
                                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                        hdbc_ind = 0;
                                        stmt_ind = 0;
                                    }
                                }
                                if( !result )
                                {
                                    ret = SQLDescribeParam( stmt_ind, 1, &DataType, &ParamSize, &DecimalDigits, &Nullable);
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_ind );
                                        result = 1;
                                        SQLFreeHandle( SQL_HANDLE_STMT, stmt_ind );
                                        SQLDisconnect( hdbc_ind );
                                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                        hdbc_ind = 0;
                                        stmt_ind = 0;
                                    }
                                }
                                if( !result )
                                {
                                    ret = SQLBindParameter( stmt_ind, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, DataType, ParamSize, DecimalDigits, table_name, 0, &cbTableName );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_ind );
                                        result = 1;
                                        SQLFreeHandle( SQL_HANDLE_STMT, stmt_ind );
                                        SQLDisconnect( hdbc_ind );
                                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                        hdbc_ind = 0;
                                        stmt_ind = 0;
                                    }
                                }
                                if( !result )
                                {
                                    ret = SQLExecute( stmt_ind );
                                    if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                    {
                                        GetErrorMessage( errorMsg, 1, stmt_ind );
                                        result = 1;
                                        SQLFreeHandle( SQL_HANDLE_STMT, stmt_ind );
                                        SQLDisconnect( hdbc_ind );
                                        SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                        hdbc_ind = 0;
                                        stmt_ind = 0;
                                    }
                                    else
                                    {
                                        SQLWCHAR name[SQL_MAX_COLUMN_NAME_LEN];
                                        SQLLEN cbIndexName;
                                        ret = SQLBindCol( stmt_ind, 1, SQL_C_WCHAR, &name, SQL_MAX_COLUMN_NAME_LEN, &cbIndexName );
                                        if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO )
                                        {
                                            GetErrorMessage( errorMsg, 1, stmt_ind );
                                            result = 1;
                                            SQLFreeHandle( SQL_HANDLE_STMT, stmt_ind );
                                            SQLDisconnect( hdbc_ind );
                                            SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                            hdbc_ind = 0;
                                            stmt_ind = 0;
                                        }
                                        else
                                        {
                                            for( ret = SQLFetch( stmt_ind ); ( ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ) && ret != SQL_NO_DATA; ret = SQLFetch( stmt_ind ) )
                                            {
                                                std::wstring temp;
                                                str_to_uc_cpy( temp, name );
                                                indexes.push_back( temp );
                                            }
                                            if( ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO && ret != SQL_NO_DATA )
                                            {
                                                GetErrorMessage( errorMsg, 1, stmt_colattr );
                                                result = 1;
                                                SQLDisconnect( hdbc_ind );
                                                SQLFreeHandle( SQL_HANDLE_DBC, hdbc_ind );
                                                stmt_ind = 0;
                                                hdbc_ind = 0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if( !result )
            {
                std::wstring tempCatalogName, tempSchemaName, tempTableName;
                str_to_uc_cpy( tempCatalogName, catalog_name );
                str_to_uc_cpy( tempSchemaName, schema_name );
                str_to_uc_cpy( tempTableName, table_name );
                if( pimpl->m_subtype == L"Microsoft SQL Server" && tempSchemaName == L"sys" )
                    tempTableName = tempSchemaName + L"." + tempTableName;
                DatabaseTable *table = new DatabaseTable( tempTableName, tempSchemaName, fields, foreign_keys );
                table->SetTableOwner( owner );
                if( GetTableId( table, errorMsg ) )
                {
                    result = 1;
                }
                else
                {
                    if( GetTableProperties( table, errorMsg ) )
                    {
                        result = 1;
                    }
                    else
                    {
                        table->SetIndexNames( indexes );
                        pimpl->m_tables[tempCatalogName].push_back( table );
                        fields.erase( fields.begin(), fields.end() );
                        foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                        fields.clear();
                        foreign_keys.clear();
                        pk_fields.clear();
                        fk_fieldNames.clear();
                        indexes.clear();
                    }
                }
            }
        }
    }
    else
    {
    }
    delete qry;
    qry = NULL;
    return result;
}

void ODBCDatabase::GetConnectedUser(const std::wstring &dsn, std::wstring &connectedUser)
{
    SQLWCHAR *connectDSN = new SQLWCHAR[dsn.length() + 2];
    SQLWCHAR *entry = new SQLWCHAR[50];
    SQLWCHAR *retBuffer = new SQLWCHAR[256];
    SQLWCHAR fileName[16];
    SQLWCHAR defValue[50];
    memset( fileName, '\0', 16 );
    memset( retBuffer, '\0', 256 );
    memset( entry, '\0', 52 );
    memset( connectDSN, '\0', dsn.length() + 2 );
    memset( defValue, '\0', 50 );
    uc_to_str_cpy( fileName, L"odbc.ini" );
    uc_to_str_cpy( retBuffer, L" " );
    uc_to_str_cpy( entry, L"UserID" );
    uc_to_str_cpy( connectDSN, dsn );
    uc_to_str_cpy( defValue, L" " );
    int ret = SQLGetPrivateProfileString( connectDSN, entry, defValue, retBuffer, 256, fileName );
    if( ret < 0 )
        connectedUser = L"";
    else if( ret == 0 )
    {
        uc_to_str_cpy( entry, L"UserName" );
        int ret = SQLGetPrivateProfileString( connectDSN, entry, defValue, retBuffer, 256, fileName );
        if( ret < 0 )
            connectedUser = L"";
        else
            str_to_uc_cpy( connectedUser, retBuffer );
    }
    else
        str_to_uc_cpy( connectedUser, retBuffer );
}

void ODBCDatabase::GetConnectionPassword(const std::wstring &dsn, std::wstring &connectionPassword)
{
    SQLWCHAR *connectDSN = new SQLWCHAR[dsn.length() + 2];
    SQLWCHAR *entry = new SQLWCHAR[50];
    SQLWCHAR *retBuffer = new SQLWCHAR[256];
    SQLWCHAR fileName[16];
    SQLWCHAR defValue[50];
    memset( fileName, '\0', 16 );
    memset( retBuffer, '\0', 256 );
    memset( entry, '\0', 52 );
    memset( connectDSN, '\0', dsn.length() + 2 );
    memset( defValue, '\0', 50 );
    uc_to_str_cpy( fileName, L"odbc.ini" );
    uc_to_str_cpy( retBuffer, L" " );
    uc_to_str_cpy( entry, L"Password" );
    uc_to_str_cpy( connectDSN, dsn );
    uc_to_str_cpy( defValue, L" " );
    int ret = SQLGetPrivateProfileString( connectDSN, entry, defValue, retBuffer, 256, fileName );
    if( ret < 0 )
        connectionPassword = L"";
    else
        str_to_uc_cpy( connectionPassword, retBuffer );
}
