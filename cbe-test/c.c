/* Cropped from shecc */
/*
 * shecc - Self-Hosting and Educational C Compiler.
 *
 * shecc is freely redistributable under the BSD 2 clause license. See the
 * file "LICENSE" for information on usage and redistribution of this file.
 */


/* minimal libc implementation */

#define NULL 0

#define bool _Bool
#define true 1
#define false 0

#define INT_BUF_LEN 16

typedef int FILE;

void abort();

int strlen(char *str)
{
    int i = 0;
    while (str[i])
        i++;
    return i;
}
