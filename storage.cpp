#include "storage.h"
#include <iostream>
#include <stdexcept>

using namespace std;

void writeHeader(ostream& out, const Header& h) {
    out.seekp(0);
    out.write(reinterpret_cast<const char*>(&h), sizeof(h));
}

Header readHeader(istream& in) {
    Header h;
    in.seekg(0);
    in.read(reinterpret_cast<char*>(&h), sizeof(h));
    return h;
}

void writeRecord(ostream& out, const Record& r) {
    out.write(reinterpret_cast<const char*>(&r), sizeof(Record));
}

bool readRecord(istream& in, Record& r) {
    return (bool) in.read(reinterpret_cast<char*>(&r), sizeof(Record));
}

bool randomAccessRecord(istream& in, int row, Record& out) {
    Header h = readHeader(in);
    if (row < 0 || row >= h.count) {
        return false;
    }
    in.seekg(HEADER_OFFSET + row * (int64_t)sizeof(Record));
    return readRecord(in, out);
}

int64_t appendRecord(fstream& fs, const Record& r) {
    if (!fs) throw runtime_error("appendRecord: stream is not open");
    Header h = readHeader(fs);
    int64_t offset = HEADER_OFFSET + h.count * (int64_t)sizeof(Record);
    fs.seekp(offset);
    writeRecord(fs, r);
    writeHeader(fs, Header{++h.count});
    return offset;
}

void printRecord(int row, const Record& r) {
    cout << "    Row " << row
         << "    ts=" << r.timestamp
         << "    value=" << r.value << "\n";
}

void printOrMissing(bool found, int row, const Record& r) {
    if (!found)
        cout << "    Row " << row << " does not exist\n";
    else
        printRecord(row, r);
}
