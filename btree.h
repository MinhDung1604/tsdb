#pragma once
#include <fstream>
#include <string>
#include <cstdint>

// Order 4 B+tree: at most ORDER-1 keys and ORDER children per node.
// Kept small on purpose so a full tree is easy to trace by hand.
const int ORDER = 4;
const std::string BTREE_PATH = "btree.bin";

// A single on-disk page. Internal nodes use `children`; leaves use `values`
// and `nextLeaf` (so leaves form a linked list for future range scans).
// Both arrays are always present for simplicity, at the cost of some
// wasted space per leaf/internal node.
struct BTreeNode {
    bool    isLeaf;
    int32_t numKeys;
    int64_t keys[ORDER - 1];
    int64_t children[ORDER];      // internal only: child page ids
    int64_t values[ORDER - 1];    // leaf only: record offset per key
    int64_t nextLeaf;             // leaf only: page id of next leaf, -1 if none
};

// Tracks the tree's entry point and the next unused page id (page allocator).
struct BTreeHeader {
    int64_t rootPageId;
    int64_t nextPageId;
};

const int64_t BTREE_HEADER_OFFSET = sizeof(BTreeHeader);

// Creates a fresh btree.bin: header (root = page 0) + one empty leaf root.
// Overwrites any existing file at BTREE_PATH.
void initBTree();

// Seeks to byte 0 and writes h.
void writeBTreeHeader(std::ostream& out, const BTreeHeader& h);

// Seeks to byte 0 and reads the header.
BTreeHeader readBTreeHeader(std::istream& in);

// Reads the node stored at pageId.
BTreeNode readNode(std::istream& in, int64_t pageId);

// Writes node at pageId.
void writeNode(std::ostream& out, int64_t pageId, const BTreeNode& node);

// Bumps and persists header.nextPageId, returning the id just allocated.
int64_t allocatePage(std::fstream& fs, BTreeHeader& header);

// Searches the tree for key. Sets valueOut to the matching record offset
// (a byte offset into data.bin). Returns false if key is not present.
bool btreeSearch(int64_t key, int64_t& valueOut);

// Inserts (key, value) into the tree, splitting full nodes on the way
// down so every insert is O(log n) — no shifting of existing entries.
void btreeInsert(int64_t key, int64_t value);
