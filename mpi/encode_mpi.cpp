#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <bitset>

//version=1.1.3
using namespace std;
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
Node* buildHuffmanTree(std::map<char, int>& frequencies) {
    std::priority_queue<Node*, std::vector<Node*>, compare> pq;

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
void serializeHuffmanTree(Node* root, std::ofstream& outputFile) {
    if (root == nullptr) {
        outputFile << "0"; // Represent null node
        return;
    }

    outputFile << "1" << root->data;
    serializeHuffmanTree(root->left, outputFile);
    serializeHuffmanTree(root->right, outputFile);
}

// Traverse the Huffman Tree and assign codes to each character
void assignHuffmanCodes(Node* root, std::string code, std::map<char, std::string>& codes) {
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
    MPI_Init(&argc, &argv);

    int my_rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc != 4) {
        if (my_rank == 0)
            std::cerr << "Usage: " << argv[0] << " <input_file> <huffman_tree_file> <encoded_text_file>" << std::endl;
        MPI_Finalize();
        return 1;
    }

    std::string inputFileName = argv[1];
    std::string huffmanTreeFileName = argv[2];
    std::string encodedTextFileName = argv[3];
    std::ifstream inputFile;
    int file_size;

    if (my_rank == 0) {
        inputFile.open(inputFileName);
        if (!inputFile) {
            std::cerr << "Error: Unable to open input file: " << inputFileName << std::endl;
            MPI_Finalize();
            return 1;
        }
    }

    // Read the entire file on process 0
    std::string text;
    if (my_rank == 0) {
        text = std::string((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
        inputFile.close();
    }

    // Broadcast the size of the text to all processes
    int text_size = text.size();
    MPI_Bcast(&text_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate the size of data for each process
    int chunk_size = text_size / num_procs;
    int remainder = text_size % num_procs;
    int my_chunk_size = (my_rank < remainder) ? chunk_size + 1 : chunk_size;

    // Allocate memory for receiving chunk
    char *my_chunk = new char[my_chunk_size + 1];

    // Scatter the text to all processes
    int *recvcounts = new int[num_procs];
    int *displs = new int[num_procs];
    for (int i = 0; i < num_procs; ++i) {
        recvcounts[i] = (i < remainder) ? chunk_size + 1 : chunk_size;
        displs[i] = (i == 0) ? 0 : (displs[i - 1] + recvcounts[i - 1]);
    }
    MPI_Scatterv(text.c_str(), recvcounts, displs, MPI_CHAR, my_chunk, my_chunk_size, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Add null terminator after receiving the chunk
    my_chunk[my_chunk_size] = '\0';

    // Collect frequency of characters from each process
    std::map<char, int> local_frequencies;
    for (int i = 0; i < my_chunk_size; ++i) {
        char c = my_chunk[i];
            if (c != '\n')
                local_frequencies[c]++;
            else
                local_frequencies['~']++;
    }

    // Combine frequencies from all processes
    std::vector<int> local_frequencies_vec(256, 0);
    for (const auto& pair : local_frequencies) {
        char c = pair.first;
        int count = pair.second;
        local_frequencies_vec[c] = count;
    }

    std::vector<int> global_frequencies(256, 0);
    MPI_Reduce(local_frequencies_vec.data(), global_frequencies.data(), 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Build Huffman Tree
    if (my_rank == 0) {
        std::map<char, int> frequencies;
        for (int i = 0; i < 256; ++i) {
            if (global_frequencies[i] > 0) {
                frequencies[static_cast<char>(i)] = global_frequencies[i];
            }
        }
        Node* root = buildHuffmanTree(frequencies);

        // Serialize Huffman tree and write to file
        std::ofstream treeFile(huffmanTreeFileName);
        if (!treeFile) {
            std::cerr << "Error: Unable to open Huffman tree file." << std::endl;
            MPI_Finalize();
            return 1;
        }
        serializeHuffmanTree(root, treeFile);
        treeFile.close();

        // Assign Huffman codes to characters
        std::map<char, std::string> codes;
        assignHuffmanCodes(root, "", codes);

        // Encode text using Huffman codes and write to output file
        std::ofstream outputFile(encodedTextFileName);
        if (!outputFile) {
            std::cerr << "Error: Unable to open output file." << std::endl;
            MPI_Finalize();
            return 1;
        }
        encodeText(text, codes, outputFile);
        outputFile.close();

        delete root;
    }

    // Cleanup
    delete[] my_chunk;

    MPI_Finalize();
    if (my_rank == 0) {
        cout << "Compression completed successfully." << endl;
    }
    return 0;
}

