#include "../../include/storage/table.h"
#include <sstream>
#include <cstdlib>
#include <climits>

// ---------------- SAFE CONVERSION ----------------
bool safe_stol(const string &s, long &out)
{
    try {
        size_t idx;
        out = stol(s, &idx);
        return idx == s.size();
    } catch (...) {
        return false;
    }
}

// ---------------- CONSTRUCTOR ----------------
Table::Table(string name, vector<ColumnDef> schema)
{
    this->name = name;
    this->schema = schema;

    system("mkdir -p data/tables");

    string file = "data/tables/" + name + ".data";
    data_file.open(file, ios::app);

    for (int i = 0; i < schema.size(); i++)
    {
        if (schema[i].name == "EXPIRES_AT")
        {
            expiry_index = i;
            break;
        }
    }
}

// ---------------- INSERT ----------------
bool Table::insert_row(vector<string> values, string &error)
{
    lock_guard<mutex> lock(mtx);

    if (values.size() != schema.size())
    {
        error = "column count mismatch";
        return false;
    }

    long exp = LONG_MAX;

    if (expiry_index != -1)
    {
        if (!safe_stol(values[expiry_index], exp))
        {
            error = "invalid expiry time";
            return false;
        }
    }

    Row r;
    r.values = values;
    r.expiry = exp;

    rows.push_back(r);

    if (!values.empty())
        primary_index[values[0]] = &rows.back();

    append_row_to_disk(r);

    return true;
}

// ---------------- DISK WRITE ----------------
void Table::append_row_to_disk(const Row &r)
{
    if (!data_file.is_open())
        return;

    for (int i = 0; i < r.values.size(); i++)
    {
        data_file << r.values[i];
        if (i != r.values.size() - 1)
            data_file << "|";
    }
    data_file << "\n";
}

void Table::flush_data()
{
    if (data_file.is_open())
        data_file.flush();
}

// ---------------- LOAD ----------------
void Table::load_data()
{
    string file = "data/tables/" + name + ".data";
    ifstream in(file);

    if (!in.is_open())
        return;

    rows.clear();
    primary_index.clear();

    string line;

    while (getline(in, line))
    {
        if (line.empty())
            continue;

        vector<string> parts;
        stringstream ss(line);
        string val;

        while (getline(ss, val, '|'))
            parts.push_back(val);

        if (parts.size() != schema.size())
            continue;

        long exp = LONG_MAX;

        if (expiry_index != -1)
        {
            if (!safe_stol(parts[expiry_index], exp))
                continue;
        }

        Row r;
        r.values = parts;
        r.expiry = exp;

        rows.push_back(r);

        if (!r.values.empty())
            primary_index[r.values[0]] = &rows.back();
    }

    in.close();
}

// ---------------- SELECT ----------------
vector<Row> Table::select_all()
{
    lock_guard<mutex> lock(mtx);

    vector<Row> valid;
    time_t now = time(nullptr);

    for (auto &r : rows)
    {
        if (r.expiry >= now)
            valid.push_back(r);
    }

    return valid;
}

// ---------------- COLUMN INDEX ----------------
int Table::get_column_index(string column_name)
{
    for (int i = 0; i < schema.size(); i++)
        if (schema[i].name == column_name)
            return i;
    return -1;
}// 

// ---------------- PRIMARY KEY ----------------
Row* Table::find_by_primary_key(string key)
{
    lock_guard<mutex> lock(mtx);

    if (primary_index.find(key) != primary_index.end())
    {
        Row* r = primary_index[key];
        if (r && r->expiry >= time(nullptr))
            return r;
    }

    return nullptr;
}