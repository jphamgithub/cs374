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
 **********************************************************/

 #include <stdio.h>  // standard stuff
 #include <math.h>   // math functions
 
 int main() 
 {
     // declare an integer to hold how many segments the user wants to calculate.
     // use a loop to make sure it's between 2 and 10.
     int n;
 
     // This do-while loop keeps running until the user enters a valid number (2–10)
     do 
     {
         printf("How many spherical segments you want to evaluate [2-10]? \n");
 
         // scanf reads input from the user and stores it in the memory location of `n`.
         // The "%d" format specifier tells scanf to expect an int.
         scanf("%d", &n);
     } while (n < 2 || n > 10);  // Loop condition: repeat if `n` is out of range.
 
     // Declare counters and accumulators for averages.
     int count = 0;  // Counts how many valid segments we've processed
     float totalSurfaceSum = 0;  // Will accumulate total surface areas
     float totalVolumeSum = 0;   // Will accumulate total volumes
 
     // This while loop repeats until we've successfully processed `n` valid segments.
     while (count < n) {
         // Declare variables for user inputs
         float R, ha, hb;
 
         // Prompt for the current spherical segment
         printf("Obtaining data for spherical segment number %d\n", count + 1);
 
         // Ask for radius of the sphere
         printf("What is the radius of the sphere (R)? \n");
         scanf("%f", &R);  // "%f" tells scanf to expect a float
 
         // Ask for top height ha
         printf("What is the height of the top area of the spherical segment (ha)? \n");
         scanf("%f", &ha);
 
         // Ask for bottom height hb
         printf("What is the height of the bottom area of the spherical segment (hb)? \n");
         scanf("%f", &hb);
 
         // Echo input values to confirm they were read correctly
         printf("Entered data: R = %.2f ha = %.2f hb = %.2f.\n", R, ha, hb);
         // "%.2f" means print the float with 2 decimal places
 
         // Check if the inputs are valid:
         // All values must be positive, ha and hb must be ≤ R, and hb ≤ ha
         if (R <= 0 || ha <= 0 || hb <= 0 || ha > R || hb > R || hb > ha) {
             printf("Invalid Input.\n");
 
             // Skip the rest of the loop and re-ask for inputs for this segment
             continue;
         }
 
         // Declare pi as a constant float value
         float pi = 3.14159265359;
 
         // Compute the top surface area using formula: π(R² - ha²)
         float topArea = pi * (R * R - ha * ha);
 
         // Compute the bottom surface area: π(R² - hb²)
         float bottomArea = pi * (R * R - hb * hb);
 
         // Compute lateral surface area: 2πR(ha - hb)
         float lateralArea = 2 * pi * R * (ha - hb);
 
         // Total surface area = top + bottom + lateral
         float totalSurfaceArea = topArea + bottomArea + lateralArea;
 
         // Correct formula for volume of spherical segment between ha and hb:
         // V = (π/6) * [(3R - ha)*ha² - (3R - hb)*hb²]
         float volume = (pi / 6.0) * ((3 * R - ha) * ha * ha - (3 * R - hb) * hb * hb);
 
         // Print out results for this segment
         printf("Total Surface Area = %.2f Volume = %.2f.\n", totalSurfaceArea, volume);
 
         // Add current segment's results to total sums for averaging later
         totalSurfaceSum += totalSurfaceArea;
         totalVolumeSum += volume;
 
         // Increment the valid segment counter
         count++;
     }
 
     // After all valid segments are processed, calculate and print averages
     printf("Total average results:\n");
 
     // Print average surface area and volume
     printf("Average Surface Area = %.2f Average Volume = %.2f.\n",
            totalSurfaceSum / n, totalVolumeSum / n);
 
     // Return 0 to indicate the program ended successfully
     return 0;
 }
 
 /*** End of File ***/
 