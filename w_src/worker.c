#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include "../j_hdr/file.h"
#include "../w_hdr/trie.h"
#include "../w_hdr/postlist.h"

#define MSGSIZE PIPE_BUF

// Global Variables We Will Need
int rfd , wfd;
char* searchAnswer;
char msgbuf[MSGSIZE+1];

extern int trieSize; // stores trie size

// hanle deadline signal
void timeup(int signo){
	if( searchAnswer == NULL)
		strcpy(msgbuf,"Too Early\n");
	else
		strcpy(msgbuf,searchAnswer);
	if(write(wfd,msgbuf,MSGSIZE+1) < 0) // send your answers to job executor , if you have any :P
		perror("Invalid write:");
}

int main(int argc,char* argv[]){
	//printf("Got ready , my work to do is :%s\n",argv[0]);
	
	// Store all dirs into an array
	int i = 0;
    int numOfDirs = 1;
    char ** dirs = malloc(numOfDirs * sizeof(*dirs)); // store all dirs
	char* pch = strtok(argv[0],"\n"); // get dir by dir fromm workload string
	dirs[i] = malloc((strlen(pch)+1) * sizeof(char)); // add first directory
	strcpy(dirs[i],pch);
	while (1){ //keep adding dirs
		pch = strtok(NULL,"\n");
	    if( pch == NULL)
	    	break;
	    dirs = realloc(dirs, (numOfDirs+1) * sizeof(*dirs));
		++numOfDirs;
		dirs[numOfDirs-1] = malloc(strlen(pch) * sizeof(char*));
		strcpy(dirs[numOfDirs-1],pch);
	}

	// Get all files workers processes and create required structures
	trieNode* root = NULL; // trie resides here
 	int maxSize = 0; // max word size
	char wc[MSGSIZE+1]; // statistics
	int Total_numOfBytes = 0,Total_numOfWords = 0,Total_numOfLines = 0;
	struct dirent *pDirent;
	DIR *pDir;
	for(int i=0;i<numOfDirs;i++){ // open each dir
		pDir = opendir (dirs[i]);
		if (pDir == NULL){
			printf("%s: \n",dirs[i]);
		    perror("");
			continue;
		}
		FILE* fp;
		char ch;
		
		while ((pDirent = readdir(pDir)) != NULL) { // open each file in dir
    		char *filename = concat(dirs[i],pDirent->d_name); // get path name
    		if(!strcmp(pDirent->d_name,"."))
    			continue;
    		if(!strcmp(pDirent->d_name,".."))
    			continue;	
    		fp = fopen(filename,"r");
		    if (fp){
		  		char buffer[150];
		  		int line = 1;
		  		// get each line of file and store words with its metadata
    			while(fgets(buffer,150,fp)){
    				char* content = strdup(buffer); // the content where word resides
    				char* pos;
    				if ((pos=strchr(content, '\n')) != NULL) // remove end line from word
    						*pos = '\0';
        			char* pch = strtok (buffer," \t"); // start getting words
			  		// start tokenizing document
					while (pch != NULL){ // create trie word by word
			    		if( strlen(pch) > maxSize) // find the biggest word in trie
			    			maxSize = strlen(pch);
			    		char* pos;
			    		if ((pos=strchr(pch, '\n')) != NULL)
    						*pos = '\0';
			    		trie_insert(&root,pch,filename,content,line); // insert the word into the trie
			    		pch = strtok (NULL, " \t");
			  		}
			  		free(content);
			  		line++;	
			  	}
			  	rewind(fp);
			  	
		  		// Calculate statistics for those files
		    	int numOfBytes,numOfWords,numOfLines;
		    	numOfBytes = 0;
				numOfWords = 0;
				numOfLines = 0;
				// start getting characters
			    while ((ch=getc(fp)) != EOF) {
			   		// character = byte 
				    ++numOfBytes;
				    // woohoo new word
				    if (ch == ' ' || ch == '\n')
				    	++numOfWords;
					// yihaa new line
					if (ch == '\n')
						++numOfLines;
				}
				if (numOfBytes > 0) {
					++numOfLines;
					++numOfWords;	
			   	}
		    	Total_numOfLines += numOfLines;
		    	Total_numOfWords += numOfWords;
		    	Total_numOfBytes += numOfBytes;
		    }
			else
		        printf("Failed to open the file %s\n",filename);
			fclose(fp);
		}
		closedir (pDir);
	}

	// store stas so we can have'em ready for the rest of the execution
	sprintf(wc,"%d %d %d", Total_numOfBytes,Total_numOfWords,Total_numOfLines);

	// Establish signal handling
	static struct sigaction act;
	act.sa_handler = timeup;
	sigfillset (&(act.sa_mask));
	sigaction(SIGCONT,&act,NULL);

	// Create log files
	char log_dir[50];
	sprintf(log_dir,"./log/Worker_%d.txt",getpid()); // the next file where worker will write his log info
	int ldir = mkdir("./log",0777);
	FILE *lf = fopen(log_dir, "w+");
	if (!lf)
    	perror("Log File Error:");

	// Open up pipeline
	if ( (wfd = open(argv[2],O_WRONLY)) < 0){
		perror("fifo open problem");
		exit(3);
	}
	if ( (rfd = open(argv[1],O_RDONLY)) < 0){
		perror("fifo open problem");
		exit(3);
	}

	// start the communication
	while(1){
		searchAnswer = NULL;

		// read command from job Executor
		int sr = read(rfd,msgbuf,MSGSIZE+1);
		if( sr < 0){
			perror("error in read");exit(5);
		}

		char* pch = strtok(msgbuf," "); // get command

		// case search
		if(!strcmp(pch,"/search")){
			// resolve the query
			char* q[10]; // store each word to be searched here
			int n = 0;
			pch = strtok (NULL, " \n");
			int deadline = 7;
			while (pch != NULL && n != 10){
				if(!strcmp("-d",pch)){
					pch = strtok (NULL, " \n");
					deadline = atoi(pch);
					pch = strtok (NULL, " \n");
				}
				else{
	    			q[n] = pch; // store all words into an array
	    			n++;
	    			pch = strtok (NULL, " \n");
	  			}
	  		}

	  		// each worker needs random time to answer back , test deadline reasons
			srand(getpid());
			int r = rand()%7;
        	printf("Estinmation searching time:%d\n",r);
	  		sleep(r);
	  		
	  		searchNode* exists = NULL;
	  		char* Answer= "";
	  		for (int j = 0; j < n; ++j){
	  			char* temp ;
	  			postNode* search_results = search_word(root,q[j]);
	  			Answer = concat(Answer,word_details(&exists,search_results,q[j],lf));
	  		}
	  		searchAnswer = strdup(Answer);

	  		sleep(deadline); // If you are done , wait for parent to stop his clock

  			free(searchAnswer);
  			continue;
  		}
		// case maxcount
		if(!strcmp(pch,"/maxcount")){
			// resolve the Query
			pch = strtok (NULL, " \n");
			char* word = strdup(pch);
			char* answer = maxcount(root,pch); // maxcount is implemented here
  			// write back
			int maxcount = term_frequency(root,answer,pch);
			if(maxcount == 0)
				maxcount -1;
			sprintf(msgbuf,"%s %d", answer,maxcount);

			if( answer != NULL){
				//write to log the answer
				char time_of_query_arrival[30];
				time_t now = time(NULL);
				struct tm *t = localtime(&now);
				strftime(time_of_query_arrival, sizeof(time_of_query_arrival)-1, "%d %m %Y %H:%M:%S", t);
				time_of_query_arrival[30] = 0;
				fprintf(lf,"%s: %s: %s: %s\n",time_of_query_arrival,"mincount",word,answer);
				free(word);
			}

			write(wfd,msgbuf,MSGSIZE+1);

  			continue;
  		}
  		// case mincount
		if(!strcmp(pch,"/mincount")){
			pch = strtok (NULL, " \n");
			char* answer = mincount(root,pch); // mincount is implemented here
			char* word = strdup(pch);
  			// write back
			int mincount = term_frequency(root,answer,pch);
			if(mincount == 0)
				mincount = 1000000;
			sprintf(msgbuf,"%s %d", answer,mincount);

			if( answer != NULL){
				//write to log the answer
				char time_of_query_arrival[30];
				time_t now = time(NULL);
				struct tm *t = localtime(&now);
				strftime(time_of_query_arrival, sizeof(time_of_query_arrival)-1, "%d %m %Y %H:%M:%S", t);
				time_of_query_arrival[30] = 0;
				fprintf(lf,"%s: %s: %s: %s\n",time_of_query_arrival,"maxcount",word,answer);
				free(word);
			}

			write(wfd,msgbuf,MSGSIZE+1);
  			free(answer);
  			continue;
  		}
  		// case wc
		if(!strcmp(pch,"/wc\n")){
			strcpy(msgbuf,wc); // handle it
			write(wfd,msgbuf,MSGSIZE+1);
  			continue;
  		}
  		// case exit
		if(!strcmp(pch,"/exit\n")){
  			// write back
			strcpy(msgbuf,"Exiting...");
			write(wfd,msgbuf,MSGSIZE+1);	
  			break;
  		}
	  	// Unknown command
		strcpy(msgbuf,"Unknown command");
		write(wfd,msgbuf,MSGSIZE+1);	
	}

	// free what you allocated / close what you opened
	trie_destroy(root);
    for (i = 0; i < numOfDirs; i++)
        free(dirs[i]);
    free(dirs);
    fclose(lf);
	close(rfd);
	close(wfd);
}