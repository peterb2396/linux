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
#define NEW_FILE_NAME 28

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
    FILE *doneFile;
    FILE *binfFile;

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

                    // Notify consumer to create chage their file reference / make the new files
                    sprintf(buffer, "%c%s", NEW_FILE_NAME, inpf);
                    write(pipe_fd[1], buffer, sizeof(buffer));

                    // close old files, making new ones
                    if (binfFile)
                        fclose(binfFile);
                    if (doneFile)
                        fclose(doneFile);

                    // prepare file.binf
                    char binf_file_name[50]; 
                    snprintf(binf_file_name, sizeof(binf_file_name), "./output/%s/%s.binf", inpf, inpf);
                    binfFile = fopen(binf_file_name, "w");

                    if (binfFile == NULL) {
                        perror("Error opening binf file");
                        fclose(input_file); // close the input fd to avoid mem leaks
                        return;
                    }

                    // prepare file.done
                    char done_file_name[50]; 
                    snprintf(done_file_name, sizeof(done_file_name), "./output/%s/%s.done", inpf, inpf);
                    doneFile = fopen(done_file_name, "w");

                    if (doneFile == NULL) {
                        perror("Error opening done file");
                        fclose(input_file); // close the input fd to avoid mem leaks
                        return;
                    }


                        // *** ALL FILES ARE NOW CREATED AND REFERENCED ***


                    // Read the input in chunks of 64, pipe & fork to frame.
                    while ((num_read = fread(buffer, 1, 64, input_file)) > 0) {
                        
                        // Create a pipe to communicate with frame.c
                        int frame_pipe[2];
                        if (pipe(frame_pipe) == -1) {
                            perror("pipe");
                            exit(EXIT_FAILURE);
                        }
                        fflush(stdout);
                        pid_t frame_pid = fork();

                        if (frame_pid == -1) {
                            perror("fork");
                            exit(EXIT_FAILURE);
                        }

                        if (frame_pid == 0) {

                            char frame_read[2]; // Buffer for converting arg1 to a string
                            char frame_write[2]; // Buffer for converting arg2 to a string

                            // Convert integers to strings
                            snprintf(frame_read, sizeof(frame_read), "%d", frame_pipe[0]);
                            snprintf(frame_write, sizeof(frame_write), "%d", frame_pipe[1]);
                            
                            // Child process (frame.c)
                            execl("frameService", "frameService", frame_read, frame_write, NULL);  // Execute frame.c
                            perror("execl");  // If execl fails
                            exit(EXIT_FAILURE);
                        } else {
                            // Parent process
                            // Write data to be framed to frame.c through the frame pipe
                            write(frame_pipe[1], buffer, num_read);
                            close(frame_pipe[1]);  // Close the write end of the frame pipe

                            // When child is done, read
                            wait(NULL);
                            // Parent reads result from the child process (the new frame)
                            char frame[68]; // The frame to be recieved will be stored here

                            // Listen for frame result
                            int frame_len = read(frame_pipe[0], frame, sizeof(frame));
                            close(frame_pipe[0]);  // Close the read end of the frame pipe

                            // Null-terminate
                            if (frame_len > 0) {
                                if (frame_len < sizeof(frame)) {
                                    frame[frame_len] = '\0';
                                } else {
                                    frame[sizeof(frame) - 1] = '\0';
                                }
                            }
                            // At this point, we have recieved the frame and can encode it.
                            //printf("%s %d\n", frame, frame_len);

                            // Pipe before forking to share a pipe for 
                            // transmission of encoding data
                            int encode_pipe[2];
                            if (pipe(encode_pipe) == -1) {
                                perror("pipe");
                                exit(EXIT_FAILURE);
                            }
                            
                            // Attempt to fork so child can exec the subroutine
                            fflush(stdout);
                            pid_t encode_pid = fork();
                            if (encode_pid == -1) {
                                perror("fork");
                                exit(EXIT_FAILURE);
                            }
                            // Encode this single frame
                            if (encode_pid == 0)
                            {
                                // Make string versions of the pipe id's to pass to argv
                                char encode_read[2]; // Buffer for converting arg1 to a string
                                char encode_write[2]; // Buffer for converting arg2 to a string

                                // Convert integers to strings
                                snprintf(encode_read, sizeof(encode_read), "%d", encode_pipe[0]);
                                snprintf(encode_write, sizeof(encode_write), "%d", encode_pipe[1]);
                                
                                // Child process: Call encode then die
                                execl("encodeService", "encodeService", encode_read, encode_write, NULL);
                                perror("execl");  // If execl fails
                                exit(EXIT_FAILURE);
                            }
                            else
                            {
                                // Write data to be encoded through encode pipe
                                write(encode_pipe[1], frame, frame_len);
                                close(encode_pipe[1]);  // Done writing frame to be encoded

                                // When child is done, read result
                                wait(NULL);

                                // Parent reads result from the child process (the encoded frame)
                                char encoded_frame[67 * 8]; // The encoded frame
                                    // NOTE 67 (frame len) * 9 with spaces, *8 without, is perfect amount

                                // Listen for & store encoded frame
                                int encoded_len = read(encode_pipe[0], encoded_frame, sizeof(encoded_frame));
                                close(encode_pipe[0]);  // Done reading encode data

                                // Here, we have the encoded_frame!
                                // Write the encoded frame to the file, AND to the consumer to decode!
                                fwrite(encoded_frame, sizeof(char), encoded_len, binfFile);
                                write(pipe_fd[1], encoded_frame, encoded_len);
                      
                            }
                            

                            //wait(NULL);  // Wait for frame.c to finish

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
    
    // Define file pointers
    FILE* outfFile;
    FILE* chckFile;

    char message[67 * 8]; // encoded stream (usually) from main pipe
    char *inpf;           // name of input file currently being processed

    

    while (1) {
        ssize_t bytes_read = read(pipe_fd[0], message, sizeof(message));

        if (bytes_read <= 0) {
            break; // End of processing
        }

        // Check if we recieved special signal to change the target file
        // Must signal this for each input file that we begin to process
        if ((int)message[0] == NEW_FILE_NAME)
        {
            // New file name is a pointer to the string beginning after the control char
            inpf = &message[1]; 
            
            // Create the new consumer files

            // prepare the outf file in the "output" folder
            char outf_file_name[256]; 
            snprintf(outf_file_name, sizeof(outf_file_name), "./output/%s/%s.outf", inpf, inpf);
            outfFile = fopen(outf_file_name, "w");


            if (outfFile == NULL) {
                perror("fopen");
                return;
            }

            // prepare file.chck
            char chck_file_name[50]; 
            snprintf(chck_file_name, sizeof(chck_file_name), "./output/%s/%s.chck", inpf, inpf);
            chckFile = fopen(chck_file_name, "w");

            if (chckFile == NULL) {
                perror("Error opening binf file");
                return;
            }

        }
        else // we are reading data
        {
                    // *** .outf PROCESS ***

                // DECODE

            // Pipe before forking to share a pipe for 
            // transmission of data
            int decode_pipe[2];
            if (pipe(decode_pipe) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            
            // Attempt to fork so child can exec the subroutine
            fflush(stdout);
            pid_t decode_pid = fork();
            if (decode_pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            // Decode this single frame
            if (decode_pid == 0)
            {
                // Make string versions of the pipe id's to pass to argv
                char decode_read[2]; // Buffer for converting arg1 to a string
                char decode_write[2]; // Buffer for converting arg2 to a string

                // Convert integers to strings
                snprintf(decode_read, sizeof(decode_read), "%d", decode_pipe[0]);
                snprintf(decode_write, sizeof(decode_write), "%d", decode_pipe[1]);
                
                // Child process: Call encode then die
                execl("decodeService", "decodeService", decode_read, decode_write, NULL);
                perror("execl");  // If execl fails
                exit(EXIT_FAILURE);
            }
            else // Parent
            {
                // Write data to be decoded through decode pipe
                write(decode_pipe[1], message, bytes_read);
                close(decode_pipe[1]);  // Done writing frame to be decoded

                // When child is done, read result
                wait(NULL);

                // Parent reads result from the child process (the decoded frame)
                char decoded_frame[67]; // The decoded frame is 1/8 the size
                    // NOTE 67 (frame len) * 9 with spaces, *8 without, is perfect amount

                // Listen for & store decoded frame
                int decoded_len = read(decode_pipe[0], decoded_frame, sizeof(decoded_frame));
                close(decode_pipe[0]);  // Done reading encode data

                // Here, we have the decoded frame!

                // Now, it's time to deframe it back to a chunk.

                // Create a pipe to communicate with deframe.c
                int deframe_pipe[2];
                if (pipe(deframe_pipe) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
                fflush(stdout);
                pid_t deframe_pid = fork();

                if (deframe_pid == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }

                if (deframe_pid == 0) {
                    char deframe_read[2]; // Buffer for converting arg1 to a string
                    char deframe_write[2]; // Buffer for converting arg2 to a string

                    // Convert integers to strings
                    snprintf(deframe_read, sizeof(deframe_read), "%d", deframe_pipe[0]);
                    snprintf(deframe_write, sizeof(deframe_write), "%d", deframe_pipe[1]);
                    
                    // Child process (deframe.c)
                    execl("deframeService", "deframeService", deframe_read, deframe_write, NULL);  // Execute deframe.c
                    perror("execl");  // If execl fails
                    exit(EXIT_FAILURE);
                } else {
                    // Parent process
                    // Write data to be framed to deframe.c through the deframe pipe
                    write(deframe_pipe[1], decoded_frame, decoded_len);
                    close(deframe_pipe[1]);

                    // When child is done, read chunk (ctrl chars removed)
                    wait(NULL);
                    char chunk[65]; // The frame to be recieved will be stored here

                    int chunk_len = read(deframe_pipe[0], chunk, sizeof(chunk));
                    close(deframe_pipe[0]); 
                    

                    // Null-terminate
                    if (chunk_len > 0) {
                        if (chunk_len < sizeof(chunk)) { //64 < 65
                            chunk[chunk_len] = '\0';
                        } else {
                            printf("WARNING! chunk size too small, overwrote last char. len: %d\n", chunk_len);
                            chunk[sizeof(chunk) - 1] = '\0';
                        }
                    }


                    // Create a pipe to communicate with capitalize.c
                    int uppercase_pipe[2];
                    if (pipe(uppercase_pipe) == -1) {
                        perror("pipe");
                        exit(EXIT_FAILURE);
                    }
                    fflush(stdout);
                    pid_t uppercase_pid = fork();

                    if (uppercase_pid == -1) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    }

                    if (uppercase_pid == 0) {

                        char uppercase_read[2]; // Buffer for converting arg1 to a string
                        char uppercase_write[2]; // Buffer for converting arg2 to a string

                        // Convert integers to strings
                        snprintf(uppercase_read, sizeof(uppercase_read), "%d", uppercase_pipe[0]);
                        snprintf(uppercase_write, sizeof(uppercase_write), "%d", uppercase_pipe[1]);
                        
                        // Child process (uppercase.c)
                        execl("uppercaseService", "uppercaseService", uppercase_read, uppercase_write, NULL);  // Execute deframe.c
                        perror("execl");  // If execl fails
                        exit(EXIT_FAILURE);
                    } else {
                        // Parent process
                        // Write chunk to be capped to uppercase.c through the uppercase pipe
                        write(uppercase_pipe[1], chunk, chunk_len);
                        close(uppercase_pipe[1]); 

                        // When child is done, read chunk (capitalized)
                        wait(NULL);
                        char cap_chunk[65]; // The capital chunk

                        int cap_chunk_len = read(uppercase_pipe[0], cap_chunk, sizeof(cap_chunk));
                        close(uppercase_pipe[0]); 

                        // Null-terminate
                        // if (chunk_len > 0) {
                        //     if (chunk_len < sizeof(chunk)) { //64 < 65
                        //         chunk[chunk_len] = '\0';
                        //     } else {
                        //         printf("WARNING! chunk size too small, overwrote last char");
                        //         chunk[sizeof(chunk) - 1] = '\0';
                        //     }
                        // }

                        // Penultimately, write to .outf
                        fwrite(cap_chunk, 1, cap_chunk_len, outfFile);


                        // *** .chck PROCESS ***

                        // First, frame the data chunk again


                        // Next, encode it


                        // Then, write chunk to .chck file


                        // Finally, send the same chunk through pipe back to producer
                    
                    
                    }
                
            }

            
        }
        

        
    }
    
    }
    // Finally, close the last opened files
    fclose(outfFile);
    fclose(chckFile);
}
