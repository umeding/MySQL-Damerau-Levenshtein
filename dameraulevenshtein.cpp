/*
 * Copyright (c) 2008 Meding Software Technik -- All Rights Reserved.
 *
 * Damerau-Levenshtein Distance Algorithm implementation as MySQL UDF
 *
 * Original Levenshtein algorithm by Joshua Drew for SpinWeb Net
 * Designs, Inc. on 2003-12-28.  Damerau extension (restricted
 * edit distance) by Dmitry Arkhipkin for JINR on 21 Nov. 2007
 *
 * Redistribute as you wish, but leave this information intact.  The
 * levenshtein function is derived from the C implementation by
 * Lorenzo Seidenari. More information about the Levenshtein
 * Distance Algorithm can be found at
 * http://www.merriampark.com/ld.htm More information on
 * Damerau-Levenshtein algrorithm can be found at
 * http://en.wikipedia.org/wiki/Damerau-Levenshtein_distance
 *
 * @author <a href="mailto:uwe@uwemeding.com">Uwe B. Meding</a>    
 */

#ifdef STANDARD
#include <string.h>
#ifdef __WIN__
typedef unsigned __int64 ulonglong;
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/
#else
#include <my_global.h>
#include <my_sys.h>
#endif
#include <mysql.h>
#include <m_ctype.h>
#include <m_string.h>

#ifdef HAVE_DLOPEN

/*
 * MySQL function declarations
 */

extern "C" {
    my_bool dameraulevenshtein_init(UDF_INIT * initid, UDF_ARGS * args,
            char *message);
    void dameraulevenshtein_deinit(UDF_INIT * initid);
    longlong dameraulevenshtein(UDF_INIT * initid, UDF_ARGS * args,
            char *is_null, char *error);
}

/**
 * Called once for each SQL statement which invokes
 * DAMERAULEVENSHTEIN(); checks arguments, sets restrictions,
 * allocates memory that will be used during the main
 * DAMERAULEVENSHTEIN() function (the same memory will be reused for
 * each row returned by the query)
 *
 * @param initid  pointer to UDF_INIT struct which is to be shared with
 *                all other functions (dameraulevenshtein() and
 *                dameraulevenshtein_deinit()) - the components of this struct are
 *                described in the MySQL manual;
 * @param args    pointer to UDF_ARGS struct which contains information
 *                about the number, size, and type of args the query will be
 *                providing to each invocation of dameraulevenshtein();
 * @param message pointer to a char array of size MYSQL_ERRMSG_SIZE in
 *                which an error message can be stored if necessary
 * @return 1 => failure; 0 => successful initialization
 */

my_bool dameraulevenshtein_init(
        UDF_INIT *initid,
        UDF_ARGS *args,
        char *message) {

    int *workspace;

    /*
     * make sure user has provided two arguments 
     */
    if (args->arg_count != 2) {
        strcpy(message, "DAMERAULEVENSHTEIN() requires two arguments");
        return 1;
    }

    /*
     * make sure both arguments are strings - they could be cast to strings,
     * but that doesn't seem useful right now 
     */
    else if (args->arg_type[0] != STRING_RESULT ||
            args->arg_type[1] != STRING_RESULT) {
        strcpy(message,
                "DAMERAULEVENSHTEIN() requires two string arguments");
        return 1;
    }

    /*
     * set the maximum number of digits MySQL should expect as the return
     * value of the DAMERAULEVENSHTEIN() function 
     */
    initid->max_length = 3;

    /*
     * dameraulevenshtein() will not be returning null 
     */
    initid->maybe_null = 0;

    /*
     * attempt to allocate memory in which to calculate dameraulevenshtein distance 
     */
    workspace = new int[(args->lengths[0] + 1) * (args->lengths[1] + 1)];

    if (workspace == NULL) {
        strcpy(message,
                "Failed to allocate memory for dameraulevenshtein function");
        return 1;
    }

    /*
     * initid->ptr is a char* which MySQL provides to share allocated memory
     * among the xxx_init(), xxx_deinit(), and xxx() functions 
     */
    initid->ptr = (char *) workspace;

    return 0;
}

/**
 * Deallocate memory allocated by dameraulevenshtein_init(); this func
 * is called once for each query which invokes DAMERAULEVENSHTEIN(),
 * it is called after all of the calls to dameraulevenshtein() are
 * done.
 *
 * @param initid to UDF_INIT struct (the same which was used by
 *               dameraulevenshtein_init() and dameraulevenshtein())
 */
void dameraulevenshtein_deinit(UDF_INIT *initid) {
    if (initid->ptr != NULL) {
        delete [] initid->ptr;
    }
}

