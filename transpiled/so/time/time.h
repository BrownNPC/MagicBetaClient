#pragma once
#include "so/builtin/builtin.h"
#include "so/c/c.h"
#include "so/errors/errors.h"
#include "so/math/bits/bits.h"

// -- Embeds --

#include "so/builtin/builtin.h"

#ifdef so_build_hosted
#include <time.h>

#define time_tm struct tm

// strptime may not be declared without _XOPEN_SOURCE before system headers.
// Provide an explicit declaration for portability (e.g. glibc with gcc).
char* strptime(const char*, const char*, struct tm*);

// wall returns the current wall clock time.
static inline so_R_i64_i32 time_wall() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (so_R_i64_i32){.val = ts.tv_sec, .val2 = (int32_t)ts.tv_nsec};
}

// mono returns the current monotonic time in nanoseconds.
static inline int64_t time_mono() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

#else

typedef struct {
    int tm_sec;    // seconds after the minute [0-60]
    int tm_min;    // minutes after the hour [0-59]
    int tm_hour;   // hours since midnight [0-23]
    int tm_mday;   // day of the month [1-31]
    int tm_mon;    // months since January [0-11]
    int tm_year;   // years since 1900
    int tm_wday;   // days since Sunday [0-6]
    int tm_yday;   // days since January 1 [0-365]
    int tm_isdst;  // Daylight Saving Time flag
} time_tm;

static inline size_t strftime(char* str, size_t count, const char* format, time_tm* tm) {
    (void)str;
    (void)count;
    (void)format;
    (void)tm;
    so_panic("time: formatting requires a hosted environment");
    return 0;
}

static inline char* strptime(const char* str, const char* format, time_tm* tm) {
    (void)str;
    (void)format;
    (void)tm;
    so_panic("time: parsing requires a hosted environment");
    return NULL;
}

static inline so_R_i64_i32 time_wall() {
    so_panic("time: wall clock time requires a hosted environment");
    return (so_R_i64_i32){0};
}

static inline int64_t time_mono() {
    return 0;
}

#endif  // so_build_hosted

// -- Types --

typedef struct time_CalDate time_CalDate;
typedef struct time_CalClock time_CalClock;
typedef struct time_Time time_Time;

// A Month specifies a month of the year (January = 1, ...).
typedef so_int time_Month;

// A Weekday specifies a day of the week (Sunday = 0, ...).
typedef so_int time_Weekday;

// CalDate is a date specified by year, month, and day.
typedef struct time_CalDate {
    so_int Year;
    time_Month Month;
    so_int Day;
} time_CalDate;

// CalClock is a time of day specified by hour, minute, and second.
typedef struct time_CalClock {
    so_int Hour;
    so_int Minute;
    so_int Second;
} time_CalClock;

// A Duration represents the elapsed time between two instants
// as an int64 nanosecond count. The representation limits the
// largest representable duration to approximately 290 years.
typedef int64_t time_Duration;

// A Time represents an instant in time with nanosecond precision.
// Time always represents UTC internally.
//
// Programs using times should typically store and pass them as values,
// not pointers. That is, time variables and struct fields should be of
// type time.Time, not *time.Time.
//
// The zero value of type Time is January 1, year 1, 00:00:00.000000000 UTC.
// As this time is unlikely to come up in practice, the [Time.IsZero] method gives
// a simple way of detecting a time that has not been initialized explicitly.
//
// In addition to the required "wall clock" reading, a Time may contain an optional
// reading of the current process's monotonic clock, to provide additional precision
// for comparison or subtraction. See the "Monotonic Clocks" section in the package
// documentation for details.
typedef struct time_Time {
    uint64_t wall;
    int64_t ext;
} time_Time;

// Offset represents a fixed offset from UTC in seconds east of UTC.
typedef so_int time_Offset;

// -- Result types --

typedef struct time_TimeResult {
    time_Time val;
    so_Error err;
} time_TimeResult;

