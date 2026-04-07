#include "../../include/storage/database.h"
#include <fstream>
#include <filesystem>

using namespace std;

void Database::create_table(string name, vector<ColumnDef> schema)
{
    lock_guard<mutex> lock(db_mtx);

    if (tables.find(name) != tables.end())
        throw runtime_error("table already exists");

    string file = "data/schema/" + name + ".meta";
    if (filesystem::exists(file))
        throw runtime_error("table already exists");

    system("mkdir -p data/schema");

    tables[name] = new Table(name, schema);

    ofstream out(file);
    for (auto &col : schema)
        out << col.name << " " << col.type << "\n";

    out.close();

    tables[name]->load_data();
}

Table* Database::get_table(string name)
{
    lock_guard<mutex> lock(db_mtx);

    if (tables.find(name) != tables.end())
        return tables[name];

    return nullptr;
}

void Database::load_all_tables()
{
    system("mkdir -p data/schema");

    for (auto &entry : filesystem::directory_iterator("data/schema"))
    {
        string path = entry.path();
        string filename = entry.path().filename().string();

        string table_name = filename.substr(0, filename.find(".meta"));

        ifstream in(path);
        if (!in.is_open())
            continue;

        vector<ColumnDef> schema;
        string name, type;

        while (in >> name >> type)
            schema.push_back({name, type});

        in.close();

        tables[table_name] = new Table(table_name, schema);
        tables[table_name]->load_data();
    }
}