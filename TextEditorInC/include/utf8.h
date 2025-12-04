//
// Created by vikto on 2025-12-03.
//

#ifndef UTF8_H
#define UTF8_H

#include "editor.h"

int utf8CharLen(const unsigned char *s);
int utf8IsStartByte(unsigned char c);

int utf8NextCharIndex(const erow *row, int cx);

int utf8PrevCharIndex(const erow *row, int cx);

#endif //UTF8_H
