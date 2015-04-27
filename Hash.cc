#include "Hash.h"
#include <iostream>
#include <cmath>
#include "myutil.h"

extern "C"
{
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
}

Hash::Hash() : collisions(0), ntopics(0)
{
  FILE *bin;
  struct stat buffer;
  Topic t;

  // Clear memory for safety
  memset(table, 0, sizeof table);
  // If table.bin in pwd, use it, else, forget about it
  if (stat("table.bin",&buffer) == 0) {
    bin = fopen("table.bin","rb");
    if (!bin)
      perror("fopen");
    while (!feof(bin)) {
      fread(&t, sizeof(struct Topic), 1, bin);
      if (insert(t))
        printf("hash: inserted topic: %s\n", t.title);
    }
    printf("hash: topics read from file: %d\n", ntopics);
    fclose(bin);
  }
}

Hash::~Hash()
{
  int s;

  printf("hash: writing to bfile...\n");
  s = writetobfile();
  if (s < 0)
    printf("hash: ...error writing to disk\n");
  else
    printf("hash: ...done\n");
  for (int i = 0; i < TABLESIZE; ++i)
    if (table[i] != NULL)
      delete table[i];
}

// param should be a null terminated char array
// This hash function was created by Dan Bernstein,
// and all credit goes to him. Implementation taken
// from "http://www.cse.yorku.ca/~oz/hash.html".
int Hash::djb2(const char *str)
{
  unsigned long r;
  unsigned long hash = 5381;
  int c;
  while ((c = *(str++)))
    hash = ((hash << 5) + hash) + c;
  return hash % TABLESIZE;
}

// returns -1 if topic already present or 10 topics
int Hash::insert(const Topic &topic)
{
  int x;
  Node *n;
  
  if (ntopics == MAXTOPICS) // too many topics!
    return -1;
  // add to hash table
  x = djb2(topic.title);
  if (table[x] == NULL) { // empty spot
    table[x] = new Node(topic);
    table[x]->prev = NULL;
    table[x]->next = NULL;
  } else { // a collision
    collisions++;
    n = table[x];
    while (n->next != NULL) {
      if (strcmp(topic.title,n->topic.title) == 0)
        return -1;
      n = n->next;
    }
    if (strcmp(topic.title,n->topic.title) == 0)
      return -1;
    n->next = new Node(topic);
    n->next->prev = n;
    n->next->next = NULL;
  }
  ntopics++;
  return 0;
}

// returns 0 on succes, -1 if not found
int Hash::remove(const char *title)
{
  int x;
  Node *n;

  // remove from table
  x = djb2(title);
  if (table[x] == NULL) { // not found!
    return -1;
  } else if (table[x]->next == NULL) { // no colsns
    delete table[x];
    table[x] = NULL;
    ntopics--;
    return 0;
  } else { // oh great there's collisions
    n = table[x];
    while (n != NULL &&
           strcmp(n->topic.title, title) != 0) {
      n = n->next;
    }
    if (n == NULL) { // not found!
      return -1;
    } else {
      if (n->prev) // link previous elem to next
        n->prev->next = n->next;
      else
        table[x] = n->next;
      if (n->next) // link next elem to prev
        n->next->prev = n->prev;
      delete n;
      collisions--;
      ntopics--;
      return 0;
    }
  }
}

int Hash::get(const char *title, Topic *buffer)
{
  Node *n;
  int x;

  memset(buffer, 0, sizeof(struct Topic));
  x = djb2(title);
  if (table[x] != NULL) {
    n = table[x];
    while (n != NULL && strcmp(n->topic.title, title) != 0) {
      n = n->next;
    }
    if (n == NULL) // if still not found
      return -1;
    memcpy(buffer, &(n->topic), sizeof(struct Topic));
    return 0;
  } else {
    return -1;
  }
}

int Hash::getcollisions()
{
  return collisions;
}

void Hash::print()
{
  Node *n;
  char buffer[USERLEN+POSTLEN];

  for (int i = 0; i < TABLESIZE; ++i) {
    if ((n = table[i]) != NULL) {
      while (n != NULL) {
        printf("%d: %s\n", i, n->topic.title);
        for (int j = 0; j < MAXPOSTS; ++j) {
          //post_to_string(&(n->topic.posts[j]), buffer, sizeof buffer);
          print_post(&(n->topic.posts[j]));
          //printf("  %s>\n", n->topic.posts[j].username);
          //for (int k = 0; k < POSTLEN; ++k) {
            //if (n->topic.post[j].text[k] == '\0')
          //}
        }
        n = n->next;
      }
    }
  }
  std::cout << "# topics: " << ntopics << std::endl;
  
}

// returns -1 if failure, otherwise 0 on success
int Hash::writetobfile()
{
  FILE *bin;
  Node *n;
  struct stat buffer;

  // Remove old, if not present, remove() returns -1 but doesn't crash
  if (stat("table.bin",&buffer) == 0)
    remove("table.bin");
  bin = fopen("table.bin", "wb");
  if (!bin) // failed to open file stream
    return -1;
  // next, we need to write the data from all the nodes
  // this is because the nodes are on the heap, not the stack
  for (int i = 0; i < TABLESIZE; ++i)
    if (table[i] != NULL) {
      n = table[i];
      while (n != NULL) {
        fwrite(&(n->topic), sizeof(struct Topic), 1, bin);
        n = n->next;
      }
    }
  // all done! That was easy, wasn't it?
  fclose(bin);
  return 0;
}

int Hash::getntopics()
{
  return ntopics;
}

void Hash::gettopics(char *buffer)
{
  Node *n;
  int bytes;
  int numtopics = 0;

  bytes = 0;
  memset(buffer, 0, sizeof(char)*MAXTOPICS*TITLELEN);
  for (int i = 0; i < TABLESIZE; ++i) {
    n = table[i];
    while (n != NULL) {
      if (bytes >= MAXTOPICS*TITLELEN) break;
      strncpy(&buffer[bytes],n->topic.title,TITLELEN);
      ++numtopics;
      bytes += TITLELEN;
      n = n->next;
    }
  }

  printf("Found: %d, ntopics = %d\n", numtopics, ntopics);
}

int Hash::insert(const Post &post, std::string topic)
{
  int index;

  index = djb2(topic.c_str());
  Node *n = table[index];
  Post *posts = n->topic.posts;
  for (int i = 0; i < MAXPOSTS-1; i++) {
    memcpy(&(posts[i]), &(posts[i+1]), sizeof(struct Post));
  }
  memcpy(&(posts[MAXPOSTS-1]), &post, sizeof(struct Post));
  return 0;
}