// -- Variables and constants --
static const time_Month time_January = 1;
static const time_Month time_February = 2;
static const time_Month time_March = 3;
static const time_Month time_April = 4;
static const time_Month time_May = 5;
static const time_Month time_June = 6;
static const time_Month time_July = 7;
static const time_Month time_August = 8;
static const time_Month time_September = 9;
static const time_Month time_October = 10;
static const time_Month time_November = 11;
static const time_Month time_December = 12;
static const time_Weekday time_Sunday = 0;
static const time_Weekday time_Monday = 1;
static const time_Weekday time_Tuesday = 2;
static const time_Weekday time_Wednesday = 3;
static const time_Weekday time_Thursday = 4;
static const time_Weekday time_Friday = 5;
static const time_Weekday time_Saturday = 6;

// Maximum length of a duration string.
// -2562047h47m16.854775808s
static const int64_t time_MaxDurationLen = 25;

// Common durations. There is no definition for units of Day or larger
// to avoid confusion across daylight savings time zone transitions.
//
// To count the number of units in a [Duration], divide:
//
//	second := time.Second
//	fmt.Print(int64(second/time.Millisecond)) // prints 1000
//
// To convert an integer number of units to a Duration, multiply:
//
//	seconds := 10
//	fmt.Print(time.Duration(seconds)*time.Second) // prints 10s
static const time_Duration time_Nanosecond = 1;
static const time_Duration time_Microsecond = 1000 * time_Nanosecond;
static const time_Duration time_Millisecond = 1000 * time_Microsecond;
static const time_Duration time_Second = 1000 * time_Millisecond;
static const time_Duration time_Minute = 60 * time_Second;
static const time_Duration time_Hour = 60 * time_Minute;

// Commonly used layouts for Format and Parse.
static const so_String time_RFC3339 = so_str("%Y-%m-%dT%H:%M:%S%z");
static const so_String time_RFC3339Nano = so_str("%Y-%m-%dT%H:%M:%S.%f%z");
static const so_String time_DateTime = so_str("%Y-%m-%d %H:%M:%S");
static const so_String time_DateOnly = so_str("%Y-%m-%d");
static const so_String time_TimeOnly = so_str("%H:%M:%S");

// Lengths of the common date/time layouts.
static const int64_t time_RFC3339Len = 25;
static const int64_t time_RFC3339NanoLen = 35;
static const int64_t time_DateTimeLen = 19;
static const int64_t time_DateOnlyLen = 10;
static const int64_t time_TimeOnlyLen = 8;

// ErrParse is returned by Parse when the input cannot be parsed.
extern so_Error time_ErrParse;

// UTC represents Universal Coordinated Time (UTC).
static const time_Offset time_UTC = 0;

// -- Functions and methods --

// Date returns the year, month, and day in which t occurs,
// adjusted by the given offset (seconds east of UTC).
time_CalDate time_Time_Date(time_Time t, time_Offset offset);

// Year returns the year in which t occurs.
so_int time_Time_Year(time_Time t);

// Month returns the month of the year specified by t.
time_Month time_Time_Month(time_Time t);

// Day returns the day of the month specified by t.
so_int time_Time_Day(time_Time t);

// Weekday returns the day of the week specified by t.
time_Weekday time_Time_Weekday(time_Time t);

// ISOWeek returns the ISO 8601 year and week number in which t occurs.
// Week ranges from 1 to 53. Jan 01 to Jan 03 of year n might belong to
// week 52 or 53 of year n-1, and Dec 29 to Dec 31 might belong to week 1
// of year n+1.
so_R_int_int time_Time_ISOWeek(time_Time t);

// Clock returns the hour, minute, and second within the day specified by t,
// adjusted by the given offset (seconds east of UTC).
time_CalClock time_Time_Clock(time_Time t, time_Offset offset);

// Hour returns the hour within the day specified by t, in the range [0, 23].
so_int time_Time_Hour(time_Time t);

// Minute returns the minute offset within the hour specified by t, in the range [0, 59].
so_int time_Time_Minute(time_Time t);

// Second returns the second offset within the minute specified by t, in the range [0, 59].
so_int time_Time_Second(time_Time t);

// Nanosecond returns the nanosecond offset within the second specified by t,
// in the range [0, 999999999].
so_int time_Time_Nanosecond(time_Time t);

// YearDay returns the day of the year specified by t, in the range [1,365] for non-leap years,
// and [1,366] in leap years.
so_int time_Time_YearDay(time_Time t);

// Add returns the time t+d.
time_Time time_Time_Add(time_Time t, time_Duration d);

