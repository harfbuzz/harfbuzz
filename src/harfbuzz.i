/* File: harfbuzz.i */
%module harfbuzz
%{
#define HB_H_IN
#include "hb-common.h"
#include "hb-buffer.h"
#include "hb-blob.h"
#include "hb-shape.h"
#include "hb-font.h"
%}
%include "arrays_java.i"
%include "stdint.i"
%define HB_H_IN
%enddef
%include "hb-common.h"
%include "hb-buffer.h"
%include "hb-blob.h"
%include "hb-shape.h"
%include "hb-font.h"

