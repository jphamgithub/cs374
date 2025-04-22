#include <stdio.h>
#include <stdlib.h> // for EXIT_SUCCESS and EXIT_FAILURE

/*
 * Function: processMovieFile
 *   Opens a file, reads and prints each line
 *   filePath: path to the file
 * 
 *  This function shows sample code that opens a file, then in a loop reads and prints each line in that file.
 *  You can use this code however you want in your Prog Assignment 2.
 */
 void processMovieFile(char* filePath){
    char *currLine = NULL;
    size_t len = 0;

    // Open the specified file for reading only
    FILE *movieFile = fopen(filePath, "r");

    // Read the file line by line
    while(getline(&currLine, &len, movieFile) != -1)
    {
        printf("%s", currLine);
    }

    // Free the memory allocated by getline for currLine
    free(currLine);
    // Close the file
    fclose(movieFile);
    printf("\nProcessed file %s\n", filePath);
}


/**
 * 
 */
int main ( int argc, char **argv ){
    if (argc < 2)
    {
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./movies movies.csv\n");
        return EXIT_FAILURE;
    }
    processMovieFile(argv[1]);
    return EXIT_SUCCESS;
}