// Sub returns the duration t-u. If the result exceeds the maximum (or minimum)
// value that can be stored in a [Duration], the maximum (or minimum) duration
// will be returned.
// To compute t-d for a duration d, use t.Add(-d).
time_Duration time_Time_Sub(time_Time t, time_Time u);

// Since returns the time elapsed since t.
// It is shorthand for time.Now().Sub(t).
time_Duration time_Since(time_Time t);

// Until returns the duration until t.
// It is shorthand for t.Sub(time.Now()).
time_Duration time_Until(time_Time t);

// AddDate returns the time corresponding to adding the
// given number of years, months, and days to t.
// For example, AddDate(-1, 2, 3) applied to January 1, 2011
// returns March 4, 2010.
//
// AddDate normalizes its result in the same way that Date does,
// so, for example, adding one month to October 31 yields
// December 1, the normalized form for November 31.
time_Time time_Time_AddDate(time_Time t, so_int years, so_int months, so_int days);

// Truncate returns the result of rounding t down to a multiple of d (since the zero time).
// If d <= 0, Truncate returns t stripped of any monotonic clock reading but otherwise unchanged.
//
// Truncate operates on the time as an absolute duration since the zero time;
// it does not operate on the presentation form of the time.
time_Time time_Time_Truncate(time_Time t, time_Duration d);

// Round returns the result of rounding t to the nearest multiple of d (since the zero time).
// The rounding behavior for halfway values is to round up.
// If d <= 0, Round returns t stripped of any monotonic clock reading but otherwise unchanged.
//
// Round operates on the time as an absolute duration since the zero time;
// it does not operate on the presentation form of the time.
time_Time time_Time_Round(time_Time t, time_Duration d);

// String returns a string representing the duration in the form "72h3m0.5s".
// Leading zero units are omitted. As a special case, durations less than one
// second format use a smaller unit (milli-, micro-, or nanoseconds) to ensure
// that the leading digit is non-zero. The zero duration formats as 0s.
// buf must have a length of at least [MaxDurationLen] bytes.
so_String time_Duration_String(time_Duration d, so_Slice buf);

// Nanoseconds returns the duration as an integer nanosecond count.
int64_t time_Duration_Nanoseconds(time_Duration d);

// Microseconds returns the duration as an integer microsecond count.
int64_t time_Duration_Microseconds(time_Duration d);

// Milliseconds returns the duration as an integer millisecond count.
int64_t time_Duration_Milliseconds(time_Duration d);

// These methods return float64 because the dominant
// use case is for printing a floating point number like 1.5s, and
// a truncation to integer would make them not useful in those cases.
// Splitting the integer and fraction ourselves guarantees that
// converting the returned float64 to an integer rounds the same
// way that a pure integer conversion would have, even in cases
// where, say, float64(d.Nanoseconds())/1e9 would have rounded
// differently.
// Seconds returns the duration as a floating point number of seconds.
double time_Duration_Seconds(time_Duration d);

// Minutes returns the duration as a floating point number of minutes.
double time_Duration_Minutes(time_Duration d);

// Hours returns the duration as a floating point number of hours.
double time_Duration_Hours(time_Duration d);

// Truncate returns the result of rounding d toward zero to a multiple of m.
// If m <= 0, Truncate returns d unchanged.
time_Duration time_Duration_Truncate(time_Duration d, time_Duration m);

// Round returns the result of rounding d to the nearest multiple of m.
// The rounding behavior for halfway values is to round away from zero.
// If the result exceeds the maximum (or minimum)
// value that can be stored in a [Duration],
// Round returns the maximum (or minimum) duration.
// If m <= 0, Round returns d unchanged.
time_Duration time_Duration_Round(time_Duration d, time_Duration m);

// Abs returns the absolute value of d.
// As a special case, Duration([math.MinInt64]) is converted to Duration([math.MaxInt64]),
// reducing its magnitude by 1 nanosecond.
time_Duration time_Duration_Abs(time_Duration d);

// Format formats the time per layout (strftime verbs like "%Y-%m-%d"),
// writing into buf. Returns the formatted string (a view into buf).
// buf length must be large enough for the formatted output
// (see [RFC3339Len], etc. for common layouts).
so_String time_Time_Format(time_Time t, so_Slice buf, so_String layout, time_Offset offset);

