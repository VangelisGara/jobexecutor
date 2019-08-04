#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include "../j_hdr/file.h"

#define MSGSIZE PIPE_BUF

/* NOTES */
/* ========================================================================== */
/*    1.gdb --args ./jobExecutor -d ./inputs/docfile.txt 	                  */
/*    2.valgrind --leak-check=full -v ./jobExecutor -d ./inputs/docfile.txt   */
/* ========================================================================== */

// Global variables , so we can access from signals
int numWorkers = -1; // number of workers
int* Workers;  	// will hold the id of each worker
int* wfd;		// will hold the pipes with each worker ( sending )
int* rfd;       // will hold the pipes with each worker ( receiving )
char** workload; // will store the amount of work each worker will be assigned
int first; // Printing menu with proper menu
int killed;

// print command menu
int print_menu(){
	if(first == 0)
		first = 1;
	else{
		printf("---------------------------------------------\n");
		printf("Awaiting command:\n");
	}
	return 1;
}

// if worker-child dies , handle signal here
void revive(int sig){
	if(killed){ // invoked if child was killed
		first = 0;
		pid_t pid;
	  	pid = wait(NULL); // catch which process sent the signal
	  	int w = 0;
	  	for (int i = 0; i < numWorkers; ++i){ // find the worker
	  		if(Workers[i] == pid)
	  			break;
	  		w++;
	   	}
	 	
	 	printf("Worker %d with pid %d exited.\n",w,pid);
	  	printf("But I will revive it\n");

	   	int workerpid;
		if( (workerpid = fork()) == 0 ){ // Create him again
			char in[20];
			char out[20];
			sprintf(in,"worker%d-in",w);	
			sprintf(out,"worker%d-out",w);
			execl("./worker",workload[w],in,out,NULL);
			exit(-1);
		}

		Workers[w] = workerpid; // store the new one

	 	// open up again the pipeline with the new worker so we can communicate
	 	char name[20];
		sprintf(name,"worker%d-out",w);
		if( (rfd[w] = open(name,O_RDONLY)) < 0){
				perror("fifo open problem");
				exit(3);
		}
		sprintf(name,"worker%d-in",w);
		if( (wfd[w] = open(name,O_WRONLY)) < 0){
			perror("fifo open problem");
			exit(3);
		}

	  	sleep(3);
	  	printf("Done\n");
		printf("---------------------------------------------\n");
		printf("Awaiting command:\n");
	}
}

// signal end of deadline signal
void timeup ( int signum ) {
	printf ( "Time's up\n");
}

