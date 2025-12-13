//
// Created by vikto on 2025-12-04.
//
#include "../include/utf8.h"
#include "stddef.h"

int utf8CharLen(const unsigned char *s) {
    if (s == NULL) return 0;

    if ((s[0] & 0x80) == 0x00) return 1;        // 0xxxxxxx (ASCII)
    if ((s[0] & 0xE0) == 0xC0) return 2;        // 110xxxxx
    if ((s[0] & 0xF0) == 0xE0) return 3;        // 1110xxxx
    if ((s[0] & 0xF8) == 0xF0) return 4;        // 11110xxx

    return 1;
}
int utf8IsStartByte(unsigned char c) {

    return (c & 0xC0) != 0x80;
}

int utf8NextCharIndex(const erow *row, int cx) {
    if (!row) return cx;
    if (cx >= row->size) return row->size;

    const unsigned char *s = (const unsigned char *)row->chars + cx;
    int len = utf8CharLen(s);
    int next = cx + len;

    if (next > row->size) next = row->size;
    return next;
}

int utf8PrevCharIndex(const erow *row, int cx) {
    if (!row) return cx;
    if (cx <= 0) return 0;

    int i = cx - 1;
    while (i > 0 && !utf8IsStartByte((unsigned char)row->chars[i])) {
        i--;
    }
    return i;
}