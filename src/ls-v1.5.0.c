#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>

/* -----------------------------
   ANSI Color Codes
----------------------------- */
#define COLOR_RESET "\033[0m"
#define COLOR_BLUE  "\033[1;34m"   /* Directory */
#define COLOR_GREEN "\033[1;32m"   /* Executable */
#define COLOR_RED   "\033[1;31m"   /* Tarballs / Archives */
#define COLOR_PINK  "\033[1;35m"   /* Symlink */
#define COLOR_REV   "\033[7m"      /* Special files (device, socket, pipe) */

/* -----------------------------
   Function Declarations
----------------------------- */
char **read_dir_names(const char *dirpath, int *out_n, int *out_maxlen);
void free_names(char **names, int n);
int get_terminal_width(void);
int cmp_names(const void *a, const void *b);
void print_columns(char **names, int n, int maxlen);
const char *get_color(const char *name);

/* -----------------------------
   Read All Visible Filenames
----------------------------- */
char **read_dir_names(const char *dirpath, int *out_n, int *out_maxlen) {
    DIR *d = opendir(dirpath);
    if (!d) {
        fprintf(stderr, "opendir(%s): %s\n", dirpath, strerror(errno));
        *out_n = 0;
        *out_maxlen = 0;
        return NULL;
    }

    size_t cap = 64;
    size_t count = 0;
    char **names = malloc(cap * sizeof(char *));
    if (!names) { closedir(d); return NULL; }

    int maxlen = 0;
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;  // skip hidden files

        if (count >= cap) {
            cap *= 2;
            char **tmp = realloc(names, cap * sizeof(char *));
            if (!tmp) break;
            names = tmp;
        }

        names[count] = strdup(entry->d_name);
        int len = (int)strlen(entry->d_name);
        if (len > maxlen) maxlen = len;
        count++;
    }

    closedir(d);
    *out_n = (int)count;
    *out_maxlen = maxlen;
    return names;
}

/* -----------------------------
   Free Allocated Memory
----------------------------- */
void free_names(char **names, int n) {
    if (!names) return;
    for (int i = 0; i < n; ++i)
        free(names[i]);
    free(names);
}

/* -----------------------------
   Get Terminal Width
----------------------------- */
int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return (int)w.ws_col;
    return 80; // fallback
}

/* -----------------------------
   Determine Color by File Type
----------------------------- */
const char *get_color(const char *name) {
    struct stat st;
    if (lstat(name, &st) == -1)
        return COLOR_RESET;

    /* Directory */
    if (S_ISDIR(st.st_mode))
        return COLOR_BLUE;

    /* Symlink */
    if (S_ISLNK(st.st_mode))
        return COLOR_PINK;

    /* Special file types: char/block dev, socket, fifo */
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) ||
        S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode))
        return COLOR_REV;

    /* Executable */
    if (st.st_mode & S_IXUSR)
        return COLOR_GREEN;

    /* Archives (tar, gz, zip) */
    if (strstr(name, ".tar") || strstr(name, ".gz") || strstr(name, ".zip"))
        return COLOR_RED;

    /* Default */
    return COLOR_RESET;
}

/* -----------------------------
   Sort Filenames Alphabetically
----------------------------- */
int cmp_names(const void *a, const void *b) {
    const char *na = *(const char **)a;
    const char *nb = *(const char **)b;
    return strcmp(na, nb);  // ascending order
}

/* -----------------------------
   Print in Columns (Down–Across)
   with Colorized Output
----------------------------- */
void print_columns(char **names, int n, int maxlen) {
    if (n <= 0) return;

    int spacing = 2;
    int termw = get_terminal_width();
    int colw = maxlen + spacing;

    int cols = termw / colw;
    if (cols < 1) cols = 1;

    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = c * rows + r;
            if (idx >= n) continue;

            const char *clr = get_color(names[idx]);
            if (c == cols - 1)
                printf("%s%s%s", clr, names[idx], COLOR_RESET);
            else
                printf("%s%-*s%s", clr, names[idx], colw, COLOR_RESET);
        }
        putchar('\n');
    }
}

/* -----------------------------
   Main Function
----------------------------- */
int main(void) {
    int n, maxlen;
    char **names = read_dir_names(".", &n, &maxlen);

    if (!names) {
        fprintf(stderr, "Failed to read directory.\n");
        return 1;
    }

    /* Sort alphabetically (carried over from v1.4.0) */
    qsort(names, n, sizeof(char *), cmp_names);

    /* Print with color */
    print_columns(names, n, maxlen);

    free_names(names, n);
    return 0;
}

