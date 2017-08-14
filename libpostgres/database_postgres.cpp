#ifdef WIN32
#include <windows.h>
#pragma execution_character_set("utf-8")
#endif

#if defined __WXMSW__ && defined __MEMORYLEAKS__
#include <vld.h>
#endif

#include <stdio.h>
#include <map>
#include <vector>
#include <string.h>
#include <string>
#include <locale>
#include <codecvt>
#include <algorithm>
#ifdef __WXGTK__
#include <arpa/inet.h>
#endif
#include <sstream>
#include "libpq-fe.h"
#include "database.h"
#include "database_postgres.h"

PostgresDatabase::PostgresDatabase() : Database()
{
    m_db = NULL;
    pimpl = new Impl;
    pimpl->m_type = L"PostgreSQL";
    pimpl->m_subtype = L"";
    m_pimpl = new PostgresImpl;
}

PostgresDatabase::~PostgresDatabase()
{
}

int PostgresDatabase::CreateDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    result = Disconnect( errorMsg );
    if( !result )
        result = Connect(name, errorMsg);
    return result;
}

int PostgresDatabase::DropDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    PGresult *res;
    std::wstring query = L"DROP DATABASE " + name;
    if( pimpl->m_dbName == name )
        result = Disconnect( errorMsg );
    if( !result )
    {
        res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str() );
        if( PQresultStatus( res ) != PGRES_COMMAND_OK )
        {
            result = 1;
            std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
            errorMsg.push_back( L"Error dropping database: " + err );
        }
        PQclear( res );
    }
    return result;
}

