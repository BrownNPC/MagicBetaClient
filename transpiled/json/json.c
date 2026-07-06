#include "json.h"

// -- Types --

typedef struct jsonError jsonError;

typedef struct jsonError {
    so_int _;
} jsonError;

// -- Forward declarations --
static so_String jsonError_Error(void* self);

// -- Variables and constants --
static jsonError _jsonErr = {0};

// -- Implementation --

so_String cJSON_Name(void* self) {
    cJSON* j = self;
    return c_String(char, (j->string));
}

so_String cJSON_String(void* self) {
    cJSON* j = self;
    return c_String(const char, (cJSON_GetStringValue(j)));
}

double cJSON_Float64(void* self) {
    cJSON* j = self;
    return cJSON_GetNumberValue(j);
}

bool cJSON_Invalid(void* self) {
    cJSON* j = self;
    return cJSON_IsInvalid(j);
}

bool cJSON_False(void* self) {
    cJSON* j = self;
    return cJSON_IsFalse(j);
}

bool cJSON_True(void* self) {
    cJSON* j = self;
    return cJSON_IsTrue(j);
}

bool cJSON_Bool(void* self) {
    cJSON* j = self;
    return cJSON_IsBool(j);
}

bool cJSON_Null(void* self) {
    cJSON* j = self;
    return cJSON_IsNull(j);
}

bool cJSON_Number(void* self) {
    cJSON* j = self;
    return cJSON_IsNumber(j);
}

bool cJSON_StringType(void* self) {
    cJSON* j = self;
    return cJSON_IsString(j);
}

bool cJSON_Array(void* self) {
    cJSON* j = self;
    return cJSON_IsArray(j);
}

bool cJSON_Object(void* self) {
    cJSON* j = self;
    return cJSON_IsObject(j);
}

cJSON* cJSON_Item(void* self, so_String name) {
    cJSON* j = self;
    return cJSON_GetObjectItem(j, so_cstr(name));
}

so_int cJSON_Len(void* self) {
    cJSON* j = self;
    return cJSON_GetArraySize(j);
}

cJSON* cJSON_Index(void* self, so_int i) {
    cJSON* j = self;
    return cJSON_GetArrayItem(j, i);
}

cJSON* cJSON_AddNumber(void* self, so_String name, double n) {
    cJSON* o = self;
    return cJSON_AddNumberToObject(o, so_cstr(name), n);
}

cJSON* cJSON_AddString(void* self, so_String name, so_String s) {
    cJSON* o = self;
    return cJSON_AddStringToObject(o, so_cstr(name), so_cstr(s));
}

cJSON* cJSON_AddArray(void* self, so_String name) {
    cJSON* o = self;
    return cJSON_AddArrayToObject(o, so_cstr(name));
}

bool cJSON_AddItem(void* self, cJSON* item) {
    cJSON* o = self;
    return cJSON_AddItemToArray(o, item);
}

so_Slice cJSON_Marshal(void* self) {
    cJSON* o = self;
    return so_string_bytes(c_String(const char, (cJSON_Print(o))));
}

static so_String jsonError_Error(void* self) {
    (void)self;
    return c_String(const char, (cJSON_GetErrorPtr()));
}

so_Error json_GetError(void) {
    return (so_Error){.self = &_jsonErr, .Error = jsonError_Error};
}

so_R_ptr_err json_Parse(so_Slice b) {
    if (so_len(b) == 0) {
        return (so_R_ptr_err){.val = NULL, .err = (so_Error){0}};
    }
    cJSON* v = cJSON_ParseWithLength(&so_at(so_byte, b, 0), so_len(b));
    if (v == NULL) {
        return (so_R_ptr_err){.val = NULL, .err = (so_Error){.self = &_jsonErr, .Error = jsonError_Error}};
    }
    return (so_R_ptr_err){.val = v, .err = (so_Error){0}};
}
