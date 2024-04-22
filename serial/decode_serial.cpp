#include <bitset>
#include <fstream>
#include <iostream>
#include <map>

using namespace std;

// Node for Huffman Tree
struct Node {
  char data;
  Node *left;
  Node *right;

  Node(char data) {
    this->data = data;
    left = right = nullptr;
  }
};

// Deserialize Huffman Tree from serialized string
Node *deserializeHuffmanTree(string &serializedTree, size_t &index) {
  if (index >= serializedTree.size())
    return nullptr;

  if (serializedTree[index] == '0') {
    index++;
    return nullptr;
  }

  index++;
  Node *root = new Node(serializedTree[index]);
  index++;
  root->left = deserializeHuffmanTree(serializedTree, index);
  root->right = deserializeHuffmanTree(serializedTree, index);
  return root;
}

// Decode text using Huffman Tree and Huffman codes
string decodeText(string binaryString, Node *root) {
  string decodedText = "";
  Node *current = root;
  for (char bit : binaryString) {
    if (bit == '0') {
      current = current->left;
    } else {
      current = current->right;
    }

    if (current->left == nullptr && current->right == nullptr) {
      if (current->data != '~')
        decodedText += current->data;
      else
        decodedText += '\n';
      current = root;
    }
  }
  return decodedText;
}

string decodeBinaryData(ifstream &encodedFile, Node *root) {
  string binaryString;
  encodedFile.seekg(0, ios::end);        // Move file pointer to end of file
  size_t fileSize = encodedFile.tellg(); // Get file size
  encodedFile.seekg(0, ios::beg);        // Move file pointer back to beginning

  // Read binary data from file
  char *buffer = new char[fileSize];
  encodedFile.read(buffer, fileSize);
  encodedFile.close();

  // Convert binary data to binary string
  for (size_t i = 0; i < fileSize; ++i) {
    bitset<8> bits(buffer[i]); // Convert each byte to binary
    binaryString += bits.to_string();
  }

  delete[] buffer;

  // Find the position of the last non-zero bit
  size_t lastNonZeroPos = binaryString.find_last_not_of('0');
  if (lastNonZeroPos != string::npos) {
    binaryString.resize(lastNonZeroPos + 1); // Remove trailing zeros
  } else {
    binaryString.clear(); // Handle case where the string is all zeros
  }

  // cout <<"binaryString: ---"<<binaryString<<"---"<<endl;

  // Decode binary string using Huffman Tree
  string decodedText = decodeText(binaryString, root);
  return decodedText;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << "Usage: " << argv[0] << " <encoded_file> <tree_file> <output_file>"
         << endl;
    return 1;
  }

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
  Node *root = deserializeHuffmanTree(serializedTree, index);

  // Read encoded text from file
  ifstream encodedFile(encodedFileName,
                       ios::binary); // Open encoded file in binary mode
  if (!encodedFile) {
    cerr << "Error: Unable to open encoded file: " << encodedFileName << endl;
    return 1;
  }
  string decodedText = decodeBinaryData(encodedFile, root);

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

  cout << "Decoding completed successfully. Decoded text saved to: "
       << outputFileName << endl;

  return 0;
}