int PostgresDatabase::Connect(const std::wstring &selectedDSN, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    PGresult *res;
    std::wstring err;
    std::string query1 = "CREATE TABLE IF NOT EXISTS abcatcol(abc_tnam char(129) NOT NULL, abc_tid integer, abc_ownr char(129) NOT NULL, abc_cnam char(129) NOT NULL, abc_cid smallint, abc_labl char(254), abc_lpos smallint, abc_hdr char(254), abc_hpos smallint, abc_itfy smallint, abc_mask char(31), abc_case smallint, abc_hght smallint, abc_wdth smallint, abc_ptrn char(31), abc_bmap char(1), abc_init char(254), abc_cmnt char(254), abc_edit char(31), abc_tag char(254), PRIMARY KEY( abc_tnam, abc_ownr, abc_cnam ));";
    std::string query2 = "CREATE TABLE IF NOT EXISTS abcatedt(abe_name char(30) NOT NULL, abe_edit char(254), abe_type smallint, abe_cntr integer, abe_seqn smallint NOT NULL, abe_flag integer, abe_work char(32), PRIMARY KEY( abe_name, abe_seqn ));";
    std::string query3 = "CREATE TABLE IF NOT EXISTS abcatfmt(abf_name char(30) NOT NULL, abf_frmt char(254), abf_type smallint, abf_cntr integer, PRIMARY KEY( abf_name ));";
    std::string query4 = "CREATE TABLE IF NOT EXISTS abcattbl(abt_snam char(129), abt_tnam char(129) NOT NULL, abt_tid integer, abt_ownr char(129) NOT NULL, abd_fhgt smallint, abd_fwgt smallint, abd_fitl char(1), abd_funl char(1), abd_fchr smallint, abd_fptc smallint, abd_ffce char(18), abh_fhgt smallint, abh_fwgt smallint, abh_fitl char(1), abh_funl char(1), abh_fchr smallint, abh_fptc smallint, abh_ffce char(18), abl_fhgt smallint, abl_fwgt smallint, abl_fitl char(1), abl_funl char(1), abl_fchr smallint, abl_fptc smallint, abl_ffce char(18), abt_cmnt char(254), PRIMARY KEY( abt_tnam, abt_ownr ));";
    std::string query5 = "CREATE TABLE IF NOT EXISTS abcatvld(abv_name char(30) NOT NULL, abv_vald char(254), abv_type smallint, abv_cntr integer, abv_msg char(254), PRIMARY KEY( abv_name ));";
    std::string query6 = "CREATE INDEX IF NOT EXISTS \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"abt_tnam\" ASC, \"abt_ownr\" ASC);";
    std::string query7 = "CREATE INDEX IF NOT EXISTS \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"abc_tnam\" ASC, \"abc_ownr\" ASC, \"abc_cnam\" ASC);";
    std::wstring errorMessage;
    m_db = PQconnectdb( m_pimpl->m_myconv.to_bytes( selectedDSN.c_str() ).c_str() );
    if( PQstatus( m_db ) != CONNECTION_OK )
    {
        err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Connection to database failed: " + err );
        result = 1;
    }
    else
    {
        result = GetServerVersion( errorMsg );
        if( result )
        {
            PQfinish( m_db );
            errorMsg.push_back( L"Problem during connection. Please fix and restart the application" );
        }
        else
        {
            res = PQexec( m_db, "START TRANSACTION" );
            if( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
                err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( L"Starting transaction failed during connection: " + err );
                result = 1;
                PQclear( res );
            }
            else
            {
                PQclear( res );
                res = PQexec( m_db, query1.c_str() );
                if( PQresultStatus( res ) == PGRES_COMMAND_OK )
                {
                    PQclear( res );
                    res = PQexec( m_db, query2.c_str() );
                    if( PQresultStatus( res ) == PGRES_COMMAND_OK )
                    {
                        PQclear( res );
                        res = PQexec( m_db, query3.c_str() );
                        if( PQresultStatus( res ) == PGRES_COMMAND_OK )
					    {
                            PQclear( res );
                            res = PQexec( m_db, query4.c_str() );
                            if( PQresultStatus( res ) == PGRES_COMMAND_OK )
                            {
                                PQclear( res );
                                res = PQexec( m_db, query5.c_str() );
                                if( PQresultStatus( res ) == PGRES_COMMAND_OK )
                                {
                                    PQclear( res );
                                    if( pimpl->m_versionMajor >= 9 && pimpl->m_versionMinor >= 5 )
                                    {
                                        res = PQexec( m_db, query6.c_str() );
                                        if( PQresultStatus( res ) == PGRES_COMMAND_OK )
                                        {
                                            PQclear( res );
                                            res = PQexec( m_db, query7.c_str() );
                                            if( PQresultStatus( res ) == PGRES_COMMAND_OK )
                                            {
                                                res = PQexec( m_db, "COMMIT" );
                                                if( PQresultStatus( res ) == PGRES_COMMAND_OK )
                                                    PQclear( res );
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if( CreateIndexesOnPostgreConnection( errorMsg ) )
                                        {
                                            err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                                            errorMsg.push_back( err );
                                            result = 1;
                                        }
                                        else
                                            res = PQexec( m_db, "COMMIT" );
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
                PQclear( res );
                err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( L"Error during database connection: " + err );
                res = PQexec( m_db, "ROLLBACK" );
                result = 1;
            }
            else
            {
                result = GetTableListFromDb( errorMsg );
                if( result )
                {
                    errorMsg.push_back( L"Problem during connection. Please fix the problem and restart the application" );
                    result = 1;
                    PQclear( res );
                }
            }
        }
        pimpl->m_dbName = pimpl->m_tables.begin()->first;
        pimpl->m_connectString = selectedDSN;
        std::wstring temp = selectedDSN.substr( selectedDSN.find( L"user" ) );
        temp = temp.substr( temp.find( '=' ) );
        std::wstring user = temp.substr( temp.find( '=' ) + 2 );
        user = user.substr( 0, user.find( ' ' ) );
        pimpl->m_connectedUser = user;
    }
    return result;
}

int PostgresDatabase::ServerConnect(const std::wstring &selectedDSN, std::vector<std::wstring> &dbList, std::vector<std::wstring> &errorMsg)
{
    PGresult *res;
    std::wstring err;
    int result = 0;
    std::wstring query = L"SELECT datname FROM pg_database";
    m_db = PQconnectdb( m_pimpl->m_myconv.to_bytes( selectedDSN.c_str() ).c_str() );
    if( PQstatus( m_db ) != CONNECTION_OK )
    {
        err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Connection to database failed: " + err );
        result = 1;
    }
    else
    {
        res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str() );
        ExecStatusType status = PQresultStatus( res );
        if( status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK )
        {
            err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
            errorMsg.push_back( L"Connection to database failed: " + err );
            result = 1;
        }
        else if( status == PGRES_TUPLES_OK )
        {
            for( int i = 0; i < PQntuples( res ); i++ )
            {
                dbList.push_back( m_pimpl->m_myconv.from_bytes( PQgetvalue( res, i, 0 ) ) );
            }
        }
    }
    return result;
}

int PostgresDatabase::Disconnect(std::vector<std::wstring> &UNUSED(errorMsg))
{
    int result = 0;
    PQfinish( m_db );
    for( std::map<std::wstring, std::vector<DatabaseTable *> >::iterator it = pimpl->m_tables.begin(); it != pimpl->m_tables.end(); it++ )
    {
        std::vector<DatabaseTable *> tableVec = (*it).second;
        for( std::vector<DatabaseTable *>::iterator it1 = tableVec.begin(); it1 < tableVec.end(); it1++ )
        {
            std::vector<Field *> fields = (*it1)->GetFields();
            for( std::vector<Field *>::iterator it2 = fields.begin(); it2 < fields.end(); it2++ )
            {
                delete (*it2);
                (*it2) = NULL;
            }
            std::map<int,std::vector<FKField *> > fk_fields = (*it1)->GetForeignKeyVector();
            for( std::map<int, std::vector<FKField *> >::iterator it2 = fk_fields.begin(); it2 != fk_fields.end(); it2++ )
            {
                for( std::vector<FKField *>::iterator it3 = (*it2).second.begin(); it3 < (*it2).second.end(); it3++ )
                {
                    delete (*it3);
                    (*it3) = NULL;
                }
            }
        }
        for( std::vector<DatabaseTable *>::iterator it1 = tableVec.begin(); it1 < tableVec.end(); it1++ )
        {
            delete (*it1);
            (*it1) = NULL;
        }
    }
    delete m_pimpl;
    m_pimpl = NULL;
    return result;
}

int PostgresDatabase::GetTableListFromDb(std::vector<std::wstring> &errorMsg)
{
    PGresult *res, *res1, *res2, *res3, *res4, *res5;
    std::vector<Field *> fields;
    std::vector<std::wstring> fk_names, indexes;
    std::map<int,std::vector<FKField *> > foreign_keys;
    std::wstring errorMessage;
    std::wstring fieldName, fieldType, fieldDefaultValue, fkTable, fkField, fkTableField, fkUpdateConstraint, fkDeleteConstraint;
    int result = 0, fieldIsNull, fieldPK, fkReference, fkId;
    FK_ONUPDATE update_constraint = NO_ACTION_UPDATE;
    FK_ONDELETE delete_constraint = NO_ACTION_DELETE;
    std::wstring query1 = L"SELECT t.table_catalog AS catalog, t.table_schema AS schema, t.table_name AS table, u.usename AS owner, c.oid AS table_id FROM information_schema.tables t, pg_catalog.pg_class c, pg_catalog.pg_user u WHERE t.table_name = c.relname AND c.relowner = usesysid AND (t.table_type = 'BASE TABLE' OR t.table_type = 'VIEW' OR t.table_type = 'LOCAL TEMPORARY') ORDER BY table_name;";
    std::wstring query2 = L"SELECT DISTINCT column_name, data_type, character_maximum_length, character_octet_length, numeric_precision, numeric_precision_radix, numeric_scale, is_nullable, column_default, CASE WHEN column_name IN (SELECT ccu.column_name FROM information_schema.constraint_column_usage ccu, information_schema.table_constraints tc WHERE ccu.constraint_name = tc.constraint_name AND tc.constraint_type = 'PRIMARY KEY' AND ccu.table_name = 'leagues') THEN 'YES' ELSE 'NO' END AS is_pk, ordinal_position FROM information_schema.columns col, information_schema.table_constraints tc WHERE tc.table_schema = col.table_schema AND tc.table_name = col.table_name AND col.table_schema = $1 AND col.table_name = $2 ORDER BY ordinal_position;";
    std::wstring query3 = L"select x.ordinal_position AS pos, x.position_in_unique_constraint AS field_pos, c.constraint_name AS name, x.table_schema as schema, x.table_name AS table, x.column_name AS column, y.table_schema as ref_schema, y.table_name as ref_table, y.column_name as ref_column, c.update_rule, c.delete_rule from information_schema.referential_constraints c, information_schema.key_column_usage x, information_schema.key_column_usage y where x.constraint_name = c.constraint_name and y.ordinal_position = x.position_in_unique_constraint and y.constraint_name = c.unique_constraint_name AND x.table_schema = $1 AND x.table_name = $2 order by c.constraint_name, x.ordinal_position;";
    std::wstring query4 = L"SELECT indexname FROM pg_indexes WHERE schemaname = $1 AND tablename = $2;";
    std::wstring query5 = L"SELECT rtrim(abt_tnam), abt_tid, rtrim(abt_ownr), abd_fhgt, abd_fwgt, abd_fitl, abd_funl, abd_fchr, abd_fptc, rtrim(abd_ffce), abh_fhgt, abh_fwgt, abh_fitl, abh_funl, abh_fchr, abh_fptc, rtrim(abh_ffce), abl_fhgt, abl_fwgt, abl_fitl, abl_funl, abl_fchr, abl_fptc, rtrim(abl_ffce), rtrim(abt_cmnt) FROM abcattbl WHERE abt_tnam = $1 AND abt_ownr = $2;";
    std::wstring query6 = L"SELECT * FROM \"abcatcol\" WHERE \"abc_tnam\" = $1 AND \"abc_ownr\" = $2 AND \"abc_cnam\" = $3;";
    res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query1.c_str() ).c_str() );
    ExecStatusType status = PQresultStatus( res ); 
    if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( err );
        PQclear( res );
        result = 1;
    }
    else
    {
        res1 = PQprepare( m_db, "get_fkeys", m_pimpl->m_myconv.to_bytes( query3.c_str() ).c_str(), 2, NULL );
        ExecStatusType status = PQresultStatus( res1 );
        if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
        {
            std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
            errorMsg.push_back( err );
            PQclear( res1 );
            result = 1;
        }
        else
        {
            res2 = PQprepare( m_db, "get_columns", m_pimpl->m_myconv.to_bytes( query2.c_str() ).c_str(), 2, NULL );
            if( PQresultStatus( res2 ) != PGRES_COMMAND_OK )
            {
                std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( L"Error executing query: " + err );
                PQclear( res2 );
                result = 1;
            }
            else
            {
                res4 = PQprepare( m_db, "get_indexes", m_pimpl->m_myconv.to_bytes( query4.c_str() ).c_str(), 2, NULL );
                if( PQresultStatus( res4 ) != PGRES_COMMAND_OK )
                {
                    std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                    errorMsg.push_back( L"Error executing query: " + err );
                    PQclear( res2 );
                    fields.erase( fields.begin(), fields.end() );
                    foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                    result = 1;
                }
                else
                {
                    res3 = PQprepare( m_db, "get_table_prop", m_pimpl->m_myconv.to_bytes( query5.c_str() ).c_str(), 3, NULL );
                    if( PQresultStatus( res3 ) != PGRES_COMMAND_OK )
                    {
                        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                        errorMsg.push_back( L"Error executing query: " + err );
                        PQclear( res3 );
                        result = 1;
                    }
                    else
                    {
                        res5 = PQprepare( m_db, "get_field_properties", m_pimpl->m_myconv.to_bytes( query6.c_str() ).c_str(), 3, NULL );
                        if( PQresultStatus( res5 ) != PGRES_COMMAND_OK )
                        {
                            std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                            errorMsg.push_back( L"Error executing query: " + err );
                            PQclear( res );
                            result = 1;
                        }
                        else
                        {
                            for( int i = 0; i < PQntuples( res ); i++ )
                            {
                                char *catalog_name = PQgetvalue( res, i, 0 );
                                char *schema_name = PQgetvalue( res, i, 1 );
                                char *table_name = PQgetvalue( res, i, 2 );
                                char *table_owner = PQgetvalue( res, i, 3 );
                                int table_id = (int *) PQgetvalue( res, i, 4 );
                                char *values1[2];
                                values1[0] = new char[strlen( schema_name ) + 1];
                                values1[1] = new char[strlen( table_name ) + 1];
                                strcpy( values1[0], schema_name );
                                strcpy( values1[1], table_name );
                                int len1 = strlen( schema_name );
                                int len2 = strlen( table_name );
                                int length1[2] = { len1, len2 };
                                int formats1[2] = { 1, 1 };
                                res1 = PQexecPrepared( m_db, "get_fkeys", 2, values1, length1, formats1, 1 );
                                status = PQresultStatus( res1 ); 
                                if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
                                {
                                    std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                                    errorMsg.push_back( L"Error executing query: " + err );
                                    PQclear( res1 );
                                    fields.erase( fields.begin(), fields.end() );
                                    foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                                    result = 1;
                                    break;
                                }
                                else if( status == PGRES_TUPLES_OK )
                                {
                                    int count = 0;
                                    for( int j = 0; j < PQntuples( res1 ); j++ )
                                    {
                                        fkField = m_pimpl->m_myconv.from_bytes( PQgetvalue( res1, j, 2 ) );
                                        fkTable = m_pimpl->m_myconv.from_bytes( PQgetvalue( res1, j, 3 ) );
                                        fkTableField = m_pimpl->m_myconv.from_bytes( PQgetvalue( res1, j, 4 ) );
                                        fkUpdateConstraint = m_pimpl->m_myconv.from_bytes( PQgetvalue( res1, j, 5 ) );
                                        fkDeleteConstraint = m_pimpl->m_myconv.from_bytes( PQgetvalue( res1, j, 6 ) );
                                        if( fkUpdateConstraint == L"NO ACTION" )
                                            update_constraint = NO_ACTION_UPDATE;
                                        if( fkUpdateConstraint == L"RESTRICT" )
                                            update_constraint = RESTRICT_UPDATE;
                                        if( fkUpdateConstraint == L"SET NULL" )
                                            update_constraint = SET_NULL_UPDATE;
                                        if( fkUpdateConstraint == L"SET DEFAULT" )
                                            update_constraint = SET_DEFAULT_UPDATE;
                                        if( fkUpdateConstraint == L"CASCADE" )
                                            update_constraint = CASCADE_UPDATE;
                                        if( fkDeleteConstraint == L"NO ACTION" )
                                            delete_constraint = NO_ACTION_DELETE;
                                        if( fkDeleteConstraint == L"RESTRICT" )
                                            delete_constraint = RESTRICT_DELETE;
                                        if( fkDeleteConstraint == L"SET NULL" )
                                            delete_constraint = SET_NULL_DELETE;
                                        if( fkDeleteConstraint == L"SET DEFAULT" )
                                            delete_constraint = SET_DEFAULT_DELETE;
                                        if( fkDeleteConstraint == L"CASCADE" )
                                            delete_constraint = CASCADE_DELETE;
                                        foreign_keys[count++].push_back( new FKField( fkReference, fkTable, fkField, fkTableField, L"", update_constraint, delete_constraint ) );
                                        fk_names.push_back( fkField );
                                    }
                                    PQclear( res1 );
                                    res2 = PQexecPrepared( m_db, "get_columns", 2, values1, length1, formats1, 1 );
                                    status = PQresultStatus( res2 );
                                    if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
                                    {
                                        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                                        errorMsg.push_back( L"Error executing query: " + err );
                                        PQclear( res2 );
                                        fields.erase( fields.begin(), fields.end() );
                                        foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                                        result = 1;
                                        break;
                                    }
                                    else if( status == PGRES_TUPLES_OK )
                                    {
                                        for( int j = 0; j < PQntuples( res2 ); j++ )
                                        {
                                            int size, precision;
                                            bool autoinc = false;
                                            fieldName = m_pimpl->m_myconv.from_bytes( PQgetvalue( res2, j, 0 ) );
                                            fieldType = m_pimpl->m_myconv.from_bytes( PQgetvalue( res2, j, 1 ) );
                                            char *char_length = PQgetvalue( res2, j, 2 );
                                            char *char_radix = PQgetvalue( res2, j, 3 );
                                            char *numeric_length = PQgetvalue( res2, j, 4 );
                                            char *numeric_radix = PQgetvalue( res2, j, 5 );
                                            char *numeric_scale = PQgetvalue( res2, j, 6 );
                                            fieldDefaultValue = m_pimpl->m_myconv.from_bytes( PQgetvalue( res2, j, 8 ) );
                                            fieldIsNull = !strcmp( PQgetvalue( res2, j, 7 ), "YES" ) ? 1 : 0;
                                            fieldPK = !strcmp( PQgetvalue( res2, j, 9 ), "YES" ) ? 1 : 0;
                                            if( *char_length == '0' )
                                            {
                                                size = atoi( numeric_length );
                                                precision = atoi( numeric_scale );
                                            }
                                            else
                                            {
                                                size = atoi( char_length );
                                                precision = 0;
                                            }
                                            if( fieldType == L"serial" || fieldType == L"bigserial" )
                                                autoinc = true;
                                            Field *field = new Field( fieldName, fieldType, size, precision, fieldDefaultValue, fieldIsNull, autoinc, fieldPK, std::find( fk_names.begin(), fk_names.end(), fieldName ) != fk_names.end() );
                                            if( GetFieldProperties( table_name, schema_name, table_owner, fieldName, errorMsg ) )
                                            {
                                                std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                                                errorMsg.push_back( err );
                                                PQclear( res2 );
                                                fields.erase( fields.begin(), fields.end() );
                                                foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                                                result = 1;
                                                break;
                                            }
                                            fields.push_back( field );
                                        }
                                        PQclear( res2 );
                                        if( result )
                                            break;
                                        else
                                        {
                                            DatabaseTable *table = new DatabaseTable( m_pimpl->m_myconv.from_bytes( table_name ), m_pimpl->m_myconv.from_bytes( schema_name ), fields, foreign_keys );
                                            if( GetTableProperties( table, errorMsg ) )
                                            {
                                                char *err = PQerrorMessage( m_db );
                                                errorMsg.push_back( m_pimpl->m_myconv.from_bytes( err ) );
                                                fields.erase( fields.begin(), fields.end() );
                                                foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                                                result = 1;
                                                break;
                                            }
                                            else
                                            {
                                                res4 = PQexecPrepared( m_db, "get_indexes", 2, values1, length1, formats1, 1 );
                                                ExecStatusType status = PQresultStatus( res4 );
                                                if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
                                                {
                                                    std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                                                    errorMsg.push_back( L"Error executing query: " + err );
                                                    PQclear( res4 );
                                                    fields.erase( fields.begin(), fields.end() );
                                                    foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                                                    result = 1;
                                                    break;
                                                }
                                                else
                                                {
                                                    for( int j = 0; j < PQntuples( res4 ); j++ )
                                                        indexes.push_back( m_pimpl->m_myconv.from_bytes( PQgetvalue( res4, j, 0 ) ) );
                                                    table->SetIndexNames( indexes );
                                                    pimpl->m_tables[m_pimpl->m_myconv.from_bytes( catalog_name )].push_back( table );
                                                    fields.erase( fields.begin(), fields.end() );
                                                    foreign_keys.erase( foreign_keys.begin(), foreign_keys.end() );
                                                    fk_names.clear();
                                                    PQclear( res4 );
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
        PQclear( res );
    }
    return result;
}

int PostgresDatabase::CreateIndex(const std::wstring &command, const std::wstring &index_name, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg)
{
    PGresult *res;
    std::wstring query;
    int result = 0;
    res = PQexec( m_db, "BEGIN TRANSACTION" );
    ExecStatusType status = PQresultStatus( res );
    if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( err );
        PQclear( res );
        result = 1;
    }
    else
    {
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
            res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( command.c_str() ).c_str() );
            if( PQresultStatus( res ) != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
            {
                std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( err );
                PQclear( res );
                result = 1;
            }
            if( result == 1 )
                query = L"ROLLBACK";
            else
                query = L"COMMIT";
            res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str() );
            if( PQresultStatus( res ) != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
            {
                std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( err );
                PQclear( res );
                result = 1;
            }
        }
    }
    return result;
}

bool PostgresDatabase::IsIndexExists(const std::wstring &indexName, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg)
{
    PGresult *res;
    bool exists = false;
    std::wstring query = L"SELECT 1 FROM pg_indexes WHERE schemaname = $1 AND tablename = $2 AND indexname = $3;";
    char *values[3];
    values[0] = NULL, values[1] = NULL, values[2] = NULL;
    values[0] = new char[schemaName.length() + 1];
    values[1] = new char[tableName.length() + 1];
    values[2] = new char[indexName.length() + 1];
    memset( values[0], '\0', schemaName.length() + 1 );
    memset( values[1], '\0', tableName.length() + 1 );
    memset( values[2], '\0', indexName.length() + 1 );
    strcpy( values[0], m_pimpl->m_myconv.to_bytes( schemaName.c_str() ).c_str() );
    strcpy( values[1], m_pimpl->m_myconv.to_bytes( tableName.c_str() ).c_str() );
    strcpy( values[2], m_pimpl->m_myconv.to_bytes( indexName.c_str() ).c_str() );
    int len1 = schemaName.length();
    int len2 = tableName.length();
    int len3 = indexName.length();
    int length[3] = { len1, len2, len3 };
    int formats[3] = { 1, 1, 1 };
    res = PQexecParams( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str(), 3, NULL, values, length, formats, 1 );
    ExecStatusType status = PQresultStatus( res ); 
    if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Error executing query: " + err );
        PQclear( res );
    }
    else if( status == PGRES_TUPLES_OK )
    {
        exists = 1;
    }
    delete values[0];
    values[0] = NULL;
    delete values[1];
    values[1] = NULL;
    delete values[2];
    values[2] = NULL;
    return exists;
}

int PostgresDatabase::GetTableProperties(DatabaseTable *table, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    std::wstring schemaName = table->GetSchemaName(), tableName = table->GetTableName(), ownerName = table->GetTableOwner();
    std::wstring t = schemaName + L".";
    t += tableName;
    char *values[2];
    values[0] = new char[t.length()];
    values[1] = new char[ownerName.length() + 1];
    memset( values[0], '\0', t.length() + 1 );
    memset( values[1], '\0', ownerName.length() + 1 );
    strcpy( values[0], m_pimpl->m_myconv.to_bytes( t.c_str() ).c_str() );
    strcpy( values[1], m_pimpl->m_myconv.to_bytes( ownerName.c_str() ).c_str() );
    int len1 = /*t.length()*/strlen( values[0] );
    int len2 = /*ownerName.length()*/strlen( values[1] );
    int length[2] = { len1, len2 };
    int formats[2] = { 1, 1 };
    PGresult *res = PQexecPrepared( m_db, "get_table_prop", 2, values, length, formats, 1 );
    ExecStatusType status = PQresultStatus( res );
    if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Error executing query: " + err );
        result = 1;
    }
    else if( status == PGRES_TUPLES_OK )
    {
        for( int i = 0; i < PQntuples( res ); i++ )
        {
            table->SetDataFontSize( atoi( PQgetvalue( res, i, 4 ) ) );
            table->SetDataFontWeight( atoi( PQgetvalue( res, i, 5 ) ) );
            table->SetDataFontItalic( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 6 ) ) == L"Y" ? true : false );
            table->SetDataFontUnderline( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 7 ) ) == L"Y" ? true : false );
            table->SetDataFontCharacterSet( atoi( PQgetvalue( res, i, 8 ) ) );
            table->SetDataFontPixelSize( atoi( PQgetvalue( res, i, 9 ) ) );
            table->SetDataFontName( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 10 ) ) );
            table->SetHeadingFontSize( atoi( PQgetvalue( res, i, 11 ) ) );
            table->SetDataFontWeight( atoi( PQgetvalue( res, i, 12 ) ) );
            table->SetHeadingFontItalic( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 13 ) ) == L"Y" ? true : false );
            table->SetHeadingFontUnderline( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 14 ) ) == L"Y" ? true : false );
            table->SetHeadingFontCharacterSet( atoi( PQgetvalue( res, i, 15 ) ) );
            table->SetHeadingFontPixelSize( atoi( PQgetvalue( res, i, 16 ) ) );
            table->SetHeadingFontName( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 17 ) ) );
            table->SetLabelFontSize( atoi( PQgetvalue( res, i, 18 ) ) );
            table->SetLabelFontWeight( atoi( PQgetvalue( res, i, 19 ) ) );
            table->SetLabelFontItalic( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 20 ) ) == L"Y" ? true : false );
            table->SetLabelFontUnderline( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 21 ) ) == L"Y" ? true : false );
            table->SetLabelFontCharacterSet( atoi( PQgetvalue( res, i, 22 ) ) );
            table->SetLabelFontPixelSize( atoi( PQgetvalue( res, i, 23 ) ) );
            table->SetLabelFontName( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 24 ) ) );
            table->SetComment( m_pimpl->m_myconv.from_bytes( (const char *) PQgetvalue( res, i, 25 ) ) );
        }
    }
    PQclear( res );
    return result;
}

