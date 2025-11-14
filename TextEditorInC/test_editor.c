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
    memset(&E, 0, sizeof(E));
    E.row = NULL;
    E.nrRows = 0;
    E.dirty = 0;
}

