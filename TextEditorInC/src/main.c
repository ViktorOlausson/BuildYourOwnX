/*** include ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include "../include/editor.h"

/*** defines ***/

struct editorConfig E = {0};
//rest in editor.h(header)

/*** data ***/

/*** filetypes ***/

char *C_HL_extensions[] = {".c", ".h", ".cpp", NULL};
char *C_HL_keywords[] = {
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "class", "case",
    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", NULL
  };

struct editorSyntax HLDB[] = {
  {
      "c",
      C_HL_extensions,
      C_HL_keywords,
      "//", "/*", "*/",
      HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
};

#define HLBD_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*** prototypes ***/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void(*callback)(char *, int));

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
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }else {
                switch (seq[1]) {
                    case 'A':return ARROW_UP;
                    case 'B':return ARROW_DOWN;
                    case 'C':return ARROW_RIGHT;
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

    char c;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

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

/*** syntax highlighting ***/

int isSeparator(char c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
    row->highlight = realloc(row->highlight, row->rsize);
    memset(row->highlight, HL_NORMAL, row->rsize);

    if (E.syntax == NULL) {
        return;
    }

    char **keywords = E.syntax->keywords;

    char *scs =  E.syntax->singelLineCommentStart;
    char *mcs = E.syntax->multilineCommentStart;
    char *mce = E.syntax->multilineCommentEnd;


    int scsLen = scs ? strlen(scs) : 0;
    int mcsLen = mcs ? strlen(mcs) : 0;
    int mceLen = mce ? strlen(mce) : 0;

    int prevSep = 1;
    int inString = 0;
    int inComment = (row->index > 0 && E.row[row->index - 1].hlOpenComment);

    int i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prevHl =  (i > 0) ? row->highlight[i-1] : HL_NORMAL;

        if (scsLen && !inString && !inComment) {
            if (!strncmp(&row->render[i], scs, scsLen)) {
                memset(&row->highlight[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }

        if (mcsLen && mceLen && !inString) {
            if (inComment) {
                row->highlight[i] = HL_COMMENT;
                if (!strncmp(&row->render[i], mce, mceLen)) {
                    memset(&row->highlight[i], HL_COMMENT, mceLen);
                    i += mceLen;
                    inComment = 0;
                    prevSep = 1;
                    continue;
                }
            }else if (!strncmp(&row->render[i], mcs, mcsLen)) {
                memset(&row->highlight[i], HL_COMMENT, mcsLen);
                i += mcsLen;
                inComment = 1;
                continue;
            }
        }

        if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (inString) {
                row->highlight[i] = HL_STRING;
                if (c == '\\' && i+1 < row->rsize) {
                    row->highlight[i+1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == inString) inString = 0;
                i++;
                prevSep = 1;
                continue;
            }else {
                if (c == '"' || c == '\'') {
                    inString = c;
                    row->highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prevSep || prevHl == HL_NUMBER)) || (c == '.' && prevHl == HL_NUMBER)) {
                row->highlight[i] = HL_NUMBER;
                i++;
                prevSep = 0;
                continue;
            }
        }

        if (prevSep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][ klen - 1 ] == '|';
                if (kw2) klen--;

                if (!strncmp(&row->render[i], keywords[j], klen) && isSeparator(row->render[i + klen])) {
                    memset(&row->highlight[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prevSep = 0;
                continue;
            }
        }

        prevSep = isSeparator(c);
        i++;
    }
    int changed = (row->hlOpenComment != inComment);
    row->hlOpenComment = inComment;
    if (changed && row->index + 1 < E.nrRows) {
        editorUpdateSyntax(&E.row[row->index + 1]);
    }
}

int editorSyntaxToColor(int hl) {
    switch (hl) {
        case HL_COMMENT:
        case HL_MLCOMMENT: return 36;
        case HL_KEYWORD1: return 33;
        case HL_KEYWORD2: return 32;
        case HL_STRING: return 35;
        case HL_NUMBER: return 31;
        case HL_MATCH: return 34;
        default: return 37;
    }
}

void editorSelectSyntaxHighlight() {
    E.syntax = NULL;
    if (E.filename == NULL) {
        return;
    }

    char *ext = strchr(E.filename, '.');

    for (unsigned int j = 0; j < HLBD_ENTRIES; j++) {
        struct editorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s-> fileMatch[i]) {
            int is_ext = (s->fileMatch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, s->fileMatch[i])) || (!is_ext && ext && ext && strcmp(ext, s->fileMatch[i]))) {
                E.syntax = s;

                int filerow;
                for (filerow = 0; filerow < E.nrRows; filerow++) {
                    editorUpdateSyntax(&E.row[filerow]);
                }

                return;
            }
            i++;
        }
    }
}

/*** row operations ***/

int editorRowCxToRx(erow *row, int cursorX) {
    int rx = 0;
    int j;
    for (j = 0; j < cursorX; j++) {
        if (row->chars[j] == '\t') {
            rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        }
        rx++;
    }
    return rx;
}

int editorRowRxToCx(erow *row, int rx) {
    int curRx = 0;
    int cx;
    for (cx = 0; cx < row->size; cx++) {
        if (row->chars[cx] == '\t') {
            curRx += (TAB_STOP - 1) - (curRx % TAB_STOP);
        }
        curRx++;

        if (curRx == rx) {
            return cx;
        }
    }
    return cx;
}

void editorUpdateRow(erow *row) {
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            tabs++;
        }
    }

    free(row->render);
    row->render = malloc(row->size + tabs*(TAB_STOP - 1) + 1);

    int index = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[index++] = ' ';
            while (index % TAB_STOP != 0) {
                row->render[index++] = ' ';
            }
        }else {
            row->render[index++] = row->chars[j];
        }
    }

    row->render[index] = '\0';
    row->rsize = index;

    editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
    if (at < 0 || at > E.nrRows) return;

    E.row = realloc(E.row, sizeof(erow) * (E.nrRows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.nrRows - at));
    for (int j = at + 1; j <= E.nrRows; j++) {
        E.row[j].index++;
    }

    E.row[at].index = at;

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    E.row[at].highlight = NULL;
    E.row[at].hlOpenComment = 0;
    editorUpdateRow(&E.row[at]);

    E.nrRows++;
    E.dirty++;
}

