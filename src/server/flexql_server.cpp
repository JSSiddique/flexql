#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>

#include "../../include/parser/parser.h"
#include "../../include/storage/database.h"
#include "../../include/cache/cache.h"

using namespace std;

#define PORT 9000
#define BUFFER_SIZE 65536

Database db;
QueryCache cache(100);

string trim(const string &s)
{
    size_t start = s.find_first_not_of(" \n\r\t");
    size_t end = s.find_last_not_of(" \n\r\t");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

void send_error(int sock, const string &msg)
{
    string err = "ERROR|" + msg + "\n";
    send(sock, err.c_str(), err.size(), 0);
    send(sock, "END\n", 4, 0);
}

void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];

    while (true)
    {
        string sql = "";

        while (true)
        {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);

            if (bytes <= 0)
            {
                close(client_socket);
                return;
            }

            sql.append(buffer, bytes);

            if (sql.find(';') != string::npos)
                break;
        }

        sql = trim(sql);

        while (!sql.empty() && (sql.back() == ';' || isspace(sql.back())))
            sql.pop_back();

        if (sql.empty())
            continue;

        Query q = parse_query(sql);

        // ================= CREATE =================
        if (q.type == QUERY_CREATE)
        {
            try
            {
                db.create_table(q.table_name, q.column_defs);
                cache.clear();
                send(client_socket, "OK\nEND\n", 7, 0);
            }
            catch (const exception &e)
            {
                send_error(client_socket, e.what());
            }
        }

        // ================= INSERT =================
        else if (q.type == QUERY_INSERT)
        {
            Table *t = db.get_table(q.table_name);

            if (!t)
            {
                send_error(client_socket, "table not found");
                continue;
            }

            string error;
            bool ok = true;

            for (const auto &row : q.values)
            {
                if (!t->insert_row(row, error))
                {
                    ok = false;
                    break;
                }
            }

            if (!ok)
            {
                send_error(client_socket, error);
                continue;
            }

            t->flush_data();
            cache.clear();

            send(client_socket, "OK\nEND\n", 7, 0);
        }

        // ================= SELECT =================
        else if (q.type == QUERY_SELECT)
        {
            string cached;
            if (cache.get(sql, cached))
            {
                send(client_socket, cached.c_str(), cached.size(), 0);
                continue;
            }

            Table *t1 = db.get_table(q.table_name);
            if (!t1)
            {
                send_error(client_socket, "table not found");
                continue;
            }

            Table *t2 = nullptr;
            if (q.has_join)
            {
                t2 = db.get_table(q.join_table);
                if (!t2)
                {
                    send_error(client_socket, "join table not found");
                    continue;
                }
            }

            vector<pair<int,int>> cols;
            bool error_flag = false;

            // ================= COLUMN RESOLUTION =================
            if (q.columns.size() == 1 && q.columns[0] == "*")
            {
                for (int i = 0; i < t1->schema.size(); i++)
                    cols.push_back({1, i});

                if (t2)
                    for (int i = 0; i < t2->schema.size(); i++)
                        cols.push_back({2, i});
            }
            else
            {
                for (auto &col : q.columns)
                {
                    string col_name = col;
                    string prefix = "";

                    if (col.find('.') != string::npos)
                    {
                        prefix = col.substr(0, col.find('.'));
                        col_name = col.substr(col.find('.') + 1);
                    }

                    int idx = -1;

                    if (prefix == "" || prefix == q.table_name)
                    {
                        idx = t1->get_column_index(col_name);
                        if (idx != -1)
                        {
                            cols.push_back({1, idx});
                            continue;
                        }
                    }

                    if (t2 && (prefix == "" || prefix == q.join_table))
                    {
                        idx = t2->get_column_index(col_name);
                        if (idx != -1)
                        {
                            cols.push_back({2, idx});
                            continue;
                        }
                    }

                    send_error(client_socket, "column not found");
                    error_flag = true;
                    break;
                }
            }

            if (error_flag) continue;

            string result = "";

            // ================= NO JOIN =================
            if (!q.has_join)
            {
                for (auto &r : t1->select_all())
                {
                    bool pass = true;

                    if (!q.where_column.empty())
                    {
                        int idx = t1->get_column_index(q.where_column);
                        if (idx == -1)
                        {
                            send_error(client_socket, "column not found");
                            error_flag = true;
                            break;
                        }

                        string val = r.values[idx];
                        string cond = q.where_value;

                        if (q.where_op == "=") pass = (val == cond);
                        else if (q.where_op == ">") pass = (stod(val) > stod(cond));
                        else if (q.where_op == "<") pass = (stod(val) < stod(cond));
                        else if (q.where_op == ">=") pass = (stod(val) >= stod(cond));
                        else if (q.where_op == "<=") pass = (stod(val) <= stod(cond));

                        if (!pass) continue;
                    }

                    result += "ROW|";
                    for (auto &c : cols)
                        result += r.values[c.second] + "|";
                    if (!cols.empty()) result.pop_back();
                    result += "\n";
                }

                if (error_flag) continue;
            }

            // ================= JOIN =================
            else
            {
                int t1_idx = t1->get_column_index(q.join_left_column);
                int t2_idx = t2->get_column_index(q.join_right_column);

                if (t1_idx == -1 || t2_idx == -1)
                {
                    send_error(client_socket, "invalid join column");
                    continue;
                }

                auto rows1 = t1->select_all();
                auto rows2 = t2->select_all();

                for (auto &r1 : rows1)
                {
                    for (auto &r2 : rows2)
                    {
                        if (r1.values[t1_idx] != r2.values[t2_idx])
                            continue;

                        bool pass = true;

                        if (!q.where_column.empty())
                        {
                            int idx = t1->get_column_index(q.where_column);
                            bool from_t2 = false;

                            if (idx == -1)
                            {
                                idx = t2->get_column_index(q.where_column);
                                from_t2 = true;
                            }

                            if (idx == -1)
                            {
                                send_error(client_socket, "column not found");
                                error_flag = true;
                                break;
                            }

                            string val = from_t2 ? r2.values[idx] : r1.values[idx];
                            string cond = q.where_value;

                            if (q.where_op == "=") pass = (val == cond);
                            else if (q.where_op == ">") pass = (stod(val) > stod(cond));
                            else if (q.where_op == "<") pass = (stod(val) < stod(cond));
                            else if (q.where_op == ">=") pass = (stod(val) >= stod(cond));
                            else if (q.where_op == "<=") pass = (stod(val) <= stod(cond));

                            if (!pass) continue;
                        }

                        result += "ROW|";
                        for (auto &c : cols)
                        {
                            if (c.first == 1)
                                result += r1.values[c.second];
                            else
                                result += r2.values[c.second];
                            result += "|";
                        }

                        if (!cols.empty()) result.pop_back();
                        result += "\n";
                    }

                    if (error_flag) break;
                }
            }

            result += "END\n";

            cache.put(sql, result);
            send(client_socket, result.c_str(), result.size(), 0);
        }

        else
        {
            send_error(client_socket, "invalid query");
        }
    }
}

int main(int argc, char** argv)
{
    if (argc > 1 && string(argv[1]) == "--reset")
    {
        system("rm -rf data/");
    }

    db.load_all_tables();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    cout << "Server running...\n";

    while (true)
    {
        socklen_t len = sizeof(addr);
        int client = accept(server_fd, (sockaddr*)&addr, &len);

        thread(handle_client, client).detach();
    }
}