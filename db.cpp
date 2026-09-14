#include "db.h"
#include <stdexcept>

using namespace std;

void initDB() {
    {
        ofstream out(DB_PATH, ios::binary | ios::trunc);
        if (!out) throw runtime_error("Cannot create data.bin");
        writeHeader(out, Header{0});
    }
    initBTree();
}

void insertRecord(fstream& fs, const Record& r) {
    int64_t offset = appendRecord(fs, r);
    btreeInsert(r.timestamp, offset);
}

bool findRecord(int64_t ts, Record& out) {
    int64_t offset;
    if (!btreeSearch(ts, offset)) return false;
    ifstream data(DB_PATH, ios::binary);
    if (!data) throw runtime_error("Cannot open data.bin");
    data.seekg(offset);
    return readRecord(data, out);
}
