//
// Created by vikto on 2025-11-14.
//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"

/*** Resets the editor for each test ***/
static void resetEditor(void) {

    if (E.row) {
        for (int i = 0; i < E.nrRows; i++) {
            editorFreeRow(&E.row[i]);
        }
        free(E.row);
    }

    memset(&E, 0, sizeof(E));

    E.row = NULL;
    E.nrRows = 0;
    E.dirty = 0;
    E.filename = NULL;
    E.syntax = NULL;
    E.screenrows = 24;
    E.screencols = 80;
}

static void test_inserAndDeleteRows(void) {
    resetEditor();

    editorInsertRow(0, "Hello", 5);
    editorInsertRow(1, "World", 5);

    assert(E.nrRows == 2);
    assert(strcmp(E.row[0].chars, "Hello") == 0);
    assert(strcmp(E.row[1].chars, "World") == 0);
    assert(E.row[0].index == 0);
    assert(E.row[1].index == 1);

    editorDelRow(0);

    assert(E.nrRows == 1);
    assert(strcmp(E.row[0].chars, "World") == 0);
    assert(E.row[0].index == 0);
}

int main(void) {
    printf("Running editor tests...\n");
    test_inserAndDeleteRows();

    printf("All tests passed\n");
    return 0;
}