/**
 * Compute the Damerau-Levenshtein distance (edit distance) between two
 *                    strings
 * @param initid   pointer to UDF_INIT struct which contains
 *                 pre-allocated memory in which work can be done
 * @param args     pointer to UDF_ARGS struct which contains the functions arguments and data
 *                 about them
 * @param is_null  pointer to mem which can be set to 1 if the result
 *                 is NULL
 * @param error    pointer to mem which can be set to 1 if the
 *                 calculation resulted in an error
 * @return the Damerau-Levenshtein distance between the two provided strings
 */

longlong dameraulevenshtein(
        UDF_INIT *initid,
        UDF_ARGS *args,
        char *is_null,
        char *error) {

    /*
     * s is the first user-supplied argument; t is the second
     * the Damerau-Levenshtein distance between s and t is to be computed 
     */
    const char *s = args->args[0];
    const char *t = args->args[1];

    /*
     * get a pointer to the memory allocated in dameraulevenshtein_init() 
     */
    int *d = (int *) initid->ptr;

    longlong n, m;
    int b, c, f, g, h, i, j, k, min;


    /*
     * STEP 1:
     *
     * if s or t is a NULL pointer, then the argument to which it points
     * is a MySQL NULL value; when testing a statement like:
     * SELECT DAMERAULEVENSHTEIN(NULL, 'test');
     * the first argument has length zero, but if some row in a table contains
     * a NULL value which is sent to DAMERAULEVENSHTEIN() (or some other UDF),
     * that NULL argument has the maximum length of the attribute (as defined
     * in the CREATE TABLE statement); therefore, the argument length is not
     * a reliable indicator of the argument's existence... checking for
     * a NULL pointer is the proper course of action
     */

    n = (s == NULL) ? 0 : args->lengths[0];
    m = (t == NULL) ? 0 : args->lengths[1];

    if (n != 0 && m != 0) {
        /*
         * STEP 2:
         */

        n++;
        m++;

        /* initialize first row to 0..n */
        for (k = 0; k < n; k++) {
            d[k] = k;
        }

        /* initialize first column to 0..m */
        for (k = 0; k < m; k++) {
            d[k * n] = k;
        }

        /*
         * STEP 3:
         *
         * throughout these loops, g will be equal to i minus one 
         */
        g = 0;
        for (i = 1; i < n; i++) {

            /*
             * STEP 4:
             */

            k = i;

            /* throughout the for j loop, f will equal j minus one */
            f = 0;
            for (j = 1; j < m; j++) {
                /*
                 * STEP 5, 6, 7:
                 */

                /*
                 * Seidenari's original was more like:
                 * d[j*n+i] = min(d[(j-1)*n+i]+1,
                 *                min(d[j*n+i-1]+1,
                 *                    d[(j-1)*n+i-1]+((s[i-1]==t[j-1]) ? 0 : 1)));
                 * 
                 * most (all?) redundant calculations have been
                 * removed algebraically.
                 */

                /*
                 * h = (j * n + i - n)  = ((j - 1) * n + i) 
                 */
                h = k;
                /*
                 * k = (j * n + i) 
                 */
                k += n;

                /*
                 * find the minimum among (the cell immediately above plus one),
                 * (the cell immediately to the left plus one), and (the cell
                 * diagonally above and to the left plus the cost [cost equals
                 * zero if argument one's character at index g equals argument
                 * two's character at index f; otherwise, cost is one]) 
                 * d[k] = min(d[h] + 1,
                 *           min(d[k-1] + 1,
                 *               d[h-1] + ((s[g] == t[f]) ? 0 : 1)));
                 */

                /*
                 * computing the minimum inline.
                 */

                min = d[h] + 1;
                b = d[k - 1] + 1;
                c = d[h - 1] + (s[g] == t[f] ? 0 : 1);

                if (b < min) {
                    min = b;
                }
                if (c < min) {
                    min = c;
                }

                d[k] = min;

                /* do the damerau inclusion */
                if (i > 1 && j > 1 && s[g] == t[f - 1] && s[g - 1] == t[f]) {
                    min = d[k - 2 * n - 2] + (s[g] == t[f] ? 0 : 1);
                    if (d[k] > min)
                        d[k] = min;
                }
                /* done */


                /*
                 * f will be equal to j minus one on the 
                 * next iteration of this loop 
                 */
                f = j;
            }

            /* g will equal i minus one for the next iteration */
            g = i;
        }

        return (longlong) d[k];
    } else if (n == 0) {
        return m;
    } else {
        return n;
    }
}

#endif                /* HAVE_DLOPEN */
