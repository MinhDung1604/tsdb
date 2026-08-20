#include "storage.h"
#include <iostream>

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

void appendRecord(fstream& fs, const Record& r) {
    if (!fs) throw runtime_error("appendRecord: stream is not open");
    Header h = readHeader(fs);
    fs.seekp(HEADER_OFFSET + h.count * (int64_t)sizeof(Record));
    writeRecord(fs, r);
    writeHeader(fs, Header{++h.count});
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

std::vector<IndexEntry> buildIndex() {
    std::vector<IndexEntry> vec;
    // Initialize the reader
    {
        ifstream in(DB_PATH, ios::binary);
        if (!in) throw runtime_error("Cannot open file for reading");
        readHeader(in); // skip the header
        Record r;
        int64_t offset = sizeof(Header); // location of the first row
        while (readRecord(in, r)) {
            IndexEntry i{r.timestamp, offset};
            vec.push_back(i);
            offset += sizeof(Record);
        } 
    }
    sort(vec.begin(),vec.end());

    return vec;
}

void writeIndex(const std::vector<IndexEntry>& index) {
    ofstream out(INDEX_PATH, ios::binary);
    if (!out) throw runtime_error("Cannot open index.bin for writing");
    int64_t count = index.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const IndexEntry& e : index)
        out.write(reinterpret_cast<const char*>(&e), sizeof(e));
}

bool lookupByTimestamp(int64_t ts, int64_t& offset) {
    ifstream in(INDEX_PATH, ios::binary);
    if (!in) throw runtime_error("Cannot open index.bin");

    int64_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    int64_t lo = 0, hi = count - 1;
    while (lo <= hi) {
        int64_t mid = (lo + hi) / 2;
        in.seekg(sizeof(int64_t) + mid * (int64_t)sizeof(IndexEntry));
        IndexEntry e;
        in.read(reinterpret_cast<char*>(&e), sizeof(e));
        if      (e.timestamp == ts) { offset = e.offset; return true; }
        else if (e.timestamp <  ts) lo = mid + 1;
        else                        hi = mid - 1;
    }
    return false;
}

bool lookupRecord(int64_t ts, Record& out) {
    int64_t offset;
    if (!lookupByTimestamp(ts, offset)) return false;
    ifstream data(DB_PATH, ios::binary);
    if (!data) throw runtime_error("Cannot open data.bin");
    data.seekg(offset);
    return readRecord(data, out);
}
