#include "btree.h"
#include <stdexcept>

using namespace std;

void writeBTreeHeader(ostream& out, const BTreeHeader& h) {
    out.seekp(0);
    out.write(reinterpret_cast<const char*>(&h), sizeof(h));
}

BTreeHeader readBTreeHeader(istream& in) {
    BTreeHeader h;
    in.seekg(0);
    in.read(reinterpret_cast<char*>(&h), sizeof(h));
    return h;
}

BTreeNode readNode(istream& in, int64_t pageId) {
    BTreeNode node;
    in.seekg(BTREE_HEADER_OFFSET + pageId * (int64_t)sizeof(BTreeNode));
    in.read(reinterpret_cast<char*>(&node), sizeof(node));
    return node;
}

void writeNode(ostream& out, int64_t pageId, const BTreeNode& node) {
    out.seekp(BTREE_HEADER_OFFSET + pageId * (int64_t)sizeof(BTreeNode));
    out.write(reinterpret_cast<const char*>(&node), sizeof(node));
}

int64_t allocatePage(fstream& fs, BTreeHeader& header) {
    int64_t pageId = header.nextPageId++;
    writeBTreeHeader(fs, header);
    return pageId;
}

void initBTree() {
    ofstream out(BTREE_PATH, ios::binary | ios::trunc);
    if (!out) throw runtime_error("Cannot create btree.bin");

    writeBTreeHeader(out, BTreeHeader{0, 1});

    BTreeNode root{};
    root.isLeaf = true;
    root.numKeys = 0;
    root.nextLeaf = -1;
    writeNode(out, 0, root);
}

// Splits the full child at parent.children[i], pushing the median key up
// into `parent` at position i. Caller guarantees parent is not full.
static void splitChild(BTreeNode& parent, int i, fstream& fs, BTreeHeader& header) {
    int64_t childPageId = parent.children[i];
    BTreeNode child = readNode(fs, childPageId);

    BTreeNode sibling{};
    sibling.isLeaf = child.isLeaf;

    int mid = (ORDER - 1) / 2;
    int64_t medianKey;

    if (child.isLeaf) {
        // Leaves keep every key; the median is duplicated as the sibling's
        // first key so it still acts as the correct routing key above.
        sibling.numKeys = child.numKeys - mid;
        for (int j = 0; j < sibling.numKeys; ++j) {
            sibling.keys[j]   = child.keys[mid + j];
            sibling.values[j] = child.values[mid + j];
        }
        child.numKeys = mid;
        medianKey = sibling.keys[0];
    } else {
        // Internal nodes: the median key moves up and is not duplicated.
        medianKey = child.keys[mid];
        sibling.numKeys = child.numKeys - mid - 1;
        for (int j = 0; j < sibling.numKeys; ++j)
            sibling.keys[j] = child.keys[mid + 1 + j];
        for (int j = 0; j <= sibling.numKeys; ++j)
            sibling.children[j] = child.children[mid + 1 + j];
        child.numKeys = mid;
    }

    int64_t siblingPageId = allocatePage(fs, header);
    if (child.isLeaf) {
        sibling.nextLeaf = child.nextLeaf;
        child.nextLeaf = siblingPageId;
    }

    // Shift parent's keys/children right to open a slot at position i.
    for (int j = parent.numKeys; j > i; --j)
        parent.keys[j] = parent.keys[j - 1];
    for (int j = parent.numKeys + 1; j > i + 1; --j)
        parent.children[j] = parent.children[j - 1];

    parent.keys[i] = medianKey;
    parent.children[i + 1] = siblingPageId;
    parent.numKeys++;

    writeNode(fs, childPageId, child);
    writeNode(fs, siblingPageId, sibling);
}

// Inserts (key, value) under the node at pageId. Caller guarantees that
// node is not full, so this never needs to split its own root call.
static void insertNonFull(int64_t pageId, int64_t key, int64_t value, fstream& fs, BTreeHeader& header) {
    BTreeNode node = readNode(fs, pageId);

    if (node.isLeaf) {
        int i = node.numKeys - 1;
        while (i >= 0 && node.keys[i] > key) {
            node.keys[i + 1]   = node.keys[i];
            node.values[i + 1] = node.values[i];
            --i;
        }
        node.keys[i + 1]   = key;
        node.values[i + 1] = value;
        node.numKeys++;
        writeNode(fs, pageId, node);
        return;
    }

    int i = node.numKeys - 1;
    while (i >= 0 && node.keys[i] > key) --i;
    ++i; // child index to descend into

    BTreeNode child = readNode(fs, node.children[i]);
    if (child.numKeys == ORDER - 1) {
        splitChild(node, i, fs, header);
        writeNode(fs, pageId, node); // persist the parent's new key/child
        if (key > node.keys[i]) ++i;
    }

    insertNonFull(node.children[i], key, value, fs, header);
}

void btreeInsert(int64_t key, int64_t value) {
    fstream fs(BTREE_PATH, ios::binary | ios::in | ios::out);
    if (!fs) throw runtime_error("Cannot open btree.bin");

    BTreeHeader header = readBTreeHeader(fs);
    BTreeNode root = readNode(fs, header.rootPageId);

    if (root.numKeys == ORDER - 1) {
        // Root is full: grow the tree by one level.
        int64_t oldRootPageId = header.rootPageId;
        int64_t newRootPageId = allocatePage(fs, header);

        BTreeNode newRoot{};
        newRoot.isLeaf = false;
        newRoot.numKeys = 0;
        newRoot.children[0] = oldRootPageId;

        header.rootPageId = newRootPageId;
        writeBTreeHeader(fs, header);

        splitChild(newRoot, 0, fs, header);
        writeNode(fs, newRootPageId, newRoot);

        int i = (key > newRoot.keys[0]) ? 1 : 0;
        insertNonFull(newRoot.children[i], key, value, fs, header);
    } else {
        insertNonFull(header.rootPageId, key, value, fs, header);
    }
}

bool btreeSearch(int64_t key, int64_t& valueOut) {
    ifstream in(BTREE_PATH, ios::binary);
    if (!in) throw runtime_error("Cannot open btree.bin");

    BTreeHeader header = readBTreeHeader(in);
    int64_t pageId = header.rootPageId;

    while (true) {
        BTreeNode node = readNode(in, pageId);

        if (node.isLeaf) {
            // Leaves need an exact match, not a boundary descent.
            for (int i = 0; i < node.numKeys; ++i) {
                if (node.keys[i] == key) {
                    valueOut = node.values[i];
                    return true;
                }
            }
            return false;
        }

        // children[i]'s subtree holds keys < keys[i], so a key equal to a
        // separator belongs in children[i+1] — this descent must mirror
        // insertNonFull's, or a key can become unreachable once a later
        // split moves it across a boundary.
        int i = 0;
        while (i < node.numKeys && key >= node.keys[i]) ++i;
        pageId = node.children[i];
    }
}
