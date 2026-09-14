// Standalone test binary — build separately from main.cpp:
//   g++ -std=c++17 -o tests tests.cpp storage.cpp btree.cpp db.cpp
//   ./tests
//
// No external framework: CHECK() logs a failure and keeps going so one
// broken assertion doesn't hide the rest of a test's results. Tests share
// the same on-disk paths as main.cpp (DB_PATH/BTREE_PATH are hardcoded
// constants), so each test starts by truncating/reinitializing them —
// don't run this against a database you care about.

#include "storage.h"
#include "btree.h"
#include "db.h"
#include <iostream>

using namespace std;

static int failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " << #cond << "\n"; \
            ++failures; \
        } \
    } while (0)

// ---------- storage.h/.cpp ----------

void test_record_roundtrip() {
    Record r{123456789, 3.14};
    {
        ofstream out(DB_PATH, ios::binary | ios::trunc);
        writeRecord(out, r);
    }
    Record loaded{};
    ifstream in(DB_PATH, ios::binary);
    CHECK(readRecord(in, loaded));
    CHECK(loaded.timestamp == r.timestamp);
    CHECK(loaded.value == r.value);
}

void test_read_record_fails_past_eof() {
    {
        ofstream out(DB_PATH, ios::binary | ios::trunc);
        writeRecord(out, Record{1, 1.0});
    }
    ifstream in(DB_PATH, ios::binary);
    Record r;
    CHECK(readRecord(in, r));   // the one record that's there
    CHECK(!readRecord(in, r)); // nothing left
}

void test_header_roundtrip() {
    {
        ofstream out(DB_PATH, ios::binary | ios::trunc);
        writeHeader(out, Header{42});
    }
    ifstream in(DB_PATH, ios::binary);
    CHECK(readHeader(in).count == 42);
}

void test_random_access_bounds() {
    {
        ofstream out(DB_PATH, ios::binary | ios::trunc);
        writeHeader(out, Header{2});
        writeRecord(out, Record{1000, 1.0});
        writeRecord(out, Record{2000, 2.0});
    }
    ifstream in(DB_PATH, ios::binary);
    Record r;
    CHECK(randomAccessRecord(in, 0, r) && r.timestamp == 1000);
    CHECK(randomAccessRecord(in, 1, r) && r.timestamp == 2000);
    CHECK(!randomAccessRecord(in, 2, r));  // one past the end
    CHECK(!randomAccessRecord(in, -1, r)); // negative row
}

void test_append_returns_offset_and_updates_count() {
    {
        ofstream out(DB_PATH, ios::binary | ios::trunc);
        writeHeader(out, Header{0});
    }
    fstream fs(DB_PATH, ios::binary | ios::in | ios::out);
    int64_t off0 = appendRecord(fs, Record{1000, 1.0});
    int64_t off1 = appendRecord(fs, Record{2000, 2.0});

    CHECK(off0 == HEADER_OFFSET);
    CHECK(off1 == HEADER_OFFSET + (int64_t)sizeof(Record));
    CHECK(readHeader(fs).count == 2);
}

// ---------- btree.h/.cpp ----------

void test_btree_single_insert_and_search() {
    initBTree();
    btreeInsert(500, 5000);
    int64_t v;
    CHECK(btreeSearch(500, v) && v == 5000);
    CHECK(!btreeSearch(999, v));
}

void test_btree_survives_many_splits() {
    initBTree();
    int64_t keys[] = {50, 10, 80, 30, 90, 20, 70, 40, 60, 15, 25, 35, 45, 55, 65, 75, 85, 95};
    for (int64_t k : keys) btreeInsert(k, k * 100);

    for (int64_t k : keys) {
        int64_t v;
        CHECK(btreeSearch(k, v) && v == k * 100);
    }
    int64_t v;
    CHECK(!btreeSearch(999, v));
}

void test_btree_ascending_inserts() {
    initBTree();
    for (int64_t k = 0; k < 20; ++k) btreeInsert(k, k);
    int64_t v;
    for (int64_t k = 0; k < 20; ++k) CHECK(btreeSearch(k, v) && v == k);
}

void test_btree_descending_inserts() {
    initBTree();
    for (int64_t k = 20; k >= 1; --k) btreeInsert(k, k);
    int64_t v;
    for (int64_t k = 1; k <= 20; ++k) CHECK(btreeSearch(k, v) && v == k);
}

void test_btree_search_on_empty_tree() {
    initBTree();
    int64_t v;
    CHECK(!btreeSearch(1, v));
}

// ---------- db.h/.cpp (storage + btree wired together) ----------

void test_db_insert_and_find() {
    initDB();
    Record records[] = {{500, 1.1}, {100, 2.2}, {900, 3.3}, {300, 4.4}};
    {
        fstream fs(DB_PATH, ios::binary | ios::in | ios::out);
        for (const Record& r : records) insertRecord(fs, r);
    }

    for (const Record& r : records) {
        Record out;
        CHECK(findRecord(r.timestamp, out));
        CHECK(out.timestamp == r.timestamp);
        CHECK(out.value == r.value);
    }
    Record out;
    CHECK(!findRecord(123456, out));
}

void test_db_find_reflects_actual_data_offset() {
    // Regression guard: findRecord must read the record at the offset
    // appendRecord actually wrote, not assume insertion order == file order.
    initDB();
    fstream fs(DB_PATH, ios::binary | ios::in | ios::out);
    insertRecord(fs, Record{2000, 20.0}); // written first, higher key
    insertRecord(fs, Record{1000, 10.0}); // written second, lower key
    fs.close();

    Record out;
    CHECK(findRecord(1000, out) && out.value == 10.0);
    CHECK(findRecord(2000, out) && out.value == 20.0);
}

int main() {
    test_record_roundtrip();
    test_read_record_fails_past_eof();
    test_header_roundtrip();
    test_random_access_bounds();
    test_append_returns_offset_and_updates_count();

    test_btree_single_insert_and_search();
    test_btree_survives_many_splits();
    test_btree_ascending_inserts();
    test_btree_descending_inserts();
    test_btree_search_on_empty_tree();

    test_db_insert_and_find();
    test_db_find_reflects_actual_data_offset();

    if (failures == 0) {
        cout << "All tests passed\n";
        return 0;
    }
    cout << failures << " check(s) failed\n";
    return 1;
}
