/* 
Main file for the Receiver application.
This file contains the main function that initializes the application and starts the event loop.
*/
#include "../logger.cpp"
#include "receiver.hpp"

int main() {
    // argc: number of arguments
    // argv: array of C-style strings (arguments)
    // if (argc != 2) 
    // {
    //     LOG_ERROR("There must be exactly one argument: path to save the received file.");
    //     std::cout << "Usage: " << argv[0] << " <output_file_path>" << std::endl;
    //     return 1; 
    // } 
    // Receiver receiver(argv[1]);
    // receiver.receiveFile();
    Receiver receiver;
    bool recieved = receiver.receiveFile();
    if (recieved == false) 
    {
        LOG_ERROR("Unable to receive file"); 
        return 1; 
    }
    return 0;
}
