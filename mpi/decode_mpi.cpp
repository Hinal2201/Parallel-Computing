#include <iostream>
#include <fstream>
#include <map>
#include <bitset>
#include <mpi.h>
#include <ctime>
#include <vector>

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
    root->left = deserializeHuffmanTree(serializedTree, index);
    root->right = deserializeHuffmanTree(serializedTree, index);
    return root;
}

// Decode text using Huffman Tree and Huffman codes
string decodeText(string binaryString, Node* root) {
    string decodedText = "";
    Node* current = root;
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

string decodeBinaryData(ifstream& encodedFile, Node* root, size_t start, size_t end) {
    string binaryString;
    encodedFile.seekg(start); // Move file pointer to start position
    size_t fileSize = end - start;

    // Read binary data from file
    char* buffer = new char[fileSize];
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

    // Decode binary string using Huffman Tree
    string decodedText = decodeText(binaryString, root);
    return decodedText;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <encoded_file> <tree_file> <output_file>" << endl;
        return 1;
    }

    double startTime = MPI_Wtime();

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
    size_t chunkSize = fileSize / size;
    size_t start = rank * chunkSize;
    size_t end = (rank == size - 1) ? fileSize : (rank + 1) * chunkSize;

    // Decode binary data using Huffman Tree
    string decodedText = decodeBinaryData(encodedFile, root, start, end);
    
     int decodedsize = decodedText.size();
     
    // Gather the sizes of decoded text from all processes
    vector<int> recvCounts(size);
    
    MPI_Gather(&decodedsize, 1, MPI_INT, recvCounts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate the displacement for the gathered data
    vector<int> displs(size, 0);
    for (int i = 1; i < size; i++) {
        displs[i] = displs[i - 1] + recvCounts[i - 1];
    }

    // Gather decoded text from all processes to process 0
    string totalDecodedText;
    if (rank == 0) {
        totalDecodedText.resize(displs[size - 1] + recvCounts[size - 1]);
    }

    MPI_Gatherv(decodedText.data(), decodedText.size(), MPI_CHAR, &totalDecodedText[0], recvCounts.data(), displs.data(), MPI_CHAR, 0, MPI_COMM_WORLD);

    // Write decoded text to output file
    if (rank == 0) {
        ofstream outputFile(outputFileName);
        if (!outputFile) {
            cerr << "Error: Unable to open output file: " << outputFileName << endl;
            return 1;
        }
        outputFile << totalDecodedText;
        outputFile.close();

        // Release memory
        delete root;

        double endTime = MPI_Wtime();
        double elapsedTime = endTime - startTime;

        cout << "Decoding completed successfully. Decoded text saved to: " << outputFileName << endl;
        cout << "Time taken: " << elapsedTime << " seconds" << endl;
    }

    MPI_Finalize();

    return 0;
}
