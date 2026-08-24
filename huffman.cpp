#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <string>
using namespace std;

// ===============================
// Huffman Tree Node
// ===============================
struct Node {
    unsigned char ch;
    long long freq;
    Node *left, *right;

    Node(unsigned char c, long long f) {
        ch = c;
        freq = f;
        left = right = nullptr;
    }

    Node(Node* l, Node* r) {
        ch = 0;
        freq = l->freq + r->freq;
        left = l;
        right = r;
    }
};

// Min-heap comparator
struct Compare {
    bool operator()(Node* a, Node* b) {
        return a->freq > b->freq;
    }
};

// ===============================
// Generate Huffman Codes
// ===============================
void generateCodes(Node* root, string code,
                   unordered_map<unsigned char, string>& codes) {

    if (!root)
        return;

    // Leaf node
    if (!root->left && !root->right) {
        // Special case: file contains only one unique character
        if (code.empty())
            code = "0";

        codes[root->ch] = code;
        return;
    }

    generateCodes(root->left, code + "0", codes);
    generateCodes(root->right, code + "1", codes);
}

// ===============================
// Delete Huffman Tree
// ===============================
void deleteTree(Node* root) {
    if (!root)
        return;

    deleteTree(root->left);
    deleteTree(root->right);

    delete root;
}

// ===============================
// COMPRESS
// ===============================
void compressFile(const string& inputFile,
                  const string& outputFile) {

    ifstream in(inputFile, ios::binary);

    if (!in) {
        cout << "Cannot open input file!\n";
        return;
    }

    // -------------------------------
    // Step 1: Count frequencies
    // -------------------------------
    unordered_map<unsigned char, long long> freq;

    unsigned char ch;

    while (in.read(reinterpret_cast<char*>(&ch), 1)) {
        freq[ch]++;
    }

    if (freq.empty()) {
        cout << "Input file is empty!\n";
        return;
    }

    // -------------------------------
    // Step 2: Build Huffman Tree
    // -------------------------------
    priority_queue<Node*, vector<Node*>, Compare> pq;

    for (auto& p : freq) {
        pq.push(new Node(p.first, p.second));
    }

    while (pq.size() > 1) {

        Node* left = pq.top();
        pq.pop();

        Node* right = pq.top();
        pq.pop();

        Node* parent = new Node(left, right);

        pq.push(parent);
    }

    Node* root = pq.top();

    // -------------------------------
    // Step 3: Generate codes
    // -------------------------------
    unordered_map<unsigned char, string> codes;

    generateCodes(root, "", codes);

    // -------------------------------
    // Step 4: Reopen input file
    // -------------------------------
    in.clear();
    in.seekg(0);

    // -------------------------------
    // Step 5: Open output
    // -------------------------------
    ofstream out(outputFile, ios::binary);

    if (!out) {
        cout << "Cannot create output file!\n";
        deleteTree(root);
        return;
    }

    /*
        File format:

        [256 frequency values]
        [compressed binary data]
    */

    // Store frequency table
    for (int i = 0; i < 256; i++) {

        unsigned char byte = static_cast<unsigned char>(i);

        long long f = freq.count(byte) ? freq[byte] : 0;

        out.write(reinterpret_cast<char*>(&f), sizeof(f));
    }

    // -------------------------------
    // Step 6: Convert codes to bits
    // -------------------------------

    unsigned char buffer = 0;
    int bitCount = 0;

    while (in.read(reinterpret_cast<char*>(&ch), 1)) {

        string& code = codes[ch];

        for (char bit : code) {

            buffer <<= 1;

            if (bit == '1')
                buffer |= 1;

            bitCount++;

            // 8 bits completed
            if (bitCount == 8) {

                out.write(reinterpret_cast<char*>(&buffer), 1);

                buffer = 0;
                bitCount = 0;
            }
        }
    }

    // -------------------------------
    // Step 7: Write remaining bits
    // -------------------------------

    if (bitCount > 0) {

        buffer <<= (8 - bitCount);

        out.write(reinterpret_cast<char*>(&buffer), 1);
    }

    in.close();
    out.close();

    deleteTree(root);

    cout << "Compression completed!\n";
    cout << "Output: " << outputFile << endl;
}

