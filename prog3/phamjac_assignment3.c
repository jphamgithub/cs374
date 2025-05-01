/*************************************************
 * Filename: phamjac_assignment3.c               
 * Author: Jacob Pham (phamjac)                  
 * Course: CS 374 Operating Systems              
 * Assignment: Programming Assignment 3 FileSearch 
 *
 * Description:                                  
 *   this program lets you choose a csv file of movies,
 *   then sorts movies into folders by year
 *
 * Compile:                                      
 gcc --std=gnu99 -Wall -o file_search phamjac_assignment3.c
 *
 * Run:                                          
 ./file_search
 *************************************************/

/*
Used Directories exploration a lot
https://canvas.oregonstate.edu/courses/1999732/pages/exploration-directories?module_item_id=25329373


Permission exploration
https://canvas.oregonstate.edu/courses/1999732/pages/exploration-permissions?module_item_id=25329374

And

stin/out

https://canvas.oregonstate.edu/courses/1999732/pages/exploration-stdin-stdout-stderr-and-c-i-slash-o-library?module_item_id=25329372
*/

 #include <stdio.h>      // for printf, scanf, file ops
 #include <stdlib.h>     // for memory functions and exit
 #include <string.h>     // for string handling
 #include <dirent.h>     // for reading folders
 #include <sys/types.h>  // for system data types
 #include <sys/stat.h>   // for file metadata and mkdir
 #include <unistd.h>     // for access and other posix stuff
 #include <fcntl.h>      // for file flags like O_CREAT
 #include <time.h>       // for time-related stuff like seeding rand
 
 #define PREFIX "movies_"          // files should start with this
 #define EXT ".csv"               // files should end with this
 #define MAX_LINE_LEN 1024         // max size of one line in csv
 #define MAX_FILENAME_LEN 256      // max length of file name
 #define ONID "phamjac"           // my id
 
 // function declarations up top so compiler knows about them
 char * find_largest_file (void);            // finds biggest csv file
 char * find_smallest_file (void);           // finds smallest csv file
 char * prompt_for_filename (void);          // asks user to type a name
 void process_file (const char * p_filename); // does all the work for a file
 char * create_directory (void);             // makes a new folder to store stuff
 void write_movies_by_year (const char * p_filename, const char * p_dirname); // splits movie titles by year
 int starts_with (const char * p_str, const char * p_prefix); // checks prefix
 int ends_with (const char * p_str, const char * p_suffix);   // checks suffix
 
 int main (void) // entry point for the program
 {
     int main_choice = 0; // holds what the user picks from main menu
 
     while (1) // loop until break
     {
         printf("\n1. Select file to process\n"); // menu option
         printf("2. Exit the program\n\n");         // menu option
         printf("Enter a choice 1 or 2: ");          // ask for input
         scanf("%d", &main_choice);                 // read input
 
         if (1 == main_choice) // they wanna process a file
         {
             int file_choice = 0; // file menu option
 
             while (1) // nested menu loop
             {
                 printf("\nWhich file you want to process?\n"); // ask again
                 printf("Enter 1 to pick the largest file\n");   // biggest
                 printf("Enter 2 to pick the smallest file\n");  // smallest
                 printf("Enter 3 to specify the name of a file\n\n"); // user typed
                 printf("Enter a choice from 1 to 3: ");        // prompt
                 scanf("%d", &file_choice);                    // read it
 
                 char * p_filename = NULL; // pointer to file name we'll use
 
                 if (1 == file_choice) // go find the biggest file
                 {
                     p_filename = find_largest_file();
                 }
                 else if (2 == file_choice) // go find the smallest
                 {
                     p_filename = find_smallest_file();
                 }
                 else if (3 == file_choice) // user enters file
                 {
                     p_filename = prompt_for_filename();
                     if (NULL == p_filename) // didn't work
                     {
                         continue; // try again
                     }
                 }
                 else // they typed something wrong
                 {
                     printf("You entered an incorrect choice. Try again.\n");
                     continue;
                 }
 
                 printf("Now processing the chosen file named %s\n", p_filename); // show filename
                 process_file(p_filename); // do the thing
                 free(p_filename);         // cleanup
                 p_filename = NULL;        // avoid dangling ptr
                 break;                    // done
             }
         }
         else if (2 == main_choice) // exit option
         {
             break; // stop the loop
         }
         else // wrong input
         {
             printf("You entered an incorrect choice. Try again.\n");
         }
     }
 
     return EXIT_SUCCESS; // success return
 }
 
 char * find_largest_file (void) // looks for biggest file
 {
     DIR * p_dp = opendir("."); // open current folder
     struct dirent * p_entry = NULL; // placeholder for files
     struct stat file_stat; // holds size and info
     off_t max_size = -1; // start low
     char * p_max_file = NULL; // best file so far
 
     if (NULL == p_dp) // failed to open
     {
         perror("Failed to open directory");
         return NULL;
     }
 
     while ((p_entry = readdir(p_dp)) != NULL) // loop all files
     {
         if (starts_with(p_entry->d_name, PREFIX) && ends_with(p_entry->d_name, EXT)) // only valid files
         {
             stat(p_entry->d_name, &file_stat); // get file info
             if (file_stat.st_size > max_size) // found a bigger one
             {
                 free(p_max_file); // cleanup
                 p_max_file = strdup(p_entry->d_name); // copy name
                 max_size = file_stat.st_size; // update size
             }
         }
     }
 
     closedir(p_dp); // done
     return p_max_file; // return result
 }
 
 char * find_smallest_file (void) // same but smallest
 {
     DIR * p_dp = opendir(".");
     struct dirent * p_entry = NULL;
     struct stat file_stat;
     off_t min_size = __LONG_MAX__; // start big
     char * p_min_file = NULL;
 
     if (NULL == p_dp)
     {
         perror("Failed to open directory");
         return NULL;
     }
 
     while ((p_entry = readdir(p_dp)) != NULL)
     {
         if (starts_with(p_entry->d_name, PREFIX) && ends_with(p_entry->d_name, EXT))
         {
             stat(p_entry->d_name, &file_stat);
             if (file_stat.st_size < min_size)
             {
                 free(p_min_file);
                 p_min_file = strdup(p_entry->d_name);
                 min_size = file_stat.st_size;
             }
         }
     }
 
     closedir(p_dp);
     return p_min_file;
 }
 
 char * prompt_for_filename (void) // user types filename
 {
     char filename[MAX_FILENAME_LEN]; // buffer
 
     printf("Enter the complete file name: ");
     scanf("%s", filename); // read it
 
     FILE * p_fp = fopen(filename, "r"); // try opening
     if (NULL == p_fp) // didn't work
     {
         printf("The file %s was not found. Try again\n", filename);
         return NULL;
     }
     fclose(p_fp); // close it
 
     return strdup(filename); // copy name and return
 }
 
 void process_file (const char * p_filename) // whole pipeline
 {
     if (NULL == p_filename) // invalid
     {
         return;
     }
 
     char * p_dirname = create_directory(); // make folder
     if (NULL != p_dirname)
     {
         printf("Created directory with name %s\n", p_dirname);
         write_movies_by_year(p_filename, p_dirname); // save files
         free(p_dirname); // cleanup
         p_dirname = NULL;
     }
 }
 
 char * create_directory (void) // makes new folder
 {
     srand(time(NULL)); // seed rng
     int rand_val = rand() % 100000; // random id
 
     char * p_dirname = calloc(100, sizeof(char)); // space for name
     if (NULL == p_dirname)
     {
         fprintf(stderr, "Memory allocation failed\n");
         return NULL;
     }
 
     sprintf(p_dirname, "%s.movies.%d", ONID, rand_val); // name it
     mkdir(p_dirname, 0750); // make it
     chmod(p_dirname, 0750); // set perms
 
     return p_dirname; // done
 }
 
 void write_movies_by_year (const char * p_filename, const char * p_dirname) // splits into files by year
 {
     if (NULL == p_filename || NULL == p_dirname) // bad input
     {
         return;
     }
 
     FILE * p_fp = fopen(p_filename, "r"); // open csv
     if (NULL == p_fp)
     {
         perror("Error opening file");
         return;
     }
 
     char line[MAX_LINE_LEN]; // to read each line
     fgets(line, MAX_LINE_LEN, p_fp); // skip header
 
     while (fgets(line, MAX_LINE_LEN, p_fp)) // read next
     {
         char * p_saveptr = NULL; // for strtok_r
         char * p_title = strtok_r(line, ",", &p_saveptr); // get title
         int year = atoi(strtok_r(NULL, ",", &p_saveptr)); // get year
         strtok_r(NULL, ",", &p_saveptr); // skip lang
         strtok_r(NULL, ",\n", &p_saveptr); // skip rating
 
         char path[300]; // file path buffer
         sprintf(path, "%s/%d.txt", p_dirname, year); // make path
 
         FILE * p_out = fopen(path, "a"); // open year file
         if (NULL != p_out)
         {
             fprintf(p_out, "%s\n", p_title); // write title
             fclose(p_out); // close file
             chmod(path, 0640); // set perms
         }
     }
 
     fclose(p_fp); // all done
 }
 
 int starts_with (const char * p_str, const char * p_prefix) // check prefix match
 {
     return (strncmp(p_str, p_prefix, strlen(p_prefix)) == 0);
 }
 
 int ends_with (const char * p_str, const char * p_suffix) // check suffix match
 {
     size_t len_str = strlen(p_str);
     size_t len_suffix = strlen(p_suffix);
 
     return (len_str >= len_suffix && strcmp(p_str + len_str - len_suffix, p_suffix) == 0);
 }
 
 /*** end of file ***/
