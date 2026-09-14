# tsdb — a from-scratch storage engine in C++

A small on-disk database storage engine built from scratch in C++, following roughly the order of CMU 15-445 (Database Systems). The end goal is a time-series-oriented storage engine; the current focus is the classic storage-engine fundamentals — binary I/O, fixed-size records, and indexing — before moving on to a B-tree.


## What's implemented

**Flat-file record storage**
- `Record { int64_t timestamp; double value; }` — fixed-size 16-byte binary records
- `Header { int64_t count; }` stored at byte 0 of the data file, updated on every append
- Sequential write/scan, O(1) random access by row index (bounds-checked), and append (updates the header count after the new record is durably written)
- All file opens are checked; failures throw `std::runtime_error`

**Sorted flat-file index**
- `IndexEntry { int64_t timestamp; int64_t offset; }` maps a timestamp to its byte offset in the data file
- `buildIndex()` scans the data file and produces a `vector<IndexEntry>` sorted by timestamp
- `writeIndex()` persists it to `index.bin` as `[count][IndexEntry...]`
- `lookupByTimestamp()` binary-searches the index **directly on disk** (seek + read per step) — O(log n) lookups without loading the index into memory
- `lookupRecord()` combines the index lookup with a fetch from the data file

**Known limitation:** inserting into the sorted flat index is O(n) — every insert can require shifting entries. This is what motivated the B+tree below.

**On-disk B+tree index**
- Order-4 B+tree (`btree.h` / `btree.cpp`), stored in `btree.bin` as fixed-size pages — a "child pointer" is just an `int64_t` page id you seek to (`HEADER_OFFSET + pageId * sizeof(BTreeNode)`)
- Internal nodes hold only keys + child page ids (pure routing); leaf nodes hold `(timestamp, offset)` pairs and a `nextLeaf` page id, so leaves form a linked list — set up for range scans later
- `btreeInsert()` uses preemptive splitting on the way down (the classic CLRS approach): any full node encountered while descending is split before you enter it, so a single top-down pass never needs to split back up
- `btreeSearch()` descends via binary comparison at each node — O(log n), same as the flat index, but insert is now O(log n) too instead of O(n)

## Layout

| File | Purpose |
|---|---|
| `storage.h` / `storage.cpp` | Record storage: binary records, header, random access, append |
| `btree.h` / `btree.cpp` | On-disk B+tree index: node/page layout, split, insert, search |
| `main.cpp` | Driver exercising both layers |

## Building & running

```bash
g++ -std=c++17 -o main main.cpp storage.cpp btree.cpp
./main
```

This writes a couple of records to `data.bin`, scans them back, does a bounds-checked random access, appends a new record, rebuilds/queries the sorted flat index, then inserts a scrambled batch of timestamps into the B+tree (forcing several splits) and looks a few of them up — printing each result to stdout.

## Roadmap

Following the CMU 15-445 order, adapted to a bounded, learnable scope:

- [x] **Stage 0 — Flat-file storage.** Fixed-size binary records, linear read/write, header-tracked count, append.
- [x] **Stage 1 — Indexing.** Sorted flat-file index with on-disk binary search.
- [x] **Stage 2 — B+tree index.** O(log n) reads *and* writes via node splitting instead of shifting.
- [ ] **Stage 3 — Durability.** Write-ahead log (WAL) and safe updates.
- [ ] **Stage 4 — LSM-tree.** Memtable + immutable sorted disk segments + compaction.
- [ ] **Stage 5 — Time-series specialization.** Range-optimized blocks, min/max timestamp index, delta/Gorilla-style compression, retention policies.

## Why this project

Built as hands-on systems material while learning C++: binary I/O, pointers and memory layout, RAII, and on-disk data structures — using a time-series database as a concrete, incrementally extensible target rather than following a tutorial end to end.
