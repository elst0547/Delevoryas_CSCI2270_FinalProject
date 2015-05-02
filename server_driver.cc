#include "server_session.h"
#include <signal.h>
#include <iostream>

server_session *ssn; // Must be declared globally for signal handler to work
std::vector<String> ports; //Must be global in order to keep track of all ports for all servers.

// Handles kill signal, ensuring that the server sessions's table destructor is called
// This is important, as otherwise the binary file will not be written before the program
// is terminated.
void ExitHandler(int x)
{
  delete ssn;
  exit(0);
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    std::cerr << "Command line arguments required! 1. Port Number" << std::endl;
    exit(1);
  }

  bool check = false; //Needed for comparison in port duplication error checking
  static struct sigaction _sigact;
  signal(SIGINT, ExitHandler); // SIGINT = CTRL-C
  signal(SIGTERM, ExitHandler); // SIGTERM = kill pid
  std::cout << "Server session being created..." << std::endl;
  String port = argv[1]; //Saves the command line argument for port number to a variable
  if(ports.size() > 0) //Checks if list of currently used ports is empty or not
  {
    for(int i = 0; i < ports.size(); i++) //Used to check if port is in list of currently used ports
    {
      if(ports.get(i) == port)
      {
        cout<<"There is another server already using the same port. This may cause problems during use."<<endl;
        check = true;
      }
    }
  }
  ssn = new server_session(port); // Make a new server session with specified port from command line arg
  if (ssn->is_valid()) { // if valid, initiate the client handling process
    ports.push_back(port); //Adds the port to the list of currently used ports after sucessful creation of the server
    std::cout << "Server session created successfully, accepting clients..." << std::endl;
    ssn->init_chat();
  } else { // else, write error message, delete object, exit program
    std::cout << "Error creating server session, quitting..." << std::endl;
  }
  delete ssn;
}