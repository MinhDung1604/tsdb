#pragma once
#include <fstream>
#include <string>

struct Record {
    int64_t timestamp;
    double  value;
};

struct Header {
    int64_t count;
};

const int64_t HEADER_OFFSET = sizeof(Header);
const std::string DB_PATH = "data.bin";

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
