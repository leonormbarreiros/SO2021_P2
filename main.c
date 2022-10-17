#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include "fs/operations.h"
#include "tecnicofs-api-constants.h"

#define MAX_INPUT_SIZE 100
#define MAX_COMMANDS 10

/* mutex for helping paralel execution */
pthread_mutex_t m;
/* condition vars for helping paralel execution with a circular vector */
pthread_cond_t canInsert, canRemove;

/* number of execution threads */
int numberThreads = 0;
/* commands vector (it's a circular vector) */
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE]; 
/* number of commands in the commands vector currently */
int numberCommands = 0;
/* position in the commands vector to be executed */
int headQueue = 0;
/* position in the commands vector to be inserted */
int count = 0;
/* flag for detecting the EOF */
int finish = FALSE;



/*
 * Creates the threads vector
 * Input:
 *  - numT: number of threads (to be converted to int)
 * Returns:
 *  - tid: pointer to the threads vector
 *  - FAIL: otherwise
 */
pthread_t * create_threads_vec(char * numT) {
    pthread_t * tid;

    if((numberThreads = * numT - '0') < 1){ /* ERROR CASE */
        fprintf(stderr, "Error: Invalid number of threads.\n");
        exit(EXIT_FAILURE);
    }

    /* create the vector */
    if(!(tid = (pthread_t *) malloc(numberThreads * sizeof(pthread_t)))){
        fprintf(stderr, "Error: No memory allocated for threads.\n");
        exit(EXIT_FAILURE);
    }   

    return tid;
}

/*
 * Puts a command in the commands vector to be executed
 * Input:
 *  - data: the command to be inserted and futurely processed
 * Returns: EXIT_SUCCESS or EXIT_FAILURE
 */
