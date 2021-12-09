#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tareador.h>

// N and MIN must be powers of 2
long N;
long MIN_SORT_SIZE;
long MIN_MERGE_SIZE;

#define T int

void basicsort(long n, T data[n]);

void basicmerge(long n, T left[n], T right[n], T result[n*2], long start, long length);

void merge(long n, T left[n], T right[n], T result[n*2], long start, long length) {
    if (length < MIN_MERGE_SIZE*2L) {
        // Base case
        tareador_start_task("BasicMerge");
        basicmerge(n, left, right, result, start, length);
        tareador_end_task("BasicMerge");
    } else {
        // Recursive decomposition
        merge(n, left, right, result, start, length/2);
        merge(n, left, right, result, start + length/2, length/2);
    }
}

void multisort(long n, T data[n], T tmp[n]) {
    if (n >= MIN_SORT_SIZE*4L) {
        // Recursive decomposition
        multisort(n/4L, &data[0], &tmp[0]);
        multisort(n/4L, &data[n/4L], &tmp[n/4L]);
        multisort(n/4L, &data[n/2L], &tmp[n/2L]);               
        multisort(n/4L, &data[3L*n/4L], &tmp[3L*n/4L]);

        merge(n/4L, &data[0], &data[n/4L], &tmp[0], 0, n/2L);
        merge(n/4L, &data[n/2L], &data[3L*n/4L], &tmp[n/2L], 0, n/2L);

        merge(n/2L, &tmp[0], &tmp[n/2L], &data[0], 0, n);
    } else {
        // Base case
        tareador_start_task("BasicSort");
        basicsort(n, data);
        tareador_end_task("BasicSort");
    }
}

static void initialize(long length, T data[length]) {
    long i;
    for (i = 0; i < length; i++) {
        if (i==0) {
            data[i] = rand();
        } else {
            data[i] = ((data[i-1]+1) * i * 104723L) % N;
        }
    }
}

static void clear(long length, T data[length]) {
    long i;
    for (i = 0; i < length; i++) {
        data[i] = 0;
    }
}

void check_sorted(long n, T data[n])
{
    int unsorted=0;
    for (int i=1; i<n; i++)
        if (data[i-1] > data[i]) unsorted++;
    if (unsorted > 0)
        printf ("\nERROR: data is NOT properly sorted. There are %d unordered positions\n\n",unsorted);
}

int main(int argc, char **argv) {

    /* Defaults for command line arguments */
    /* Importsant: all of them should be powers of two */
    N = 32 * 1024;
    MIN_SORT_SIZE = 1024;
    MIN_MERGE_SIZE = 1024;

    /* Process command-line arguments */
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-n")==0) {
            N = atol(argv[++i]) * 1024;
        }
        else if (strcmp(argv[i], "-s")==0) {
            MIN_SORT_SIZE = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "-m")==0) {
            MIN_MERGE_SIZE = atol(argv[++i]);
        }
        else {
            fprintf(stderr, "Usage: %s [-n vector_size -s MIN_SORT_SIZE -m MIN_MERGE_SIZE]\n", argv[0]);
            fprintf(stderr, "       -n to specify the size of the vector (in Kelements) to sort (default 32)\n");
            fprintf(stderr, "       -s to specify the size of the vector (in elements) that breaks recursion in the sort phase (default 1024)\n");
            fprintf(stderr, "       -m to specify the size of the vector (in elements) that breaks recursion in the merge phase (default 1024)\n");
            return EXIT_FAILURE;
        }
    }

    fprintf(stdout, "*****************************************************************************************\n");
    fprintf(stdout, "Problem size (in number of elements): N=%ld, MIN_SORT_SIZE=%ld, MIN_MERGE_SIZE=%ld\n", N/1024, MIN_SORT_SIZE, MIN_MERGE_SIZE);
    fprintf(stdout, "*****************************************************************************************\n");

    T *data = malloc(N*sizeof(T));
    T *tmp = malloc(N*sizeof(T));

    initialize(N, data);
    clear(N, tmp);

    tareador_ON();
    multisort(N, data, tmp);
    tareador_OFF();

    check_sorted (N, data);

    fprintf(stdout, "Multisort program finished\n");
    fprintf(stdout, "*****************************************************************************************\n");
    return 0;
}