int PostgresDatabase::SetTableProperties(const DatabaseTable *table, const TableProperties &properties, bool isLog, std::wstring &command, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    std::wstring err;
    bool exist;
    std::wostringstream istr;
    std::wstring query = L"BEGIN TRANSACTION";
    PGresult *res;
    res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str() );
    if( PQresultStatus( res ) != PGRES_COMMAND_OK )
    {
        PQclear( res );
        err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( err );
        result = 1;
    }
    else
    {
        std::wstring tableName = const_cast<DatabaseTable *>( table )->GetTableName();
        std::wstring schemaName = const_cast<DatabaseTable *>( table )->GetSchemaName();
        std::wstring comment = const_cast<DatabaseTable *>( table )->GetComment();
        int tableId = const_cast<DatabaseTable *>( table )->GetTableId();
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
                command += pimpl->m_connectedUser;
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
                command += L"\', \"abd_funl\" = \'";
                command += properties.m_isDataFontUnderlined ? L"Y" : L"N";
                command += L"\', \"abd_fchr\" = ";
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
                command += L", \"abd_fwgt\" = ";
                istr << properties.m_isHeadingFontBold;
                command += istr.str();
                istr.clear();
                istr.str( L"" );
                command += L", \"abh_fitl\" = \'";
                command += properties.m_isHeadingFontItalic ? L"Y" : L"N";
                command += L"\', \"abh_funl\" = \'";
                command += properties.m_isHeadingFontUnderlined ? L"Y" : L"N";
                command += L"\', \"abh_fchr\" = ";
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
                command += L"\', \"abl_funl\" = \'";
                command += properties.m_isLabelFontUnderlined ? L"Y" : L"N";
                command += L"\', \"abl_fchr\" = ";
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
                command += comment;
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
                command += pimpl->m_connectedUser;
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
                command += L"\', \'";
                command += properties.m_isDataFontUnderlined ? L"Y" : L"N";
                command += L"\', ";
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
                command += L"\', \'";
                command += properties.m_isHeadingFontUnderlined ? L"Y" : L"N";
                command += L"\', ";
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
                command += L"\', \'";
                command += properties.m_isLabelFontUnderlined ? L"Y" : L"N";
                command += L"\', ";
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
                command += comment;
                command += L"\' );";
            }
            if( !isLog )
            {
                res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( command.c_str() ).c_str() );
                if( PQresultStatus( res ) != PGRES_COMMAND_OK )
                {
                    PQclear( res );
                    err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                    errorMsg.push_back( err );
                    result = 1;
                }
            }
        }
    }
    if( result == 1 )
        query = L"ROLLBACK";
    else
        query = L"COMMIT";
    res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str() );
    if( PQresultStatus( res ) != PGRES_COMMAND_OK )
    {
        PQclear( res );
        err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( err );
        result = 1;
    }
    return result;
}

