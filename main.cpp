#include <iostream>
#include <fstream>

using namespace std;

struct Record {
    int64_t timestamp;
    double  value;
};

struct Header {
    int64_t count;
};

const int64_t HEADER_OFFSET = sizeof(Header);
const string DB_PATH = "data.bin";

void writeHeader(ostream& out, const Header& header) {
    out.seekp(0);
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
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

bool randomAccessRecord(istream& in, int target_row, Record& target) {
    Header h = readHeader(in);
    if (target_row < 0 || target_row >= h.count) {
        return false;
    }
    in.seekg(HEADER_OFFSET + target_row * sizeof(Record));
    
    return readRecord(in, target);
}

void appendRecord(fstream& fs, const Record& r) {
    
    string message = "Can't open " + DB_PATH;
    if (!fs) throw runtime_error(message);

    Header h = readHeader(fs);
    fs.seekp(HEADER_OFFSET + h.count * sizeof(Record)); // move the position to after the latest row
    writeRecord(fs, r);
    writeHeader(fs, Header{++h.count});

}

void printRecord(int row, const Record& r) {
    cout << "    Row " << row;
    cout << "    ts=" << r.timestamp;
    cout << "    value=" << r.value << "\n";
}

void printOrMissing(bool success, int target_row, Record &target)
{
    if (!success)
    {
        cout << "    Row " << target_row << " does not exist" << "\n";
    }
    else
    {
        printRecord(target_row, target);
    }
}



int main()
{

    // Write the data into the file
    Record r0{1000, 23.5};
    Record r1{2000, 25};

    ofstream out(DB_PATH, ios::binary); // initialize the "writer cursor"
    if (!out) throw runtime_error("Cannot open file");

    writeHeader(out, Header{2});
    writeRecord(out, r0); 
    writeRecord(out, r1);
    out.close();

    // Read the data from the file
    Record loaded;

    ifstream in(DB_PATH, ios::binary); // initialize the "reader cursor"
    if (!in) throw runtime_error("Cannot open file");
    readHeader(in);
    int row = 0;
    while(readRecord(in, loaded)) {
        printRecord(row++, loaded);
    }
    in.close();


    // Randomly read the data from the file
    ifstream in2(DB_PATH, ios::binary); // initialize the "reader cursor" for random access
    if (!in2) throw runtime_error("Cannot open file");

    int target_row = 0;
    Record target; 
    bool success = randomAccessRecord(in2, target_row, target);
    printOrMissing(success, target_row, target);

    target_row = 2; 
    success = randomAccessRecord(in2, target_row, target);
    printOrMissing(success, target_row, target);
    in2.close();    

    // Append new record
    fstream file(DB_PATH, ios::binary | ios::in | ios::out);
    if (file) throw runtime_error("Cannot open file");
    appendRecord(file, Record{3000, 10.5});
    file.close();
    ifstream in3(DB_PATH, ios::binary); 
    if (!in3) throw runtime_error("Cannot open file");
    success = randomAccessRecord(in3, 2, target);
    printOrMissing(success, target_row, target);
    in3.close();
    return 0;
}