// ===============================
// DECOMPRESS
// ===============================
void decompressFile(const string& inputFile,
                    const string& outputFile) {

    ifstream in(inputFile, ios::binary);

    if (!in) {
        cout << "Cannot open compressed file!\n";
        return;
    }

    // -------------------------------
    // Step 1: Read frequency table
    // -------------------------------
    unordered_map<unsigned char, long long> freq;

    long long totalCharacters = 0;

    for (int i = 0; i < 256; i++) {

        long long f;

        in.read(reinterpret_cast<char*>(&f), sizeof(f));

        if (f > 0) {

            unsigned char ch =
                static_cast<unsigned char>(i);

            freq[ch] = f;

            totalCharacters += f;
        }
    }

    if (totalCharacters == 0) {
        cout << "Invalid compressed file!\n";
        return;
    }

    // -------------------------------
    // Step 2: Rebuild Huffman Tree
    // -------------------------------
    priority_queue<Node*, vector<Node*>, Compare> pq;

    for (auto& p : freq) {
        pq.push(new Node(p.first, p.second));
    }

    while (pq.size() > 1) {

        Node* left = pq.top();
        pq.pop();

        Node* right = pq.top();
        pq.pop();

        pq.push(new Node(left, right));
    }

    Node* root = pq.top();

    ofstream out(outputFile, ios::binary);

    if (!out) {
        cout << "Cannot create output file!\n";
        deleteTree(root);
        return;
    }

    // -------------------------------
    // Special case:
    // Only one unique character
    // -------------------------------
    if (!root->left && !root->right) {

        for (long long i = 0; i < totalCharacters; i++) {

            unsigned char c = root->ch;

            out.write(reinterpret_cast<char*>(&c), 1);
        }

        out.close();
        in.close();

        deleteTree(root);

        cout << "Decompression completed!\n";
        cout << "Output: " << outputFile << endl;

        return;
    }

    // -------------------------------
    // Step 3: Decode bits
    // -------------------------------

    Node* current = root;

    unsigned char buffer;

    long long decoded = 0;

    while (in.read(reinterpret_cast<char*>(&buffer), 1)
           && decoded < totalCharacters) {

        // Process 8 bits
        for (int i = 7; i >= 0 && decoded < totalCharacters; i--) {

            int bit = (buffer >> i) & 1;

            if (bit == 0)
                current = current->left;
            else
                current = current->right;

            // Leaf reached
            if (!current->left && !current->right) {

                unsigned char c = current->ch;

                out.write(reinterpret_cast<char*>(&c), 1);

                decoded++;

                current = root;
            }
        }
    }

    in.close();
    out.close();

    deleteTree(root);

    cout << "Decompression completed!\n";
    cout << "Output: " << outputFile << endl;
}

// ===============================
// MAIN
// ===============================
int main() {

    int choice;

    cout << "==============================\n";
    cout << "      HUFFMAN COMPRESSOR\n";
    cout << "==============================\n";

    cout << "1. Compress\n";
    cout << "2. Decompress\n";
    cout << "Choose: ";

    cin >> choice;

    string inputFile;
    string outputFile;

    if (choice == 1) {

        cout << "Enter input file: ";
        cin >> inputFile;

        cout << "Enter output file: ";
        cin >> outputFile;

        compressFile(inputFile, outputFile);
    }

    else if (choice == 2) {

        cout << "Enter compressed file: ";
        cin >> inputFile;

        cout << "Enter output file: ";
        cin >> outputFile;

        decompressFile(inputFile, outputFile);
    }

    else {
        cout << "Invalid choice!\n";
    }

    return 0;
}