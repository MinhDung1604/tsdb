#include <iostream>
#include <fstream>

using namespace std;

struct Record {
    int64_t timestamp;
    double  value;
};


 
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

Record randomAccessRecord(ifstream& in, int target_row) {
    Record target;
    in.seekg(target_row * sizeof(target));
    if (!readRecord(in, target)) {
                
    };

    return target;
}

int main() {

    // Write the data into the file
    Record r0{1000, 23.5};
    Record r1{2000, 25};

    ofstream out("data.bin", ios::binary); // initialize the "writer cursor"
    writeRecord(out, r0); 
    writeRecord(out, r1);
    out.close();

    // Read the data from the file
    Record loaded;

    int row = 1;
    ifstream in("data.bin", ios::binary); // initialize the "reader cursor"

    while(readRecord(in, loaded)) {
        printRecord(row++, loaded);
    }
    in.close();

    ifstream in2("data.bin", ios::binary); // initialize the "reader cursor" for random access
    int target_row = 2;
    Record target = randomAccessRecord(in2, target_row);
    in2.close();    

    printRecord(target_row, target);

    return 0;
}