void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->highlight);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.nrRows) return;
    editorFreeRow(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.nrRows - at - 1));
    E.nrRows--;

    for (int j = at; j < E.nrRows; j++) {
        E.row[j].index = j;
    }

    E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
    if (at < 0 || at > row-> size) {
        at = row->size;
    }
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at+1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowAppenString(erow *row, char *s, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memmove(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
    if (at < 0 || at >= row->size) return;
    memmove(&row-> chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

/*** editor operations ***/

void editorInserChar(int c) {
    if (E.cursorY == E.nrRows) {
        editorInsertRow(E.nrRows ,"", 0);
    }
    editorRowInsertChar(&E.row[E.cursorY], E.cursorX, c);
    E.cursorX++;
}

void editorInsertNewLine() {
    if (E.cursorX == 0) {
        editorInsertRow(E.cursorY, "", 0);
    }else {
        erow *row = &E.row[E.cursorY];
        editorInsertRow(E.cursorY + 1, &row->chars[E.cursorX], row->size - E.cursorX);
        row->size = E.cursorX;
        row->chars[row->size] = '\0';
        editorUpdateRow(row);
    }
    E.cursorY++;
    E.cursorX = 0;
}

void editorDelChar() {
    if (E.cursorY == E.nrRows) return;
    if (E.cursorX == 0 && E.cursorY == 0) return;

    erow *row = &E.row[E.cursorY];
    if (E.cursorX > 0) {
        editorRowDelChar(row, E.cursorX - 1);
        E.cursorX--;
    }else {
        E.cursorX = E.row[E.cursorY - 1].size;
        editorRowAppenString(&E.row[E.cursorY - 1], row->chars, row->size);
        editorDelRow(E.cursorY);
        E.cursorY--;
    }
}

/*** file I/O ***/

char *editorRowToString(int *bufLen) {
    int totLen = 0;
    int j;
    for (j = 0; j < E.nrRows; j++) {
        totLen += E.row[j].size + 1;
    }
    *bufLen = totLen;

    char *buf = malloc(totLen);
    char *p = buf;
    for (j = 0; j < E.nrRows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

void editorOpen(char *filename) {
    free(E.filename);
    E.filename = strdup(filename);

    editorSelectSyntaxHighlight();

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        die("fopen");
    }

    char *line = NULL;
    size_t lineCap = 0;
    ssize_t lineLen;
    while ((lineLen = getline(&line, &lineCap, fp)) != -1) {
        while (lineLen > 0 && (line[lineLen - 1] == '\n' || line[lineLen - 1] == '\r')) {
            lineLen--;
        }

        editorInsertRow(E.nrRows ,line, lineLen);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

void editorSave() {
    static int saveTimes = SAVE_TIMES;

    if (E.filename == NULL) {
        E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
        if (E.filename == NULL) {
            editorSetStatusMessage("Save aborted");
            return;
        }
        editorSelectSyntaxHighlight();
    }

    char *dot = strrchr(E.filename, '.');
    if ((dot == NULL || dot[1] == '\0') && saveTimes > 0) {
        editorSetStatusMessage("WARNING: no file extantions. "
                        "Press CTRL-S %d more times to confirm save.", saveTimes);

        saveTimes--;
        return;
    }

    int len;
    char *buf = editorRowToString(&len);

    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/*** find ***/

void editorFindCallback(char *query, int key) {
    static int lastMatch = -1;
    static int direction = 1;

    static int savedHighlightLine;
    static int *savedHighlightChar = NULL;

    if (savedHighlightChar) {
        memcpy(E.row[savedHighlightLine].highlight, savedHighlightChar, E.row[savedHighlightLine].size);
        free(savedHighlightChar);
        savedHighlightChar = NULL;
    }

    if (key == '\r' || key == '\x1b') {
        lastMatch = -1;
        direction = 1;
        return;
    }else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    }else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    }else {
        lastMatch = -1;
        direction = 1;
    }


    if (lastMatch == -1) {
        direction = 1;
    }
    int current = lastMatch;
    int i;
    for (i = 0; i < E.nrRows; i++) {
        current += direction;
        if (current == -1) {
            current = E.nrRows - 1;
        }else if (current == E.nrRows) {
            current = 0;
        }
        erow *row = &E.row[current];
        char *match = strstr(row->render, query);
        if (match) {
            lastMatch = current;
            E.cursorY = current;
            E.cursorX = editorRowRxToCx(row, match - row->render);
            E.rowOff = E.nrRows;

            savedHighlightLine = current;
            savedHighlightChar = malloc(row->rsize);
            memcpy(savedHighlightChar, row->highlight, row->rsize);
            memset(&row->highlight[match - row->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void editorFind() {
    int savedCursorX = E.cursorX;
    int savedCursorY = E.cursorY;
    int savedColOff = E.colOff;
    int savedRowOff = E.rowOff;

    char *query = editorPrompt("Find: %s (use ESC/Arrows/Enter)", editorFindCallback);
    if (query) {
        free(query);
    }else {
        E.cursorX = savedCursorX;
        E.cursorY = savedCursorY;
        E.colOff = savedColOff;
        E.rowOff = savedRowOff;
    }
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

void editorScroll() {
    E.rx = 0;
    if (E.cursorY < E.nrRows) {
        E.rx = editorRowCxToRx(&E.row[E.cursorY], E.cursorX);
    }

    if (E.cursorY < E.rowOff) {
        E.rowOff = E.cursorY;
    }
    if (E.cursorY >= E.rowOff + E.screenrows) {
        E.rowOff = E.cursorY - E.screenrows + 1;
    }
    if (E.rx < E.colOff) {
        E.colOff = E.rx;
    }
    if (E.rx >= E.colOff + E.screencols) {
        E.colOff = E.rx - E.screencols + 1;
    }
}

void editorDrawRows(struct abuf *ab) {
    for (int y = 0; y < E.screenrows; y++) {
        int fileRow = y + E.rowOff;
        if (fileRow >= E.nrRows) {
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
            int len = E.row[fileRow].rsize - E.colOff;
            if (len < 0) {
                len = 0;
            }
            if (len > E.screencols) {
                len = E.screencols;
            }
            char *c = &E.row[fileRow].render[E.colOff];
            unsigned char *hl = &E.row[fileRow].highlight[E.colOff];
            int currentColor = -1;
            int j;
            for (j = 0; j < len; j++) {
                if (iscntrl(c[j])) {
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    abAppend(ab, "\x1b[7m", 4);
                    abAppend(ab, &sym, 1);
                    abAppend(ab, "\x1b[m", 3);
                    if (currentColor == -1) {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", currentColor);
                        abAppend(ab, buf, clen);
                    }
                }else if (hl[j] == HL_NORMAL) {
                    if (currentColor != -1) {
                        abAppend(ab, "\x1b[39m", 5);
                        currentColor = -1;
                    }
                    abAppend(ab, &c[j], 1);
                }else {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != currentColor) {
                        currentColor = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        abAppend(ab, buf, clen);
                    }
                    abAppend(ab, &c[j], 1);
                }
            }
            abAppend(ab, "\x1b[39m", 5);
        }

        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct abuf *ab) {
    abAppend(ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s", E.filename ? E.filename : "[No Filename]", E.nrRows, E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", E.syntax ? E.syntax -> fileType : "no fit", E.cursorY + 1, E.nrRows);
    if (len > E.screencols) {
        len = E.screencols;
    }
    abAppend(ab, status, len);

    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        }else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
    abAppend(ab, "\x1b[K", 3);
    int msgLen = strlen(E.statusMSG);
    if (msgLen > E.screencols) {
        msgLen = E.screencols;
    }
    if (msgLen && time(NULL) - E.statusMsgTime < 5) {
        abAppend(ab, E.statusMSG, msgLen);
    }
}

void editorRefreshScreen() {
    editorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cursorY - E.rowOff) + 1, (E.rx - E.colOff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.buf, ab.len);
    abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusMSG, sizeof(E.statusMSG), fmt, ap);
    va_end(ap);
    E.statusMsgTime = time(NULL);
}

/*** Input ***/

char *editorPrompt(char *prompt, void(*callback)(char *, int)) {
    size_t bufSize = 128;
    char *buf = malloc(bufSize);

    size_t bufLen = 0;
    buf[0] = '\0';

    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == DELETE_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (bufLen != 0) {
                buf[--bufLen] = '\0';
            }
        }
        else if (c == '\x1b') {
            editorSetStatusMessage("");
            if (callback) {
                callback(buf, c);
            }
            free(buf);
            return NULL;
        }else if (c == '\r') {
            if (bufLen != 0) {
                editorSetStatusMessage("");
                if (callback) {
                    callback(buf, c);
                }
                return buf;
            }
        }else if (!iscntrl(c) && c < 128) {
            if (bufLen == bufSize - 1) {
                bufSize *= 2;
                buf = realloc(buf, bufSize);
            }
            buf[bufLen++] = c;
            buf[bufLen] = '\0';
        }
        if (callback) {
            callback(buf, c);
        }
    }
}

void editorMoveCursor(int key) {
    erow *row = (E.cursorY >= E.nrRows) ? NULL : &E.row[E.cursorY];
    switch (key) {
        case ARROW_LEFT:
            if (E.cursorX != 0) {
                E.cursorX--;
            }else if (E.cursorY > 0) {
                E.cursorY--;
                E.cursorX = E.row[E.cursorY].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cursorX < row->size) {
                E.cursorX++;
            }
            else if (row && E.cursorX == row->size) {
                E.cursorY++;
                E.cursorX = 0;
            }
            break;
        case ARROW_UP:
            if (E.cursorY != 0) {
                E.cursorY--;
            }
            break;
        case ARROW_DOWN:
            if (E.cursorY < E.nrRows) {
                E.cursorY++;
            }
            break;
    }

    row = (E.cursorY >= E.nrRows) ? NULL : &E.row[E.cursorY];
    int rowLen = row ? row->size : 0;
    if (E.cursorX > rowLen) {
        E.cursorX = rowLen;
    }
}

void editorProcessKeypress() {
    static int quitTimes = QUIT_TIMES;
    static int saveTimes = SAVE_TIMES;

    int c = editorReadKey();
    switch (c) {
        case '\r':
            editorInsertNewLine();
            break;

        case CTRL_KEY('q'):
            if (E.dirty && quitTimes > 0) {
                editorSetStatusMessage("WARNING!!!!! File has unsaved changes. "
                                       "Press CTRL-Q %d more times to quit", quitTimes);
                quitTimes--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;

        case CTRL_KEY('s'):

            editorSave();
            break;

        case HOME_KEY:
            E.cursorX = 0;
            break;

        case END_KEY:
            E.cursorX = E.row[E.cursorY].size;
            break;

        case CTRL_KEY('f'):
            editorFind();
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DELETE_KEY:
            if (c == DELETE_KEY) editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;

        case PAGE_UP:
        case PAGE_DOWN: {

            if (c == PAGE_UP) {
                E.cursorY = E.rowOff;
            }
            else if (c == PAGE_DOWN) {
                E.cursorY = E.rowOff + E.screenrows - 1;
                if (E.cursorY > E.nrRows) {
                    E.cursorY = E.nrRows;
                }
            }

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

        case CTRL_KEY('l'):
        case'\x1b':
            break;

        default:
            editorInserChar(c);
            break;
    }
}

/*** signals ***/

void handelSignal(int sig) {
    disableRawMode();
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    fprintf(stdout, "crash with error: %d (%s) \n", sig, strsignal(sig));

    exit(1);
}

void setupSignalHandler() {
    signal(SIGINT, handelSignal);
    signal(SIGTERM, handelSignal);
    signal(SIGSEGV, handelSignal);
}

/*** init/main function ***/

void initEditor() {
    E.cursorX = 0;
    E.cursorY = 0;
    E.rx = 0;
    E.rowOff = 0;
    E.colOff = 0;
    E.nrRows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.statusMSG[0] = '\0';
    E.statusMsgTime = 0;
    E.syntax = NULL;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }
    E.screenrows -= 2;
}

int main(int argc, char *argv[]) {
    enableRawMode();
    setupSignalHandler();
    initEditor();
    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    editorSetStatusMessage("HELP: CTRL-S = save | CTRL-Q = quit | CTRL-F = find");

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}