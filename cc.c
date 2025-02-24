/*
 * cc - Ultimate, Ultra-Optimized Cat Replacement (Version 1.1)
 *
 * This utility concatenates files to standard output while providing enhanced
 * text processing and additional features expected by experienced developers.
 *
 * Features:
 *   - Fast, raw file output with minimal overhead.
 *   - Enhanced text formatting:
 *       - Number all lines (-n)
 *       - Number nonblank lines (-b)
 *       - Squeeze repeated blank lines (-s)
 *       - Display end-of-line markers (-e)
 *       - Visualize TAB characters as "^I" (-T)
 *       - Convert nonprinting characters (-v)
 *       - The -A flag is equivalent to -v -T -e.
 *   - Follow mode (-f): Continuously output appended data (tail -f style).
 *
 * Performance:
 *   - Uses a larger buffer (8192 bytes) to reduce system calls.
 *   - Memory mapping is employed for files ≥1MB to avoid extra copying.
 *   - A fast path in text processing bypasses per-character handling when possible.
 *
 * Usage: cc [OPTION]... [FILE]...
 * If FILE is "-" or omitted, input is read from standard input.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #include <fcntl.h>
  #include <sys/stat.h>  /* For struct stat and stat() on Windows */
#else
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <unistd.h>
#endif

/* Buffer size for I/O */
#define BUFSIZE 8192
/* 1MB threshold for memory mapping */
#define MMAP_THRESHOLD (1024 * 1024)

/* Options structure */
typedef struct {
    int flag_num;         /* -n: number all lines */
    int flag_nnb;         /* -b: number nonblank lines */
    int flag_squeeze;     /* -s: suppress repeated blank lines */
    int flag_ends;        /* -e: show end-of-line marker ($) */
    int flag_tabs;        /* -T: show TAB as "^I" */
    int flag_nonprinting; /* -v: show nonprinting characters (except TAB/NL) */
    int flag_follow;      /* -f: follow file (tail -f style) */
    int squeeze_limit;    /* Maximum allowed consecutive blank lines */
    const char *line_format; /* Format for line numbers */
    const char *tab_repr;    /* Replacement for TAB characters */
    const char *end_marker;  /* Marker appended at end-of-line */
} Options;

/* Global default options */
static const Options global_defaults = {
    .flag_num = 0, .flag_nnb = 0, .flag_squeeze = 0, .flag_ends = 0,
    .flag_tabs = 0, .flag_nonprinting = 0, .flag_follow = 0,
    .squeeze_limit = 1,
    .line_format = "%6d\t", .tab_repr = "^I", .end_marker = "$"
};

/* Centralized error logging.
 * If fatal is non-zero, the program exits immediately.
 */
static void log_error(const char *msg, int fatal) {
    fprintf(stderr, "[%s:%d %s] ERROR: %s: %s\n",
            __FILE__, __LINE__, __func__, msg, strerror(errno));
    if (fatal)
        exit(EXIT_FAILURE);
}

/*
 * Print usage information.
 */
static void usage(void) {
    fprintf(stderr,
        "Usage: cc [OPTION]... [FILE]...\n"
        "Concatenate FILE(s) to standard output with enhanced formatting and follow mode.\n\n"
        "Options:\n"
        "  -n       number all output lines\n"
        "  -b       number nonblank lines\n"
        "  -s       suppress repeated blank lines\n"
        "  -e       display end-of-line marker (default \"$\")\n"
        "  -T       display TAB as \"^I\"\n"
        "  -v       display nonprinting characters (except TAB and NL)\n"
        "  -A       equivalent to -v -T -e\n"
        "  -f       follow file (continuously output appended data)\n"
        "  -h       display this help and exit\n"
        "  -V       output version information and exit\n"
    );
}

/*
 * Print version information.
 */
static void version(void) {
    printf("cc version 1.1\n");
}

/*
 * Retrieve file size in bytes.
 * Returns -1 on error (and logs a detailed error message).
 */
static long long get_file_size(const char *fname) {
    FILE *f = fopen(fname, "rb");
    if (!f) { log_error(fname, 0); return -1; }
    if (fseek(f, 0, SEEK_END) != 0) { log_error("fseek failed", 0); fclose(f); return -1; }
    long long size = ftell(f);
    if (size < 0) log_error("ftell failed", 0);
    fclose(f);
    return size;
}

/*
 * Process a single line with optional formatting.
 * Uses a fast path when no transformations are requested.
 */
static inline void process_line_buffer(const char *line, size_t len, Options *opts, int *line_no) {
    int is_blank = (len == 1 && line[0] == '\n');
    if (opts->flag_num || (opts->flag_nnb && !is_blank))
        printf(opts->line_format, (*line_no)++);

    if (!opts->flag_tabs && !opts->flag_nonprinting && !opts->flag_ends) {
        if (fwrite(line, 1, len, stdout) != len)
            log_error("fwrite failed in fast path", 0);
        return;
    }
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)line[i];
        if (c == '\t' && opts->flag_tabs)
            fputs(opts->tab_repr, stdout);
        else if (c == '\n') {
            if (opts->flag_ends)
                fputs(opts->end_marker, stdout);
            putchar('\n');
        } else if (opts->flag_nonprinting && (c < 32 || c == 127)) {
            if (c == 127)
                fputs("^?", stdout);
            else { putchar('^'); putchar(c + 64); }
        } else {
            putchar(c);
        }
    }
}

