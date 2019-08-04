## Job Executor
The job executor is an engine that queries big text files. It takes a file containing a list with paths of the text files we want to query and creates a number of workers with fork() to process them.

The detailed schema that showcases how the job executor creates and communicates with the workers is the following:

![Job Executor Schema](https://github.com/VangelisGara/jobexecutor/blob/master/jobexecutor%20schema/Selection_006.png)

Along with the named pipes, the job executor communicates with the workers with low level I/O and signals that may signal time-out signals or revives dead workers.

Each worker creates a log file about the queries ran.  In the bash script folder there is a scripts that process those log files and log some further info.

## Compile & Execution

    make clean && make compile
    ./jobExecutor –d <docfile> –w <numWorkers>

Possible commands:

 - /search q1 q2 q3 ... qN –d deadline
 
Searches the files for the words given in *d* seconds, and returns the lines containing those words.


- /maxcount keyword

It returns the file containing the keyword most times.


- /maxcount keyword

It returns the file containing the keyword the least times.


- /wc

The sum of bytes, words and lines of all files given to workers


- /exit

To exit


