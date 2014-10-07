/*
 * fileform.h
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
 * Unicode support by Jim Park -- 08/13/2007
 */

#ifndef ___MAKENSIS_FILEFORM_H___
#define ___MAKENSIS_FILEFORM_H___

#include "exehead/fileform.h"
#include "writer.h"

#define DECLARE_WRITER(x) \
  class x##_writer : public writer \
  { \
  public: \
    x##_writer(writer_sink *sink) : writer(sink) {} \
    void write(const x *data); \
    static void write_block(IGrowBuf *buf, writer_sink *sink) \
    { \
      x *arr = (x *) buf->get(); \
      size_t l = buf->getlen() / sizeof(x); \
      x##_writer writer(sink); \
      for (size_t i = 0; i < l; i++) \
      { \
        writer.write(&arr[i]); \
      } \
    } \
  }

#define DECLARE_PLATFORMITEMWRITER(x) class x##_writer : public writer \
  { public: \
    x##_writer(writer_sink *sink) : writer(sink) {} \
    void writeplatformitem(const void *data, bool wide, bool x64); \
    static void write_block(IGrowBuf *pGB, writer_sink *pS, bool wide, bool x64) \
    { \
      x##_writer writer(pS); \
      for (size_t l = pGB->getlen() / sizeof(x), i = 0; i < l; i++) \
        writer.writeplatformitem(&(((x*)pGB->get())[i]), wide, x64); \
    } \
  }


DECLARE_WRITER(firstheader);
DECLARE_WRITER(block_header);
DECLARE_WRITER(header);
DECLARE_WRITER(section);
DECLARE_WRITER(entry);
DECLARE_WRITER(page);
DECLARE_PLATFORMITEMWRITER(ctlcolors);
DECLARE_WRITER(LOGFONT);

class lang_table_writer : public writer
{
public:
  lang_table_writer(writer_sink *sink, const size_t lang_strings) :
    writer(sink), m_lang_strings(lang_strings) {}
  void write(const unsigned char *data);
  static void write_block(IGrowBuf *buf, writer_sink *sink, const size_t table_size);

private:
  size_t m_lang_strings;

};

#endif//!___MAKENSIS_FILEFORM_H___