bool PostgresDatabase::IsTablePropertiesExist(const DatabaseTable *table, std::vector<std::wstring> &errorMsg)
{
    bool result = false;
    std::wstring query = L"SELECT 1 FROM abcattbl WHERE abt_tnam = $1 AND abt_ownr = $2;";
    std::wstring tname = const_cast<DatabaseTable *>( table )->GetSchemaName() + L".";
    tname += const_cast<DatabaseTable *>( table )->GetTableName();
    std::wstring owner = const_cast<DatabaseTable *>( table )->GetTableOwner();
    int tableId = htonl( const_cast<DatabaseTable *>( table )->GetTableId() );
    char *values[2];
    values[0] = new char[tname.length() + 1];
    values[1] = new char[owner.length() + 1];
    memset( values[0], '\0', tname.length() + 1 );
    memset( values[1], '\0', owner.length() + 1 );
    strcpy( values[0], m_pimpl->m_myconv.to_bytes( tname.c_str() ).c_str() );
    strcpy( values[1], m_pimpl->m_myconv.to_bytes( owner.c_str() ).c_str() );
    values[2] = (char *) &tableId;
    int len1 = tname.length();
    int len2 = owner.length();
    int length[2] = { len1, len2 };
    int formats[2] = { 1, 1 };
    PGresult *res = PQexecParams( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str(), 2, NULL, values, length, formats, 1 );
    ExecStatusType status = PQresultStatus( res ); 
    if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Error executing query: " + err );
        result = 1;
    }
    else if( status == PGRES_TUPLES_OK )
    {
        result = true;
    }
    PQclear( res );
    return result;
}

