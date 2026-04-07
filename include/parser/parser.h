#ifndef PARSER_H
#define PARSER_H

#include "../query/query.h"

Query parse_query(const std::string &sql);

#endif