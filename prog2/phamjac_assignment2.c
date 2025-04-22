/*************************************************
 * Filename: phamjac_assignment2.c
 * Author: Jacob Pham (phamjac)
 * Course: CS 374 - Operating Systems
 * Assignment: Programming Assignment 2 - Movies
 *
 * Description:
 *   This program reads a movie dataset from a CSV file and builds a 
 *   linked list of `movie_t` structs dynamically. It then offers a menu 
 *   interface for querying by release year, language, or by best-rated
 *   movie per year
 *   
 *   It demonstrates core C concepts such as file I/O,
 *   pointers, structs, linked lists, and dynamic memory.
 *
 * How to Compile:
 *   gcc --std=gnu99 -Wall -o movies phamjac_assignment2.c
 *
 * How to Run:
 *   ./movies movies_sample_1.csv
 * 
 * Notes:
 *   - Used syntax tips from BARR C i.e. p_ pp_ fp_ for pointers:
 *     Used this in Army and helps keep track of pointers for me
 *     https://barrgroup.com/sites/default/files/barr_c_coding_standard_2018.pdf
 *   - getline + file opening logic was originally shown in `movies.c`
 *   - linked list + malloc pattern adapted from Exploration: Memory Allocation in Canvas
 *   https://canvas.oregonstate.edu/courses/1999732/pages/exploration-memory-allocation?module_item_id=25329354
 *
 *************************************************/

 #include <stdio.h>      // for printf, scanf, getline
 #include <stdlib.h>     // for calloc, free, EXIT_SUCCESS, EXIT_FAILURE
 #include <string.h>     // for strtok_r, strdup, strcmp
 
 #define MAX_LANGUAGES 5
 #define MAX_LANGUAGE_LENGTH 20
 
/* Each movie with all its attributes much like an Object in Python*/  
 typedef struct movie
 {
     char * p_title;
     int release_year;
     char * pp_languages[MAX_LANGUAGES]; // list of languages, like [English;Spanish]
     float rating;
     struct movie * p_next; // next node in the list
 } movie_t;
 
 /* same deal as malloc example in exploration 
 setting up a node with heap mem 
 Return a pointer to movie node after creation*/
 movie_t * create_movie_node(char * p_title, int release_year, char ** pp_languages, float rating)
 {
     movie_t * p_new_movie = calloc(1, sizeof(movie_t));
     if (NULL == p_new_movie)
     {
         fprintf(stderr, "couldn’t make space for a new movie node\n");
         return NULL;
     }
 
     p_new_movie->p_title = strdup(p_title);
     p_new_movie->release_year = release_year;
 
     for (int index = 0; index < MAX_LANGUAGES && pp_languages[index] != NULL; index++)
     {
         p_new_movie->pp_languages[index] = strdup(pp_languages[index]);
     }
 
     p_new_movie->rating = rating;
     p_new_movie->p_next = NULL;
 
     return p_new_movie;
 }
 
/* this rips out the [English;French] and gives us a clean array of strings */
void parse_languages(char * p_lang_field, char ** pp_output_langs)
{
    // strdup makes a copy of the original string (heap-allocated) so we don't mess up the original
    // strdup is short for "string duplicate"  allocates memory and copies the content
    char * p_copy = strdup(p_lang_field);

    // strtok_r needs this to save where it left off between calls (it's re-entrant version of strtok safe in loops)
    char * p_save_ptr = NULL;

    // strcspn returns the index of the first '[' in the string — we replace it with '\0' to cut it off
    // basically, this makes sure p_copy doesn't contain any leading bracket
    p_copy[strcspn(p_copy, "[")] = '\0';

    // strchr finds the first '[' in the original string, then we jump one char forward to skip it
    char * p_start = strchr(p_lang_field, '[') + 1;

    // strchr again to find the closing bracket ']'
    char * p_end = strchr(p_start, ']');

    // if we found the end bracket, replace it with null terminator to cut off the rest
    if (p_end != NULL)
        *p_end = '\0';

    // strtok_r splits the string using ';' as the delimiter
    // returns the first chunk before the first semicolon
    // p_save_ptr remembers where we left off so we can get the next token later
    char * p_token = strtok_r(p_start, ";", &p_save_ptr);

    int lang_index = 0;

    // loop through each token and copy it into the output array
    // we make a separate strdup so each language has its own memory on the heap
    while ((NULL != p_token) && (lang_index < MAX_LANGUAGES))
    {
        // strdup allocates a new copy of the language string so it doesn't rely on temp memory
        pp_output_langs[lang_index] = strdup(p_token);
        lang_index++;

        // get the next token (next language)
        p_token = strtok_r(NULL, ";", &p_save_ptr);
    }

    // cleanup: we free the string we strdup’d earlier (p_copy)
    // even though we didn't use it for parsing, we allocated it so we have to clean it up
    free(p_copy);
}
 
 /* this clears out all the memory in the list. again, 
 just like exploration example. 
 avoid memory leaks on program shut down*/
 void free_movie_list(movie_t *p_head)
 {
     movie_t *p_temp = NULL;
 
     while (NULL != p_head)
     {
         p_temp = p_head;
         p_head = p_head->p_next;
 
         free(p_temp->p_title);
         for (int i = 0; i < MAX_LANGUAGES && p_temp->pp_languages[i] != NULL; i++)
             free(p_temp->pp_languages[i]);
 
         free(p_temp);
     }
 }
 
 /* this came straight from movies.c — adapted it to build up our own linked list */
