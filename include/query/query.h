#ifndef QUERY_H
#define QUERY_H

#include <string>
#include <vector>

using namespace std;

enum QueryType
{
    QUERY_CREATE,
    QUERY_INSERT,
    QUERY_SELECT,
    QUERY_UNKNOWN
};

struct ColumnDef
{
    string name;
    string type;
};

struct Query
{
    QueryType type = QUERY_UNKNOWN;

    string table_name;

    vector<string> columns;
    vector<vector<string>> values;

    vector<ColumnDef> column_defs;

    string where_column;
    string where_value;
    string where_op;

    string join_table;
    string join_left_column;
    string join_right_column;
    bool has_join = false;
};

#endif