#ifndef _POSTLISTH_
#define _POSTLISTH_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct searchNode{
	char* content;
	int line;
	struct searchNode* next;
}searchNode;

typedef struct postNode{
	char* filename;
	int tf;
	searchNode* searchList; 
	struct postNode* next; 
}postNode;

// creates a node for our postlist
postNode* create_postNode(char* filename,char* content,int line);
// prints postlist
void print_postList(postNode* start);
// inserts a postnode
int insert_postNode(postNode** node,char* fname,char* content,int line);
// return the size of postlist
int get_df(postNode* start);
// destroy the postlist
void destroy_postlist(postNode* head);
// creates a search node
searchNode* create_searchNode(char* content,int line);
// inserts a search node in search list
int insert_searchNode(searchNode** node,char* content,int line);
// print a search list
void print_searchList(searchNode* start);
// destroy a postlist
void destroy_searchList(searchNode* head);
// check if we have that line
int exists_inSearchList(searchNode* start,char* content,int line);
// remove duplicate lines
char* word_details(searchNode** existList,postNode* start,char* word,FILE* lf);


#endif