#ifndef DBMANAGER_SQLITE
#define DBMANAGER_SQLITE

#ifdef WIN32
class __declspec(dllexport) SQLiteDatabase : public Database
#else
class SQLiteDatabase : public Database
#endif
{
public:
    SQLiteDatabase();
    virtual ~SQLiteDatabase();
    virtual int ServerConnect(const std::wstring &selectedDSN, std::vector<std::wstring> &dbList, std::vector<std::wstring> &errorMsg);
    virtual int Connect(const std::wstring &selectedDSN, std::vector<std::wstring> &errorMsg);
    virtual int CreateDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg);
    virtual int DropDatabase(const std::wstring &name, std::vector<std::wstring> &errorMsg);
    virtual int Disconnect(std::vector<std::wstring> &errorMsg);
    virtual int CreateIndex(const std::wstring &command, const std::wstring &index_name, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
    virtual int GetTableProperties(DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    virtual int GetFieldProperties(const char *tableName, const char *schemaName, const char *ownerName, const char *fieldName, std::vector<std::wstring> &errorMsg);
    virtual int SetTableProperties(const DatabaseTable *table, const TableProperties &properties, bool isLog, std::wstring &command, std::vector<std::wstring> &errorMsg);
    virtual int SetFieldProperties(const std::wstring &command, std::vector<std::wstring> &errorMsg);
    virtual int ApplyForeignKey(const std::wstring &command, const std::wstring &keyName, DatabaseTable &tableName, std::vector<std::wstring> &errorMsg);
    virtual int DeleteTable(const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
protected:
    struct SQLiteImpl;
    SQLiteImpl *sqlite_pimpl;
    void GetErrorMessage(int code, std::wstring &errorMsg);
    void SetFullType(Field *field, const std::wstring &type);
    virtual int GetTableListFromDb(std::vector<std::wstring> &errorMsg);
    virtual bool IsTablePropertiesExist(const DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    virtual bool IsIndexExists(const std::wstring &indexName, const std::wstring &schemaName, const std::wstring &tableName, std::vector<std::wstring> &errorMsg);
    virtual int GetTableId(DatabaseTable *table, std::vector<std::wstring> &errorMsg);
    virtual int GetServerVersion(std::vector<std::wstring> &errorMsg);
private:
    sqlite3 *m_db;
};

struct SQLiteDatabase::SQLiteImpl
{
    std::wstring m_catalog;
    std::wstring_convert<std::codecvt_utf8<wchar_t> > m_myconv;
};

#endif