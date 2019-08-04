#ifndef _TRIEH_
#define _TRIEH_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "postlist.h"

typedef struct trieNode{
	char data;
	struct trieNode* thisLevel;
	struct trieNode* nextLevel;
	postNode* postList;
} trieNode;

// creates and initializes an empty trie node
trieNode* create_trieNode(char letter);
// inserts a word to the trie
void trie_insert(trieNode** root,const char* word,char* filename,char* content,int line);
// print a trie node
void print_trieNode(trieNode* node);
// insert a char to a trie level
trieNode* insert_char(trieNode** root,char letter);
// print trie
void print_trie(trieNode* root);
// return word's posting list
postNode* search_word(trieNode* root,const char* word);
// returns word's frequency in id document 
int term_frequency(trieNode* root,char* fname ,const char* word);
// teturns word's document frequency
int document_frequency(trieNode* root,const char* word);
// print a trie
void printTrie(trieNode* trie,char path[], int pathLen);
// print a char array
void printArray(char word[], int len); 
// df command
void df_All(trieNode* root,int max);
// destroy a trie
void trie_destroy(trieNode* root);
// returns the filename where word occurs the max
char* maxcount(trieNode *root,const char* word);
// returns the filename where word occurs the min
char* mincount(trieNode *root,const char* word);

#endif