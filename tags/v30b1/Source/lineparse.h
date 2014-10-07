/*
 * lineparse.h
 * 
 * This file is a part of NSIS.
 * 
 * Copyright (C) 1999-2014 Nullsoft and Contributors
 * 
 * Licensed under the zlib/libpng license (the "License");
 * you may not use this file except in compliance with the License.
 * 
 * Licence details can be found in the file COPYING.
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.
 *
 * Unicode support by Jim Park -- 08/09/2007
 */

#ifndef _LINEPARSE_H_
#define _LINEPARSE_H_

#include "tchar.h"

class LineParser {
  public:

    LineParser(bool bCommentBlock);
    virtual ~LineParser();

    bool inComment();
    bool inCommentBlock();
    int parse(TCHAR *line, int ignore_escaping=0); // returns -1 on error
    int getnumtokens();
    void eattoken();
    double gettoken_float(int token, int *success=0) const;
    int gettoken_int(int token, int *success=0) const;
    double gettoken_number(int token, int *success=0) const;
    TCHAR *gettoken_str(int token) const;
    int gettoken_enum(int token, const TCHAR *strlist); // null separated list

  private:

    void freetokens();
    int doline(TCHAR *line, int ignore_escaping=0);

    int m_eat;
    int m_nt;
    bool m_incommentblock;
    bool m_incomment;
    TCHAR **m_tokens;
};
#endif//_LINEPARSE_H_
