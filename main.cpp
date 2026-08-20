#include <fstream>
#include <stdexcept>
#include "storage.h"

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

    return 0;
}
