#include <iostream>
#include <fstream>
#include <vector>
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


    return 0;
}