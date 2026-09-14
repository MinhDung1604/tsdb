# tsdb — a from-scratch storage engine in C++

An on-disk database storage engine built from scratch in C++: fixed-size binary record storage plus an on-disk B+tree index, following roughly the order of CMU 15-445 (Database Systems).

## What's implemented

**Binary record storage** (`storage.h` / `storage.cpp`)
- `Record { int64_t timestamp; double value; }` — fixed-size 16-byte binary records
- `Header { int64_t count; }` at byte 0 of the data file, updated on every append
- Sequential write/scan, O(1) random access by row index (bounds-checked), and append
- All file opens are checked; failures throw `std::runtime_error`

**On-disk B+tree index** (`btree.h` / `btree.cpp`)
- Order-4 B+tree stored in `btree.bin` as fixed-size pages — a "child pointer" is just an `int64_t` page id you seek to
- Internal nodes hold only keys + child page ids (pure routing); leaves hold `(timestamp, offset)` pairs plus a `nextLeaf` page id, forming a linked list across leaves
- Insert uses preemptive splitting on the way down: any full node encountered while descending is split before you enter it, so a single top-down pass never needs to split back up
- O(log n) insert and search

**Wiring layer** (`db.h` / `db.cpp`)
- `initDB()` — creates a fresh `data.bin` + `btree.bin` pair
- `insertRecord()` — appends a record and indexes its offset in the B+tree in one call, so the two files can't drift out of sync
- `findRecord()` — looks a timestamp up via the B+tree, then fetches the record from `data.bin`

## Layout

| File | Purpose |
|---|---|
| `storage.h` / `storage.cpp` | Binary record storage: header, random access, append |
| `btree.h` / `btree.cpp` | On-disk B+tree index: node/page layout, split, insert, search |
| `db.h` / `db.cpp` | Wires record storage and the B+tree together into insert/find |
| `main.cpp` | Small driver demoing the wired-up read/write paths |
| `tests.cpp` | Standalone test binary covering storage, B+tree, and the wiring layer |

## Building & running

```bash
g++ -std=c++17 -o main main.cpp storage.cpp btree.cpp db.cpp
./main
```

```bash
g++ -std=c++17 -o tests tests.cpp storage.cpp btree.cpp db.cpp
./tests
```

Both share the same `data.bin`/`btree.bin` paths (hardcoded constants), so running `tests` after `main` overwrites what `main` wrote.
