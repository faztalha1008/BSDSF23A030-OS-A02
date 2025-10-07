#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* -----------------------------
   Function Declarations
----------------------------- */
char **read_dir_names(const char *dirpath, int *out_n, int *out_maxlen);
void free_names(char **names, int n);
int get_terminal_width(void);
void print_columns(char **names, int n, int maxlen);

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
   Print in Columns (Down–Across)
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
            int idx = c * rows + r;  // down-then-across
            if (idx >= n) continue;

            if (c == cols - 1)
                printf("%s", names[idx]);          // last column
            else
                printf("%-*s", colw, names[idx]);  // align columns
        }
        putchar('\n');
    }
}

/* -----------------------------
   Main Function
----------------------------- */
/* -----------------------------
   Sort Filenames Alphabetically
----------------------------- */
int cmp_names(const void *a, const void *b) {
    const char *na = *(const char **)a;
    const char *nb = *(const char **)b;
    return strcmp(na, nb);  // ascending order
}

int main(void) {
    int n, maxlen;
    char **names = read_dir_names(".", &n, &maxlen);

    if (!names) {
        fprintf(stderr, "Failed to read directory.\n");
        return 1;
    }

    printf("Terminal width: %d\n", get_terminal_width());
    printf("Files: %d, Longest name: %d\n\n", n, maxlen);
    qsort(names, n, sizeof(char *), cmp_names);
	
    print_columns(names, n, maxlen);

    free_names(names, n);
    return 0;
}

