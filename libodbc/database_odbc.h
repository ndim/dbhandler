#ifndef DBMANAGER_ODBC
#define DBMANAGER_ODBC

struct SQLTablesDataBinding
{
    SQLSMALLINT TargetType;
    SQLPOINTER TargetValuePtr;
    SQLINTEGER BufferLength;
    SQLLEN StrLen_or_Ind;
};

#ifdef UNIXODBC
#define TRUE 1
#define FALSE 0
#endif

#ifdef WIN32
class __declspec(dllexport) ODBCDatabase : public Database
#else
class ODBCDatabase : public Database
#endif
{
public:
    ODBCDatabase();
    virtual ~ODBCDatabase();
    virtual int Connect(const std::wstring &selectedDSN, std::vector<std::wstring> &dbList, std::vector<std::wstring> &errorMsg);
    virtual int CreateDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg);
    virtual int DropDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg);
    virtual int Disconnect(std::vector<std::wstring> &errorMsg);
    void SetWindowHandle(SQLHWND handle);
    void AskForConnectionParameter(bool ask);
    bool GetDriverList(std::map<std::wstring, std::vector<std::wstring> > &driversDSN, std::vector<std::wstring> &errMsg);
    bool AddDsn(SQLHWND hwnd, const std::wstring &driver, std::vector<std::wstring> &errorMsg);
    bool EditDsn(SQLHWND hwnd, const std::wstring &driver, const std::wstring &dsn, std::vector<std::wstring> &errorMsg);
    bool RemoveDsn(const std::wstring &driver, const std::wstring &dsn, std::vector<std::wstring> &errorMsg);
    bool GetDSNList(std::vector<std::wstring> &dsn, std::vector<std::wstring> &errorMsg);
    virtual int CreateIndex(const std::wstring &command, const std::wstring &index_name, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
    virtual int GetTableProperties(DatabaseTable *table, std::vector<std::wstring> &errorMsg);
	virtual int GetFieldProperties(const char *UNUSED(tableName), const char *UNUSED(schemaName), const char *UNUSED(ownerName), const char *UNUSED(fieldName), Field *UNUSED(table), std::vector<std::wstring> &UNUSED(errorMsg)) { return 0; }
    virtual int SetTableProperties(const DatabaseTable *table, const TableProperties &properties, bool isLog, std::wstring &command, std::vector<std::wstring> &errorMsg);
    virtual int SetFieldProperties(const std::wstring &tableName, const std::wstring &ownerName, const std::wstring &fieldName, const Field *field, bool isLogOnly, std::vector<std::wstring> &errorMsg);
    virtual int ApplyForeignKey(std::wstring &command, const std::wstring &keyName, DatabaseTable &tableName, const std::vector<std::wstring> &foreignKeyFields, const std::wstring &refTableName, const std::vector<std::wstring> &refKeyFields, int deleteProp, int updateProp, bool logOnly, std::vector<FKField *> &newFK, bool isNew, int UNUSED(match), std::vector<std::wstring> &errorMsg);
    virtual int DeleteTable(const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
    int CreateSystemObjectsAndGetDatabaseInfo(std::vector<std::wstring> &errorMsg);
    virtual int NewTableCreation(std::vector<std::wstring> &errorMsg);
    void GetConnectedUser(const std::wstring &dsn, std::wstring &connectedUser);
    void GetConnectionPassword(const std::wstring &dsn, std::wstring &connectionPassword);
protected:
    struct ODBCImpl;
    ODBCImpl *odbc_pimpl;
    int GetDriverForDSN(SQLWCHAR *dsn, SQLWCHAR *driver, std::vector<std::wstring> &errorMsg);
    int GetSQLStringSize(SQLWCHAR *str);
    void str_to_uc_cpy(std::wstring &dest, const SQLWCHAR *src);
    void uc_to_str_cpy(SQLWCHAR *dest, const std::wstring &src);
    void copy_uc_to_uc(SQLWCHAR *dest, SQLWCHAR *src);
    bool equal(SQLWCHAR *dest, SQLWCHAR *src);
    int GetErrorMessage(std::vector<std::wstring> &errorMsg, int type, SQLHSTMT stmt = 0);
    int GetDSNErrorMessage(std::vector<std::wstring> &errorMsg);
    virtual int GetTableListFromDb(std::vector<std::wstring> &errorMsg);
    virtual bool IsTablePropertiesExist(const DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    virtual bool IsIndexExists(const std::wstring &indexName, const std::wstring &schema_name, const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
//    int GetTableId(DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    int GetTableId(const std::wstring &catalog, const std::wstring &schemaName, const std::wstring &tableName, int &tableId, std::vector<std::wstring> &errorMsg);
    int GetTableOwner(const std::wstring &catalog, const std::wstring &schemaName, const std::wstring &tableName, std::wstring &owner, std::vector<std::wstring> &errorMsg);
    void SetFullType(Field *field);
    virtual int GetServerVersion(std::vector<std::wstring> &errorMsg);
    int CreateIndexesOnPostgreConnection(std::vector<std::wstring> &errorMsg);
    int GetFieldProperties(const SQLWCHAR *tableName, const SQLWCHAR *schemaName, const SQLWCHAR *ownerName, const SQLWCHAR *fieldName, Field *field, std::vector<std::wstring> &errorMsg);
    virtual int ServerConnect(std::vector<std::wstring> &dbList, std::vector<std::wstring> &errorMsg);
    int DropForeignKey(std::wstring &command, const std::wstring &keyName, DatabaseTable &tableName, const std::vector<std::wstring> &foreignKeyFields, const std::wstring &refTableName, const std::vector<std::wstring> &refKeyFields, int deleteProp, int updateProp, bool logOnly, std::vector<FKField *> &newFK, std::vector<std::wstring> &errorMsg);
    virtual int AddDropTable(const std::wstring &catalogName, const std::wstring &schemaName, const std::wstring &tableName, const std::wstring &tableOwner, int tableId, bool tableAdded, std::vector<std::wstring> &errorMsg);
private:
    SQLHENV m_env;
    SQLHDBC m_hdbc;
    SQLHSTMT m_hstmt;
    SQLHWND m_handle;
    bool m_ask;
    SQLUSMALLINT m_statementsNumber;
    bool m_oneStatement, m_isConnected;
    SQLWCHAR *m_connectString;
};

struct ODBCDatabase::ODBCImpl
{
    std::wstring m_currentTableOwner;
};

#endif
