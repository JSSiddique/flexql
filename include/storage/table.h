#ifndef TABLE_H
#define TABLE_H

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <ctime>
#include <fstream>
#include "../query/query.h"

using namespace std;

struct Row
{
    vector<string> values;
    time_t expiry;
};

class Table
{
public:
    string name;
    vector<ColumnDef> schema;
    vector<Row> rows;

    unordered_map<string, Row*> primary_index;

    mutex mtx;

    ofstream data_file;

    int expiry_index = -1;

    Table(string name, vector<ColumnDef> schema);

    bool insert_row(vector<string> values, string &error);

    vector<Row> select_all();

    int get_column_index(string column_name);

    Row* find_by_primary_key(string key);

    void append_row_to_disk(const Row &r);
    void load_data();

    void flush_data();
};

#endif