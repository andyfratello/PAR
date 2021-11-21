/*
 * OpenMP Mandelbrot program
 *
 * This program computes and displays all or part of the Mandelbrot
 * set. By default, it examines all points in the complex plane
 * that have both real and imaginary parts between -2 and 2.
 * Command-line parameters allow zooming in on a specific part of
 * this range.
 *
 * Basic usage:
 *   mandel [-d -i maxiter -c x0 y0 -s size -w windowsize]
 * where
 *   maxiter denotes the maximum number of iterations at each point -- by default 1000
 *   x0, y0, and size specify the range to examine (a square
 *     centered at (x0 + iy0) of size 2*size by 2*size -- by default,
 *     a square of size 4 by 4 centered at the origin)
 *   windowsize denotes the size of the image (diplay window) to compute
 *
 * Output: if -d option is used, a graphical display as described in
 *         Wilkinson & Allen, is displayed using the X Window system,
 *         plus text output to standard output showing the above parameters,
 *         plus execution time in seconds.
 *
 * Additional parameters:
 *   If -h option is used, the program computes the histogram of values in the image
 *   If -o option is used, the program saves output and histogram (if computed) to file
 *
 * Code based on the original code from Web site for Wilkinson and Allen's
 * text on parallel programming:
 * http://www.cs.uncc.edu/~abw/parallel/par_prog/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <malloc.h>

#include "omp.h"

// include files to visualize computed image
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

// include file and stuff to measure execution times
#include <sys/time.h>
double getusec_() {
  struct timeval time;
  gettimeofday(&time, NULL);
  return ((double)time.tv_sec * (double)1e6 + (double)time.tv_usec);
}

double stamp;
#define START_COUNT_TIME stamp = getusec_();
#define STOP_COUNT_TIME(_m) stamp = getusec_() - stamp;\
                        stamp = stamp/1e6;\
                        printf ("%s %0.6f\n",(_m), stamp);

// Default values for things
#define N           2           /* size of problem space (x, y from -N to N) */
#define NPIXELS     800         /* size of display window in pixels */

// Structure definition for complex numbers
typedef struct {
    double real, imag;
} complex;

// Functions for GUI
#include "mandelbrot-gui.h"     /* has setup(), interact() */

// Global variables to output results
// output to file
int output2file = 0;
FILE *fp = NULL;

// output to display
int output2display = 0;
Display *display;
Window win;
GC gc;
int setup_return;
long min_color = 0, max_color = 0;
double scale_color;
double scale_real, scale_imag;

// output as histogram
int output2histogram = 0;
int * histogram;

// dummy parameter
int user_param = 1;

