static int b;
int a = 900;
int **d = 0;

enum {
    A,
    B
} as = A;

char dc = '\0';
char *hw = "HELLO WORLD\\";
char hw2[13] = "HELLO WORLD\\";
char hw3[13][1];
char *multiline = "LINE1\x21\n"
    "LINE2\n";
int func2(int a);

typedef char *str;

int func1(int a, int b[10], int c) {
    int d = 1, *e = b, f = c += 1;
    str s1 = "D", *s2 = 0, *s3 = &multiline;
    str s4 = "S", s5[10];

    if (a + c)
        return 1;
    else if (a - c) {
        return 0;
    } else {
        return -1;
    }

    for (int i = 0; i; i++) {
        if (i + 1)
            return 0;
    }

    for (int i = 10; i; i--);

    for (int i = 0; i; ++i);

    for (int i = 10; i; --i);

    return (a + 1) * c;
}


