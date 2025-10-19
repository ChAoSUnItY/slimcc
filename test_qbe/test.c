int a, b, c;

int main(void) {
    a = 1;
    b = 1;
    c = 2;
    if (a && b && c) {
        c = 0;
    }
    return c;
}