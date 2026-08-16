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

void writeHeader(ofstream& out, const Header& header) {
    out.seekp(0);
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
}

Header readHeader(ifstream& in) {
    Header h;
    in.seekg(0);
    in.read(reinterpret_cast<char*>(&h), sizeof(h));
    return h;
}

 
void writeRecord(ofstream& out, const Record& r) {
    out.write(reinterpret_cast<const char*>(&r), sizeof(r));
}

bool readRecord(ifstream& in, Record& r) {
    return (bool) in.read(reinterpret_cast<char*>(&r), sizeof(r));
}

void printRecord(int row, Record& r) {
    cout << "    Row " << row;
    cout << "    ts=" << r.timestamp;
    cout << "    value=" << r.value << endl;
}

bool randomAccessRecord(ifstream& in, int target_row, Record& target) {
    Header h = readHeader(in);
    if (target_row < 0 || target_row >= h.count) {
        return false;
    };
    in.seekg(HEADER_OFFSET + target_row * sizeof(target));
    
    return readRecord(in, target);
}

int main() {

    // Write the data into the file
    Record r0{1000, 23.5};
    Record r1{2000, 25};

    ofstream out("data.bin", ios::binary); // initialize the "writer cursor"
    writeHeader(out, Header{2});
    writeRecord(out, r0); 
    writeRecord(out, r1);
    out.close();

    // Read the data from the file
    Record loaded;

    ifstream in("data.bin", ios::binary); // initialize the "reader cursor"
    readHeader(in);
    int row = 0;
    while(readRecord(in, loaded)) {
        printRecord(row++, loaded);
    }
    in.close();


    // Randomly read the data from the file
    ifstream in2("data.bin", ios::binary); // initialize the "reader cursor" for random access
    int target_row = 2;
    Record target; 
    bool success = randomAccessRecord(in2, target_row, target);
    if (!success) {
        cout << "    Row " << target_row << " does not exist" << endl;
    } else {
        printRecord(target_row, target);
    }
    in2.close();    


    return 0;
}

