#include <iostream>
#include "Hash.h"
extern "C"
{
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <time.h>
#include "ChatUtil.h"
}

using std::cin;
using std::cout;
using std::endl;
using std::string;

int Socket(const char *ip, const char *port);
int Connect(const char *ip, const char *port);
void Shell();
ssize_t Request(int fd, char type, struct Post *p, struct Topic *t);
void printlisttopics(char *buffer, size_t size);

int main(int argc, char **argv)
{
  Shell();
  cout << "Goodbye!\n";
}

int Socket(const char *ip, const char *port)
{
  int sfd, n;
  struct addrinfo hints, *servinfo, *p;
  int yes=1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  //TODO FIX THIS
  if ((n = getaddrinfo(ip, port, &hints, &servinfo)) == -1) {
    perror("getaddrinfo");
    return -1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("socket");
      continue;
    }
    if (connect(sfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sfd);
      perror("connect");
      continue;
    }
    break;
  }
  if (p == NULL) {
    perror("getting socket");
    return -1;
  }
  freeaddrinfo(servinfo);
  return sfd;
}

int Connect(const char *ip, const char *port)
{
  int sfd;
  sfd = Socket(ip, port);
  if (sfd == -1)
    cout << "Error: attempt to connect to host failed\n";
  return sfd;
}

void Shell()
{
  const size_t BUFSIZE = sizeof(struct Request);
  const size_t POSTSIZE = sizeof(struct Post);
  const int INTOPIC = 1;
  const string DEFAULT = "chatclient";
  int servfd = -1;
  int bytes;
  string currenttopic = DEFAULT;
  string line;
  char ip[32];
  char port[5];
  int location = 0;
  bool running = true;
  bool connected = false;
  char buffer[POSTSIZE];
  char topicsbuffer[MAXTOPICS*TITLELENGTH];
  char request_type;
  struct Post post;
  Topic current_topic;
  
  // TODO use hashtable to implement command if/else structure
  while (running) {
    cout << currenttopic << ">";
    getline(cin,line);

    if (line == "connect") {
      memset(ip, 0, sizeof(ip));
      memset(port, 0, sizeof(port));
      cout << "chatclient: enter host ip>";
      getline(cin,line);
      strcpy(ip, line.c_str());
      cout << "chatclient: enter port>";
      getline(cin,line);
      strcpy(port, line.c_str());
      servfd = Connect(ip, port);
      if (servfd != -1)
        connected = true;
    } else if (line == "ls") {
      // get topics
      if (connected) {
        bytes = Request(servfd, GETLISTTOPICS, NULL, NULL);
        if (bytes < sizeof(struct Request)) {
          cout << "chatclient: error, request failed\n";
        } else {
          //memset(&buffer, 0, sizeof buffer); // Modifying this, trying out the first
          // version of attempting to send full list of topics
          memset(&topicsbuffer, 0, sizeof topicsbuffer);
          bytes = readn(servfd, topicsbuffer, sizeof topicsbuffer);      // TESTING SIZE
          if (bytes < sizeof topicsbuffer)
            cout << "Error! not all bytes read\n";
          else {
            printf("=======Topics=======\n");
            for (int i = 0; i < MAXTOPICS; ++i)
              if (topicsbuffer[i*TITLELENGTH] != '\0')
                printf("%d: %s\n",i,&topicsbuffer[i*TITLELENGTH]);
          }
        }
      } else {
        cout << "chatclient: currently disconnected from host\n";
      }
    } else if (line == "help") {
      // print commands
    } else if (line.substr(0,2) == "cd") {
      if (connected) {
        string requestedtopictitle = line.substr(3); // assuming that only one space cd_s
        if (requestedtopictitle.length() > 19) {
          cout << "chatclient: error, topics are 19 characters or less\n";
          continue; // skip the rest
        }
        memset(&current_topic, 0, sizeof(current_topic));
        //strncpy instead of strcpy to prevent overflow
        strncpy(current_topic.title, requestedtopictitle.c_str(),
                requestedtopictitle.size());
        // testing
        printf("chatclient: requesting topic: %s\n", current_topic.title);
        bytes = Request(servfd, GETTOPIC, NULL, &current_topic);
        if (bytes < sizeof(current_topic)) {
          cout << "chatclient: error, requesting topics, request\n";
        } else {
          // RECEIVING TOPIC STRUCT
          bytes = readn(servfd, &current_topic, sizeof current_topic);
          if (bytes < sizeof(current_topic)) {
            cout << "chatclient: error receiving topics\n";
          } else {
            // Check if topic not found 404 error
            if (strcmp(current_topic.title,"404 ERROR") == 0) {
              cout << "chatclient: 404 error, topic not found\n";
              continue;
            }
            cout << "Title: " << current_topic.title << endl;
            cout << "Messages:\n";
            //cout << current_topic.posts[0].username[0] << ": "
                 //<< current_topic.posts[0].text << '\n';
            for (int i = 0; i < MAXPOSTS; ++i) {
              if (current_topic.posts[i].username[0] != '\0') {
                cout << current_topic.posts[i].username << ": "
                     << current_topic.posts[i].text << '\n';
              }
            }
          }
        }
      } else { // not connected
        cout << "chatclient: current disconnected from host\n";
      }
    } else if (line == "exit" || line == "quit") {
      running = false;
    } else {
      cout << "chatclient: " << line << ": command not found\n";
    }
  }
  if (servfd != -1)
    close(servfd);
}

ssize_t Request(int fd, char type, struct Post *p, struct Topic *t)
{
  ssize_t bytes;
  struct Request request;
  memset(&request, 0, sizeof(struct Request));
  
  if (type == GETLISTTOPICS) {
    request.type = GETLISTTOPICS;
  } else if (type == POSTINTOPIC) {
    request.type = POSTINTOPIC;
    memcpy( &(request.post), p, sizeof(struct Post));
  } else if (type == GETTOPIC) {
    request.type = GETTOPIC;
    memcpy( &(request.topic), t, sizeof(struct Topic));
  }

  bytes = writen(fd, &request, sizeof(struct Request));
  printf("%lu/%lu bytes written\n", bytes, sizeof(struct Request));
  return bytes;
}

void printlisttopics(char *buffer, size_t size)
{
  int c;

  for (int i = 0; i < size; ++i) {
    c = buffer[i];
    if (c >= 32 && c <= 126)
      printf("%c", c);
  }
}
