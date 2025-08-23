#include <iostream>
#include <fstream>
#include <vector>
#include "sender.hpp"
/* 
Main file for the Sender application.
This file contains the main function that initializes the application and starts the event loop.

*/
using namespace std;

/**
 * @brief Reads the entire contents of a file into a vector of characters
 * 
 * This function opens a binary file, reads its contents, and returns them
 * as a vector of characters. If the file cannot be opened, an empty vector
 * is returned and an error message is printed to the console.
 * 
 * @param filePath The path to the file to be read
 * @return vector<char> A vector containing the file contents, or empty vector on error
 * 
 * @throws None (uses error checking instead)
 */
vector<char> readFile(const string& filePath) 
{

    ifstream file(filePath, ios::binary);

    if (!file) {
        cout << "File at " << filePath << " could not be opened!" << endl;
        return {};
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    
    return buffer;
}

int main(int argc, char* argv[]) {
    // argc: number of arguments
    // argv: array of C-style strings (arguments)

    if (argc != 2) {
        cout << "Incorrect number of arguments. Expected: 2. Actual: " << argc << endl;
        return 1;
    }

    string filePath = argv[1]; 
    vector<char> fileData = readFile(filePath);

    if (fileData.empty()) {
        cout << "File is empty, nothing to send" << endl;
        return 1;  // Exit if no data to send
    }

    // Create sender object with destination IP and port (hard coded to be changed)
    try {
        Sender sender("127.0.0.1", 12345);  // localhost, port 12345 
        
        // Send the file
        bool success = sender.sendFile(fileData);
        
        if (success) {
            cout << "File sent successfully!" << endl;
        } else {
            cout << "Failed to send file!" << endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        cout << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}