#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <list>
#include <string>
#include <mutex>

using namespace std;

class QueryCache
{
private:
    int capacity;
    list<pair<string, string>> lru;
    unordered_map<string, list<pair<string, string>>::iterator> map;
    mutex mtx;

public:
    QueryCache(int cap = 100) : capacity(cap) {}

    bool get(string key, string &value)
    {
        lock_guard<mutex> lock(mtx);

        if (map.find(key) == map.end())
            return false;

        auto it = map[key];
        value = it->second;

        lru.erase(it);
        lru.push_front({key, value});
        map[key] = lru.begin();

        return true;
    }

    void put(string key, string value)
    {
        lock_guard<mutex> lock(mtx);

        if (map.find(key) != map.end())
        {
            lru.erase(map[key]);
        }
        else if (lru.size() == capacity)
        {
            auto last = lru.back();
            map.erase(last.first);
            lru.pop_back();
        }

        lru.push_front({key, value});
        map[key] = lru.begin();
    }

    void clear()
    {
        lock_guard<mutex> lock(mtx);
        lru.clear();
        map.clear();
    }
};

#endif