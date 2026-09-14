#include <fstream>
#include <iostream>
#include <stdexcept>
#include "storage.h"
#include "btree.h"
#include "db.h"

using namespace std;

int main() {
    initDB();

    // --- Insert records out of order, indexing each one in the B+tree ---
    {
        fstream fs(DB_PATH, ios::binary | ios::in | ios::out);
        if (!fs) throw runtime_error("Cannot open file for read+write");
        insertRecord(fs, Record{2000, 25.0});
        insertRecord(fs, Record{1000, 23.5});
        insertRecord(fs, Record{3000, 10.5});
    }

    // --- Sequential scan (order records were appended, not key order) ---
    {
        ifstream in(DB_PATH, ios::binary);
        if (!in) throw runtime_error("Cannot open file for reading");
        readHeader(in); // advance past header before scanning
        Record r;
        int row = 0;
        while (readRecord(in, r)) printRecord(row++, r);
    }

    // --- Random access by row index ---
    {
        ifstream in(DB_PATH, ios::binary);
        if (!in) throw runtime_error("Cannot open file for reading");
        Record r;
        printOrMissing(randomAccessRecord(in, 0, r), 0, r);
        printOrMissing(randomAccessRecord(in, 3, r), 3, r); // out of range — expects "does not exist"
    }

    // --- Lookup by timestamp via the B+tree index ---
    {
        Record r;
        for (int64_t ts : {1000, 3000, 9999}) {
            if (findRecord(ts, r))
                cout << "    find: ts=" << ts << " -> value=" << r.value << "\n";
            else
                cout << "    find: ts=" << ts << " not found\n";
        }
    }

    return 0;
}
