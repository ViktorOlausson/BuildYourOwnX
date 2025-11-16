//
// Created by vikto on 2025-11-14.
//

#ifndef EDITOR_H
#define EDITOR_H



#include <stddef.h>
#include <time.h>
#include <termios.h>

/*** Defines ***/

#define EDITOR_VERSION "0.0.3"
#define TAB_STOP 8
#define QUIT_TIMES 3
#define SAVE_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKeys {
    BACKSPACE = 127,
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

enum editorHighlight {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/*** data ***/

struct editorSyntax {
    char *fileType;
    char **fileMatch;
    char **keywords;
    char *singelLineCommentStart;
    char* multilineCommentStart;
    char *multilineCommentEnd;
    int flags;
};

typedef struct erow {
    int index;
    int size;
    int rsize;
    char *chars;
    char *render;
    unsigned char *highlight;
    int hlOpenComment;
} erow;

struct editorConfig{
    int cursorX, cursorY;
    int rx;
    int rowOff;
    int colOff;
    int screenrows;
    int screencols;
    int nrRows;
    erow *row;
    int dirty;
    char *filename;
    char statusMSG[80];
    time_t statusMsgTime;
    struct editorSyntax *syntax;
    struct termios orig_termios;
};

extern struct editorConfig E;

// row operations
int  editorRowCxToRx(erow *row, int cursorX);
int  editorRowRxToCx(erow *row, int rx);
void editorUpdateRow(erow *row);
void editorInsertRow(int at, char *s, size_t len);
void editorDelRow(int at);
void editorRowInsertChar(erow *row, int at, int c);
void editorRowAppenString(erow *row, char *s, size_t len);
void editorRowDelChar(erow *row, int at);
void editorFreeRow(erow *row);

// editor operations
void editorInserChar(int c);
void editorInsertNewLine(void);
void editorDelChar(void);

// file I/O helpers
char *editorRowToString(int *bufLen);


#endif //EDITOR_H
