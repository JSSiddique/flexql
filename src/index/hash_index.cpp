#include "index/hash_index.h"

using namespace std;

void HashIndex::insert(string key, int row_id)
{
    index[key] = row_id;
}

int HashIndex::find(string key)
{
    if (index.find(key) != index.end())
        return index[key];

    return -1;
}