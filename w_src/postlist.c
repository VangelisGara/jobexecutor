#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../j_hdr/file.h"
#include "../w_hdr/postlist.h"

// create a new node 
postNode* create_postNode(char* fname,char* content,int line){
	postNode* node = (postNode*)malloc(sizeof(postNode));
	node->filename = (char*)malloc((strlen(fname)+1)*sizeof(char));
	strcpy(node->filename,fname);
	node->tf = 1;
	node->next = NULL;
	node->searchList = create_searchNode(content,line);
	return node;
}

// create new search node
searchNode* create_searchNode(char* content,int line){
	searchNode* node = (searchNode*)malloc(sizeof(searchNode));
	node->content = strdup(content);
	node->line = line;
	node->next = NULL;
}

// insert a node into post list , if it doesn't exist
int insert_postNode(postNode** node,char* fname,char* content,int line){
	postNode* current = *node;
	while( current != NULL){
		if( !strcmp(current->filename,fname)){
			current->tf = current->tf + 1;
			insert_searchNode(&(current->searchList),content,line);
			return 1;
		}
		current = current->next;
	}
	//insert to postList
	postNode* newNode = create_postNode(fname,content,line);
	newNode->next = *node;
	*node = newNode;
	return 0;
}

// insert a node into search list
int insert_searchNode(searchNode** node,char* content,int line){
	searchNode* current = *node;
	while( current != NULL){
			if( current->line == line)
				if( !strcmp(current->content,content))
					return 1;
			current = current->next;
	}
	//insert to postList
	searchNode* newNode = create_searchNode(content,line);
	newNode->next = *node;
	*node = newNode;
	return 0;	
}

// print searchList
void print_searchList(searchNode* start){
	searchNode* current = start;
	while(current != NULL){
		printf(",");
		printf("  %d ", current->line);
		printf(" %s ", current->content);
		current = current->next;
	}	
	printf(".\n");
}

// print postlist
void print_postList(postNode* start){
	postNode* current = start;
	while(current != NULL){
		printf(" %s ", current->filename);
		printf("%d ", current->tf);
		print_searchList(current->searchList);
		current = current->next;
	}
}

// destroy a postlist
void destroy_postlist(postNode* head){
	if(head!=NULL){
		destroy_postlist(head->next);
		free(head->filename);
		free(head);
		destroy_searchList(head->searchList);
	}
}

// destroy a postlist
void destroy_searchList(searchNode* head){
	if(head!=NULL){
		destroy_searchList(head->next);
		free(head->content);
		free(head);
	}
}

// check if we have that line
int exists_inSearchList(searchNode* start,char* content,int line){
	searchNode* current_line = start;
	while(current_line != NULL){
	//	printf("Comparing %s %d with %s %d\n",content,line,current_line->content,current_line->line);
		if(current_line->line == line){
			if(!strcmp(current_line->content,content)){
	//			printf("exist\n");
				return 1;		
			}
		}
		current_line = current_line->next;
	}	
	//printf("doesnt exist\n");
	return 0;	
}

// remove duplicate lines
char* word_details(searchNode** existList,postNode* start,char* word,FILE* lf){
	char* answer = "";

	postNode* current_file = start;
	while(current_file != NULL){ // for each file word exists
		searchNode* current_line = current_file->searchList;
		while(current_line != NULL){
			char temp[100];	
			if(existList == NULL){
				insert_searchNode(existList,current_line->content,current_line->line);
				sprintf(temp,"%s %d %s\n",current_file->filename,current_line->line,current_line->content);
				answer = concat(answer,temp);
			}
			else if(!exists_inSearchList(*existList,current_line->content,current_line->line)){
				char temp[100];
				insert_searchNode(existList,current_line->content,current_line->line);
				sprintf(temp,"%s %d %s\n",current_file->filename,current_line->line,current_line->content);
				answer = concat(answer,temp);
			}
			
			//write to log the answer
			char time_of_query_arrival[30];
			time_t now = time(NULL);
			struct tm *t = localtime(&now);
			strftime(time_of_query_arrival, sizeof(time_of_query_arrival)-1, "%d %m %Y %H:%M:%S", t);
			time_of_query_arrival[30] = 0;
			fprintf(lf,"%s: %s: %s: %s\n",time_of_query_arrival,"search",word,current_file->filename);

			current_line = current_line->next;
		}	
		current_file = current_file->next;
	}
	return answer;
}


// get document frequency of word , using postlist
int get_df(postNode* start){
	int count = 0;
	postNode* current = start;
	while(current != NULL){
		count++; 
		current = current->next;
	}
	return count;
}