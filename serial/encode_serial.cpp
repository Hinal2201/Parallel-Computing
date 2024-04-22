#include <iostream>
#include <fstream>
#include <queue>
#include <map>
#include <bitset>

using namespace std;

//version=1.1.3
// Node for Huffman Tree
struct Node {
    char data;
    int frequency;
    Node* left;
    Node* right;

    Node(char data, int frequency) {
        this->data = data;
        this->frequency = frequency;
        left = right = nullptr;
    }
};

// Comparison function for priority queue
struct compare {
    bool operator()(Node* left, Node* right) {
        return left->frequency > right->frequency;
    }
};

// Build Huffman Tree
Node* buildHuffmanTree(map<char, int>& frequencies) {
    priority_queue<Node*, vector<Node*>, compare> pq;

    // Populate priority queue with leaf nodes
    for (const auto& entry : frequencies) {
        auto node = new Node(entry.first, entry.second);
        pq.push(node);
    }

    // Combine nodes until there is only one root node left
    while (pq.size() > 1) {
        Node* left = pq.top();
        pq.pop();
        Node* right = pq.top();
        pq.pop();

        // Create a new node with combined frequency
        Node* combined = new Node('\0', left->frequency + right->frequency);
        combined->left = left;
        combined->right = right;

        // Push the combined node back to the priority queue
        pq.push(combined);
    }

    // Return the root of the Huffman tree
    return pq.top();
}

// Serialize Huffman Tree and write to file
void serializeHuffmanTree(Node* root, ofstream& outputFile) {
    if (root == nullptr) {
        outputFile << "0"; // Represent null node
        return;
    }

    outputFile << "1" << root->data;
    serializeHuffmanTree(root->left, outputFile);
    serializeHuffmanTree(root->right, outputFile);
}

// Assign Huffman codes to characters
void assignHuffmanCodes(Node* root, string code, map<char, string>& codes) {
    if (root == nullptr)
        return;

    if (root->data != '\0') {
        codes[root->data] = code;
    }

    assignHuffmanCodes(root->left, code + "0", codes);
    assignHuffmanCodes(root->right, code + "1", codes);
}

void encodeText(const string& text, const map<char, string>& codes, ofstream& outputFile) {
    // Encode text using Huffman codes
    string encodedText;
    for (char c : text) {
        if (c != '\n')
            encodedText += codes.at(c);
        else
            encodedText += codes.at('~');
    }

    //cout <<"encodedText: ---"<<encodedText<<"---"<<endl;

    // Convert encoded text to bytes
    vector<unsigned char> bytes;
    int bitsRemaining = 0;
    unsigned char currentByte = 0;
    for (char bit : encodedText) {
        currentByte |= (bit - '0') << (7 - bitsRemaining);
        bitsRemaining++;
        if (bitsRemaining == 8) {
            bytes.push_back(currentByte);
            currentByte = 0;
            bitsRemaining = 0;
        }
    }
    if (bitsRemaining != 0) {
        bytes.push_back(currentByte);
    }

    // Write bytes to output file
    outputFile.write(reinterpret_cast<const char*>(&bytes[0]), bytes.size());
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file> <tree_file>" << endl;
        return 1;
    }

    string inputFileName = argv[1];
    string outputFileName = argv[2];
    string treeFileName = argv[3]; // File to store Huffman tree

    // Read input text file
    ifstream inputFile(inputFileName);
    if (!inputFile) {
        cerr << "Error: Unable to open input file: " << inputFileName << endl;
        return 1;
    }
    

    // Calculate frequencies of characters in the text
    map<char, int> frequencies;
    char c;
    int counter= 0;
    while (inputFile.get(c)) {
        if (c != '\n')
            frequencies[c]++;
        else
            frequencies['~']++;
    }
    inputFile.close();

    // Build Huffman Tree
    Node* root = buildHuffmanTree(frequencies);

    // Serialize Huffman tree and write to file
    ofstream treeFile(treeFileName);
    if (!treeFile) {
        cerr << "Error: Unable to open Huffman tree file: " << treeFileName << endl;
        return 1;
    }
    serializeHuffmanTree(root, treeFile);
    treeFile.close();

    // Assign Huffman codes to characters
    map<char, string> codes;
    assignHuffmanCodes(root, "", codes);

    // Encode text using Huffman codes and write to output file
    ofstream outputFile(outputFileName, ios::binary); // Open output file in binary mode
    if (!outputFile) {
        cerr << "Error: Unable to open output file: " << outputFileName << endl;
        return 1;
    }

    // Read input text file again
    inputFile.open(inputFileName);
    string text((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
    encodeText(text, codes, outputFile);

    // Close files and release memory
    inputFile.close();
    outputFile.close();
    delete root;

    cout << "Compression completed successfully." << endl;

    return 0;
}

