int service_pipe[2];
if (pipe(service_pipe) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
}

// Attempt to fork so child can exec the subroutine
fflush(stdout);
pid_t service_pid = fork();
if (service_pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
}
// service
if (service_pid == 0)
{
    // Make string versions of the pipe id's to pass to argv
    char service_read[2]; // Buffer for converting arg1 to a string
    char service_write[2]; // Buffer for converting arg2 to a string

    // Convert integers to strings
    snprintf(service_read, sizeof(service_read), "%d", service_pipe[0]);
    snprintf(service_write, sizeof(service_write), "%d", service_pipe[1]);
    
    // Child process: Call service then die
    execl("serviceService", "serviceService", service_read, service_write, NULL);
    perror("execl");  // If execl fails
    exit(EXIT_FAILURE);
}
else //parent
{
    // Write data to be serviced through service pipe
    write(service_pipe[1], data, data_len);
    close(service_pipe[1]);  // Done writing frame to be serviced

    // When child is done, read result
    wait(NULL);

    // Parent reads result from the child process (the serviced data)
    char serviced_data[RESPONSE_LEN]; // The serviced data

    // Listen for & store serviced frame
    int serviced_len = read(service_pipe[0], serviced_data, sizeof(serviced_data));
    close(service_pipe[0]);  // Done reading service data

    // Here, we have the serviced_data! Use it.

    fwrite(serviced_data, sizeof(char), serviced_data, file);

}