int main(int argc,char* argv[]){
 	int i;

	// Initialization of program
 	FILE *df; // docfile
 	int input = 0; // got input
	for(i = 0; i < argc; i++){ // get arguments
		if(!strcmp(argv[i],"-w"))
			numWorkers = atoi(argv[++i]);
		if(!strcmp(argv[i],"-d")){
			df = fopen(argv[++i], "r"); // open our file
			if ( df == NULL ){
				printf("Error opening docfile\n"); // error handling
				return -1;
			}
			input = 1 ; // we got a proper file ready
		}
	}
	if(input != 1){
		printf("Please give docfile\n"); // error handling
		return -1;
	}
	if(numWorkers < 0){
		printf("Give number of workers\n");
		exit(9);
	}
	int numDirectories = countlines(df)+1;
	if(numWorkers > numDirectories) // if we have more workers than directories , fire them
		numWorkers = numWorkers - (numWorkers - numDirectories);
	
	// Divide workload and assign it to workers balanced
	//printf("Dividing work...\n");
	int work[numWorkers]; // will hold the number of directores each worker will get
	for(i=0;i<numWorkers;i++)
		work[i] = 0;
	int worker = 0;
	while( numDirectories > 0){ // count them the number of directories they'll get
		if(worker == numWorkers)
			worker = 0;
		work[worker]++;
		numDirectories--;
		worker++;
	}
	workload = (char**)calloc(numWorkers,sizeof(char*)); // will hold the directories each worker will get 
	size_t len = 0;
	ssize_t length ;
	int l = 0;
	for (int i =0; i < numWorkers; i++){ // for each worker
		char* result = "" ;
		char* temp;
		for (int j=0; j < work[i]; j++){
			char* buffer = NULL;
			length = getline(&buffer,&len,df);
			if( length != -1){
				temp = result;	
				result = concat(result,buffer); // start appending directories worker will get
				if( j != 0)
					free(temp);
			}
			free(buffer);
		}
		workload[i] = result;
	}	
  	rewind(df);

	// Create workers and open up the pipeline
	//printf("Creating pipeline...\n");
	for(int i=0; i< numWorkers; i++){
		char name[20];
		sprintf(name,"worker%d-in",i);
		if(mkfifo(name,0666) == -1){
			printf("make fifo error\n");
		}
		sprintf(name,"worker%d-out",i);
		if(mkfifo(name,0666) == -1){
			printf("make fifo error\n");
		}
	}
	Workers = (int*)malloc(numWorkers*sizeof(int)); // store the ids of each workers
	// create workers
	//printf("Creating processes...\n");
	pid_t workerpid;
	pid_t jobExecutorpid;
	// start N workers and give them parts of the docfile
	for(i=0; i<numWorkers; i++){
		sleep(0.5);
		if( (workerpid = fork()) == 0 ) // child starts working
			break;
		Workers[i] = workerpid; // store worker's id
	}
	if ( workerpid == 0){ // worker
		char in[20];
		char out[20];
		sprintf(in,"worker%d-in",i);	
		sprintf(out,"worker%d-out",i);
		execl("./worker",workload[i],in,out,NULL); // start worker and give workload with argument
		exit(-1);
	}
	else{ // job Exec
		//printf("jobExecutor with pid %d is here , too\n",getpid());
		sleep(1);
	}

	// open pipelines
	///printf("Open pipeline...\n");
	wfd = (int*)malloc(numWorkers*sizeof(int)); // store the write file descriptors
	rfd = (int*)malloc(numWorkers*sizeof(int)); // store the write file descriptors
	for(int i=0; i< numWorkers; i++){
		char name[20];
		sprintf(name,"worker%d-out",i);
		if( (rfd[i] = open(name,O_RDONLY)) < 0){
			perror("fifo open problem");
			exit(3);
		}
		sprintf(name,"worker%d-in",i);
		if( (wfd[i] = open(name,O_WRONLY)) < 0){
			perror("fifo open problem");
			exit(3);
		}
	}

    // establish signal handler
    signal(SIGALRM,timeup);
	signal(SIGCHLD,revive);

	// Start getting commands and sending them to workers
	char *command = NULL;
	char msgbuf[MSGSIZE+1];
    len = 0;
	first = 1;
	killed = 1;
	while(print_menu() && (length= getline(&command,&len,stdin) != -1)){
		printf("---------------------------------------------\n");
		// input error handling
		if ( strlen(command) > MSGSIZE){
			fflush(stdout);
			printf("Command too big\n");				
			continue;
		}

		char *temp = strdup (command);
		char* pch = strtok(temp," "); // get command
		
		// case search
		if(!strcmp(pch,"/search")){
			// resolve the Query
			pch = strtok (NULL, " \n"); // start getting queries
			int n = 0;
			pch = strtok (NULL, " \n");
			int deadline = 7;
			while (pch != NULL && n != 10){
				if(!strcmp("-d",pch)){ // get deadline argument
					pch = strtok (NULL, " \n");
					if( pch == NULL)
						break;
					deadline = atoi(pch);
					pch = strtok (NULL, " \n");
				}
				else
	    			pch = strtok (NULL, " \n");
	  		}
	  		if( deadline <= 0){
	  			printf("deadline should be greater than one\n");
	  			continue;
	  		}
			printf("Workers you have %d seconds\n", deadline);	// inform workers about their deadline
  			// send command to workers
			for (int i = 0; i < numWorkers; ++i){
				strcpy(msgbuf,command);
				if( (write(wfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("Error in writing"); exit(2);
				}
			}

			// Start countdowning for the deadline
			alarm (deadline) ;
			pause () ;

			printf ( "Let's see what you did!\n");
			for (int  k= 0; k <numWorkers; ++k)
			   	kill(Workers[k],SIGCONT); // inform each worker that their time is up

  			char* Answers = "";
  			int answered = 0;
			// read answers from workers
			for (int i = 0; i < numWorkers; ++i){
				if ((read(rfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("error in reading");
					exit(5);
				}
				if(strcmp(msgbuf,"Too Early\n")){ // see who answered
					answered++;
					char* temp ;
					temp = Answers;
					Answers = concat(Answers,msgbuf);
					if( i != 0){
						if( temp != NULL)
							free(temp);
						temp = NULL;
					}
				}
			}
			printf("%d/%d Answered\n",answered,numWorkers);
			if( answered > 0){
				printf("%s\n",Answers);
				if(Answers != NULL)
			  		free(Answers);
			}
  		}
  		// case maxcount
		else if(!strcmp(pch,"/maxcount")){
			// send command to workers
			for (int i = 0; i < numWorkers; ++i){
				strcpy(msgbuf,command);
				if( (write(wfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("Error in writing"); exit(2);
				}
			}

			// read answers from workers
			int Max = -1;
			char Answer[MSGSIZE+1];
			for (int i = 0; i < numWorkers; ++i){
				int temp;
				char answer[MSGSIZE+1];
				if ((read(rfd[i],msgbuf,MSGSIZE+1)) < 0){ // get workers statistics
					perror("error in reading");
					exit(5);
				}
				// get the answer
				sscanf(msgbuf,"%s %d",answer,&temp);
				if( temp > Max){ // find the max
					Max = temp;
					strcpy(Answer,answer);
				}
				else if( temp == Max){
					if(strcmp(answer,Answer) < 0)
						strcpy(Answer,answer);
				}
			}
			if( Max == 0)
				printf("Word doesn't exist\n");
			else
				printf("%s , %d \n",Answer,Max);
		}
		// mincount case
		else if(!strcmp(pch,"/mincount")){
			// send command to workers
			for (int i = 0; i < numWorkers; ++i){
				strcpy(msgbuf,command);
				if( (write(wfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("Error in writing"); exit(2);
				}
			}

			// read answer from workers
			int Min = 100000;
			char Answer[MSGSIZE+1];
			for (int i = 0; i < numWorkers; ++i){
				int temp;
				char answer[MSGSIZE+1];
				if ((read(rfd[i],msgbuf,MSGSIZE+1)) < 0){ // get workers statistics
					perror("error in reading");
					exit(5);
				}
				// get answer
				sscanf(msgbuf,"%s %d",answer,&temp);
				if( temp < Min){ // find min
					Min = temp;
					strcpy(Answer,answer);
				}
				else if( temp == Min){
					if(strcmp(answer,Answer) < 0){
						strcpy(Answer,answer);
					}
				}
			}
			if ( Min == 100000)
				printf("word doesn't exist\n");
			else
				printf("%s , %d \n",Answer,Min);
		}
		// wc case
		else if(!strcmp(pch,"/wc\n")){
			// send command to workers
			for (int i = 0; i < numWorkers; ++i){
				strcpy(msgbuf,command);
				if( (write(wfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("Error in writing"); exit(2);
				}
			}		
			// read answers from workers
			int Total_Bytes =0 , Total_Words =0 , Total_Lines =0;
			for (int i = 0; i < numWorkers; ++i){
				int b,w,l;
				if ((read(rfd[i],msgbuf,MSGSIZE+1)) < 0){ // get workers statistics
					perror("error in reading");
					exit(5);
				}
				// get the answer and sum them
				sscanf(msgbuf,"%d %d %d",&b,&w,&l);
				Total_Lines += l;
				Total_Words += w;
				Total_Bytes += b;
			}
			printf("Statistics of all files: \n Bytes: %d \n Lines: %d \n Words: %d\n",Total_Bytes,Total_Lines,Total_Words);
		}
		// extra case ,kill.
		else if(!strcmp(command,"/kill\n")){
			killed = 1;
			kill(Workers[1], SIGTERM);
		}
		// exit case
		else if(!strcmp(command,"/exit\n")){
			killed = 0; // we are just exiting
			// send command to workers
			for (int i = 0; i < numWorkers; ++i){
				strcpy(msgbuf,command);
				if( (write(wfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("Error in writing"); exit(2);
				}
			}

			for (int i = 0; i < numWorkers; ++i){
				// read here
				if ((read(rfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("error in reading");
					exit(5);
				}
			}
			printf("Exiting\n");
			free(temp);
			break;
		}
		// unknown command case
		else{
			// send command to workers
			for (int i = 0; i < numWorkers; ++i){
				strcpy(msgbuf,command);
				if( (write(wfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("Error in writing"); exit(2);
				}
			}

			// read answer from workers
			for (int i = 0; i < numWorkers; ++i){
				if ((read(rfd[i],msgbuf,MSGSIZE+1)) < 0){
					perror("error in reading");
					exit(5);
				}
			}
			printf("Unknown command\n");
		}

		free(temp);
		char *command = NULL;
		sleep(1);
	}

	// free and closes
	free(command);
	for (int i = 0; i < numWorkers; ++i){
		close(rfd[i]);
		close(wfd[i]); 

		char name[20];		
		sprintf(name,"worker%d-in",i);
		unlink(name);
		sprintf(name,"worker%d-out",i);
		unlink(name);		

		if( workload[i] != "")
			free(workload[i]);
	}

	free(wfd);
	free(rfd);
	free(workload);
 	fclose(df);
}