#include "tt.h"
#include "move.h"
#include <cstring>
#include <iostream>
#include <cstdlib>

TranspositionTable TT;

Move TTEntry::move() const
{
    return Move(move16);
}

TTEntry* TranspositionTable::probe(const Key key, bool& found) const
{
    TTEntry* const tte = first_entry(key);
    const uint16_t key16 = uint16_t(key >> 48);

    for (int i = 0; i < ClusterSize; ++i)
        if (!tte[i].key16 || tte[i].key16 == key16)
        {
            tte[i].genBound8 = uint8_t(generation8 | (tte[i].genBound8 & 0x3));

            return found = (bool)tte[i].key16, &tte[i];
        }

    TTEntry* replace = tte;
    for (int i = 1; i < ClusterSize; ++i)
        if (replace->depth() - ((259 + generation8 - replace->genBound8) & 0xFC) * 2
    > tte[i].depth() - ((259 + generation8 - tte[i].genBound8) & 0xFC) * 2)
            replace = &tte[i];

    return found = false, replace;
}

int TranspositionTable::hashfull() const
{
    int cnt = 0;
    for (int i = 0; i < 1000; ++i)
        for (int j = 0; j < ClusterSize; ++j)
            cnt += (table[i].entry[j].genBound8 & 0xFC) == generation8;

    return cnt / ClusterSize;
}

void TranspositionTable::resize(size_t mbSize)
{
    aligned_ttmem_free(mem);

    clusterCount = mbSize * 1024 * 1024 / sizeof(Cluster);

    table = static_cast<Cluster*>(aligned_ttmem_alloc(clusterCount * sizeof(Cluster), mem));
    if (!table)
    {
        std::cerr << "Failed to allocate " << mbSize << "MB for transposition table." << std::endl;
        exit(EXIT_FAILURE);
    }

    clear();
}

void TranspositionTable::clear()
{
    std::memset(table, 0, clusterCount * sizeof(Cluster));
}

void TTEntry::save(Key k, Value v, bool ttPv, Bound b, Depth d, Move m, Value ev)
{
    if (m != MOVE_NONE || (k >> 48) != key16)
        move16 = uint16_t(m);

    if ((k >> 48) != key16 || d + 2 > depth8 - 4)
    {
        key16 = uint16_t(k >> 48);
        value16 = int16_t(v);
        eval16 = int16_t(ev);
        genBound8 = uint8_t(TT.generation8 | uint8_t(ttPv) << 2 | b);
        depth8 = int8_t(d);
    }
}

void* TranspositionTable::aligned_ttmem_alloc(size_t allocSize, void*& mem)
{
    constexpr size_t alignment = 2 * 1024 * 1024;
    size_t size = allocSize + alignment - 1;
    mem = std::malloc(size);
    void* ret = reinterpret_cast<void*>((uintptr_t(mem) + alignment - 1) & ~uintptr_t(alignment - 1));
    return ret;
}

void TranspositionTable::aligned_ttmem_free(void* mem)
{
    if (mem)
        std::free(mem);
}