movie_t *load_movies_from_csv(char *p_filename, int *p_total_count)
{
    // try to open the file in read mode
    FILE *p_file = fopen(p_filename, "r");

    // if file doesn't open properly, bail out
    if (NULL == p_file)
    {
        fprintf(stderr, "couldn't open that file!\n");
        return NULL;
    }

    // buffer that will hold one full line of text from the file
    char *p_curr_line = NULL;

    // getline needs us to pass in the size of the buffer — this will get updated by getline
    size_t line_length = 0;

    // 'read' will store how many characters were read by getline
    ssize_t read;

    // flag to skip the first line (the header row in the CSV)
    int is_first_line = 1;

    // start of the linked list
    movie_t *p_head = NULL;

    // end of the linked list (lets us add to the tail easily)
    movie_t *p_tail = NULL;

    // we're gonna count how many movies we successfully parsed
    *p_total_count = 0;

    // read the file line-by-line
    while ((read = getline(&p_curr_line, &line_length, p_file)) != -1)
    {
        // first line is just column headers — skip it
        if (is_first_line)
        {
            is_first_line = 0;
            continue;
        }

        // we'll use this to keep track of strtok_r's internal state
        char *p_save_ptr = NULL;

        // first chunk of data: movie title
        char *p_title = strtok_r(p_curr_line, ",", &p_save_ptr);

        // if for some reason there’s no title, skip this line
        if (NULL == p_title)
            continue;

        // next chunk: year as a string (need to convert it later)
        char *p_year_str = strtok_r(NULL, ",", &p_save_ptr);

        // turn year from string to int
        int release_year = atoi(p_year_str);

        // next chunk: the languages field (like [English;French])
        char *p_langs_str = strtok_r(NULL, ",", &p_save_ptr);

        // we'll use this to store individual language strings
        char *pp_langs[MAX_LANGUAGES] = {NULL};

        // turn [English;French] into { "English", "French", NULL, ... }
        parse_languages(p_langs_str, pp_langs);

        // last chunk: rating (as a float like 8.7)
        char *p_rating_str = strtok_r(NULL, "\n", &p_save_ptr);
        float rating = strtof(p_rating_str, NULL);

        // now that we have all fields, build a new node on the heap
        movie_t *p_new_node = create_movie_node(p_title, release_year, pp_langs, rating);

        // if something went wrong, skip this one
        if (NULL == p_new_node)
            continue;

        // if this is the first valid movie, it becomes both head and tail
        if (NULL == p_head)
            p_head = p_tail = p_new_node;
        else
        {
            // otherwise, we add to the end of the list and move the tail forward
            p_tail->p_next = p_new_node;
            p_tail = p_new_node;
        }

        // keep track of how many good movies we added to the list
        (*p_total_count)++;
    }

    // getline allocates memory for the line buffer, so we free it here
    free(p_curr_line);

    // done reading — close the file
    fclose(p_file);

    // return the head of our linked list (could be NULL if nothing parsed)
    return p_head;
}
 
 /* this one loops through the list and prints movies from a specific year */
void print_movies_by_year(movie_t *p_head, int target_year)
{
    int found = 0; // flag to check if we printed anything

    // go through each node in the linked list
    for (movie_t *p_curr = p_head; p_curr != NULL; p_curr = p_curr->p_next)
    {
        // check if the year matches what the user asked for
        if (p_curr->release_year == target_year)
        {
            // print the movie title
            printf("%s\n", p_curr->p_title);
            found = 1; // yep, we found at least one
        }
    }

    // if we didn't find any movie from that year, say so
    if (!found)
    {
        printf("No data about movies released in the year %d\n", target_year);
    }
}


