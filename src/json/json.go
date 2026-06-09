package json

//so:embed cjson.c
var _ string

//so:embed cjson.h
var _ string

//so:extern cJSON
type JSON struct{}

//so:extern cJSON_GetStringValue
func (j *JSON) Value() string

//so:extern cJSON_GetNumberValue
func (j *JSON) ValueF() float64

//so:extern cJSON_IsInvalid
func (*JSON) IsInvalid() bool

//so:extern cJSON_IsFalse
func (*JSON) IsFalse() bool

//so:extern cJSON_IsTrue
func (*JSON) IsTrue() bool

//so:extern cJSON_IsBool
func (*JSON) IsBool() bool

//so:extern cJSON_IsNull
func (*JSON) IsNull() bool

//so:extern cJSON_IsNumber
func (*JSON) IsNumber() bool

//so:extern cJSON_IsString
func (*JSON) IsString() bool

//so:extern cJSON_IsArray
func (*JSON) IsArray() bool

//so:extern cJSON_IsObject
func (*JSON) IsObject() bool

//so:extern cJSON_IsRaw
func (*JSON) IsRaw() bool

//so:extern cJSON_GetObjectItem
func (*JSON) GetObjectItem(string) *JSON

//so:extern cJSON_GetArraySize
func (*JSON) GetArraySize() int

//so:extern cJSON_GetArraySize
func (*JSON) GetArrayItem(i int) *JSON

//so:extern cJSON_CreateObject
func CreateObject() *JSON

//so:extern cJSON_AddNumberToObject
func (*JSON) AddNumber(name string, n float64) *JSON

//so:extern cJSON_AddStringToObject
func (*JSON) AddString(name string, s string) *JSON

//so:extern cJSON_AddArrayToObject
func (*JSON) AddArray(name string) *JSON

//so:extern cJSON_AddItemToArray
func (*JSON) AddItemToArray(name string, item *JSON) *JSON

//so:extern cJSON_Print
func (*JSON) Print() string

//so:extern cJSON_ParseWithLength
func parseWithLength(v *byte, len int) *JSON

type jsonError struct{}

//so:extern cJSON_GetErrorPtr
func getErrorPtr() string
func (*jsonError) Error() string {
	return getErrorPtr()
}

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
