#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

struct Record {
    int64_t timestamp;
    double  value;
};

struct Header {
    int64_t count;
};

// Maps a timestamp to its byte offset in data.bin.
// Sorted by timestamp so lookups can use binary search.
struct IndexEntry {
    int64_t timestamp;
    int64_t offset;
    bool operator<(const IndexEntry& other) const {
        return timestamp < other.timestamp;
    }
};

const int64_t HEADER_OFFSET = sizeof(Header);
const std::string DB_PATH   = "data.bin";
const std::string INDEX_PATH = "index.bin";

// Seeks to byte 0 and writes h. Leaves the write cursor after the header.
void writeHeader(std::ostream& out, const Header& h);

// Seeks to byte 0, reads, and returns the header. Leaves the read cursor after the header.
Header readHeader(std::istream& in);

// Writes r at the current write position. Advances the cursor by sizeof(Record).
void writeRecord(std::ostream& out, const Record& r);

// Reads one record into r at the current read position. Returns false on EOF or error.
bool readRecord(std::istream& in, Record& r);

// Reads the record at 0-based row index into out. Returns false if out of range.
bool randomAccessRecord(std::istream& in, int row, Record& out);

// Appends r after the last record and increments the header count.
// fs must be opened with ios::in | ios::out.
void appendRecord(std::fstream& fs, const Record& r);

// Prints row and record fields to stdout.
void printRecord(int row, const Record& r);

// Prints the record if found, or a "does not exist" message otherwise.
void printOrMissing(bool found, int row, const Record& r);

// Scans data.bin and builds a sorted vector of IndexEntry.
std:: vector<IndexEntry> buildIndex();

// Builds the index from data.bin and writes it to index.bin.
// Format: [int64_t count][IndexEntry 0][IndexEntry 1]...
void writeIndex(const std::vector<IndexEntry>& index);

// Binary searches index.bin for ts. Sets offset to the matching byte position in data.bin.
// Returns false if ts is not found. Does not load the full index into memory.
bool lookupByTimestamp(int64_t ts, int64_t& offset);

// Looks up ts in index.bin, then fetches the matching record from data.bin into out.
// Returns false if ts is not found.
bool lookupRecord(int64_t ts, Record& out);