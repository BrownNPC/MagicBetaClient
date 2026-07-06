#pragma once
#include "so/builtin/builtin.h"
#include "so/bytealg/bytealg.h"
#include "so/mem/mem.h"

// -- Functions and methods --
so_String stringslite_Clone(mem_Allocator a, so_String s);
so_R_str_str stringslite_Cut(so_String s, so_String sep);
so_R_str_bool stringslite_CutPrefix(so_String s, so_String prefix);
so_R_str_bool stringslite_CutSuffix(so_String s, so_String suffix);
bool stringslite_HasPrefix(so_String s, so_String prefix);
bool stringslite_HasSuffix(so_String s, so_String suffix);
so_int stringslite_Index(so_String s, so_String substr);
so_int stringslite_IndexByte(so_String s, so_byte c);
so_String stringslite_TrimPrefix(so_String s, so_String prefix);
so_String stringslite_TrimSuffix(so_String s, so_String suffix);