int PostgresDatabase::GetFieldProperties(const char *tableName, const char *schemaName, const char *ownerName, const char *fieldName, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    int len = strlen( tableName ) + strlen( schemaName ) + 2;
    char *tname = new char[len];
    memset( tname, '\0', len );
    strcpy( tname, schemaName );
    strcat( tname, "." );
    strcat( tname, tableName );
    char *values[3];
    values[0] = NULL, values[1] = NULL, values[2] = NULL;
    values[0] = new char[strlen( tname ) + 1];
    values[1] = new char[strlen( ownerName ) + 1];
    values[2] = new char[table->GetFieldName().length() + 1];
    memset( values[0], '\0', strlen( tname ) + 1 );
    memset( values[1], '\0', strlen( ownerName ) + 1 );
    memset( values[2], '\0', strlen( fieldName ) + 1 );
    strcpy( values[0], tname );
    strcpy( values[1], ownerName );
    strcpy( values[2], fieldName );
    int len1 = strlen( values[0] );
    int len2 = strlen( values[1] );
    int len3 = strlen( values[2] );
    int length[3] = { len1, len2, len3 };
    int formats[3] = { 1, 1, 1 };
    PGresult *res = PQexecPrepared( m_db, "get_field_properties", 3, values, length, formats, 1 );
    ExecStatusType status = PQresultStatus( res );
    if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Error executing query: " + err );
        result = 1;
    }
    else if( status == PGRES_TUPLES_OK )
    {
        for( int i = 0; i < PQntuples( res ); i++ )
        {
        }
    }
    delete values[0];
    values[0] = NULL;
    delete values[1];
    values[1] = NULL;
    delete values[2];
    values[2] = NULL;
    delete tname;
    tname = NULL;
    return result;
}

