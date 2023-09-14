#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ftw.h>

#define MAX_BUFFER_SIZE 65
#define _XOPEN_SOURCE 500

void producer(int pipe_fd[2], const char* folder_path);
void consumer(int pipe_fd[2]);


int main() {
    int pipe_fd[2];
    
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process (consumer)
        close(pipe_fd[1]); // Close the write end of the pipe
        consumer(pipe_fd);
        close(pipe_fd[0]); // Close the read end of the pipe
    } else {
        // Parent process (producer)
        close(pipe_fd[0]); // Close the read end of the pipe
        const char* folder_path = "./input"; // Change this to your input folder path
        producer(pipe_fd, folder_path);
        close(pipe_fd[1]); // Close the write end of the pipe

        // Wait for the child process to complete
        wait(NULL);
    }

    return 0;
}

void producer(int pipe_fd[2], const char* folder_path) {
    DIR* dir;
    struct dirent* ent;

    if ((dir = opendir(folder_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                char input_file_path[256];
                snprintf(input_file_path, sizeof(input_file_path), "%s/%s", folder_path, ent->d_name);
                FILE* input_file = fopen(input_file_path, "r");

                if (input_file != NULL) {
                    char buffer[MAX_BUFFER_SIZE];
                    int num_read;

                    // Store the file name without extension for naming future files
                    char *inpf = strndup(ent->d_name, strchr(ent->d_name, '.') - ent->d_name);

                    // Make a directory for these output files
                    char dirname[256]; 
                    snprintf(dirname, sizeof(dirname), "./output/%s", inpf);
                    if (mkdir(dirname, 0700))
                    {
                        // File already existed: Empty it
                        DIR *dir = opendir(dirname);
                        struct dirent *entry;

                        while ((entry = readdir(dir)) != NULL) {
                                char filePath[256];
                                snprintf(filePath, sizeof(filePath), "%s/%s", dirname, entry->d_name);
                                unlink(filePath);
                        }
                        closedir(dir);
                    }

                    while ((num_read = fread(buffer, 1, 64, input_file)) > 0) {
                        // Create a pipe to communicate with frame.c
                        int frame_pipe[2];
                        if (pipe(frame_pipe) == -1) {
                            perror("pipe");
                            exit(EXIT_FAILURE);
                        }

                        pid_t frame_pid = fork();

                        if (frame_pid == -1) {
                            perror("fork");
                            exit(EXIT_FAILURE);
                        }

                        if (frame_pid == 0) {
                            
                            // Child process (frame.c)
                            close(frame_pipe[1]);  // Close the write end of the frame pipe
                            dup2(frame_pipe[0], STDIN_FILENO);  // Redirect stdin from the frame pipe
                            close(frame_pipe[0]);  // Close the read end of the frame pipe
                            execl("frameService", "frameService", inpf, NULL);  // Execute frame.c
                            perror("execl");  // If execl fails
                            exit(EXIT_FAILURE);
                        } else {
                            // Parent process
                            close(frame_pipe[0]);  // Close the read end of the frame pipe

                            // Write data to frame.c through the frame pipe
                            write(frame_pipe[1], buffer, num_read);

                            close(frame_pipe[1]);  // Close the write end of the frame pipe
                            wait(NULL);  // Wait for frame.c to finish

                            // ** THIS IS WHERE WE WILL RIGHT TO CONSUMER!
                            // Read the framed data from the "framed_data.bin" file
                            // FILE* framed_data_file = fopen("framed_data.binf", "rb");
                            // if (framed_data_file == NULL) {
                            //     perror("fopen");
                            //     exit(EXIT_FAILURE);
                            // }

                            // char framed_buffer[100];
                            // int framed_length = fread(framed_buffer, 1, sizeof(framed_buffer), framed_data_file);
                            // fclose(framed_data_file);

                            // // Write the framed data to the main pipe
                            // write(pipe_fd[1], framed_buffer, framed_length);
                        }
                    }

                    // File has been framed, now encode it
                    int encode_pid = fork();
                    if (encode_pid == 0)
                    {
                        // Child process: Call encode then die
                        execl("encodeService", "encodeService", inpf, NULL);
                    }
                    else if (encode_pid > 0)
                    {
                        wait(NULL);
                        // Parent: completed encoding.

                    }
                    else
                    {
                        printf("Forking Error\n");
                    }

                    fclose(input_file);
                } else {
                    perror("fopen");
                }
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
    }

    // Signal the end of processing to the consumer
    close(pipe_fd[1]);
}

void consumer(int pipe_fd[2]) {

    char message[100];

    // Open the output file in the "output" folder
    char output_file_name[256]; 
    snprintf(output_file_name, sizeof(output_file_name), "./output/%s/%s.out", "file", "file");
    FILE* output_file = fopen(output_file_name, "w");

    if (output_file == NULL) {
        perror("fopen");
        return;
    }

    while (1) {
        ssize_t bytes_read = read(pipe_fd[0], message, sizeof(message));

        if (bytes_read <= 0) {
            break; // End of processing
        }

        // Write the message to the output file
        fwrite(message, 1, bytes_read, output_file);
    }

    fclose(output_file);
}