int insertCommand(char* data) {

    if (pthread_mutex_lock(&m)) {
        fprintf(stderr, "Error: Unable to lock.\n");
        exit(EXIT_FAILURE);
    }
    
    /* waiting while the commands vector is full */
    while(numberCommands == MAX_COMMANDS) { 
        if (pthread_cond_wait(&canInsert, &m)) {
            fprintf(stderr, "Error: Unable to wait.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* inserting the command in the commands vector */
    strcpy(inputCommands[count], data);
    count = (count + 1) % MAX_COMMANDS;
    numberCommands++;
    
    /* allowing for a command to be executed (removed from the vector) */
    if (pthread_cond_signal(&canRemove)) {
        fprintf(stderr, "Error: Unable to signal.\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_unlock(&m)) {
        fprintf(stderr, "Error: Unable to unlock.\n");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}

/*
 * Removes a command from the commands vector, in order to execute it
 * Returns:
 *  - data: pointer to the command-string
 *  - NULL: otherwise
 */
char* removeCommand() {

    if (pthread_mutex_lock(&m)) {
        fprintf(stderr, "Error: Unable to lock.\n");
        exit(EXIT_FAILURE);
    }
    
    /* waiting while the commands vector is empty */
    while(numberCommands == 0){
        if (pthread_cond_wait(&canRemove, &m)) {
            fprintf(stderr, "Error: Unable to wait.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* removing the command from the commands vector */
    char *data = malloc(sizeof(char) * (1 + strlen(inputCommands[headQueue])));
    strcpy(data, inputCommands[headQueue]);
    headQueue = (headQueue + 1) % MAX_COMMANDS;

    numberCommands--;

    /* allowing for a command to be inserted (in the vector) */
    if (pthread_cond_signal(&canInsert)) {
        fprintf(stderr, "Error: Unable to signal.\n");
        free(data);
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_unlock(&m)) {
        fprintf(stderr, "Error: Unable to unlock.\n");
        free(data);
        exit(EXIT_FAILURE);
    }

    return data;
}

/*
 * Auxiliary funcion for error messages
 */
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/*
 * Takes the data from the input file and puts it in the commands vector
 * Input:
 *  - inputfile: name of the input file
 */
void processInput(char * inputfile) {
    char line[MAX_INPUT_SIZE];
    FILE* file;
    
    /* open the input file (...) */
    if(!(file = fopen(inputfile, "r"))) {
        perror("Could not open the input file");
        exit(EXIT_FAILURE);
    }

    /* (...) process its information (...) */
    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), file)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }

        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(!insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(!insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(!insertCommand(line))
                    break;
                return;
            
            case 'm':
                if (numTokens != 3)
                    errorParse();
                if (!insertCommand(line))
                    break;
                return;
                
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    
    /* when we're finished processing the input file */
    finish = TRUE;
    /* we can free the commands waiting to be executed */
    if (pthread_cond_broadcast(&canRemove)) {
        fprintf(stderr, "Error: Unable to broadcast\n");
        exit(EXIT_FAILURE);
    }

    /* (...) close the input file */
    if (fclose(file)) {
        perror("Could not close the input file");
        exit(EXIT_FAILURE);
    }
}

/*
 * Treats the outcome of the fs when it's done executing commands
 * Input:
 *  - outputfile: name of the output file
 */
void processOutput(char * outputfile) {
    FILE * file;

    /* open the output file*/
    if(!(file = fopen(outputfile, "w"))){
        perror("Could not open the output file");
        exit(EXIT_FAILURE);
    }

    /* print the fs tree to the outputfile*/
    print_tecnicofs_tree(file);

    /* close the output file */
    if (fclose(file)) {
        perror("Could not close the input file");
        exit(EXIT_FAILURE);
    }
}

/*
 * Executes the commands in the commands vector
 */
void applyCommands(){
    while (1){
        
        /* when it's done reading the input file and executing, we're done */
        if (finish == TRUE && numberCommands == 0)
            return;
        
        char* command = removeCommand();
        
        if (command == NULL){
            continue;
        }
        
        char token/*, type*/;
        char name[MAX_INPUT_SIZE], sec_argument[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %s", &token, name, sec_argument);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        free(command);

        int searchResult;
        switch (token) {
            case 'c':
                switch (sec_argument[0]) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;

            case 'm':
                printf("Move: %s to %s\n", name, sec_argument);
                move(name, sec_argument);
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*
 * Auxiliary function for using applyCommands with threads.
 */
void * fnThread(void * arg) {
    applyCommands();
    return NULL;
}

int main(int argc, char* argv[]) {

    int numT;
    struct timeval tv1, tv2;
    pthread_t * tid;

    /* validation of arguments */
    if(argc != 4){
        fprintf(stderr, "Expected Format: ./tecnicofs <inputfile> <outputfile> <numthreads>\n");
        exit(EXIT_FAILURE);
    }
    
    /* create the threads vector */
    tid = create_threads_vec(argv[3]);  
    
    /* init filesystem */
    init_fs();
    
    /* init condition vars */
    if (pthread_cond_init(&canInsert, NULL) || pthread_cond_init(&canRemove, NULL)) {
        fprintf(stderr, "Error: Unable to initialize condition vars.\n");
        exit (EXIT_FAILURE);
    }
    
    /* init global lock */
    if (pthread_mutex_init(&m, NULL)) {
        fprintf(stderr, "Error: Unable to initialize locks.\n");
        exit(EXIT_FAILURE);
    }
    
    /* measuring the execution time (begin time) */
    gettimeofday(&tv1, NULL);

    /* create the execution threads */
    for (numT = 0; numT < numberThreads; numT++) {
        if (pthread_create(&tid[numT], NULL, fnThread, NULL) != 0) {
            exit(EXIT_FAILURE);
        }
    }
    
    /* process input */
    processInput(argv[1]);

    /* waiting for all the threads to finish */
    for (numT = 0; numT < numberThreads; numT++) {
        if (pthread_join(tid[numT], NULL) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    /* measuring the execution time (end time) */
    gettimeofday(&tv2, NULL);

    /* calculating the execution time */
    printf ("TecnicoFS completed in %.4f seconds.\n",
         (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
         (double) (tv2.tv_sec - tv1.tv_sec));

    /* process output (results, final fs) */
    processOutput(argv[2]);

    /* release allocated memory */
    if (pthread_mutex_destroy(&m)) {
        fprintf(stderr, "Error: Unable to destroy locks.\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_destroy(&canInsert) || pthread_cond_destroy(&canRemove)) {
        fprintf(stderr, "Error: Unable to destroy condition vars.\n");
        exit(EXIT_FAILURE);
    }
    
    destroy_fs();

    exit(EXIT_SUCCESS);
}