int PostgresDatabase::ApplyForeignKey(const std::wstring &command, const std::wstring &keyName, DatabaseTable &tableName, std::vector<std::wstring> &errorMsg)
{
    bool exist = false;
    int result = 0;
    std::wstring err;
    std::wstring query1 = L"SELECT 1 FROM information_schema.table_constraints WHERE constraint_name = $1 AND table_schema = $2 AND table_name = $2";
    char *values[3];
    values[0] = new char[keyName.length() + 1];
    values[1] = new char[tableName.GetSchemaName().length() + 1];
    values[2] = new char[tableName.GetTableName().length() + 1];
    memset( values[0], '\0', keyName.length() + 1 );
    memset( values[1], '\0', tableName.GetSchemaName().length() + 1 );
    memset( values[2], '\0', tableName.GetTableName().length() + 1 );
    strcpy( values[0], m_pimpl->m_myconv.to_bytes( keyName.c_str() ).c_str() );
    strcpy( values[1], m_pimpl->m_myconv.to_bytes( tableName.GetSchemaName().c_str() ).c_str() );
    strcpy( values[2], m_pimpl->m_myconv.to_bytes( tableName.GetTableName().c_str() ).c_str() );
    int len0 = keyName.length();
    int len1 = tableName.GetSchemaName().length();
    int len2 = tableName.GetTableName().length();
    int length[3] = { len0, len1, len2 };
    int formats[3] = { 1, 1, 1 };
    PGresult *res = PQexec( m_db, "START TRANSACTION" );
    if( PQresultStatus( res ) != PGRES_COMMAND_OK )
    {
        PQclear( res );
        err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Starting transaction failed during connection: " + err );
        result = 1;
    }
    else
    {
        res = PQexecParams( m_db, m_pimpl->m_myconv.to_bytes( query1.c_str() ).c_str(), 3, NULL, values, length, formats, 1 );
        ExecStatusType status = PQresultStatus( res );
        if( status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK )
        {
            std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
            errorMsg.push_back( L"Error executing query: " + err );
            PQclear( res );
            result = 1;
        }
        else if( status == PGRES_TUPLES_OK )
        {
            exist = true;
        }
        PQclear( res );
        if( !exist )
        {
            res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( command.c_str() ).c_str() );
            if( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
                PQclear( res );
                err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( err );
                result = 1;
            }
        }
        else
        {
            PQclear( res );
            errorMsg.push_back( L"Foreign key specified already exist!" );
            result = 1;
        }
        if( !result )
        {
            res = PQexec( m_db, "COMMIT" );
            if( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
                PQclear( res );
                err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( L"Starting transaction failed during connection: " + err );
                result = 1;
            }
        }
        else
        {
            res = PQexec( m_db, "ROLLBACK" );
            if( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
                PQclear( res );
                err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( L"Starting transaction failed during connection: " + err );
                result = 1;
            }
        }
    }
    return result;
}

