#ifndef DBMANAGER_MYSQL
#define DBMANAGER_MYSQL

#ifdef WIN32
class __declspec(dllexport) MySQLDatabase : public Database
#else
class MySQLDatabase : public Database
#endif
{
public:
    MySQLDatabase();
    virtual ~MySQLDatabase();
    virtual int ServerConnect(const std::wstring &selectedDSN, std::vector<std::wstring> &dbList, std::vector<std::wstring> &errorMsg);
    virtual int Connect(const std::wstring &selectedDSN, std::vector<std::wstring> &errorMsg);
    virtual int CreateDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg);
    virtual int DropDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg);
    int mySQLConnect();
    virtual int Disconnect(std::vector<std::wstring> &errorMsg);
    virtual int CreateIndex(const std::wstring &command, const std::wstring &index_name, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
    virtual int GetTableProperties(DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    virtual int GetFieldProperties(const char *tableName, const char *schemaName, const char *ownerName, const char *fieldName, std::vector<std::wstring> &errorMsg);
    virtual int SetTableProperties(const DatabaseTable *table, const TableProperties &properties, bool isLog, std::wstring &command, std::vector<std::wstring> &errorMsg);
    virtual int SetFieldProperties(const std::wstring &command, std::vector<std::wstring> &errorMsg);
    virtual int ApplyForeignKey(const std::wstring &command, const std::wstring &keyName, DatabaseTable &tableName, std::vector<std::wstring> &errorMsg);
    virtual int DeleteTable(const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
protected:
    struct MySQLImpl;
    MySQLImpl *m_pimpl;
    virtual int GetTableListFromDb(std::vector<std::wstring> &errorMsg);
    virtual bool IsTablePropertiesExist(const DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    virtual bool IsIndexExists(const std::wstring &indexName, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
    bool IsSystemIndexExists(const std::wstring &indexName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
    int TokenizeConnectionString(const std::wstring &connectStr, std::vector<std::wstring> &errorMsg);
    virtual int GetTableId(DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    virtual int GetServerVersion(std::vector<std::wstring> &errorMsg);
private:
    MYSQL *m_db;
    int m_port, m_flags;
};

struct MySQLDatabase::MySQLImpl
{
    std::wstring_convert<std::codecvt_utf8<wchar_t> > m_myconv;
    std::wstring m_host, m_user, m_password, m_dbName, m_socket, m_catalog;
};
#endif