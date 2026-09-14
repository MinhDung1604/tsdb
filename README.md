# tsdb — a from-scratch storage engine in C++

A small on-disk database storage engine built from scratch in C++, following roughly the order of CMU 15-445 (Database Systems). The end goal is a time-series-oriented storage engine; the current focus is the classic storage-engine fundamentals — binary I/O, fixed-size records, and indexing — before moving on to a B-tree.

This is an active learning project, not a production database. Code is small enough to read end to end in a few minutes.

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

**Known limitation (by design, for now):** inserting into a sorted flat index is O(n) — every insert can require shifting entries. This is the concrete motivation for the next stage.

## Layout

| File | Purpose |
|---|---|
| `storage.h` | Structs, constants, and function declarations (each documented with a short spec comment) |
| `storage.cpp` | All storage-engine implementations |
| `main.cpp` | Driver exercising write, sequential scan, random access, append, and index lookups |

## Building & running

```bash
g++ -std=c++17 -o main main.cpp storage.cpp
./main
```

This writes a couple of records to `data.bin`, scans them back, does a bounds-checked random access, appends a new record, and rebuilds/queries the on-disk index — printing each result to stdout.

## Roadmap

Following the CMU 15-445 order, adapted to a bounded, learnable scope:

- [x] **Stage 0 — Flat-file storage.** Fixed-size binary records, linear read/write, header-tracked count, append.
- [x] **Stage 1 — Indexing.** Sorted flat-file index with on-disk binary search.
- [ ] **Stage 2 — B-tree index.** O(log n) reads *and* writes via node splitting instead of shifting.
- [ ] **Stage 3 — Durability.** Write-ahead log (WAL) and safe updates.
- [ ] **Stage 4 — LSM-tree.** Memtable + immutable sorted disk segments + compaction.
- [ ] **Stage 5 — Time-series specialization.** Range-optimized blocks, min/max timestamp index, delta/Gorilla-style compression, retention policies.

A B-tree is being built before LSM/time-series specialization — it's the more bounded scope for learning the mechanics of on-disk index structures. A thin API layer (likely TypeScript/Node.js) on top of the engine is a possible later addition.

## Why this project

Built as hands-on systems material while learning C++: binary I/O, pointers and memory layout, RAII, and on-disk data structures — using a time-series database as a concrete, incrementally extensible target rather than following a tutorial end to end.
