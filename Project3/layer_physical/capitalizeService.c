#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_QUEUE_SIZE 5
#define MAX_FRAME_SIZE 64

// The pipe to recieve lowercase data
int capitalize_pipe[2];

// Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// A queue of length 5 to fill up and then 
// dequeue to the next.
typedef struct {
    char *data;
    int head;
    int tail;
} Queue;

// To pass the prior queue and the
// vowel to change to the thread.
struct ThreadParams {
    Queue* input;
    char vowel;
};

// Initialize pointers
void initQueue(Queue* queue) {
    queue->head = 0;
    queue->tail = 0;
}

// Enqueue
void enqueue(Queue* queue, char* str) {
    queue->data = strdup(str);
    queue->tail = strlen(str);
}

// Dequeue
char* dequeue(Queue* queue) {
    if (queue->head == queue->tail) {
        // Queue is empty
        return NULL;
    }

    char* str = strdup(queue->data + queue->head);

    return str;
}

// Replace a character in the string with its uppercase
// a - A represents the difference between lowercase and capital letters
void replaceChar(char* str, char targetChar) {
    for (int i = 0; str[i] != '\0'; i++) 
            str[i] -= ('a' - 'A');
}

// The threadA, threadE etc functions
// Will replace all instances of the parameter with its capital
// and pass the result to the next thread.
void* charThread(void* args) {
    struct ThreadParams *params = (struct ThreadParams *)args;

    // Parameters: The input queue and the vowel to change
    Queue* inputQueue = params->input;
    char vowel = params->vowel;

    pthread_mutex_lock(&mutex);

    while (dequeue(inputQueue) == NULL) {
        // Wait until the queue fills up
        pthread_cond_wait(&cond, &mutex);
    }

    pthread_mutex_unlock(&mutex);

    char* inputStr = dequeue(inputQueue);

    //printf("Thread %c: %s\n", vowel, inputStr);

    // Perform replacement
    replaceChar(inputStr, vowel);

    // Enqueue the capped string to the next thread
    enqueue(inputQueue + 1, inputStr);

    pthread_cond_broadcast(&cond); // Signal waiting threads

    pthread_exit(NULL);
}

// writerThread will write results of the last queue to the server
void* writerThread(void* arg) {
    Queue* inputQueue = (Queue*)arg;
    char* outputStr;

    pthread_mutex_lock(&mutex);

    while (dequeue(inputQueue) == NULL) {
        // Wait until the queue has contents from the last queue
        pthread_cond_wait(&cond, &mutex);
    }

    pthread_mutex_unlock(&mutex);

    outputStr = dequeue(inputQueue);

    //printf("to server: %s\n", outputStr);
    write(capitalize_pipe[1], outputStr, strlen(outputStr));

    free(outputStr);

    pthread_exit(NULL);
}

// Perform the service
int capitalizeFrame(int capitalize_pipe[2])
{
    // The input: Original, decoded frame. (data section)
    char buffer[65];
    bzero(buffer, sizeof(buffer));

    // Read the encoded chunk from the consumer through the capitalize pipe
    __ssize_t num_read = read(capitalize_pipe[0], buffer, sizeof(buffer));
    buffer[sizeof(buffer)] = '\0';
    
    // The helper is done reading
    close(capitalize_pipe[0]);

    // THREADED CAP

    // Initialize queues
    Queue charQueues[6];
    for (int i = 0; i < 6; i++) {
        initQueue(&charQueues[i]);
    }
    

    // Enqueue the first frame
    enqueue(&charQueues[0], buffer);
    

    // Create threads
    pthread_t threads[5];
    
    // Vowels for each thread
    char vowels[5] = {'a', 'e', 'i', 'o', 'u'};


    // Create char threads passing the previous queue and each vowel
    for (int i = 0; i < 5; i++) {
        struct ThreadParams *params = (struct ThreadParams *)malloc(sizeof(struct ThreadParams));
        params->input = &charQueues[i];
        params->vowel = vowels[i];
        pthread_create(&threads[i], NULL, charThread, (void *)params);
    }


    // Create the writer thread
    pthread_create(&threads[5], NULL, writerThread, &charQueues[5]);

    // Wait for all threads to finish
    for (int i = 0; i < 6; i++) {
        pthread_join(threads[i], NULL);
    }

    // SIMPLE CAPITALIZATON
    // for (ssize_t i = 0; i < num_read; ++i) {
    //     char c = buffer[i];

    //     res[i] = (c >= 'a' && c <= 'z' && (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')) ? c - ('a' - 'A') : c;

    // }
    
    
    // Done writing
    close(capitalize_pipe[1]); 
    return EXIT_SUCCESS;

}


int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }

    // Set the pipe fd from execl arguments
    
    capitalize_pipe[0] = atoi(argv[1]); // Assign the first integer
    capitalize_pipe[1] = atoi(argv[2]); // Assign the second integer
    
    return capitalizeFrame(capitalize_pipe);
}
