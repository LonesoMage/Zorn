#ifndef TT_H
#define TT_H

#include "types.h"

enum Bound : uint8_t
{
    BOUND_NONE,
    BOUND_UPPER,
    BOUND_LOWER,
    BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

struct TTEntry
{
    Move move() const;
    Value value() const { return Value(value16); }
    Value eval() const { return Value(eval16); }
    Depth depth() const { return Depth(depth8); }
    Bound bound() const { return Bound(genBound8 & 0x3); }

    void save(Key k, Value v, bool ttPv, Bound b, Depth d, Move m, Value ev);

private:
    friend class TranspositionTable;

    uint16_t key16;
    uint16_t move16;
    int16_t value16;
    int16_t eval16;
    uint8_t genBound8;
    int8_t depth8;
};

class TranspositionTable
{
public:
    TranspositionTable() : clusterCount(0), table(nullptr), mem(nullptr), generation8(8) {}
    ~TranspositionTable() { aligned_ttmem_free(mem); }

    void new_search() { generation8 += 8; }
    TTEntry* probe(const Key key, bool& found) const;
    int hashfull() const;
    void resize(size_t mbSize);
    void clear();

    TTEntry* first_entry(const Key key) const
    {
        return &table[(size_t)key & (clusterCount - 1)].entry[0];
    }

private:
    friend struct TTEntry;

    size_t clusterCount;

    static constexpr int ClusterSize = 3;

    struct Cluster
    {
        TTEntry entry[ClusterSize];
        int8_t padding[2];
    };

    Cluster* table;
    void* mem;
    uint8_t generation8;

    static_assert(sizeof(Cluster) == 32, "Cluster size incorrect");

    void* aligned_ttmem_alloc(size_t allocSize, void*& mem);
    void aligned_ttmem_free(void* mem);
};

extern TranspositionTable TT;

#endif