#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// prints contents of a file
void printfile(FILE* df)
{
	// Read contents from file
    int c = fgetc(df);
    while (c != EOF){
        printf ("%c", c);
        c = fgetc(df);
    }
    printf("\n");
    rewind(df);
}

// count the number of lines in a file
int countlines(FILE* fp)
{
	int c,count = 0;
	int emptyLine = 0;
 	// Extract characters from file and store in character c
    for (c = getc(fp); c != EOF; c = getc(fp)){
    	emptyLine++;
    	if (c == '\n' && emptyLine != 1){ // Empty lines aren't count
        	count = count + 1;
        	emptyLine = 0;
        }
	}
	rewind(fp);
    return count;
}

// concatenate 2 string into 1
char* concat(const char* s1, const char* s2){
	const size_t len1 = strlen(s1);
	const size_t len2 = strlen(s2);
	char* result = malloc((len1 + len2 + 1)*sizeof(char));
	memcpy(result,s1,len1);
	memcpy(result+len1,s2,len2+1);
    return result;
}