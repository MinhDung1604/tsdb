#pragma once
#include "storage.h"
#include "btree.h"

// Creates a fresh, empty database: data.bin (Header{0}) and an empty
// B+tree root in btree.bin. Overwrites any existing files at DB_PATH/BTREE_PATH.
void initDB();

// Appends r to data.bin and indexes it in the B+tree in the same call,
// so the two files never drift out of sync. fs must be opened with
// ios::in | ios::out on DB_PATH.
void insertRecord(std::fstream& fs, const Record& r);

// Looks up ts via the B+tree, then fetches the matching record from
// data.bin. Returns false if ts is not present in the index.
bool findRecord(int64_t ts, Record& out);
