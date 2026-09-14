# tsdb

An on-disk storage engine written from scratch in C++, implementing the core techniques behind production database internals: a binary record store and a B+tree index, wired together so every write stays consistent between the two.

- Binary on-disk record format with `fstream`-based file I/O — fixed-size records, header-tracked count, O(1) random access, and append.
- B+tree index (order 4, page-based, on-disk) replacing linear scan with O(log n) lookup **and** O(log n) insert via node splitting instead of data shifting.
- A wiring layer that keeps the record store and index consistent on every write, plus a standalone test suite that caught a real off-by-one in the tree's search/insert descent logic before it shipped.

## Design

**Record storage** (`storage.h` / `storage.cpp`) — `Record { int64_t timestamp; double value; }` is a fixed 16-byte struct, serialized directly via `reinterpret_cast` and `fstream::read`/`write`. A `Header` at byte 0 tracks the record count so any reader knows how far to scan without a sentinel value. `appendRecord()` returns the byte offset it wrote to, which is what the index needs to point back at a record.

**B+tree index** (`btree.h` / `btree.cpp`) — an order-4 B+tree stored in its own file as fixed-size pages; a "pointer" between nodes is just an `int64_t` page id you seek to (`offset = HEADER_OFFSET + pageId * sizeof(BTreeNode)`). Internal nodes hold only keys and child page ids (pure routing, no data); leaf nodes hold the actual `(timestamp, offset)` pairs and a `nextLeaf` page id, so leaves form a linked list — the standard B+tree layout for supporting range scans over point lookups alone. Insert uses preemptive splitting on the way down (the CLRS approach): a full node is split before you descend into it, so a single top-down pass never needs a second pass to propagate a split back up.

**Wiring layer** (`db.h` / `db.cpp`) — `insertRecord()` appends to the record store and indexes the resulting offset in the same call, so the two files can never drift apart; `findRecord()` reverses that: look up the offset via the B+tree, then fetch the record.

**Testing** (`tests.cpp`) — a small dependency-free test binary (no framework) covering the record store, the B+tree in isolation (including a scrambled multi-key insert that forces splits across several tree levels), and the wiring layer end to end. The multi-level test is what surfaced a real bug: the tree's search descent and insert descent disagreed on which child a key equal to a separator belonged to, so a key could search correctly right after insertion and silently become unreachable once a later split moved it across that boundary.

## Layout

| File | Purpose |
|---|---|
| `storage.h` / `storage.cpp` | Binary record storage: header, random access, append |
| `btree.h` / `btree.cpp` | On-disk B+tree index: node/page layout, split, insert, search |
| `db.h` / `db.cpp` | Wires record storage and the B+tree together into insert/find |
| `main.cpp` | Driver demoing the wired-up read/write paths |
| `tests.cpp` | Standalone test binary covering storage, the B+tree, and the wiring layer |

## Building & running

```bash
g++ -std=c++17 -o main main.cpp storage.cpp btree.cpp db.cpp
./main
```

```bash
g++ -std=c++17 -o tests tests.cpp storage.cpp btree.cpp db.cpp
./tests
```

`main` and `tests` both operate on the same `data.bin`/`btree.bin` paths, so running one after the other overwrites what the previous run wrote.
