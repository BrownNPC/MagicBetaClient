#include "stringslite.h"

// -- Implementation --

so_String stringslite_Clone(mem_Allocator a, so_String s) {
    if (so_len(s) == 0) {
        return so_str("");
    }
    so_Slice b = mem_AllocSlice(so_byte, (a), (so_len(s)), (so_len(s)));
    so_copy_string(b, s);
    return so_bytes_string(b);
}

so_R_str_str stringslite_Cut(so_String s, so_String sep) {
    {
        so_int i = stringslite_Index(s, sep);
        if (i >= 0) {
            return (so_R_str_str){.val = so_string_slice(s, 0, i), .val2 = so_string_slice(s, i + so_len(sep), s.len)};
        }
    }
    return (so_R_str_str){.val = s, .val2 = so_str("")};
}

so_R_str_bool stringslite_CutPrefix(so_String s, so_String prefix) {
    if (!stringslite_HasPrefix(s, prefix)) {
        return (so_R_str_bool){.val = s, .val2 = false};
    }
    return (so_R_str_bool){.val = so_string_slice(s, so_len(prefix), s.len), .val2 = true};
}

so_R_str_bool stringslite_CutSuffix(so_String s, so_String suffix) {
    if (!stringslite_HasSuffix(s, suffix)) {
        return (so_R_str_bool){.val = s, .val2 = false};
    }
    return (so_R_str_bool){.val = so_string_slice(s, 0, so_len(s) - so_len(suffix)), .val2 = true};
}

bool stringslite_HasPrefix(so_String s, so_String prefix) {
    return so_len(s) >= so_len(prefix) && so_string_eq(so_string_slice(s, 0, so_len(prefix)), prefix);
}

bool stringslite_HasSuffix(so_String s, so_String suffix) {
    return so_len(s) >= so_len(suffix) && so_string_eq(so_string_slice(s, so_len(s) - so_len(suffix), s.len), suffix);
}

so_int stringslite_Index(so_String s, so_String substr) {
    so_int n = so_len(substr);
    if (n == 0) {
        return 0;
    } else if (n == 1) {
        return stringslite_IndexByte(s, so_at(so_byte, substr, 0));
    } else if (n == so_len(s)) {
        if (so_string_eq(substr, s)) {
            return 0;
        }
        return -1;
    } else if (n > so_len(s)) {
        return -1;
    }
    so_byte c0 = so_at(so_byte, substr, 0);
    so_byte c1 = so_at(so_byte, substr, 1);
    so_int i = 0;
    so_int t = so_len(s) - n + 1;
    so_int fails = 0;
    for (; i < t;) {
        if (so_at(so_byte, s, i) != c0) {
            so_int o = stringslite_IndexByte(so_string_slice(s, i + 1, t), c0);
            if (o < 0) {
                return -1;
            }
            i += o + 1;
        }
        if (so_at(so_byte, s, i + 1) == c1 && so_string_eq(so_string_slice(s, i, i + n), substr)) {
            return i;
        }
        i++;
        fails++;
        if (fails >= (4 + (i >> 4)) && i < t) {
            // See comment in [bytes.Index].
            so_int j = bytealg_IndexRabinKarp(so_string_bytes(so_string_slice(s, i, s.len)), so_string_bytes(substr));
            if (j < 0) {
                return -1;
            }
            return i + j;
        }
    }
    return -1;
}

so_int stringslite_IndexByte(so_String s, so_byte c) {
    return bytealg_IndexByteString(s, c);
}

so_String stringslite_TrimPrefix(so_String s, so_String prefix) {
    if (stringslite_HasPrefix(s, prefix)) {
        return so_string_slice(s, so_len(prefix), s.len);
    }
    return s;
}

so_String stringslite_TrimSuffix(so_String s, so_String suffix) {
    if (stringslite_HasSuffix(s, suffix)) {
        return so_string_slice(s, 0, so_len(s) - so_len(suffix));
    }
    return s;
}
