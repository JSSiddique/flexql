#include <iostream>
#include <string>
#include "../../include/common/flexql.h"

using namespace std;

bool is_buffer_empty(const string &buf)
{
    return buf.find_first_not_of(" \n\r\t") == string::npos;
}

int print_callback(void *arg, int cols, char **values, char **columns)
{
    for (int i = 0; i < cols; i++)
        cout << values[i] << " ";

    cout << endl;
    return 0;
}

int main()
{
    FlexQL *db;

    if (flexql_open("127.0.0.1", 9000, &db) != FLEXQL_OK)
    {
        cerr << "Failed to connect to server\n";
        return 1;
    }

    string line;
    string buffer = "";

    while (true)
    {
        if (is_buffer_empty(buffer))
            cout << "flexql> ";
        else
            cout << "   -> ";

        getline(cin, line);

        if (line == "exit")
            break;

        buffer += line + " ";

        size_t pos;

        while ((pos = buffer.find(';')) != string::npos)
        {
            string query = buffer.substr(0, pos);

            query.erase(0, query.find_first_not_of(" \n\r\t"));
            if (!query.empty())
                query.erase(query.find_last_not_of(" \n\r\t") + 1);

            if (!query.empty())
            {
                flexql_exec(db, (query+';').c_str(), print_callback, NULL, NULL);
            }

            buffer = buffer.substr(pos + 1);

            buffer.erase(0, buffer.find_first_not_of(" \n\r\t"));
            if (buffer.empty())
                buffer = "";
        }
    }

    flexql_close(db);
    return 0;
}