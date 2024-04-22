#include <iostream>
#include <fstream>
#include <map>
#include <bitset>
#include <ctime>
#include <vector>
#include <omp.h>

using namespace std;

// Node for Huffman Tree
struct Node {
    char data;
    Node* left;
    Node* right;

    Node(char data) {
        this->data = data;
        left = right = nullptr;
    }
};

// Deserialize Huffman Tree from serialized string
Node* deserializeHuffmanTree(string& serializedTree, size_t& index) {
    if (index >= serializedTree.size())
        return nullptr;

    if (serializedTree[index] == '0') {
        index++;
        return nullptr;
    }

    index++;
    Node* root = new Node(serializedTree[index]);
    index++;

#pragma omp task shared(root, serializedTree, index)
   root->left = deserializeHuffmanTree(serializedTree, index);


#pragma omp task shared(root, serializedTree, index)
    root->right = deserializeHuffmanTree(serializedTree, index);

#pragma omp taskwait
    return root;
}

// Decode text using Huffman Tree and Huffman codes
string decodeText(string binaryString, Node* root) {
    string decodedText = "";
    Node* current = root;
    for (char bit : binaryString) {
        if (current == nullptr) {
            cerr << "Error: Invalid Huffman code" << endl;
            return "";
        }

        if (bit == '0') {
            current = current->left;
        } else {
            current = current->right;
        }

        if (current != nullptr && current->left == nullptr && current->right == nullptr) {
            if (current->data != '~')
                decodedText += current->data;
            else
                decodedText += '\n';
            current = root;
        }
    }
    
    return decodedText;
}


string decodeBinaryData(ifstream &encodedFile, Node *root, size_t start, size_t end) {

  int p = omp_get_max_threads();

  string binaryStrings[p];
  int i;
  for (i = 0; i < p; i++) {
    binaryStrings[i] = "";
  }
  string binaryString = "";

  encodedFile.seekg(start); // Move file pointer to start position
  size_t fileSize = end - start;

  // Read binary data from file
  char *buffer = new char[fileSize];
  encodedFile.read(buffer, fileSize);
  encodedFile.close();

// Convert binary data to binary string
#pragma omp parallel for firstprivate(buffer)
  for (size_t i = 0; i < fileSize; ++i) {
    bitset<8> bits(
        static_cast<unsigned char>(buffer[i])); // Convert each byte to binary
    binaryStrings[omp_get_thread_num()] += bits.to_string();
  }

  delete[] buffer;

  for (i = 0; i < p; i++) {
    binaryString += binaryStrings[i];
  }

  // Find the position of the last non-zero bit
  size_t lastNonZeroPos = binaryString.find_last_not_of('0');
  if (lastNonZeroPos != string::npos) {
    binaryString.resize(lastNonZeroPos + 1); // Remove trailing zeros
  } else {
    binaryString.clear(); // Handle case where the string is all zeros
  }

  // Decode binary string using Huffman Tree
  string decodedText = decodeText(binaryString, root);
  
  return decodedText;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <encoded_file> <tree_file> <output_file>" << endl;
        return 1;
    }

    double startTime = omp_get_wtime();

    string encodedFileName = argv[1];
    string treeFileName = argv[2];
    string outputFileName = argv[3];

    // Read serialized Huffman tree
    ifstream treeFile(treeFileName);
    if (!treeFile) {
        cerr << "Error: Unable to open Huffman tree file: " << treeFileName << endl;
        return 1;
    }
    string serializedTree;
    getline(treeFile, serializedTree);
    treeFile.close();

    // Deserialize Huffman tree
    size_t index = 0;
    Node* root = deserializeHuffmanTree(serializedTree, index);

    // Read encoded text from file
    ifstream encodedFile(encodedFileName, ios::binary); // Open encoded file in binary mode
    if (!encodedFile) {
        cerr << "Error: Unable to open encoded file: " << encodedFileName << endl;
        return 1;
    }

    // Calculate chunk size for each process
    encodedFile.seekg(0, ios::end);
    size_t fileSize = encodedFile.tellg();
    size_t start = 0;
    size_t end = fileSize;

    // Decode binary data using Huffman Tree
    string decodedText = decodeBinaryData(encodedFile, root, start, end);

    // Write decoded text to output file
    ofstream outputFile(outputFileName);
    if (!outputFile) {
        cerr << "Error: Unable to open output file: " << outputFileName << endl;
        return 1;
    }
    outputFile << decodedText;
    outputFile.close();

    // Release memory
    delete root;

    double endTime = omp_get_wtime(); // Stop measuring time
    double elapsedTime = endTime - startTime;

    cout << "Decoding completed successfully. Decoded text saved to: " << outputFileName << endl;
    cout << "Time taken: " << elapsedTime << " seconds" << endl;

    return 0;
}
