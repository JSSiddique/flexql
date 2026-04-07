#ifndef DATABASE_H
#define DATABASE_H

#include <unordered_map>
#include <mutex>
#include "table.h"

using namespace std;

class Database
{
public:
    unordered_map<string, Table*> tables;
    mutex db_mtx;

    void create_table(string name, vector<ColumnDef> schema);

    Table* get_table(string name);

    void load_all_tables();
};

#endif