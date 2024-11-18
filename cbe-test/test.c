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

int func1(int a, int b[10], int c) {
    func2(1);
    if (a + c)
        return 1;
    else if (a - c) {
        return 0;
    } else {
        return -1;
    }
    return (a + 1) * c;
}

int func2(int a) {
    return a * a;
}

int main(int a, int b)
{
    return 1;
}

void functionA();
void functionB();

void functionA() {
    functionB(); // Function B is used before its implementation
}

void functionC() {}

