#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>


#define MAX_CHAR 6 // struct only hold the words if letters bigger and equal to six
#define NUMBER_OF_DISPLAY 10 // user can change the number of words deplay here easily
#define MAX_WORDS_COUNT 14000 // total words count: 13972

// You may find this Useful
char * delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

int fd;
int dividedBufferSize;
int numberOfWords = 0;
pthread_mutex_t wordCountLock;


struct Words{
    char * word;
    size_t count;
} Words;

struct Words wordsList[839889]; // ensure big enought to hold words

void * storeWords()
{
    char * buff;

    // printf("dividedBufferSize: %d\n", dividedBufferSize);

    buff = malloc(dividedBufferSize);

    int readReturnValue;

    // read function returns the number of bytes readed
    readReturnValue = read(fd, buff, dividedBufferSize);
    
    // printf("read function return value: %d\n", readReturnValue);

    if (readReturnValue == -1) // error occurs on return -1
    {
        printf("ERROR OCCUR: READ FILE FAILED\n");
        exit(1);
    }
    

    int check = -1;
    int mutexLockReturnValue, mutexUnlockReturnValue;

    // break each words in buff into ptr using the delimiter
    char * ptr = strtok(buff, delim);


    /* the while loop should run until every words store in to the ptr,
       a null pointer is returned if there are no ptr left in the buff*/
    while (ptr != NULL)
    {
        int lengthOfWord = strlen(ptr);

        // keep words into struct if its letters are larger and equal than MAX_CHAR
        if (lengthOfWord >= MAX_CHAR)
        {
            // starting searching each words in the wordsList contains ptr or not
            for (int i = 0; i < numberOfWords; i++)
            {
                /* strcasesmp will compare two string without case sensitivity,
                   return value 0 -> two string equivalent with each other*/
                check = strcasecmp(wordsList[i].word, ptr);
                if (check == 0) // word already inside of the array
                {

                    /* in case mutex currently is locked by another thread,
                       this mutex lock simply to ensure an atomic update of the shared variable
                       it will block until other thread doing count words and unclock mutex*/
                    mutexLockReturnValue = pthread_mutex_lock(&wordCountLock);

                    /* pthread_mutex_lock() will return zero if completing sucessfully,
                       if-statement to check if it failed */
                    if (mutexLockReturnValue != 0)
                    {
                        printf("ERROR OCCUR: MUTEX LOCK FAILED\n");
                        exit(1);
                    }

                    // count plus one for this words
                    wordsList[i].count = wordsList[i].count + 1;
                    
                    // finish counting, unlock mutex 
                    mutexUnlockReturnValue = pthread_mutex_unlock(&wordCountLock);

                    /* pthread_mutex_unlock() will return zero if completing sucessfully,
                       if-statement to check if it failed */
                    if (mutexUnlockReturnValue != 0)
                    {
                        printf("ERROR OCCUR: MUTEX UNLOCK FAILED\n");
                        exit(1);
                    }

                    /* since we've already found the words do in the wordsList,
                       break out of the for loop and get next ptr(word) 
                       by using while loop agian */
                    break;
                }
            }

            // return non-zero value -> two string differenct between each other
            if (check != 0) // word is not in the array 
            {

                mutexLockReturnValue = pthread_mutex_lock(&wordCountLock);

                if (mutexLockReturnValue != 0) // if failed
                {
                    printf("ERROR OCCUR: MUTEX LOCK FAILED\n");
                    exit(1);
                }

                /* input this new words into wordsList
                   count the number plus one */
                strcpy(wordsList[numberOfWords].word, ptr);
                wordsList[numberOfWords].count = 1;

                mutexUnlockReturnValue = pthread_mutex_unlock(&wordCountLock);

                if (mutexUnlockReturnValue != 0) // if failed
                {
                    printf("ERROR OCCUR: MUTEX UNLOCK FAILED\n");
                    exit(1);
                }

                numberOfWords++; // count total number of words in the wordsList
            }
        }
        ptr = strtok(NULL, delim); //get next words in the file and while loop again
    }

    // printf("total number of words: %d\n", numberOfWords);  // 13972 total words

    // dynamically de-allocate the memory
    free(buff);
    buff = NULL;
}