/*
 * Process a text file line by line.
 */
static void process_text(const char *fname, Options *opts, int *line_no) {
    FILE *f = (strcmp(fname, "-") ? fopen(fname, "r") : stdin);
    if (!f) { log_error(fname, 0); return; }
    char buf[BUFSIZE];
    int blank_count = 0;
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        int is_blank = (len == 1 && buf[0] == '\n');
        if (opts->flag_squeeze) {
            if (is_blank && ++blank_count > opts->squeeze_limit)
                continue;
            else if (!is_blank)
                blank_count = 0;
        }
        process_line_buffer(buf, len, opts, line_no);
    }
    if (ferror(f))
        log_error("Error reading file", 0);
    if (f != stdin && fclose(f) != 0)
        log_error("Failed to close file in process_text", 0);
}

/*
 * Process a file in binary mode with minimal overhead.
 */
static void process_binary(const char *fname) {
    FILE *f = (strcmp(fname, "-") ? fopen(fname, "rb") : stdin);
    if (!f) { log_error(fname, 0); return; }
    char buf[BUFSIZE];
    size_t n;
    while ((n = fread(buf, 1, BUFSIZE, f)) > 0) {
        if (fwrite(buf, 1, n, stdout) != n) {
            log_error("fwrite failed in process_binary", 0);
            break;
        }
    }
    if (ferror(f))
        log_error("Error reading binary file", 0);
    if (f != stdin && fclose(f) != 0)
        log_error("Failed to close file in process_binary", 0);
}

#ifdef _WIN32
/*
 * Process file using memory mapping on Windows.
 */
static void process_file_mmap(const char *fname, int text_mode, Options *opts, int *line_no) {
    HANDLE hFile = CreateFileA(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) { log_error("Error opening file", 0); return; }
    LARGE_INTEGER fsize;
    if (!GetFileSizeEx(hFile, &fsize)) { log_error("GetFileSizeEx failed", 0); CloseHandle(hFile); return; }
    if (fsize.QuadPart == 0) { CloseHandle(hFile); return; }
    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap) { log_error("CreateFileMapping failed", 0); CloseHandle(hFile); return; }
    char *data = (char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!data) { log_error("MapViewOfFile failed", 0); CloseHandle(hMap); CloseHandle(hFile); return; }
    size_t size = (size_t)fsize.QuadPart;
    if (!text_mode) {
        if (fwrite(data, 1, size, stdout) != size)
            log_error("fwrite failed in mmap binary mode", 0);
    } else {
        int blank_count = 0;
        size_t i = 0;
        while (i < size) {
            size_t ls = i;
            while (i < size && data[i] != '\n') i++;
            if (i < size && data[i] == '\n') i++;
            size_t ll = i - ls;
            if (opts->flag_squeeze && (ll == 1 && data[ls] == '\n')) {
                if (++blank_count > opts->squeeze_limit)
                    continue;
            } else {
                blank_count = 0;
            }
            process_line_buffer(data + ls, ll, opts, line_no);
        }
    }
    UnmapViewOfFile(data);
    CloseHandle(hMap);
    CloseHandle(hFile);
}
#else
/*
 * Process file using memory mapping on POSIX systems.
 */
static void process_file_mmap(const char *fname, int text_mode, Options *opts, int *line_no) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) { log_error(fname, 0); return; }
    struct stat st;
    if (fstat(fd, &st) < 0) { log_error("fstat failed", 0); close(fd); return; }
    if (st.st_size == 0) { close(fd); return; }
    char *data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) { log_error("mmap failed", 0); close(fd); return; }
    size_t size = st.st_size;
    if (!text_mode) {
        if (fwrite(data, 1, size, stdout) != size)
            log_error("fwrite failed in mmap binary mode", 0);
    } else {
        int blank_count = 0;
        size_t i = 0;
        while (i < size) {
            size_t ls = i;
            while (i < size && data[i] != '\n') i++;
            if (i < size && data[i] == '\n') i++;
            size_t ll = i - ls;
            if (opts->flag_squeeze && (ll == 1 && data[ls] == '\n')) {
                if (++blank_count > opts->squeeze_limit)
                    continue;
            } else {
                blank_count = 0;
            }
            process_line_buffer(data + ls, ll, opts, line_no);
        }
    }
    if (munmap(data, st.st_size) < 0)
        log_error("munmap failed", 0);
    if (close(fd) < 0)
        log_error("close failed for mmap file", 0);
}
#endif

/* Global flag for follow mode termination */
static volatile sig_atomic_t stop_follow = 0;

/*
 * Signal handler for graceful termination (e.g., on SIGINT).
 */
static void handle_sigint(int sig) {
    (void)sig; /* Unused parameter */
    stop_follow = 1;
}

/*
 * Follow mode processing (tail -f style) for text files.
 * This loop now checks for SIGINT to allow graceful exit.
 */
