#include <stdio.h>
#include <stdlib.h>

// prints contents of a file
void printfile(FILE* df);

// count lines of a file
int countlines(FILE* fp);

// merge two strings into one
char* concat(const char* s1, const char* s2);

// count words in file
int countWords(FILE* f);