int main (int argc, char *argv[])
{
    //***TO DO***  Look at arguments, open file, divide by threads
    //             Allocate and Initialize and storage structures

    // look at arguments
    // for (int i = 0; i < strlen(argv); i++)
    // {
    //     printf("argv %d: %s\n", i, argv[i]);
    // }
    /*
        argument result:
            argv 0: ./template_HW4_main
            argv 1: WarAndPeace.txt
            argv 2: 2
            argv 3: (null)
            argv 4: DESKTOP_SESSION=ubuntu
            argv 5: XDG_SESSION_TYPE=x11
    */

    /* argv[2] present the number of thread we need to use in this program
       atoi() function is used to convert char string to integer*/
    int threadNum = atoi(argv[2]);

    /* print out the text file's name by calling the command line arguments
       after looking at the output (shows above) found that argv[1] is presenting the file name*/
    printf("\n\nWord Frequency Count on %s with %d threads\n",argv[1], threadNum);
   
    // printf("test open function: %d\n", open(argv[1], O_RDONLY));  // file descriptor of the file
    // O_RDONLY -> open for reading only
    // fd -> file descriptor

    fd = open(argv[1], O_RDONLY);

    /* open() will return a non-negative integer representing 
       the lowest numbered unused file descriptor */
    if (fd < 0)
    {
        printf("ERROR OCCUR: FAILD FILE DESCRIPTOR\n");
        exit(1);
    }

    // get size of the file plus offset(0) bytes
    int fileSize = lseek(fd, 0, SEEK_END);

    // printf("File size: %d\n", fileSize); // file size -> 3359549

    // set to the offset(0) bytes which means set to the beginning of the file
    lseek(fd, 0, SEEK_SET);


    // devide file by thread
    /* make sure the buffer size is bigger than the result if it has decimal,
       since we may have remainder when we are doing devide by threadNum,
       decided to set up the size plus one to make sure every characters can be stored*/
    dividedBufferSize = ((fileSize / threadNum) + 1);

    // printf("buffer size: %d\n", dividedBufferSize);


    // initialize words struct to clean the garbage value
    for (int i = 0; i < MAX_WORDS_COUNT; i++)
    {
        /* initial the malloc size(in bytes) is big enough to hold the word
           the longest word in the world has 45 letters -> 45 char = 45 byte */
        wordsList[i].word = malloc(45);
    }

    // initial mutex to its defaul value
    int ret = pthread_mutex_init(&wordCountLock, NULL);
    /* ret return 0 after initial mutex completing succesfully
       other returned value indicates that an error occured */
    if (ret != 0)
    {
        printf("ERROR OCCUR: INITIAL MUTEX\n");
        exit(1);
    }


    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************
    // *** TO DO ***  start your thread processing
    //                wait for the threads to finish


    /* create array of threads since we're going to have multiple threads
       number of the threads are depends on the number in command line arg,
       so initial the size of thread array by calling threadNum -> argv[2] (shows line 185)*/
    pthread_t pthreads[threadNum];


    int threadCreateResult, threadJoinResult;
    // create independent threads, each of those will execute storeWords()
    for (int i = 0; i < threadNum; i++)
    {
        threadCreateResult = pthread_create(&pthreads[i], NULL, storeWords, NULL);

        /* pthread_create function return 0 if successfully
           check if pthread_create failed -> reutrn non-zero number*/
        if (threadCreateResult != 0)
        {
            printf("ERROR OCCUR: PTHREAD_CREATE %d FAILD\n", i);
            exit(1);
        }

        /* Wait for all threads to finish before moving on to main. If we don't wait, 
           we face the risk of executing an exit that kills the process and all running 
           threads before they've finished. */
        threadJoinResult = pthread_join(pthreads[i], NULL);

        /* pthread_join function return 0 if successfully
           check if pthread join faild -> return non-zero number */
        if (threadJoinResult != 0)
        {
            printf("ERROR OCCUR: PTHREAD_JION %d FAILD\n", i);
            exit(1);
        }
    }


    for (int i = 0; i < threadNum; i++)
    {
        
    }


    // ***TO DO *** Process TOP 10 and display


    // sort words counts highest to lowest

    int i;
    int indexOfWordWithHighestCount;
    char * currentWord;


    for (int j = 0; j < NUMBER_OF_DISPLAY; j++) {
        int count = 0;
        for (i = 0; i < numberOfWords; i++)
        {
            /* find the highest count words
               store the value and the index of the word in wordsList */
            if (wordsList[i].count > count)
            {
                count = wordsList[i].count;
                indexOfWordWithHighestCount = i;
                currentWord = wordsList[i].word;
            }
        }

        // print out the result
        printf("Number %d is %s with a count of %d\n", j+1, currentWord, count);

        // set this word's count become zero to ensure not search this word become the biggest count word again
        wordsList[indexOfWordWithHighestCount].count = 0;
    }


    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
        {
        --sec;
        n_sec = n_sec + 1000000000L;
        }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************


    // ***TO DO *** cleanup

    // close file descriptor
    close(fd);

    /* since we initialized words struct to clean the garbage value 
       by using memory location,
       do dynamically de-allocate the memory */
    for (int i = 0; i < MAX_WORDS_COUNT; i++)
    {
        free(wordsList[i].word);
        wordsList[i].word = NULL;
    }
    
    // desotry the mutex that mutex can no longer used
    pthread_mutex_destroy(&wordCountLock);
}