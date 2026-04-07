#ifndef HASH_INDEX_H
#define HASH_INDEX_H

#include <unordered_map>
#include <string>

using namespace std;

class HashIndex
{
public:
    unordered_map<string, int> index;

    void insert(string key, int row_id);

    int find(string key);
};

#endif