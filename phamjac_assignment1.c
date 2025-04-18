/*************************************************
 * Filename: phamjac_assignment1.c
 * Description: Calculates total surface area and volume of spherical segments
 * Author: Jacob Pham (phamjac)
 * Course: CS 374 - Operating Systems
 * Assignment: Programming Assignment 1 - Basic Formulas
 *
 * How to Compile:
 *   gcc --std=gnu99 -o a.out phamjac_assignment1.c -lm
 *   (The -lm links the math library for sqrt(), pow(), etc.)
 *
 * How to Run:
 *   ./a.out
 *************************************************/

 #include <stdio.h>  // standard input/output functions like printf and scanf
 #include <math.h>   // needed for math operations, like square root (not used here, but required for -lm)
 
 // Main function - where the program starts running
 int main() 
 {
     int n;  // how many spherical segments the user wants to evaluate
 
     // Ask user for a number between 2 and 10
     do 
     {
         printf("How many spherical segments you want to evaluate [2-10]? \n");
         scanf("%d", &n);  // read user input into variable 'n'
     } while (n < 2 || n > 10);  // repeat until a valid number is given
 
     // Initialize counters and totals for surface area and volume
     int count = 0;  
     double totalSurfaceSum = 0;
     double totalVolumeSum = 0;
 
     // Loop until we've collected and calculated info for 'n' valid segments
     while (count < n) {
         double R, ha, hb;
 
         // Ask for input
         printf("Obtaining data for spherical segment number %d\n", count + 1);
         printf("What is the radius of the sphere (R)? \n");
         scanf("%lf", &R);
         printf("What is the height of the top area of the spherical segment (ha)? \n");
         scanf("%lf", &ha);
         printf("What is the height of the bottom area of the spherical segment (hb)? \n");
         scanf("%lf", &hb);
 
         // Echo the inputs to confirm they were read correctly
         printf("Entered data: R = %.2f ha = %.2f hb = %.2f.\n", R, ha, hb);
 
         // Make sure the inputs are valid:
         // R, ha, and hb must all be positive
         // ha and hb must be less than or equal to R
         // hb must be less than ha (strictly)
         if (R <= 0 || ha <= 0 || hb <= 0 || ha > R || hb > R || hb >= ha) {
             printf("Invalid Input. Ensure R > 0, ha > hb > 0, and both <= R.\n");
             continue;  // skip this iteration and re-ask for input
         }
 
         double pi = 3.14159265359;
 
         // Surface area calculations using formulas
         // topArea = pi * (R^2 - ha^2)
         double topArea = pi * (R * R - ha * ha);
 
         // bottomArea = pi * (R^2 - hb^2)
         double bottomArea = pi * (R * R - hb * hb);
 
         // lateralArea = 2 * pi * R * (ha - hb)
         double lateralArea = 2 * pi * R * (ha - hb);
 
         // total surface area = top + bottom + lateral
         double totalSurfaceArea = topArea + bottomArea + lateralArea;
 
         // Volume = (1/6) * pi * h * (3a^2 + 3b^2 + h^2)
         // where a and b are the base radiuses, and h is the height between them
         double h = ha - hb;
         double a = sqrt(R * R - hb * hb);  // bottom base radius
         double b = sqrt(R * R - ha * ha);  // top base radius
         double volume = (1.0 / 6.0) * pi * h * (3 * a * a + 3 * b * b + h * h);
 
         // Print the results for this segment
         printf("Total Surface Area = %.2f Volume = %.2f.\n", totalSurfaceArea, volume);
 
         // Add to total for averaging later
         totalSurfaceSum += totalSurfaceArea;
         totalVolumeSum += volume;
 
         // One valid segment processed, move to next
         count++;
     }
 
     // Print the average values for all segments
     printf("Total average results:\n");
     printf("Average Surface Area = %.2f Average Volume = %.2f.\n",
            totalSurfaceSum / n, totalVolumeSum / n);
 
     return 0;  // indicate that program ran successfully
 }
 
 /*** End of File ***/
 