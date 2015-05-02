# Messageboard App
## How to Run
PSA: This is for Unix OS's, Windows support not yet available! Sorry!

1. Download the project files to your favorite directory (for example, ~/Downloads)
2. Use the makefile to compile files
3. Run client program, connecting to the remote server I setup on port 9000

If you're new to using the command line, here's some more specific instructions:

1. Download or clone this git repository to your Downloads folder (if you download it, make sure to decompress the zip file)

2. Open a terminal application

3. Enter this command to change directories to the project folder:

        cd ~/Downloads/Delevoryas_CSCI2270_FinalProject

4. Then enter this command into the terminal to use the makefile I provided:

        make

5. To run the client program (i.e., the program that accesses the messageboard) enter this command:

        ./client 108.61.224.250 9000
    
6. If the server is running (I leave it running all the time, occassionally rebooting it at night), you should connect successfully. If you don't for whatever reason, try running it again a few times, just in case it's a random network routing issue/etc. Once you connect successfully, the program will ask you for your username, and after that, you can enter "help" to see a list of commands you can use to browse and post on the messageboard.

# Project Summary
This library provides code for a simple messageboard server and client. The messageboard is structured as a collection of topics, each containing up to 10 posts at a time. 

The server uses a hash table to store the topics: this way, users can be served in close to O(1) time (because I use an extremely efficient hash function called djb2, and the ratio of the number of items stored to the number of spaces in my table is very small, there are usually 0 collisions). In the case of collisions, however, a linked list is used to chain elements at that index of the table.

After the server program is started, the client can connect using the IP address of the machine running the server and the port which the server is using. From there, clients can post topics, post messages within topics, view the messages in topics, and view a list of the topics that have been created.

When the server process is terminated, the server will write its current state to a binary file. Later, when the server is restarted, if the binary file is in that directory, the server will reconstruct the hash table using the binary file.

The primary features of this library are in the hash table, including basic insertion and deletion operations, as well as the more complex binary file operations. They are documented in the FunctionDocumentation.md file. The rest of the functions I use (including the various socket wrappers and server/client functions) are documented in their header and implementation files.

# Dependencies
* This program uses the standard C and C++ libraries, and it is necessary to have access to GCC for compilation.
* Serialization of transmitted bytes has not been implemented, so it is important that systems using the program are using GCC 4.8.2 (for struct padding). However, I have tried connecting from my Mac (running OS X Yosemite 10.10.2 and not using gcc but rather Apple's LLVM version 6.1.0 and clang-602.0.49) and it works just as well as when I connected using the virtual machine.
* This code was written for Unix systems, and tested on Mac OS X and Ubuntu Linux.

# System Requirements
* C/C++ compiler is required to compile the source code.
* Unix systems should be able to run the server and client programs.
* Windows users will not be able to use this library unless they change the system calls, Solaris users may have issues as well.

# Group Members
Peter Delevoryas

# Contributors

# Open Issues/Bugs
* Issue: very little error checking between processes
* Issue: if two servers are running on the same port, client does not indicate there is an issue
	-Fix: Added a vector to keep track of the ports that are currently in use and then compare the new port being created to the vector of ports to make sure it hasn't been used previously. Outputs error message if found to already be in use. One problem with my fix might be that it never removes the ports from the vector, so if one was deleted it would not remove the port from the vector. -elst0547
* Issue: Windows support
* If You have a bug to submit, add it to this portion of the README, and submit a pull request!

# More Information About the Transmission Process
This program does not use serialization because the data I'm transmitting is in the form of character arrays: even though I'm using structs, since all the fields are character arrays, it works out fine. This is of course because the read() and write() functions automatically take the the information being transmitted and convert it to network byte order, then convert it back to the native byte order on the other side. Using a different compiler might affect the struct padding and thus the sender might send a number of bytes that is smaller/larger than the expected size, and the receiver would be expecting a number of bytes that is larger/smaller than what it would receive. This would produce an error message and prevent certain functions from operating correctly. I have only tested with Apple's LLVM version 6.1.0, and GCC 4.8.2, but they both produce the same size  struct for Title and Post, so systems compiling with either should work together. I have not examined the size of the structs on Windows. To see if the program will work on your machine, write a small test program including "myutil.h", and link "myutil.cc" in the compilation of the test program. Here's what the test program should do:

            cout << sizeof(Post) << ' ' << sizeof(Test) << endl;

The result you should get, for compatability, is "2102 21121"