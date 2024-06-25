int main(int argc, char** argv);
int cprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void
umain(int argc, char** argv) {
    cprintf("Hello world!\n");
    main(argc, argv);
}
