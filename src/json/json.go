package json

import "solod.dev/so/c"

//so:embed cjson.h
var _ string

//so:embed cjson.c
var _ string

//so:extern cJSON
type JSON struct {
	string *c.Char
}

//so:extern size_t
type Size_t int

//so:extern cJSON_Hooks
type cJSON_Hooks struct {
	malloc_fn func(Size_t) any
	free_fn   func(any)
}

//so:extern cJSON_InitHooks
func cJSON_InitHooks(*cJSON_Hooks)

var hooks cJSON_Hooks

type MallocFN func(Size_t) any
type FreeFN func(any)

func InitHooks(
	malloc_fn MallocFN,
	free_fn func(any),
) {
	hooks.malloc_fn = malloc_fn
	hooks.free_fn = free_fn
	cJSON_InitHooks(&hooks)
}

func (j *JSON) Name() string { return c.String(j.string) }

//so:extern cJSON_free
func Free(any)

//so:extern cJSON_GetStringValue
func stringValue(*JSON) *c.ConstChar

func (j *JSON) String() string {
	return c.String(stringValue(j))
}

//so:extern cJSON_GetNumberValue
func numberValue(*JSON) float64

func (j *JSON) Float64() float64 {
	return numberValue(j)
}

//so:extern cJSON_IsInvalid
func isInvalid(*JSON) bool

func (j *JSON) Invalid() bool {
	return isInvalid(j)
}

//so:extern cJSON_IsFalse
func isFalse(*JSON) bool

func (j *JSON) False() bool {
	return isFalse(j)
}

//so:extern cJSON_IsTrue
func isTrue(*JSON) bool

func (j *JSON) True() bool {
	return isTrue(j)
}

//so:extern cJSON_IsBool
func isBool(*JSON) bool

func (j *JSON) Bool() bool {
	return isBool(j)
}

//so:extern cJSON_IsNull
func isNull(*JSON) bool

func (j *JSON) Null() bool {
	return isNull(j)
}

//so:extern cJSON_IsNumber
func isNumber(*JSON) bool

func (j *JSON) Number() bool {
	return isNumber(j)
}

//so:extern cJSON_IsString
func isString(*JSON) bool

func (j *JSON) StringType() bool {
	return isString(j)
}

//so:extern cJSON_IsArray
func isArray(*JSON) bool

func (j *JSON) Array() bool {
	return isArray(j)
}

//so:extern cJSON_IsObject
func isObject(*JSON) bool

func (j *JSON) Object() bool {
	return isObject(j)
}

// //so:extern cJSON_IsRaw
// func isRaw(*JSON) bool

// func (j *JSON) Raw() bool {
// 	return isRaw(j)
// }

//so:extern cJSON_GetObjectItem
func getObjectItem(*JSON, string) *JSON

func (j *JSON) Item(name string) *JSON {
	return getObjectItem(j, name)
}

//so:extern cJSON_GetArraySize
func getArraySize(*JSON) int

func (j *JSON) Len() int {
	return getArraySize(j)
}

//so:extern cJSON_GetArrayItem
func getArrayItem(*JSON, int) *JSON

func (j *JSON) Index(i int) *JSON {
	return getArrayItem(j, i)
}

//so:extern cJSON_CreateObject
func CreateObject() *JSON

//so:extern cJSON_AddNumberToObject
func addNumberToObject(obj *JSON, name string, n float64) *JSON

func (o *JSON) AddNumber(name string, n float64) *JSON {
	return addNumberToObject(o, name, n)
}

//so:extern cJSON_AddStringToObject
func addStringToObject(obj *JSON, name string, s string) *JSON

func (o *JSON) AddString(name string, s string) *JSON {
	return addStringToObject(o, name, s)
}

//so:extern cJSON_AddArrayToObject
func addArrayToObject(obj *JSON, name string) *JSON

func (o *JSON) AddArray(name string) *JSON {
	return addArrayToObject(o, name)
}

//so:extern cJSON_AddItemToArray
func addItemToArray(array *JSON, item *JSON) bool

func (o *JSON) AddItem(item *JSON) bool {
	return addItemToArray(o, item)
}

//so:extern cJSON_Print
func printJSON(obj *JSON) *c.ConstChar

func (o *JSON) Marshal() []byte {
	return []byte(c.String(printJSON(o)))
}

//so:extern cJSON_ParseWithLength
func parseWithLength(v *byte, len int) *JSON

type jsonError struct{ _ int }

//so:extern cJSON_GetErrorPtr
func getErrorPtr() *c.ConstChar
func (*jsonError) Error() string {
	return c.String(getErrorPtr())
}
func GetError() error { return &_jsonErr }

var _jsonErr jsonError

func Parse(b []byte) (*JSON, error) {
	if len(b) == 0 {
		return nil, nil
	}
	v := parseWithLength(&b[0], len(b))
	if v == nil {
		return nil, &_jsonErr
	}
	return v, nil
}