void mandelbrot(int height, int width, double real_min, double imag_min,
                double scale_real, double scale_imag, int maxiter, int **output) {

    // Calculate points and generate appropriate output
    #pragma omp parallel
    #pragma omp single
    for (int row = 0; row < height; ++row) {
		#pragma omp taskloop num_tasks(user_param)
        for (int col = 0; col < width; ++col) {
            complex z, c;

            z.real = z.imag = 0;

            /* Scale display coordinates to actual region  */
            c.real = real_min + ((double) col * scale_real);
            c.imag = imag_min + ((double) (height-1-row) * scale_imag);
                                        /* height-1-row so y axis displays
                                         * with larger values at top
                                         */

            // Calculate z0, z1, .... until divergence or maximum iterations
            int k = 0;
            double lengthsq, temp;
            do  {
                temp = z.real*z.real - z.imag*z.imag + c.real;
                z.imag = 2*z.real*z.imag + c.imag;
                z.real = temp;
                lengthsq = z.real*z.real + z.imag*z.imag;
                ++k;
            } while (lengthsq < (N*N) && k < maxiter);

	        output[row][col]=k;

            if (output2histogram)
	        {
            #pragma omp atomic
            histogram[k-1]++;
            }
		
            if (output2display) {
                /* Scale color and display point  */
                long color = (long) ((k-1) * scale_color) + min_color;
                {
                    if (setup_return == EXIT_SUCCESS) {
                        #pragma omp critical
                        {
                            XSetForeground (display, gc, color);
                            XDrawPoint (display, win, gc, col, row);
                
                        }
                    }	
                }
            }
    }
}

int main(int argc, char *argv[]) {
    int maxiter = 1000;
    double real_min;
    double real_max;
    double imag_min;
    double imag_max;
    int width  = NPIXELS;         // dimensions of display window
    int height = NPIXELS;
    double size = N, x0 = 0, y0 = 0;
    int ** output;
    char filename[32];

    // fake parallel region to delimit the start of the program (for instrumentation purposes)
    #pragma omp parallel
    ;

    // Process command-line arguments
    for (int i=1; i<argc; i++) {
	      if (strcmp(argv[i], "-d")==0) {
			      output2display = 1;
	      }
	      else if (strcmp(argv[i], "-h")==0) {
			      output2histogram = 1;
	      }
	      else if (strcmp(argv[i], "-i")==0) {
			      maxiter = atoi(argv[++i]);
	      }
	      else if (strcmp(argv[i], "-w")==0) {
			      width = atoi(argv[++i]);
			      height = width;
	      }
	      else if (strcmp(argv[i], "-c")==0) {
			      x0 = atof(argv[++i]);
			      y0 = atof(argv[++i]);
	      }
	      else if (strcmp(argv[i], "-u")==0) {
			      user_param = atof(argv[++i]);
	      }
	      else if (strcmp(argv[i], "-s")==0) {
			      size = atof(argv[++i]);
	      }
	      else if (strcmp(argv[i], "-o")==0) {
                              output2file = 1;
			      sprintf(filename, "output_omp_%d.out", omp_get_max_threads());
    			      if((fp=fopen(filename, "wb"))==NULL) {
				      fprintf(stderr, "Unable to open file\n");
				      return EXIT_FAILURE;
			      }
	      }
	      else {
		      fprintf(stderr, "Usage: %s [-o -h -d -i maxiter -w windowsize -c x0 y0 -s size]\n", argv[0]);
		      fprintf(stderr, "       -o to write computed image and histogram to disk (default no file generated)\n");
		      fprintf(stderr, "       -h to produce histogram of values in computed image (default no histogream)\n");
		      fprintf(stderr, "       -d to display computed image (default no display)\n");
		      fprintf(stderr, "       -i to specify maximum number of iterations at each point (default 1000)\n");
		      fprintf(stderr, "       -w to specify the size of the image to compute (default 800x800 elements)\n");
		      fprintf(stderr, "       -c to specify the center x0+iy0 of the square to compute (default origin)\n");
		      fprintf(stderr, "       -s to specify the size of the square to compute (default 2, i.e. size 4 by 4)\n");
        	      return EXIT_FAILURE;
	      }
    }
    real_min = x0 - size;
    real_max = x0 + size;
    imag_min = y0 - size;
    imag_max = y0 + size;

    // Produce text output
    fprintf(stdout, "\n");
    fprintf(stdout, "Computation of the Mandelbrot set with:\n");
    fprintf(stdout, "    center = (%g, %g) \n    size = %g\n    maximum iterations = %d\n",
            (real_max + real_min)/2, (imag_max + imag_min)/2,
            (real_max - real_min)/2,
            maxiter);
    fprintf(stdout, "\n");

    output = malloc(height*sizeof(int *));
    for (int row = 0; row < height; ++row)
	    output[row] = malloc(width*sizeof(int));

    if (output2histogram) histogram = calloc(maxiter, sizeof(int));

    if (output2display) {
        /* Initialize for graphical display */
        setup_return =
            setup(width, height, &display, &win, &gc, &min_color, &max_color);
        if (setup_return != EXIT_SUCCESS) {
            fprintf(stderr, "Unable to initialize display, continuing\n");
            return EXIT_FAILURE;
       }
    }

    // Compute factors to scale computational region to window
    scale_real = (double) (real_max - real_min) / (double) width;
    scale_imag = (double) (imag_max - imag_min) / (double) height;

    if (output2display) {
        // Compute factor for color scaling
        scale_color = (double) (max_color - min_color) / (double) (maxiter - 1);
    }

    // Start timing
    if (!output2display) {
        START_COUNT_TIME;
    }

    mandelbrot(height, width, real_min, imag_min, scale_real, scale_imag, maxiter, output);

    // End timing
    if (!output2display) {
        STOP_COUNT_TIME("Total execution time (in seconds):");
        fprintf(stdout, "\n");
    }

    fprintf(stdout, "Mandelbrot set: Computed\n");
    if (output2histogram) fprintf(stdout, "Histogram for Mandelbrot set: Computed\n");
    else fprintf(stdout, "Histogram for Mandelbrot set: Not computed\n");

    // Make sure all output is written
    if ((output2display) && (setup_return == EXIT_SUCCESS)) XFlush (display);

    if ((output2file) && (fp != NULL)) {
        fprintf(stdout, "Writing output file to disk: %s\n", filename);
        for (int row = 0; row < height; ++row)
            if(fwrite(output[row], sizeof(int), width, fp) != width)
	        fprintf(stderr, "Error when writing output to file\n");
        if (output2histogram)
            if(fwrite(histogram, sizeof(int), maxiter, fp) != maxiter)
	        fprintf(stderr, "Error when writing histogram to file\n");
    }

    if (output2display) {
        /* Wait for user response, then exit program */
        if (setup_return == EXIT_SUCCESS) {
            interact(display, &win, width, height,
                real_min, real_max, imag_min, imag_max);
        }
        return EXIT_SUCCESS;
    }

    // fake parallel region to delimit the end of the program (for instrumentation purposes)
    #pragma omp parallel
    ;
}