// String formats the time as ISO 8601 "2006-01-02T15:04:05Z",
// writing into buf. Returns the formatted string (a view into buf).
// buf length must be at least [RFC3339Len] bytes.
so_String time_Time_String(time_Time t, so_Slice buf);

// Parse parses value per layout (strptime verbs) and returns the Time.
// offset specifies what timezone the input value is in.
time_TimeResult time_Parse(so_String layout, so_String value, time_Offset offset);

// Date returns the Time in UTC corresponding to
//
//	yyyy-mm-dd hh:mm:ss + nsec nanoseconds
//
// with respect to the given offset (seconds east of UTC).
//
// The month, day, hour, min, sec, and nsec values may be outside
// their usual ranges and will be normalized during the conversion.
// For example, October 32 converts to November 1.
//
// A daylight savings time transition skips or repeats times.
// For example, in the United States, March 13, 2011 2:15am never occurred,
// while November 6, 2011 1:15am occurred twice. In such cases, the
// choice of time zone, and therefore the time, is not well-defined.
// Date returns a time that is correct in one of the two zones involved
// in the transition, but it does not guarantee which.
time_Time time_Date(so_int year, time_Month month, so_int day, so_int hour, so_int min, so_int sec, so_int nsec, time_Offset offset);

// Now returns the current time in UTC.
time_Time time_Now(void);

// These helpers for manipulating the wall and monotonic clock readings
// take pointer receivers, even when they don't modify the time,
// to make them cheaper to call.
// IsZero reports whether t represents the zero time instant,
// January 1, year 1, 00:00:00 UTC.
bool time_Time_IsZero(time_Time t);

// After reports whether the time instant t is after u.
bool time_Time_After(time_Time t, time_Time u);

// Before reports whether the time instant t is before u.
bool time_Time_Before(time_Time t, time_Time u);

// Compare compares the time instant t with u. If t is before u, it returns -1;
// if t is after u, it returns +1; if they're the same, it returns 0.
so_int time_Time_Compare(time_Time t, time_Time u);

// Equal reports whether t and u represent the same time instant.
// Unlike the == operator, Equal ignores monotonic clock readings.
bool time_Time_Equal(time_Time t, time_Time u);

// Unix returns t as a Unix time, the number of seconds elapsed
// since January 1, 1970 UTC.
//
// Unix-like operating systems often record time as a 32-bit
// count of seconds, but since the method here returns a 64-bit
// value it is valid for billions of years into the past or future.
int64_t time_Time_Unix(time_Time t);

// UnixMilli returns t as a Unix time, the number of milliseconds elapsed since
// January 1, 1970 UTC. The result is undefined if the Unix time in
// milliseconds cannot be represented by an int64 (a date more than 292 million
// years before or after 1970).
int64_t time_Time_UnixMilli(time_Time t);

// UnixMicro returns t as a Unix time, the number of microseconds elapsed since
// January 1, 1970 UTC. The result is undefined if the Unix time in
// microseconds cannot be represented by an int64 (a date before year -290307 or
// after year 294246).
int64_t time_Time_UnixMicro(time_Time t);

// UnixNano returns t as a Unix time, the number of nanoseconds elapsed
// since January 1, 1970 UTC. The result is undefined if the Unix time
// in nanoseconds cannot be represented by an int64 (a date before the year
// 1678 or after 2262). Note that this means the result of calling UnixNano
// on the zero Time is undefined.
int64_t time_Time_UnixNano(time_Time t);

// Unix returns the local Time corresponding to the given Unix time,
// sec seconds and nsec nanoseconds since January 1, 1970 UTC.
// It is valid to pass nsec outside the range [0, 999999999].
// Not all sec values have a corresponding time value. One such
// value is 1<<63-1 (the largest int64 value).
time_Time time_Unix(int64_t sec, int64_t nsec);

// UnixMilli returns the local Time corresponding to the given Unix time,
// msec milliseconds since January 1, 1970 UTC.
time_Time time_UnixMilli(int64_t msec);

// UnixMicro returns the local Time corresponding to the given Unix time,
// usec microseconds since January 1, 1970 UTC.
time_Time time_UnixMicro(int64_t usec);
