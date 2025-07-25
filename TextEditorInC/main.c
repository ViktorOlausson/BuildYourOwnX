/*** include ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/*** defines ***/
#define EDITOR_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKeys {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DELETE_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
};

/*** data ***/

typedef struct erow {
    int size;
    char *chars;
} erow;

struct editorConfig {
    int cursorX, cursorY;
    int screenrows;
    int screencols;
    int nrRows;
    erow *row;
    struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/
void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
        die("tcgetattr");
    }
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcgetattr");
    }
}

int editorReadKey() {
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }
    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DELETE_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }else {
                switch (seq[1]) {
                    case 'A':return ARROW_UP;
                    case 'B':return ARROW_DOWN;
                    case 'C':return ARROW_DOWN;
                    case 'D':return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F':return END_KEY;
                }
            }
        }else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    }else {
        return c;
    }
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    printf("\r\n");
    char c;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    //printf("\r\n&buf[1]: '%s' \r\n", &buf[1]);

    //editorReadKey();

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return getCursorPosition(rows,cols);
    }
    else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
        return 0;
    }
}

/*** row operations ***/

void editorAppendRow(char *s, size_t len) {
    E.row = realloc(E.row, sizeof(erow) * (E.nrRows + 1));

    int at = E.nrRows;

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.nrRows ++;
}

/*** file I/O ***/

void editorOpen(char *filename) {
    FILE *fp = fopen(filename, "r");

    char *line = NULL;
    size_t lineCap = 0;
    ssize_t lineLen;
    //lineLen = getline(&line, &lineCap, fp);
    while ((lineLen = getline(&line, &lineCap, fp)) != -1) {
        while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r')) {
            lineLen--;
        }

        editorAppendRow(line, lineLen);
    }
    free(line);
    fclose(fp);
}

/*** Append buffer ***/

struct abuf {
    char *buf;
    int len;
};

#define ABUF_INIT { NULL, 0 }

void abAppend(struct abuf *ab, const char *str, int len) {
    char *new = realloc(ab -> buf, ab -> len + len);

    if (new == NULL) return;
    memcpy(&new[ab -> len], str, len);
    ab -> buf = new;
    ab -> len += len;
}

void abFree(struct abuf *ab) {
    free(ab -> buf);
}

/*** Output ***/

void editorDrawRows(struct abuf *ab) {
    for (int y = 0; y < E.screenrows; y++) {
        if (y >= E.nrRows) {
            if (E.nrRows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomeLen = snprintf(welcome, sizeof(welcome),
                    "Text Editor -- version %s", EDITOR_VERSION);
                if (welcomeLen > E.screencols) welcomeLen = E.screencols;
                int padding = (E.screencols - welcomeLen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, welcome, welcomeLen);
            }else {
                abAppend(ab, "~", 1);
            }
        }else {
            int len = E.row[y].size;
            if (len > E.screencols) {
                len = E.screencols;
            }
            abAppend(ab, E.row[y].chars, len);
        }

        abAppend(ab, "\x1b[K", 3);
        if (y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    //abAppend(&ab, "\x1b[2J", 4);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cursorY + 1, E.cursorX + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.buf, ab.len);
    abFree(&ab);
}

/*** Input ***/

void editorMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            if (E.cursorX != 0) {
                E.cursorX--;
            }
            break;
        case ARROW_RIGHT:
            if (E.cursorX != E.screencols - 1) {
                E.cursorX++;
            }
            break;
        case ARROW_UP:
            if (E.cursorY != 0) {
                E.cursorY--;
            }
            break;
        case ARROW_DOWN:
            if (E.cursorY != E.screenrows - 1) {
                E.cursorY++;
            }
            break;
    }
}

void editorProcessKeypress() {
    int c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case HOME_KEY:
            E.cursorX = 0;
            break;
        case END_KEY:
            E.cursorX = E.screencols - 1;
            break;
        case PAGE_UP:
        case PAGE_DOWN: {
            int times = E.screenrows;
            while (times--) {
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;
        }
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

/*** init/main function ***/

void initEditor() {
    E.cursorX = 0;
    E.cursorY = 0;
    E.nrRows = 0;
    E.row = NULL;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }
}

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();
    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