int PostgresDatabase::DeleteTable(const std::wstring &tableName, std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    std::wstring query = L"DROP TABLE ";
    query += tableName;
    query += L" CASCADE;";
    PGresult *res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query.c_str() ).c_str() );
    if( PQresultStatus( res ) != PGRES_COMMAND_OK )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Starting transaction failed during connection: " + err );
        result = 1;
    }
    PQclear( res );
    return result;
}

int PostgresDatabase::SetFieldProperties(const std::wstring &command, std::vector<std::wstring> &errorMsg)
{
    int res = 0;
    return res;
}

int PostgresDatabase::GetServerVersion(std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    int versionInt = PQserverVersion( m_db );
    if( !versionInt )
    {
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( L"Error executing query: " + err );
        result = 1;
    }
    else
    {
        pimpl->m_serverVersion = m_pimpl->m_myconv.from_bytes( PQparameterStatus( m_db, "server_version" ) );
        pimpl->m_versionMajor = versionInt / 10000;
        pimpl->m_versionMinor = ( versionInt - pimpl->m_versionMajor * 10000 ) / 100;
    }
    return result;
}

int PostgresDatabase::CreateIndexesOnPostgreConnection(std::vector<std::wstring> &errorMsg)
{
    int result = 0;
    PGresult *res;
    std::wstring query1 = L"SELECT 1 FROM pg_class c, pg_namespace n WHERE n.oid = c.relnamespace AND c.relname = \'abcattbl_tnam_ownr\' AND n.nspname = \'public\';";
    std::wstring query3 = L"CREATE INDEX  \"abcattbl_tnam_ownr\" ON \"abcattbl\"(\"abt_tnam\" ASC, \"abt_ownr\" ASC);";
    std::wstring query2 = L"SELECT 1 FROM pg_class c, pg_namespace n WHERE n.oid = c.relnamespace AND c.relname = \'abcatcol_tnam_ownr_cnam\' AND n.nspname = \'public\'";
    std::wstring query4 = L"CREATE INDEX \"abcatcol_tnam_ownr_cnam\" ON \"abcatcol\"(\"abc_tnam\" ASC, \"abc_ownr\" ASC, \"abc_cnam\" ASC);";
    res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query1.c_str() ).c_str() );
    if( PQresultStatus( res ) != PGRES_TUPLES_OK )
    {
        result = 1;
        std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
        errorMsg.push_back( err );
        PQclear( res );
    }
    if( !result )
    {
        if( PQntuples( res ) == 0 )
        {
            res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query3.c_str() ).c_str() );
            if( PQresultStatus( res ) != PGRES_COMMAND_OK )
            {
                result = 1;
                std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                errorMsg.push_back( err );
                PQclear( res );
            }
            if( !result )
            {
                res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query2.c_str() ).c_str() );
                if( PQresultStatus( res ) != PGRES_TUPLES_OK )
                {
                    result = 1;
                    std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                    errorMsg.push_back( err );
                    PQclear( res );
                }
                if( !result )
                {
                    if( PQntuples( res ) == 0 )
                    {
                        res = PQexec( m_db, m_pimpl->m_myconv.to_bytes( query4.c_str() ).c_str() );
                        if( PQresultStatus( res ) != PGRES_COMMAND_OK )
                        {
                            result = 1;
                            std::wstring err = m_pimpl->m_myconv.from_bytes( PQerrorMessage( m_db ) );
                            errorMsg.push_back( err );
                        }
                        PQclear( res );
                    }
                }
            }
        }
    }
    return result;
}
