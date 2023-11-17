#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



// Perform the service
int capitalizeFrame(int capitalize_pipe[2])
{
    // The input: Original, decoded frame. (data section)
    char buffer[65];
    bzero(buffer, sizeof(buffer));

    // The output: Capitalized, decoded frame
    char res[65];
    

    // Read the encoded chunk from the consumer through the capitalize pipe
    __ssize_t num_read = read(capitalize_pipe[0], buffer, sizeof(buffer));
    buffer[sizeof(buffer)] = '\0';
    
    // The helper is done reading
    close(capitalize_pipe[0]);

    // FUNCTION HERE: PERFORM CAPITALIZATON
    for (ssize_t i = 0; i < num_read; ++i) {
        char c = buffer[i];

        res[i] = (c >= 'a' && c <= 'z' && (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')) ? c - ('a' - 'A') : c;

    }
    
    

    // Write the result
    write(capitalize_pipe[1], res, strlen(res));
    
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
    int capitalize_pipe[2];
    capitalize_pipe[0] = atoi(argv[1]); // Assign the first integer
    capitalize_pipe[1] = atoi(argv[2]); // Assign the second integer
    
    return capitalizeFrame(capitalize_pipe);
}
