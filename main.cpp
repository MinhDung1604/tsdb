#include <fstream>
#include <iostream>
#include <stdexcept>
#include "storage.h"
#include "btree.h"

using namespace std;

int main() {
    // --- Write two records from scratch ---
    {
        ofstream out(DB_PATH, ios::binary);
        if (!out) throw runtime_error("Cannot open file for writing");
        writeHeader(out, Header{2});
        writeRecord(out, Record{1000, 23.5});
        writeRecord(out, Record{2000, 25.0});
    }

    // --- Sequential scan ---
    {
        ifstream in(DB_PATH, ios::binary);
        if (!in) throw runtime_error("Cannot open file for reading");
        readHeader(in); // advance past header before scanning
        Record r;
        int row = 0;
        while (readRecord(in, r)) printRecord(row++, r);
    }

    // --- Random access ---
    {
        ifstream in(DB_PATH, ios::binary);
        if (!in) throw runtime_error("Cannot open file for reading");
        Record r;
        printOrMissing(randomAccessRecord(in, 0, r), 0, r);
        printOrMissing(randomAccessRecord(in, 2, r), 2, r); // out of range — expects "does not exist"
    }

    // --- Append a record, then verify it's readable ---
    {
        fstream fs(DB_PATH, ios::binary | ios::in | ios::out);
        if (!fs) throw runtime_error("Cannot open file for read+write");
        appendRecord(fs, Record{3000, 10.5});
    }
    {
        ifstream in(DB_PATH, ios::binary);
        if (!in) throw runtime_error("Cannot open file for reading");
        Record r;
        printOrMissing(randomAccessRecord(in, 2, r), 2, r); // should now exist
    }

    // --- B+tree: insert out of order to force splits, then search ---
    {
        initBTree();
        int64_t timestamps[] = {5000, 1000, 8000, 3000, 9000, 2000, 7000, 4000, 6000};
        for (int64_t ts : timestamps)
            btreeInsert(ts, ts * 10); // fake "offset" = ts * 10, just to see it round-trip

        int64_t value;
        for (int64_t ts : {3000, 6000, 12345}) {
            if (btreeSearch(ts, value))
                cout << "    btree: ts=" << ts << " -> offset=" << value << "\n";
            else
                cout << "    btree: ts=" << ts << " not found\n";
        }
    }

    return 0;
}