static void process_follow_text(const char *fname, Options *opts, int *line_no) {
    FILE *f = fopen(fname, "r");
    if (!f) { log_error(fname, 0); return; }
    if (fseek(f, 0, SEEK_END) != 0) { log_error("Initial fseek failed in follow mode", 0); fclose(f); return; }
    long current_offset = ftell(f);
    if (current_offset < 0) { log_error("Initial ftell failed in follow mode", 0); fclose(f); return; }

    /* Register signal handler for graceful termination */
    #ifndef _WIN32
    signal(SIGINT, handle_sigint);
    #endif

    char buf[BUFSIZE];
    while (!stop_follow) {
        struct stat st;
        if (stat(fname, &st) < 0) {
            log_error("stat failed in follow mode", 0);
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
            continue;
        }
        if (st.st_size > (size_t)current_offset) {
            if (fseek(f, current_offset, SEEK_SET) != 0) {
                log_error("fseek failed in follow mode", 0);
                break;
            }
            while (fgets(buf, sizeof(buf), f)) {
                size_t len = strlen(buf);
                current_offset = ftell(f);
                process_line_buffer(buf, len, opts, line_no);
            }
            if (ferror(f))
                log_error("Error reading in follow mode", 0);
        }
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }
    fclose(f);
}

/*
 * Parse command-line flags and collect file names.
 * Exits immediately on allocation or parsing errors.
 */
static int parse_global_flags(int argc, char *argv[], Options *opts, char ***file_list) {
    int fileCount = 0, parsing_flags = 1;
    char **files = malloc(sizeof(char*) * argc);
    if (!files) { log_error("malloc failed in parse_global_flags", 1); }
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (parsing_flags && strcmp(arg, "--") == 0) { parsing_flags = 0; continue; }
        if (parsing_flags && arg[0] == '-' && arg[1]) {
            if (arg[0] == '-' && arg[1] == '-') {
                if (!strcmp(arg, "--help")) { usage(); exit(EXIT_SUCCESS); }
                else if (!strcmp(arg, "--version")) { version(); exit(EXIT_SUCCESS); }
                else { fprintf(stderr, "Unknown option: %s\n", arg); exit(EXIT_FAILURE); }
            } else {
                for (int j = 1; arg[j]; j++) {
                    switch(arg[j]) {
                        case 'n': opts->flag_num = 1; break;
                        case 'b': opts->flag_nnb = 1; break;
                        case 's': opts->flag_squeeze = 1; break;
                        case 'e': opts->flag_ends = 1; break;
                        case 'T': opts->flag_tabs = 1; break;
                        case 'v': opts->flag_nonprinting = 1; break;
                        case 'A': opts->flag_nonprinting = opts->flag_tabs = opts->flag_ends = 1; break;
                        case 'f': opts->flag_follow = 1; break;
                        case 'h': usage(); exit(EXIT_SUCCESS);
                        case 'V': version(); exit(EXIT_SUCCESS);
                        default:
                            fprintf(stderr, "Unknown flag: -%c\n", arg[j]);
                            exit(EXIT_FAILURE);
                    }
                }
            }
        } else {
            files[fileCount++] = arg;
        }
    }
    if (fileCount == 0)
        files[fileCount++] = "-";
    *file_list = files;
    return fileCount;
}

/*
 * Main entry point.
 * Determines processing mode (text, binary, follow, or memory mapping) and handles each file.
 * If no file is specified and STDIN is interactive, usage is printed to avoid hanging.
 */
int main(int argc, char *argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    // _setmode(_fileno(stdout), _O_BINARY); // Uncomment if binary output is needed
#endif
    setvbuf(stdout, NULL, _IOFBF, BUFSIZE);
    Options opts = global_defaults;
    char **files;
    int fileCount = parse_global_flags(argc, argv, &opts, &files);

    /* If running interactively with no file redirection, show usage instead of hanging */
    if (fileCount == 1 && strcmp(files[0], "-") == 0) {
#ifdef _WIN32
        if (_isatty(_fileno(stdin))) { usage(); free(files); return EXIT_SUCCESS; }
#else
        if (isatty(fileno(stdin))) { usage(); free(files); return EXIT_SUCCESS; }
#endif
    }

    int use_text = (opts.flag_num || opts.flag_nnb || opts.flag_squeeze ||
                    opts.flag_ends || opts.flag_tabs || opts.flag_nonprinting);
    int line_no = 1;
    for (int i = 0; i < fileCount; i++) {
        const char *fname = files[i];
        if (opts.flag_follow && strcmp(fname, "-") != 0) {
            process_follow_text(fname, &opts, &line_no);
            continue;
        }
        int use_mmap_local = (strcmp(fname, "-") && (get_file_size(fname) >= MMAP_THRESHOLD));
        if (use_mmap_local) {
            process_file_mmap(fname, use_text, &opts, &line_no);
        } else {
            if (use_text)
                process_text(fname, &opts, &line_no);
            else
                process_binary(fname);
        }
    }
    free(files);
    fflush(stdout);
    return EXIT_SUCCESS;
}
