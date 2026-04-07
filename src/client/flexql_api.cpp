#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <vector>
#include "../../include/common/flexql.h"

using namespace std;

#define BUFFER_SIZE 4096

struct FlexQL
{
    int socket_fd;
};

int flexql_open(const char *host, int port, FlexQL **db)
{
    *db = new FlexQL();

    (*db)->socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if ((*db)->socket_fd < 0)
    {
        delete *db;
        *db = nullptr;
        return FLEXQL_ERROR;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0)
    {
        close((*db)->socket_fd);
        delete *db;
        *db = nullptr;
        return FLEXQL_ERROR;
    }

    if (connect((*db)->socket_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close((*db)->socket_fd);
        delete *db;
        *db = nullptr;
        return FLEXQL_ERROR;
    }

    return FLEXQL_OK;
}

int flexql_close(FlexQL *db)
{
    close(db->socket_fd);
    delete db;
    return FLEXQL_OK;
}

int flexql_exec(
    FlexQL *db,
    const char *sql,
    int (*callback)(void *, int, char **, char **),
    void *arg,
    char **errmsg
)
{
    send(db->socket_fd, sql, strlen(sql), 0);

    char buffer[BUFFER_SIZE];
    string full_response = "";

    while (true)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(db->socket_fd, buffer, BUFFER_SIZE, 0);

        if (bytes <= 0)
            break;

        full_response.append(buffer, bytes);

        if (full_response.find("\nEND\n") != string::npos ||
            full_response.find("END\n") != string::npos)
        {
            break;
        }
    }

    stringstream ss(full_response);
    string line;

    while (getline(ss, line))
    {
        if (line == "END")
            break;

        if (line == "OK")
        {
            cout << "OK" << endl;
            continue;
        }

        if (line.find("ERROR|") == 0)
        {
            if (errmsg)
                *errmsg = strdup(line.substr(6).c_str());

            cerr << line.substr(6) << endl;
            return FLEXQL_ERROR;
        }

        if (line.find("ROW|") == 0)
        {
            line = line.substr(4);

            vector<string> values;
            string val;
            stringstream row_stream(line);

            while (getline(row_stream, val, '|'))
                values.push_back(val);

            int cols = values.size();

            char **c_values = new char*[cols];

            for (int i = 0; i < cols; i++)
                c_values[i] = strdup(values[i].c_str());

            if (callback)
                callback(arg, cols, c_values, NULL);

            for (int i = 0; i < cols; i++)
                free(c_values[i]);

            delete[] c_values;
        }
    }

    return FLEXQL_OK;
}

void flexql_free(void *ptr)
{
    free(ptr);
}