#include <sstream>
#include <algorithm>
#include "../../include/parser/parser.h"

using namespace std;

static string trim(const string &s)
{
    size_t start = s.find_first_not_of(" \n\r\t");
    size_t end = s.find_last_not_of(" \n\r\t");

    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

static string to_lower(string s)
{
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

Query parse_query(const string &sql)
{
    Query q;

    string cleaned = trim(sql);

    while (!cleaned.empty() && (cleaned.back() == ';' || isspace(cleaned.back())))
        cleaned.pop_back();

    string lower = to_lower(cleaned);

    stringstream ss(cleaned);
    string word;

    // ================= CREATE =================
    if (lower.find("create table") == 0)
    {
        q.type = QUERY_CREATE;

        ss >> word >> word >> q.table_name;

        size_t pos = q.table_name.find('(');
        if (pos != string::npos)
            q.table_name = q.table_name.substr(0, pos);

        q.table_name = trim(q.table_name);

        while (!q.table_name.empty() && !isalnum(q.table_name.back()))
            q.table_name.pop_back();

        size_t start = cleaned.find('(');
        size_t end = cleaned.find_last_of(')');

        if (start != string::npos && end != string::npos)
        {
            string cols = cleaned.substr(start + 1, end - start - 1);
            stringstream col_stream(cols);
            string col;

            while (getline(col_stream, col, ','))
            {
                col = trim(col);
                stringstream cs(col);

                ColumnDef cd;
                cs >> cd.name >> cd.type;

                q.column_defs.push_back(cd);
            }
        }
    }

    // ================= INSERT =================
    else if (lower.find("insert into") == 0)
    {
        q.type = QUERY_INSERT;

        size_t into_pos = lower.find("into") + 5;
        size_t values_pos = lower.find("values", into_pos);

        if (values_pos == string::npos)
        {
            q.type = QUERY_UNKNOWN;
            return q;
        }

        string table_part = cleaned.substr(into_pos, values_pos - into_pos);
        q.table_name = trim(table_part);

        size_t start = values_pos + 6;
        string vals_str = cleaned.substr(start);

        size_t i = 0;

        while (i < vals_str.size())
        {
            while (i < vals_str.size() && vals_str[i] != '(') i++;
            if (i >= vals_str.size()) break;

            size_t paren_start = i;
            int depth = 1;
            i++;

            while (i < vals_str.size() && depth > 0)
            {
                if (vals_str[i] == '(') depth++;
                else if (vals_str[i] == ')') depth--;
                i++;
            }

            if (depth != 0) break;

            string tuple = vals_str.substr(paren_start + 1, i - paren_start - 2);

            stringstream vs(tuple);
            string val;
            vector<string> row_values;

            while (getline(vs, val, ','))
            {
                val = trim(val);
                if (!val.empty() && val.front() == '\'' && val.back() == '\'')
                    val = val.substr(1, val.size() - 2);
                row_values.push_back(val);
            }

            if (!row_values.empty())
                q.values.push_back(row_values);
        }
    }

    // ================= SELECT =================
    else if (lower.find("select") == 0)
    {
        q.type = QUERY_SELECT;

        size_t from_pos = lower.find("from");

        string cols = cleaned.substr(6, from_pos - 6);
        stringstream col_stream(cols);
        string col;

        while (getline(col_stream, col, ','))
            q.columns.push_back(trim(col));

        string remaining = cleaned.substr(from_pos + 4);
        remaining = trim(remaining);

        stringstream rs(remaining);
        rs >> q.table_name;
        q.table_name = trim(q.table_name);

        // JOIN
        size_t join_pos = lower.find("inner join");
        if (join_pos != string::npos)
        {
            q.has_join = true;

            size_t on_pos = lower.find("on");

            string join_part = cleaned.substr(join_pos + 10, on_pos - (join_pos + 10));
            q.join_table = trim(join_part);

            string condition = cleaned.substr(on_pos + 2);

            size_t where_pos = to_lower(condition).find("where");
            if (where_pos != string::npos)
                condition = condition.substr(0, where_pos);

            size_t eq_pos = condition.find('=');

            string left = trim(condition.substr(0, eq_pos));
            string right = trim(condition.substr(eq_pos + 1));

            if (left.find('.') != string::npos)
                left = left.substr(left.find('.') + 1);

            if (right.find('.') != string::npos)
                right = right.substr(right.find('.') + 1);

            q.join_left_column = left;
            q.join_right_column = right;
        }

        // WHERE
        size_t where_pos = lower.find("where");
        if (where_pos != string::npos)
        {
            string condition = cleaned.substr(where_pos + 5);
            condition = trim(condition);

            size_t pos;

            if ((pos = condition.find(">=")) != string::npos)
                q.where_op = ">=";
            else if ((pos = condition.find("<=")) != string::npos)
                q.where_op = "<=";
            else if ((pos = condition.find("=")) != string::npos)
                q.where_op = "=";
            else if ((pos = condition.find(">")) != string::npos)
                q.where_op = ">";
            else if ((pos = condition.find("<")) != string::npos)
                q.where_op = "<";

            if (!q.where_op.empty())
            {
                q.where_column = trim(condition.substr(0, pos));
                q.where_value = trim(condition.substr(pos + q.where_op.length()));

                if (q.where_column.find('.') != string::npos)
                    q.where_column = q.where_column.substr(q.where_column.find('.') + 1);
            }
        }
    }
    else
    {
        q.type = QUERY_UNKNOWN;
    }

    return q;
}