/* this one finds the highest-rated movie for each year */
void print_highest_rated_by_year(movie_t *p_head)
{
    // this goes year-by-year from 1900 to 2025 (as per assignment spec)
    // not super efficient — we're looping over the list for every year
    for (int year = 1900; year <= 2025; year++)
    {
        movie_t *p_best = NULL; // will point to the best-rated movie found so far for this year

        // go through the list again and look for movies from this year
        for (movie_t *p_curr = p_head; p_curr != NULL; p_curr = p_curr->p_next)
        {
            if (p_curr->release_year == year)
            {
                // if this is the first match or has a better rating than current best, update p_best
                if (p_best == NULL || p_curr->rating > p_best->rating)
                {
                    p_best = p_curr;
                }
            }
        }

        // if we found at least one movie for this year, print the best one
        if (p_best != NULL)
        {
            printf("%d %.1f %s\n", p_best->release_year, p_best->rating, p_best->p_title);
        }
    }
}


/* this one shows movies that were available in a specific language */
void print_movies_by_language(movie_t *p_head, const char *p_target_language)
{
    int found = 0; // again, flag to check if we found anything

    // loop through the list of all movies
    for (movie_t *p_curr = p_head; p_curr != NULL; p_curr = p_curr->p_next)
    {
        // each movie might have multiple languages, so we loop through those
        for (int i = 0; i < MAX_LANGUAGES && p_curr->pp_languages[i] != NULL; i++)
        {
            // do a case-sensitive string comparison 
            if (strcmp(p_curr->pp_languages[i], p_target_language) == 0)
            {
                // found a match, print the year and title
                printf("%d %s\n", p_curr->release_year, p_curr->p_title);
                found = 1; // yep, found something
                break;     // no need to check more languages for this movie
            }
        }
    }

    // if we never hit a match, tell the user
    if (!found)
    {
        printf("No data about movies released in %s\n", p_target_language);
    }
}
 
int main(int argc, char **pp_args)
{
    // check if the user gave us a filename (we expect: ./movies somefile.csv)
    if (argc < 2)
    {
        // let them know what they did wrong and how to fix it
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./movies movies_sample_1.csv\n");
        return EXIT_FAILURE; // standard error code for bad run
    }

    // this will hold how many movies we loaded
    int total_movies = 0;

    // call the CSV loading function and also give it a pointer to total_movies
    // so it can update it for us as it reads the file
    // good example of passing around pointers vs global variables
    movie_t * p_movie_list = load_movies_from_csv(pp_args[1], &total_movies);

    // if loading failed or list came back empty, bail
    if (NULL == p_movie_list)
    {
        fprintf(stderr, "Failed to process file or no movies parsed.\n");
        return EXIT_FAILURE;
    }

    // success! show how many movies we parsed
    printf("Processed file %s and parsed data for %d movies\n", pp_args[1], total_movies);

    // start a menu loop for the user to pick options
    int user_choice = 0;
    while (1)
    {
        // print out the menu
        printf("\n1. Show movies released in the specified year\n");
        printf("2. Show highest rated movie for each year\n");
        printf("3. Show the title and year of release of all movies in a specific language\n");
        printf("4. Exit from the program\n\n");
        printf("Enter a choice from 1 to 4: ");

        // grab their menu choice
        scanf("%d", &user_choice);

        // option 1: filter by year
        if (user_choice == 1)
        {
            int target_year;
            printf("Enter the year for which you want to see movies: ");
            scanf("%d", &target_year); // get the year they want
            print_movies_by_year(p_movie_list, target_year); // call helper to print matches
        }
        // option 2: show top-rated movie per year
        else if (user_choice == 2)
        {
            print_highest_rated_by_year(p_movie_list);
        }
        // option 3: filter by language
        else if (user_choice == 3)
        {
            char input_language[MAX_LANGUAGE_LENGTH]; // where we'll put their input
            printf("Enter the language for which you want to see movies: ");
            scanf("%s", input_language); // grab a string from the user
            print_movies_by_language(p_movie_list, input_language); // show results
        }
        // option 4: exit the loop and end the program
        else if (user_choice == 4)
        {
            break;
        }
        // catch-all: user entered something dumb like 9
        else
        {
            printf("You entered an incorrect choice. Try again.\n");
        }
    }

    // before we quit, clean up the memory we allocated for the movie list
    free_movie_list(p_movie_list);

    return EXIT_SUCCESS; // everything went fine turns
}
/*** End of File ***/
