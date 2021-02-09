#ifndef P4SFCSTATE_HH
#define P4SFCSTATE_HH

#include "p4sfcstate.pb.h"

#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#define WINDOW_SIZE 5

namespace P4SFCState
{
    using namespace std;

    // declare
    extern int cur_pos;
    class TableEntryImpl : public TableEntry
    {
    public:
        TableEntryImpl();

        void inc_slot();
        string unparse() const;

        void build_slots();

        uint64_t slots[WINDOW_SIZE];
        uint64_t slots_sum;

    };

    template <typename Key, typename Hash>
    class Table
    {
    public:
        Table() {}
        int size();

        void insert(const Key &, TableEntryImpl *);
        TableEntryImpl *lookup(const Key &);
        // void remove(const Key &);

        unordered_map<Key, TableEntryImpl *, Hash> _map;
    };

    void deleteTableEntries();
    void startServer(int click_instance_id = 1, string addr = "0.0.0.0:28282");
    void shutdownServer();

    // should not be exploded

    // inline definition
    template <typename Key, typename Hash>
    inline int Table<Key, Hash>::size()
    {
        return _map.size();
    }

    template <typename Key, typename Hash>
    inline void Table<Key, Hash>::insert(const Key &key, TableEntryImpl *entry)
    {
        _map[key] = entry;
    }

    template <typename Key, typename Hash>
    inline TableEntryImpl *Table<Key, Hash>::lookup(const Key &key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return 0;
        TableEntryImpl *e = it->second;
        e->inc_slot();
        return e;
    }

    inline void TableEntryImpl::inc_slot()
    {
        slots[cur_pos] += 1;
    }

} // namespace P4SFCState

#endif
