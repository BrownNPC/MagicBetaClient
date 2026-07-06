#include "time.h"

// -- Types --

typedef struct absSplit absSplit;

// An absSeconds counts the number of seconds since the absolute zero instant.
typedef uint64_t absSeconds;

// An absDays counts the number of days since the absolute zero instant.
typedef uint64_t absDays;

// An absCentury counts the number of centuries since the absolute zero instant.
typedef uint64_t absCentury;

// An absCyear counts the number of years since the start of a century.
typedef so_int absCyear;

// An absYday counts the number of days since the start of a year.
// Note that absolute years start on March 1.
typedef so_int absYday;

// An absMonth counts the number of months since the start of a year.
// absMonth=0 denotes March.
typedef so_int absMonth;

// An absLeap is a single bit (0 or 1) denoting whether a given year is a leap year.
typedef so_int absLeap;

// An absJanFeb is a single bit (0 or 1) denoting whether a given day falls in
// January or February. That is a special case because the absolute years start
// in March (unlike normal calendar years).
typedef so_int absJanFeb;

// absSplit is the result of splitting absolute days
// into century, year within century, and day within year.
typedef struct absSplit {
    absCentury century;
    absCyear cyear;
    absYday ayday;
} absSplit;

// -- Forward declarations --
static absDays absSeconds_days(absSeconds abs);
static time_CalClock absSeconds_clock(absSeconds abs);
static absDays dateToAbsDays(int64_t year, time_Month month, so_int day);
static absSplit absDays_split(absDays days);
static time_CalDate absDays_date(absDays days);
static so_R_int_int absDays_yearYday(absDays days);
static time_Weekday absDays_weekday(absDays days);
static absLeap absCentury_Leap(absCentury century, absCyear cyear);
static so_int absCentury_Year(absCentury century, absCyear cyear, absJanFeb janFeb);
static so_R_int_int absYday_split(absYday ayday);
static absJanFeb absYday_janFeb(absYday ayday);
static so_int absYday_yday(absYday ayday, absJanFeb janFeb, absLeap leap);
static time_Month absMonth_month(absMonth m, absJanFeb janFeb);
static time_Duration subMono(int64_t t, int64_t u);
static absSeconds time_Time_absSec(time_Time t);
static void time_Time_addSec(void* self, int64_t d);
static time_Duration time_div(time_Time t, time_Duration d);
static so_int time_Duration_format(time_Duration d, so_byte (*buf)[32]);
static so_R_int_u64 fmtFrac(so_Slice buf, uint64_t v, so_int prec);
static so_int fmtInt(so_Slice buf, uint64_t v);
static bool lessThanHalf(time_Duration x, time_Duration y);
static so_int fmtDate(so_Slice buf, so_int i, time_CalDate date);
static so_int fmtClock(so_Slice buf, so_int i, time_CalClock clock);
static so_int fmtOffset(so_Slice buf, so_int i, time_Offset offset);
static so_int fmtNano(so_Slice buf, so_int i, so_int ns);
static so_int fmt2(so_Slice buf, so_int i, so_int v);
static so_int fmt4(so_Slice buf, so_int i, so_int v);
static time_TimeResult parseRFC3339(so_String value, time_Offset offset);
static time_TimeResult parseRFC3339Nano(so_String value, time_Offset offset);
static time_TimeResult parseDateTime(so_String value, time_Offset offset);
static time_TimeResult parseDateOnly(so_String value, time_Offset offset);
static time_TimeResult parseTimeOnly(so_String value, time_Offset offset);
static so_R_int_bool parseOffset(so_String value, so_int i);
static so_int parse2(so_String s, so_int i);
static so_int parse4(so_String s, so_int i);
static so_int parse9(so_String s, so_int i);
static int32_t time_Time_nsec(void* self);
static int64_t time_Time_sec(void* self);
static void time_Time_stripMono(void* self);
static so_R_int_int norm(so_int hi, so_int lo, so_int base);
static int64_t time_Time_unixSec(void* self);
static time_Time unixTime(int64_t sec, int32_t nsec);

// -- Variables and constants --

// Computations on Times
//
// The zero value for a Time is defined to be
//	January 1, year 1, 00:00:00.000000000 UTC
// which (1) looks like a zero, or as close as you can get in a date
// (1-1-1 00:00:00 UTC), (2) is unlikely enough to arise in practice to
// be a suitable "not set" sentinel, unlike Jan 1 1970, and (3) has a
// non-negative year even in time zones west of UTC, unlike 1-1-0
// 00:00:00 UTC, which would be 12-31-(-1) 19:00:00 in New York.
//
// The zero Time value does not force a specific epoch for the time
// representation. For example, to use the Unix epoch internally, we
// could define that to distinguish a zero value from Jan 1 1970, that
// time would be represented by sec=-1, nsec=1e9. However, it does
// suggest a representation, namely using 1-1-1 00:00:00 UTC as the
// epoch, and that's what we do.
//
// The Add and Sub computations are oblivious to the choice of epoch.
//
// The presentation computations - year, month, minute, and so on - all
// rely heavily on division and modulus by positive constants. For
// calendrical calculations we want these divisions to round down, even
// for negative values, so that the remainder is always positive, but
// Go's division (like most hardware division instructions) rounds to
// zero. We can still do those computations and then adjust the result
// for a negative numerator, but it's annoying to write the adjustment
// over and over. Instead, we can change to a different epoch so long
// ago that all the times we care about will be positive, and then round
// to zero and round down coincide. These presentation routines already
// have to add the zone offset, so adding the translation to the
// alternate epoch is cheap. For example, having a non-negative time t
// means that we can write
//
//	sec = t % 60
//
// instead of
//
//	sec = t % 60
//	if sec < 0 {
//		sec += 60
//	}
//
// everywhere.
//
// The calendar runs on an exact 400 year cycle: a 400-year calendar
// printed for 1970-2369 will apply as well to 2370-2769. Even the days
// of the week match up. It simplifies date computations to choose the
// cycle boundaries so that the exceptional years are always delayed as
// long as possible: March 1, year 0 is such a day:
// the first leap day (Feb 29) is four years minus one day away,
// the first multiple-of-4 year without a Feb 29 is 100 years minus one day away,
// and the first multiple-of-100 year with a Feb 29 is 400 years minus one day away.
// March 1 year Y for any Y = 0 mod 400 is also such a day.
//
// Finally, it's convenient if the delta between the Unix epoch and
// long-ago epoch is representable by an int64 constant.
//
// These three considerations—choose an epoch as early as possible, that
// starts on March 1 of a year equal to 0 mod 400, and that is no more than
// 2⁶³ seconds earlier than 1970—bring us to the year -292277022400.
// We refer to this moment as the absolute zero instant, and to times
// measured as a uint64 seconds since this year as absolute times.
//
// Times measured as an int64 seconds since the year 1—the representation
// used for Time's sec field—are called internal times.
//
// Times measured as an int64 seconds since the year 1970 are called Unix
// times.
//
// It is tempting to just use the year 1 as the absolute epoch, defining
// that the routines are only valid for years >= 1. However, the
// routines would then be invalid when displaying the epoch in time zones
// west of UTC, since it is year 0. It doesn't seem tenable to say that
// printing the zero time correctly isn't supported in half the time
// zones. By comparison, it's reasonable to mishandle some times in
// the year -292277022400.
//
// All this is opaque to clients of the API and can be changed if a
// better implementation presents itself.
//
// The date calculations are implemented using the following clever math from
// Cassio Neri and Lorenz Schneider, "Euclidean affine functions and their
// application to calendar algorithms," SP&E 2023. https://doi.org/10.1002/spe.3172
//
// Define a "calendrical division" (f, f°, f*) to be a triple of functions converting
// one time unit into a whole number of larger units and the remainder and back.
// For example, in a calendar with no leap years, (d/365, d%365, y*365) is the
// calendrical division for days into years:
//
//	(f)  year := days/365
//	(f°) yday := days%365
//	(f*) days := year*365 (+ yday)
//
// Note that f* is usually the "easy" function to write: it's the
// calendrical multiplication that inverts the more complex division.
//
// Neri and Schneider prove that when f* takes the form
//
//	f*(n) = (a n + b) / c
//
// using integer division rounding down with a ≥ c > 0,
// which they call a Euclidean affine function or EAF, then:
//
//	f(n) = (c n + c - b - 1) / a
//	f°(n) = (c n + c - b - 1) % a / c
//
// This gives a fairly direct calculation for any calendrical division for which
// we can write the calendrical multiplication in EAF form.
// Because the epoch has been shifted to March 1, all the calendrical
// multiplications turn out to be possible to write in EAF form.
// When a date is broken into [century, cyear, amonth, mday],
// with century, cyear, and mday 0-based,
// and amonth 3-based (March = 3, ..., January = 13, February = 14),
// the calendrical multiplications written in EAF form are:
//
//	yday = (153 (amonth-3) + 2) / 5 = (153 amonth - 457) / 5
//	cday = 365 cyear + cyear/4 = 1461 cyear / 4
//	centurydays = 36524 century + century/4 = 146097 century / 4
//	days = centurydays + cday + yday + mday.
//
// We can only handle one periodic cycle per equation, so the year
// calculation must be split into [century, cyear], handling both the
// 100-year cycle and the 400-year cycle.
//
// The yday calculation is not obvious but derives from the fact
// that the March through January calendar repeats the 5-month
// 153-day cycle 31, 30, 31, 30, 31 (we don't care about February
// because yday only ever count the days _before_ February 1,
// since February is the last month).
//
// Using the rule for deriving f and f° from f*, these multiplications
// convert to these divisions:
//
//	century := (4 days + 3) / 146097
//	cdays := (4 days + 3) % 146097 / 4
//	cyear := (4 cdays + 3) / 1461
//	ayday := (4 cdays + 3) % 1461 / 4
//	amonth := (5 ayday + 461) / 153
//	mday := (5 ayday + 461) % 153 / 5
//
// The a in ayday and amonth stands for absolute (March 1-based)
// to distinguish from the standard yday (January 1-based).
//
// After computing these, we can translate from the March 1 calendar
// to the standard January 1 calendar with branch-free math assuming a
// branch-free conversion from bool to int 0 or 1, denoted int(b) here:
//
//	isJanFeb := int(yday >= marchThruDecember)
//	month := amonth - isJanFeb*12
//	year := century*100 + cyear + isJanFeb
//	isLeap := int(cyear%4 == 0) & (int(cyear != 0) | int(century%4 == 0))
//	day := 1 + mday
//	yday := 1 + ayday + 31 + 28 + isLeap&^isJanFeb - 365*isJanFeb
//
// isLeap is the standard leap-year rule, but the split year form
// makes the divisions all reduce to binary masking.
// Note that day and yday are 1-based, in contrast to mday and ayday.
// To keep the various units separate, we define integer types
// for each. These are never stored in interfaces nor allocated,
// so their type information does not appear in Go binaries.
static const int64_t secondsPerMinute = 60;
static const int64_t secondsPerHour = 60 * secondsPerMinute;
static const int64_t secondsPerDay = 24 * secondsPerHour;
static const int64_t marchThruDecember = 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31;
static const int64_t absoluteYears = 292277022400;
static const int64_t absoluteToInternal = -(absoluteYears * 146097 / 400 + marchThruDecember) * secondsPerDay;
static const int64_t internalToAbsolute = -absoluteToInternal;
static const int64_t unixToInternal = (1969 * 365 + 1969 / 4 - 1969 / 100 + 1969 / 400) * secondsPerDay;
static const int64_t internalToUnix = -unixToInternal;
static const int64_t absoluteToUnix = absoluteToInternal + internalToUnix;
static const int64_t wallToInternal = (1884 * 365 + 1884 / 4 - 1884 / 100 + 1884 / 400) * secondsPerDay;
static const int64_t maxint64 = (int64_t)(~(uint64_t)(0) >> 1);
static const time_Duration minDuration = (time_Duration)(~(int64_t)(~(uint64_t)(0) >> 1));
static const time_Duration maxDuration = (time_Duration)((int64_t)(~(uint64_t)(0) >> 1));

// Monotonic times are reported as offsets from monoStart.
// We initialize monoStart to time_mono() - 1 so that on systems where
// monotonic time resolution is fairly low (e.g. Windows 2008
// which appears to have a default resolution of 15ms),
// we avoid ever reporting a monotonic time of 0.
// (Callers may want to use 0 as "time not set".)
static int64_t monoStart = 0;

// ErrParse is returned by Parse when the input cannot be parsed.
so_Error time_ErrParse = errors_New("time: cannot parse");
static const uint64_t hasMonotonic = 0x8000000000000000;
static const int64_t minWall = wallToInternal;
static const int64_t nsecMask = ((int64_t)1 << 30) - 1;
static const int64_t nsecShift = 30;

// -- abs.go --

// absSeconds_days converts absolute seconds to absolute days.
static absDays absSeconds_days(absSeconds abs) {
    return (absDays)(abs / secondsPerDay);
}

// absSeconds_clock returns the hour, minute, and second within the day specified by abs.
static time_CalClock absSeconds_clock(absSeconds abs) {
    so_int sec = (so_int)(abs % secondsPerDay);
    so_int hour = sec / secondsPerHour;
    sec -= hour * secondsPerHour;
    so_int min = sec / secondsPerMinute;
    sec -= min * secondsPerMinute;
    return (time_CalClock){hour, min, sec};
}

// dateToAbsDays takes a standard year/month/day and returns the
// number of days from the absolute epoch to that day.
// The days argument can be out of range and in particular can be negative.
static absDays dateToAbsDays(int64_t year, time_Month month, so_int day) {
    // See "Computations on Times" comment above.
    uint32_t amonth = (uint32_t)(month);
    uint32_t janFeb = (uint32_t)(0);
    if (amonth < 3) {
        janFeb = 1;
    }
    amonth += 12 * janFeb;
    uint64_t y = (uint64_t)(year) - (uint64_t)(janFeb) + absoluteYears;
    // For amonth is in the range [3,14], we want:
    //
    //	ayday := (153*amonth - 457) / 5
    //
    // (See the "Computations on Times" comment above
    // as well as Neri and Schneider, section 7.)
    //
    // That is equivalent to:
    //
    //	ayday := (979*amonth - 2919) >> 5
    //
    // and the latter form uses a couple fewer instructions,
    // so use it, saving a few cycles.
    // See Neri and Schneider, section 8.3
    // for more about this optimization.
    //
    // (Note that there is no saved division, because the compiler
    // implements / 5 without division in all cases.)
    uint32_t ayday = ((979 * amonth - 2919) >> 5);
    uint64_t century = y / 100;
    uint32_t cyear = (uint32_t)(y % 100);
    uint32_t cday = 1461 * cyear / 4;
    uint64_t centurydays = 146097 * century / 4;
    return (absDays)(centurydays + (uint64_t)((int64_t)(cday + ayday) + (int64_t)(day) - 1));
}

// absDays_split splits days into century, cyear, ayday.
static absSplit absDays_split(absDays days) {
    // See "Computations on Times" comment above.
    uint64_t d = 4 * (uint64_t)(days) + 3;
    absCentury century = (absCentury)(d / 146097);
    // This should be
    //	cday := uint32(d % 146097) / 4
    //	cd := 4*cday + 3
    // which is to say
    //	cday := uint32(d % 146097) >> 2
    //	cd := cday<<2 + 3
    // but of course (x>>2<<2)+3 == x|3,
    // so do that instead.
    uint32_t cd = ((uint32_t)(d % 146097) | 3);
    // For cdays in the range [0,146097] (100 years), we want:
    //
    //	cyear := (4 cdays + 3) / 1461
    //	yday := (4 cdays + 3) % 1461 / 4
    //
    // (See the "Computations on Times" comment above
    // as well as Neri and Schneider, section 7.)
    //
    // That is equivalent to:
    //
    //	cyear := (2939745 cdays) >> 32
    //	yday := (2939745 cdays) & 0xFFFFFFFF / 2939745 / 4
    //
    // so do that instead, saving a few cycles.
    // See Neri and Schneider, section 8.3
    // for more about this optimization.
    so_R_u32_u32 _res1 = bits_Mul32(2939745, cd);
    uint32_t hi = _res1.val;
    uint32_t lo = _res1.val2;
    absCyear cyear = (absCyear)(hi);
    absYday ayday = (absYday)(lo / 2939745 / 4);
    return (absSplit){century, cyear, ayday};
}

// absDays_date converts days into standard year, month, day.
static time_CalDate absDays_date(absDays days) {
    absSplit split = absDays_split(days);
    so_R_int_int _res1 = absYday_split(split.ayday);
    absMonth amonth = _res1.val;
    so_int day = _res1.val2;
    absJanFeb janFeb = absYday_janFeb(split.ayday);
    so_int year = absCentury_Year(split.century, split.cyear, janFeb);
    time_Month month = absMonth_month(amonth, janFeb);
    return (time_CalDate){year, month, day};
}

// absDays_yearYday converts days into the standard year and 1-based yday.
static so_R_int_int absDays_yearYday(absDays days) {
    absSplit split = absDays_split(days);
    absJanFeb janFeb = absYday_janFeb(split.ayday);
    so_int year = absCentury_Year(split.century, split.cyear, janFeb);
    absLeap leap = absCentury_Leap(split.century, split.cyear);
    so_int yday = absYday_yday(split.ayday, janFeb, leap);
    return (so_R_int_int){.val = year, .val2 = yday};
}

// absDays_weekday returns the day of the week specified by days.
static time_Weekday absDays_weekday(absDays days) {
    // March 1 of the absolute year, like March 1 of 2000, was a Wednesday.
    return (time_Weekday)(((uint64_t)(days) + (uint64_t)(time_Wednesday)) % 7);
}

// absCentury_Leap returns 1 if (century, cyear) is a leap year, 0 otherwise.
static absLeap absCentury_Leap(absCentury century, absCyear cyear) {
    // See "Computations on Times" comment above.
    so_int y4ok = 0;
    if (cyear % 4 == 0) {
        y4ok = 1;
    }
    so_int y100ok = 0;
    if (cyear != 0) {
        y100ok = 1;
    }
    so_int y400ok = 0;
    if (century % 4 == 0) {
        y400ok = 1;
    }
    return (absLeap)(y4ok & (y100ok | y400ok));
}

// absCentury_Year returns the standard year for (century, cyear, janFeb).
static so_int absCentury_Year(absCentury century, absCyear cyear, absJanFeb janFeb) {
    // See "Computations on Times" comment above.
    return (so_int)((uint64_t)(century) * 100 - absoluteYears) + (so_int)(cyear) + (so_int)(janFeb);
}

// absYday_split splits ayday into absolute month and standard (1-based) day-in-month.
static so_R_int_int absYday_split(absYday ayday) {
    // See "Computations on Times" comment above.
    //
    // For yday in the range [0,366],
    //
    //	amonth := (5 yday + 461) / 153
    //	mday := (5 yday + 461) % 153 / 5
    //
    // is equivalent to:
    //
    //	amonth = (2141 yday + 197913) >> 16
    //	mday = (2141 yday + 197913) & 0xFFFF / 2141
    //
    // so do that instead, saving a few cycles.
    // See Neri and Schneider, section 8.3.
    uint32_t d = 2141 * (uint32_t)(ayday) + 197913;
    absMonth month = (absMonth)(d >> 16);
    so_int mday = 1 + (so_int)((d & 0xFFFF) / 2141);
    return (so_R_int_int){.val = month, .val2 = mday};
}

// absYday_janFeb returns 1 if the March 1-based ayday
// is in January or February, 0 otherwise.
static absJanFeb absYday_janFeb(absYday ayday) {
    // See "Computations on Times" comment above.
    absJanFeb jf = (absJanFeb)(0);
    if (ayday >= marchThruDecember) {
        jf = 1;
    }
    return jf;
}

// absYday_yday returns the standard 1-based yday for (ayday, janFeb, leap).
static so_int absYday_yday(absYday ayday, absJanFeb janFeb, absLeap leap) {
    // See "Computations on Times" comment above.
    return (so_int)(ayday) + (1 + 31 + 28) + ((so_int)(leap) & ~(so_int)(janFeb)) - 365 * (so_int)(janFeb);
}

// absMonth_month returns the standard Month for (m, janFeb)
static time_Month absMonth_month(absMonth m, absJanFeb janFeb) {
    // See "Computations on Times" comment above.
    return (time_Month)(m) - (time_Month)(janFeb) * 12;
}

// -- arithm.go --

// Date returns the year, month, and day in which t occurs,
// adjusted by the given offset (seconds east of UTC).
time_CalDate time_Time_Date(time_Time t, time_Offset offset) {
    absSeconds sec = time_Time_absSec(t) + (absSeconds)(offset);
    absDays days = absSeconds_days(sec);
    return absDays_date(days);
}

// Year returns the year in which t occurs.
so_int time_Time_Year(time_Time t) {
    absSeconds sec = time_Time_absSec(t);
    absDays days = absSeconds_days(sec);
    absSplit split = absDays_split(days);
    absJanFeb janFeb = absYday_janFeb(split.ayday);
    return absCentury_Year(split.century, split.cyear, janFeb);
}

// Month returns the month of the year specified by t.
time_Month time_Time_Month(time_Time t) {
    absSeconds sec = time_Time_absSec(t);
    absDays days = absSeconds_days(sec);
    absSplit split = absDays_split(days);
    so_R_int_int _res1 = absYday_split(split.ayday);
    absMonth month = _res1.val;
    absJanFeb janFeb = absYday_janFeb(split.ayday);
    return absMonth_month(month, janFeb);
}

// Day returns the day of the month specified by t.
so_int time_Time_Day(time_Time t) {
    absSeconds sec = time_Time_absSec(t);
    absDays days = absSeconds_days(sec);
    absSplit split = absDays_split(days);
    so_R_int_int _res1 = absYday_split(split.ayday);
    so_int day = _res1.val2;
    return day;
}

// Weekday returns the day of the week specified by t.
time_Weekday time_Time_Weekday(time_Time t) {
    absSeconds sec = time_Time_absSec(t);
    absDays days = absSeconds_days(sec);
    return absDays_weekday(days);
}

// ISOWeek returns the ISO 8601 year and week number in which t occurs.
// Week ranges from 1 to 53. Jan 01 to Jan 03 of year n might belong to
// week 52 or 53 of year n-1, and Dec 29 to Dec 31 might belong to week 1
// of year n+1.
so_R_int_int time_Time_ISOWeek(time_Time t) {
    // According to the rule that the first calendar week of a calendar year is
    // the week including the first Thursday of that year, and that the last one is
    // the week immediately preceding the first calendar week of the next calendar year.
    // See https://www.iso.org/obp/ui#iso:std:iso:8601:-1:ed-1:v1:en:term:3.1.1.23 for details.
    // weeks start with Monday
    // Monday Tuesday Wednesday Thursday Friday Saturday Sunday
    // 1      2       3         4        5      6        7
    // +3     +2      +1        0        -1     -2       -3
    // the offset to Thursday
    absSeconds sec = time_Time_absSec(t);
    absDays days = absSeconds_days(sec);
    time_Weekday wday = absDays_weekday(days - 1) + 1;
    absDays thu = days + (absDays)(time_Thursday - wday);
    so_R_int_int _res1 = absDays_yearYday(thu);
    so_int year = _res1.val;
    so_int yday = _res1.val2;
    so_int week = (yday - 1) / 7 + 1;
    return (so_R_int_int){.val = year, .val2 = week};
}

// Clock returns the hour, minute, and second within the day specified by t,
// adjusted by the given offset (seconds east of UTC).
time_CalClock time_Time_Clock(time_Time t, time_Offset offset) {
    absSeconds sec = time_Time_absSec(t) + (absSeconds)(offset);
    return absSeconds_clock(sec);
}

// Hour returns the hour within the day specified by t, in the range [0, 23].
so_int time_Time_Hour(time_Time t) {
    return (so_int)(time_Time_absSec(t) % secondsPerDay) / secondsPerHour;
}

// Minute returns the minute offset within the hour specified by t, in the range [0, 59].
so_int time_Time_Minute(time_Time t) {
    return (so_int)(time_Time_absSec(t) % secondsPerHour) / secondsPerMinute;
}

// Second returns the second offset within the minute specified by t, in the range [0, 59].
so_int time_Time_Second(time_Time t) {
    return (so_int)(time_Time_absSec(t) % secondsPerMinute);
}

// Nanosecond returns the nanosecond offset within the second specified by t,
// in the range [0, 999999999].
so_int time_Time_Nanosecond(time_Time t) {
    return (so_int)(time_Time_nsec(&t));
}

// YearDay returns the day of the year specified by t, in the range [1,365] for non-leap years,
// and [1,366] in leap years.
so_int time_Time_YearDay(time_Time t) {
    absSeconds sec = time_Time_absSec(t);
    absDays days = absSeconds_days(sec);
    so_R_int_int _res1 = absDays_yearYday(days);
    so_int yday = _res1.val2;
    return yday;
}

// Add returns the time t+d.
time_Time time_Time_Add(time_Time t, time_Duration d) {
    int64_t dsec = (int64_t)(d / 1000000000);
    int32_t nsec = time_Time_nsec(&t) + (int32_t)(d % 1000000000);
    if (nsec >= 1000000000) {
        dsec++;
        nsec -= 1000000000;
    } else if (nsec < 0) {
        dsec--;
        nsec += 1000000000;
    }
    // update nsec
    t.wall = ((t.wall & ~nsecMask) | (uint64_t)(nsec));
    time_Time_addSec(&t, dsec);
    if ((t.wall & hasMonotonic) != 0) {
        int64_t te = t.ext + (int64_t)(d);
        if ((d < 0 && te > t.ext) || (d > 0 && te < t.ext)) {
            // Monotonic clock reading now out of range; degrade to wall-only.
            time_Time_stripMono(&t);
        } else {
            t.ext = te;
        }
    }
    return t;
}

// Sub returns the duration t-u. If the result exceeds the maximum (or minimum)
// value that can be stored in a [Duration], the maximum (or minimum) duration
// will be returned.
// To compute t-d for a duration d, use t.Add(-d).
time_Duration time_Time_Sub(time_Time t, time_Time u) {
    if (((t.wall & u.wall) & hasMonotonic) != 0) {
        return subMono(t.ext, u.ext);
    }
    time_Duration d = (time_Duration)(time_Time_sec(&t) - time_Time_sec(&u)) * time_Second + (time_Duration)(time_Time_nsec(&t) - time_Time_nsec(&u));
    // Check for overflow or underflow.
    if (time_Time_Equal(time_Time_Add(u, d), t)) {
        // d is correct
        return d;
    } else if (time_Time_Before(t, u)) {
        // t - u is negative out of range
        return minDuration;
    } else {
        // t - u is positive out of range
        return maxDuration;
    }
}

static time_Duration subMono(int64_t t, int64_t u) {
    time_Duration d = (time_Duration)(t - u);
    if (d < 0 && t > u) {
        // t - u is positive out of range
        return maxDuration;
    }
    if (d > 0 && t < u) {
        // t - u is negative out of range
        return minDuration;
    }
    return d;
}

// Since returns the time elapsed since t.
// It is shorthand for time.Now().Sub(t).
time_Duration time_Since(time_Time t) {
    if ((t.wall & hasMonotonic) != 0) {
        // Common case optimization: if t has monotonic time, then Sub will use only it.
        return subMono(time_mono() - monoStart, t.ext);
    }
    return time_Time_Sub(time_Now(), t);
}

// Until returns the duration until t.
// It is shorthand for t.Sub(time.Now()).
time_Duration time_Until(time_Time t) {
    if ((t.wall & hasMonotonic) != 0) {
        // Common case optimization: if t has monotonic time, then Sub will use only it.
        return subMono(t.ext, time_mono() - monoStart);
    }
    return time_Time_Sub(t, time_Now());
}

// AddDate returns the time corresponding to adding the
// given number of years, months, and days to t.
// For example, AddDate(-1, 2, 3) applied to January 1, 2011
// returns March 4, 2010.
//
// AddDate normalizes its result in the same way that Date does,
// so, for example, adding one month to October 31 yields
// December 1, the normalized form for November 31.
time_Time time_Time_AddDate(time_Time t, so_int years, so_int months, so_int days) {
    time_CalDate date = time_Time_Date(t, time_UTC);
    time_CalClock clock = time_Time_Clock(t, time_UTC);
    return time_Date(date.Year + years, date.Month + (time_Month)(months), date.Day + days, clock.Hour, clock.Minute, clock.Second, (so_int)(time_Time_nsec(&t)), time_UTC);
}

// Truncate returns the result of rounding t down to a multiple of d (since the zero time).
// If d <= 0, Truncate returns t stripped of any monotonic clock reading but otherwise unchanged.
//
// Truncate operates on the time as an absolute duration since the zero time;
// it does not operate on the presentation form of the time.
time_Time time_Time_Truncate(time_Time t, time_Duration d) {
    time_Time_stripMono(&t);
    if (d <= 0) {
        return t;
    }
    time_Duration r = time_div(t, d);
    return time_Time_Add(t, -r);
}

// Round returns the result of rounding t to the nearest multiple of d (since the zero time).
// The rounding behavior for halfway values is to round up.
// If d <= 0, Round returns t stripped of any monotonic clock reading but otherwise unchanged.
//
// Round operates on the time as an absolute duration since the zero time;
// it does not operate on the presentation form of the time.
time_Time time_Time_Round(time_Time t, time_Duration d) {
    time_Time_stripMono(&t);
    if (d <= 0) {
        return t;
    }
    time_Duration r = time_div(t, d);
    if (lessThanHalf(r, d)) {
        return time_Time_Add(t, -r);
    }
    return time_Time_Add(t, d - r);
}

// absSec returns the time t as absolute seconds.
// It is called when computing a presentation property like Month or Hour.
static absSeconds time_Time_absSec(time_Time t) {
    return (absSeconds)(time_Time_unixSec(&t) + (unixToInternal + internalToAbsolute));
}

// addSec adds d seconds to the time.
static void time_Time_addSec(void* self, int64_t d) {
    time_Time* t = self;
    if ((t->wall & hasMonotonic) != 0) {
        int64_t sec = (int64_t)((t->wall << 1) >> (nsecShift + 1));
        int64_t dsec = sec + d;
        if ((0 <= dsec) && (dsec <= 8589934591)) {
            // 1<<33 - 1
            t->wall = (((t->wall & nsecMask) | ((uint64_t)(dsec) << nsecShift)) | hasMonotonic);
            return;
        }
        // Wall second now out of range for packed field.
        // Move to ext.
        time_Time_stripMono(t);
    }
    // Check if the sum of t.ext and d overflows and handle it properly.
    int64_t sum = t->ext + d;
    if ((sum > t->ext) == (d > 0)) {
        t->ext = sum;
    } else if (d > 0) {
        t->ext = maxint64;
    } else {
        t->ext = -maxint64;
    }
}

// div divides t by d and returns the remainder.
static time_Duration time_div(time_Time t, time_Duration d) {
    time_Duration r = 0;
    bool neg = false;
    int32_t nsec = time_Time_nsec(&t);
    int64_t sec = time_Time_sec(&t);
    if (sec < 0) {
        // Operate on absolute value.
        neg = true;
        sec = -sec;
        nsec = -nsec;
        if (nsec < 0) {
            nsec += 1000000000;
            // sec >= 1 before the -- so safe
            sec--;
        }
    }
    // Special case: 2d divides 1 second.
    if (d < time_Second && time_Second % (d + d) == 0) {
        r = (time_Duration)(nsec % (int32_t)(d));
    } else if (d % time_Second == 0) {
        int64_t d1 = (int64_t)(d / time_Second);
        r = (time_Duration)(sec % d1) * time_Second + (time_Duration)(nsec);
    } else {
        // Compute nanoseconds as 128-bit number.
        uint64_t usec = (uint64_t)(sec);
        uint64_t tmp = (usec >> 32) * 1000000000;
        uint64_t u1 = (tmp >> 32);
        uint64_t u0 = (tmp << 32);
        tmp = (usec & 0xFFFFFFFF) * 1000000000;
        uint64_t u0x = u0;
        u0 += tmp;
        if (u0 < u0x) {
            u1++;
        }
        u0x = u0;
        u0 += (uint64_t)(nsec);
        if (u0 < u0x) {
            u1++;
        }
        // Compute remainder by subtracting r<<k for decreasing k.
        // Quotient parity is whether we subtract on last round.
        uint64_t d1 = (uint64_t)(d);
        for (; (d1 >> 63) != 1;) {
            d1 <<= 1;
        }
        uint64_t d0 = (uint64_t)(0);
        for (;;) {
            if (u1 > d1 || (u1 == d1 && u0 >= d0)) {
                // subtract
                u0x = u0;
                u0 -= d0;
                if (u0 > u0x) {
                    u1--;
                }
                u1 -= d1;
            }
            if (d1 == 0 && d0 == (uint64_t)(d)) {
                break;
            }
            d0 >>= 1;
            d0 |= ((d1 & 1) << 63);
            d1 >>= 1;
        }
        r = (time_Duration)(u0);
    }
    if (neg && r != 0) {
        r = d - r;
    }
    return r;
}

// -- duration.go --

// String returns a string representing the duration in the form "72h3m0.5s".
// Leading zero units are omitted. As a special case, durations less than one
// second format use a smaller unit (milli-, micro-, or nanoseconds) to ensure
// that the leading digit is non-zero. The zero duration formats as 0s.
// buf must have a length of at least [MaxDurationLen] bytes.
so_String time_Duration_String(time_Duration d, so_Slice buf) {
    so_byte local[32] = {0};
    so_int n = time_Duration_format(d, &local);
    so_int m = so_copy(so_byte, buf, so_array_slice(so_byte, local, n, 32, 32));
    return so_bytes_string(so_slice(so_byte, buf, 0, m));
}

// format formats the representation of d into the end of buf and
// returns the offset of the first character.
static so_int time_Duration_format(time_Duration d, so_byte (*buf)[32]) {
    // Largest time is 2540400h10m10.000000000s
    so_int w = 32;
    uint64_t u = (uint64_t)(d);
    bool neg = d < 0;
    if (neg) {
        u = -u;
    }
    if (u < (uint64_t)(time_Second)) {
        // Special case: if duration is smaller than a second,
        // use smaller units, like 1.2ms
        so_int prec = 0;
        w--;
        (*buf)[w] = 's';
        w--;
        if (u == 0) {
            (*buf)[w] = '0';
            return w;
        } else if (u < (uint64_t)(time_Microsecond)) {
            // print nanoseconds
            prec = 0;
            (*buf)[w] = 'n';
        } else if (u < (uint64_t)(time_Millisecond)) {
            // print microseconds
            prec = 3;
            // U+00B5 'µ' micro sign == 0xC2 0xB5
            // Need room for two bytes.
            w--;
            so_copy_string(so_array_slice(so_byte, (*buf), w, 32, 32), so_str("µ"));
        } else {
            // print milliseconds
            prec = 6;
            (*buf)[w] = 'm';
        }
        so_R_int_u64 _res1 = fmtFrac(so_array_slice(so_byte, (*buf), 0, w, 32), u, prec);
        w = _res1.val;
        u = _res1.val2;
        w = fmtInt(so_array_slice(so_byte, (*buf), 0, w, 32), u);
    } else {
        w--;
        (*buf)[w] = 's';
        so_R_int_u64 _res2 = fmtFrac(so_array_slice(so_byte, (*buf), 0, w, 32), u, 9);
        w = _res2.val;
        u = _res2.val2;
        // u is now integer seconds
        w = fmtInt(so_array_slice(so_byte, (*buf), 0, w, 32), u % 60);
        u /= 60;
        // u is now integer minutes
        if (u > 0) {
            w--;
            (*buf)[w] = 'm';
            w = fmtInt(so_array_slice(so_byte, (*buf), 0, w, 32), u % 60);
            u /= 60;
            // u is now integer hours
            // Stop at hours because days can be different lengths.
            if (u > 0) {
                w--;
                (*buf)[w] = 'h';
                w = fmtInt(so_array_slice(so_byte, (*buf), 0, w, 32), u);
            }
        }
    }
    if (neg) {
        w--;
        (*buf)[w] = '-';
    }
    return w;
}

// fmtFrac formats the fraction of v/10**prec (e.g., ".12345") into the
// tail of buf, omitting trailing zeros. It omits the decimal
// point too when the fraction is 0. It returns the index where the
// output bytes begin and the value v/10**prec.
static so_R_int_u64 fmtFrac(so_Slice buf, uint64_t v, so_int prec) {
    // Omit trailing zeros up to and including decimal point.
    so_int w = so_len(buf);
    bool print = false;
    for (so_int i = 0; i < prec; i++) {
        uint64_t digit = v % 10;
        print = print || digit != 0;
        if (print) {
            w--;
            so_at(so_byte, buf, w) = (so_byte)(digit) + '0';
        }
        v /= 10;
    }
    if (print) {
        w--;
        so_at(so_byte, buf, w) = '.';
    }
    return (so_R_int_u64){.val = w, .val2 = v};
}

// fmtInt formats v into the tail of buf.
// It returns the index where the output begins.
static so_int fmtInt(so_Slice buf, uint64_t v) {
    so_int w = so_len(buf);
    if (v == 0) {
        w--;
        so_at(so_byte, buf, w) = '0';
    } else {
        for (; v > 0;) {
            w--;
            so_at(so_byte, buf, w) = (so_byte)(v % 10) + '0';
            v /= 10;
        }
    }
    return w;
}

// Nanoseconds returns the duration as an integer nanosecond count.
int64_t time_Duration_Nanoseconds(time_Duration d) {
    return (int64_t)(d);
}

// Microseconds returns the duration as an integer microsecond count.
int64_t time_Duration_Microseconds(time_Duration d) {
    return (int64_t)(d) / 1000;
}

// Milliseconds returns the duration as an integer millisecond count.
int64_t time_Duration_Milliseconds(time_Duration d) {
    return (int64_t)(d) / 1000000;
}

// These methods return float64 because the dominant
// use case is for printing a floating point number like 1.5s, and
// a truncation to integer would make them not useful in those cases.
// Splitting the integer and fraction ourselves guarantees that
// converting the returned float64 to an integer rounds the same
// way that a pure integer conversion would have, even in cases
// where, say, float64(d.Nanoseconds())/1e9 would have rounded
// differently.
// Seconds returns the duration as a floating point number of seconds.
double time_Duration_Seconds(time_Duration d) {
    time_Duration sec = d / time_Second;
    time_Duration nsec = d % time_Second;
    return (double)(sec) + (double)(nsec) / 1e9;
}

// Minutes returns the duration as a floating point number of minutes.
double time_Duration_Minutes(time_Duration d) {
    time_Duration min = d / time_Minute;
    time_Duration nsec = d % time_Minute;
    return (double)(min) + (double)(nsec) / (60 * 1e9);
}

// Hours returns the duration as a floating point number of hours.
double time_Duration_Hours(time_Duration d) {
    time_Duration hour = d / time_Hour;
    time_Duration nsec = d % time_Hour;
    return (double)(hour) + (double)(nsec) / (60 * 60 * 1e9);
}

// Truncate returns the result of rounding d toward zero to a multiple of m.
// If m <= 0, Truncate returns d unchanged.
time_Duration time_Duration_Truncate(time_Duration d, time_Duration m) {
    if (m <= 0) {
        return d;
    }
    return d - d % m;
}

// lessThanHalf reports whether x+x < y but avoids overflow,
// assuming x and y are both positive (Duration is signed).
static bool lessThanHalf(time_Duration x, time_Duration y) {
    return (uint64_t)(x) + (uint64_t)(x) < (uint64_t)(y);
}

// Round returns the result of rounding d to the nearest multiple of m.
// The rounding behavior for halfway values is to round away from zero.
// If the result exceeds the maximum (or minimum)
// value that can be stored in a [Duration],
// Round returns the maximum (or minimum) duration.
// If m <= 0, Round returns d unchanged.
time_Duration time_Duration_Round(time_Duration d, time_Duration m) {
    if (m <= 0) {
        return d;
    }
    time_Duration r = d % m;
    if (d < 0) {
        r = -r;
        if (lessThanHalf(r, m)) {
            return d + r;
        }
        {
            time_Duration d1 = d - m + r;
            if (d1 < d) {
                return d1;
            }
        }
        // overflow
        return minDuration;
    }
    if (lessThanHalf(r, m)) {
        return d - r;
    }
    {
        time_Duration d1 = d + m - r;
        if (d1 > d) {
            return d1;
        }
    }
    // overflow
    return maxDuration;
}

// Abs returns the absolute value of d.
// As a special case, Duration([math.MinInt64]) is converted to Duration([math.MaxInt64]),
// reducing its magnitude by 1 nanosecond.
time_Duration time_Duration_Abs(time_Duration d) {
    if (d >= 0) {
        return d;
    } else if (d == minDuration) {
        return maxDuration;
    } else {
        return -d;
    }
}

// -- extern.go --

// -- format.go --

// Format formats the time per layout (strftime verbs like "%Y-%m-%d"),
// writing into buf. Returns the formatted string (a view into buf).
// buf length must be large enough for the formatted output
// (see [RFC3339Len], etc. for common layouts).
so_String time_Time_Format(time_Time t, so_Slice buf, so_String layout, time_Offset offset) {
    absSeconds sec = time_Time_absSec(t) + (absSeconds)(offset);
    absDays days = absSeconds_days(sec);
    time_CalClock clock = absSeconds_clock(sec);
    // Fast paths for known layouts - avoid strftime overhead.
    if (so_string_eq(layout, time_RFC3339)) {
        time_CalDate date = absDays_date(days);
        so_int n = fmtDate(buf, 0, date);
        so_at(so_byte, buf, n) = 'T';
        n = fmtClock(buf, n + 1, clock);
        n = fmtOffset(buf, n, offset);
        return so_bytes_string(so_slice(so_byte, buf, 0, n));
    }
    if (so_string_eq(layout, time_RFC3339Nano)) {
        time_CalDate date = absDays_date(days);
        so_int n = fmtDate(buf, 0, date);
        so_at(so_byte, buf, n) = 'T';
        n = fmtClock(buf, n + 1, clock);
        so_at(so_byte, buf, n) = '.';
        n = fmtNano(buf, n + 1, (so_int)(time_Time_nsec(&t)));
        n = fmtOffset(buf, n, offset);
        return so_bytes_string(so_slice(so_byte, buf, 0, n));
    }
    if (so_string_eq(layout, time_DateTime)) {
        time_CalDate date = absDays_date(days);
        so_int n = fmtDate(buf, 0, date);
        so_at(so_byte, buf, n) = ' ';
        n = fmtClock(buf, n + 1, clock);
        return so_bytes_string(so_slice(so_byte, buf, 0, n));
    }
    if (so_string_eq(layout, time_DateOnly)) {
        time_CalDate date = absDays_date(days);
        so_int n = fmtDate(buf, 0, date);
        return so_bytes_string(so_slice(so_byte, buf, 0, n));
    }
    if (so_string_eq(layout, time_TimeOnly)) {
        so_int n = fmtClock(buf, 0, clock);
        return so_bytes_string(so_slice(so_byte, buf, 0, n));
    }
    // General case: strftime.
    time_CalDate date = absDays_date(days);
    absSplit split = absDays_split(days);
    absJanFeb janFeb = absYday_janFeb(split.ayday);
    time_Weekday wday = absDays_weekday(days);
    absLeap leap = absCentury_Leap(split.century, split.cyear);
    so_int yday = absYday_yday(split.ayday, janFeb, leap);
    time_tm tm = {0};
    tm.tm_year = (int)(date.Year - 1900);
    tm.tm_mon = (int)((so_int)(date.Month) - 1);
    tm.tm_mday = (int)(date.Day);
    tm.tm_hour = (int)(clock.Hour);
    tm.tm_min = (int)(clock.Minute);
    tm.tm_sec = (int)(clock.Second);
    tm.tm_wday = (int)(wday);
    tm.tm_yday = (int)(yday - 1);
    tm.tm_isdst = 0;
    uintptr_t n = strftime((char*)(unsafe_SliceData(buf)), (uintptr_t)(so_len(buf)), so_cstr(layout), &tm);
    return so_bytes_string(so_slice(so_byte, buf, 0, n));
}

// String formats the time as ISO 8601 "2006-01-02T15:04:05Z",
// writing into buf. Returns the formatted string (a view into buf).
// buf length must be at least [RFC3339Len] bytes.
so_String time_Time_String(time_Time t, so_Slice buf) {
    return time_Time_Format(t, buf, time_RFC3339, time_UTC);
}

// fmtDate writes "YYYY-MM-DD" into buf at position i.
// Returns the position after the last byte written.
static so_int fmtDate(so_Slice buf, so_int i, time_CalDate date) {
    i = fmt4(buf, i, date.Year);
    so_at(so_byte, buf, i) = '-';
    i = fmt2(buf, i + 1, (so_int)(date.Month));
    so_at(so_byte, buf, i) = '-';
    return fmt2(buf, i + 1, date.Day);
}

// fmtClock writes "HH:MM:SS" into buf at position i.
// Returns the position after the last byte written.
static so_int fmtClock(so_Slice buf, so_int i, time_CalClock clock) {
    i = fmt2(buf, i, clock.Hour);
    so_at(so_byte, buf, i) = ':';
    i = fmt2(buf, i + 1, clock.Minute);
    so_at(so_byte, buf, i) = ':';
    return fmt2(buf, i + 1, clock.Second);
}

// fmtOffset writes "Z" (for UTC) or "+HH:MM"/"-HH:MM" into buf at position i.
static so_int fmtOffset(so_Slice buf, so_int i, time_Offset offset) {
    if (offset == time_UTC) {
        so_at(so_byte, buf, i) = 'Z';
        return i + 1;
    }
    so_int off = (so_int)(offset);
    if (off < 0) {
        so_at(so_byte, buf, i) = '-';
        off = -off;
    } else {
        so_at(so_byte, buf, i) = '+';
    }
    i = fmt2(buf, i + 1, off / 3600);
    so_at(so_byte, buf, i) = ':';
    return fmt2(buf, i + 1, (off % 3600) / 60);
}

// fmtNano writes a 9-digit zero-padded nanosecond value into buf at position i.
static so_int fmtNano(so_Slice buf, so_int i, so_int ns) {
    for (so_int j = 8; j >= 0; j--) {
        so_at(so_byte, buf, i + j) = (so_byte)(U'0' + ns % 10);
        ns /= 10;
    }
    return i + 9;
}

// fmt2 writes a 2-digit zero-padded number into buf at position i.
static so_int fmt2(so_Slice buf, so_int i, so_int v) {
    so_at(so_byte, buf, i) = (so_byte)(U'0' + v / 10);
    so_at(so_byte, buf, i + 1) = (so_byte)(U'0' + v % 10);
    return i + 2;
}

// fmt4 writes a 4-digit zero-padded number into buf at position i.
static so_int fmt4(so_Slice buf, so_int i, so_int v) {
    so_at(so_byte, buf, i) = (so_byte)(U'0' + v / 1000);
    so_at(so_byte, buf, i + 1) = (so_byte)(U'0' + (v / 100) % 10);
    so_at(so_byte, buf, i + 2) = (so_byte)(U'0' + (v / 10) % 10);
    so_at(so_byte, buf, i + 3) = (so_byte)(U'0' + v % 10);
    return i + 4;
}

// -- parse.go --

// Parse parses value per layout (strptime verbs) and returns the Time.
// offset specifies what timezone the input value is in.
time_TimeResult time_Parse(so_String layout, so_String value, time_Offset offset) {
    // Fast paths for known layouts - avoid strptime
    // overhead and verb issues (%z, %f).
    if (so_string_eq(layout, time_RFC3339)) {
        return parseRFC3339(value, offset);
    }
    if (so_string_eq(layout, time_RFC3339Nano)) {
        return parseRFC3339Nano(value, offset);
    }
    if (so_string_eq(layout, time_DateTime)) {
        return parseDateTime(value, offset);
    }
    if (so_string_eq(layout, time_DateOnly)) {
        return parseDateOnly(value, offset);
    }
    if (so_string_eq(layout, time_TimeOnly)) {
        return parseTimeOnly(value, offset);
    }
    // General case: strptime.
    time_tm tm = {0};
    char* end = strptime(so_cstr(value), so_cstr(layout), &tm);
    if (end == NULL) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    return (time_TimeResult){.val = time_Date((so_int)(tm.tm_year) + 1900, (time_Month)((so_int)(tm.tm_mon) + 1), (so_int)(tm.tm_mday), (so_int)(tm.tm_hour), (so_int)(tm.tm_min), (so_int)(tm.tm_sec), 0, offset), .err = (so_Error){0}};
}

// parseRFC3339 parses "YYYY-MM-DDTHH:MM:SSZ" or "YYYY-MM-DDTHH:MM:SS+HH:MM".
static time_TimeResult parseRFC3339(so_String value, time_Offset offset) {
    // YYYY-MM-DDTHH:MM:SSZ      (20)
    // YYYY-MM-DDTHH:MM:SS+HH:MM (25)
    // 0123456789012345678901234
    so_int n = so_len(value);
    bool ok = (n == 20 || n == 25) && so_at(so_byte, value, 4) == '-' && so_at(so_byte, value, 7) == '-' && so_at(so_byte, value, 10) == 'T' && so_at(so_byte, value, 13) == ':' && so_at(so_byte, value, 16) == ':';
    if (!ok) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    so_int year = parse4(value, 0);
    so_int month = parse2(value, 5);
    so_int day = parse2(value, 8);
    so_int hour = parse2(value, 11);
    so_int min = parse2(value, 14);
    so_int sec = parse2(value, 17);
    if ((((((year | month) | day) | hour) | min) | sec) < 0) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    so_R_int_bool _res1 = parseOffset(value, 19);
    time_Offset off = _res1.val;
    ok = _res1.val2;
    if (!ok) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    return (time_TimeResult){.val = time_Date(year, (time_Month)(month), day, hour, min, sec, 0, offset + off), .err = (so_Error){0}};
}

// parseRFC3339Nano parses "YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ" or with +HH:MM/-HH:MM.
static time_TimeResult parseRFC3339Nano(so_String value, time_Offset offset) {
    // YYYY-MM-DDTHH:MM:SS.nnnnnnnnnZ      (30)
    // YYYY-MM-DDTHH:MM:SS.nnnnnnnnn+HH:MM (35)
    // 01234567890123456789012345678901234
    so_int n = so_len(value);
    bool ok = (n == 30 || n == 35) && so_at(so_byte, value, 4) == '-' && so_at(so_byte, value, 7) == '-' && so_at(so_byte, value, 10) == 'T' && so_at(so_byte, value, 13) == ':' && so_at(so_byte, value, 16) == ':' && so_at(so_byte, value, 19) == '.';
    if (!ok) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    so_int year = parse4(value, 0);
    so_int month = parse2(value, 5);
    so_int day = parse2(value, 8);
    so_int hour = parse2(value, 11);
    so_int min = parse2(value, 14);
    so_int sec = parse2(value, 17);
    so_int nsec = parse9(value, 20);
    if (((((((year | month) | day) | hour) | min) | sec) | nsec) < 0) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    so_R_int_bool _res1 = parseOffset(value, 29);
    time_Offset off = _res1.val;
    ok = _res1.val2;
    if (!ok) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    return (time_TimeResult){.val = time_Date(year, (time_Month)(month), day, hour, min, sec, nsec, offset + off), .err = (so_Error){0}};
}

// parseDateTime parses "YYYY-MM-DD HH:MM:SS".
static time_TimeResult parseDateTime(so_String value, time_Offset offset) {
    // YYYY-MM-DD HH:MM:SS
    // 0123456789012345678
    if (so_len(value) != 19 || so_at(so_byte, value, 4) != '-' || so_at(so_byte, value, 7) != '-' || so_at(so_byte, value, 10) != ' ' || so_at(so_byte, value, 13) != ':' || so_at(so_byte, value, 16) != ':') {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    so_int year = parse4(value, 0);
    so_int month = parse2(value, 5);
    so_int day = parse2(value, 8);
    so_int hour = parse2(value, 11);
    so_int min = parse2(value, 14);
    so_int sec = parse2(value, 17);
    if ((((((year | month) | day) | hour) | min) | sec) < 0) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    return (time_TimeResult){.val = time_Date(year, (time_Month)(month), day, hour, min, sec, 0, offset), .err = (so_Error){0}};
}

// parseDateOnly parses "YYYY-MM-DD".
static time_TimeResult parseDateOnly(so_String value, time_Offset offset) {
    // YYYY-MM-DD
    // 0123456789
    if (so_len(value) != 10 || so_at(so_byte, value, 4) != '-' || so_at(so_byte, value, 7) != '-') {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    so_int year = parse4(value, 0);
    so_int month = parse2(value, 5);
    so_int day = parse2(value, 8);
    if (((year | month) | day) < 0) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    return (time_TimeResult){.val = time_Date(year, (time_Month)(month), day, 0, 0, 0, 0, offset), .err = (so_Error){0}};
}

// parseTimeOnly parses "HH:MM:SS".
// Returns a time with date January 1, year 0.
static time_TimeResult parseTimeOnly(so_String value, time_Offset offset) {
    // HH:MM:SS
    // 01234567
    if (so_len(value) != 8 || so_at(so_byte, value, 2) != ':' || so_at(so_byte, value, 5) != ':') {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    so_int hour = parse2(value, 0);
    so_int min = parse2(value, 3);
    so_int sec = parse2(value, 6);
    if (((hour | min) | sec) < 0) {
        return (time_TimeResult){.val = (time_Time){}, .err = time_ErrParse};
    }
    return (time_TimeResult){.val = time_Date(0, (time_Month)(1), 1, hour, min, sec, 0, offset), .err = (so_Error){0}};
}

// parseOffset parses "Z", "+HH:MM", or "-HH:MM" at position i.
// Returns the offset in seconds.
static so_R_int_bool parseOffset(so_String value, so_int i) {
    if (so_at(so_byte, value, i) == 'Z') {
        return (so_R_int_bool){.val = time_UTC, .val2 = true};
    }
    if (so_at(so_byte, value, i) != '+' && so_at(so_byte, value, i) != '-') {
        return (so_R_int_bool){.val = 0, .val2 = false};
    }
    if (so_at(so_byte, value, i + 3) != ':') {
        return (so_R_int_bool){.val = 0, .val2 = false};
    }
    so_int h = parse2(value, i + 1);
    so_int m = parse2(value, i + 4);
    if ((h | m) < 0) {
        return (so_R_int_bool){.val = 0, .val2 = false};
    }
    time_Offset off = (time_Offset)(h * 3600 + m * 60);
    if (so_at(so_byte, value, i) == '-') {
        off = -off;
    }
    return (so_R_int_bool){.val = off, .val2 = true};
}

// parse2 parses a 2-digit decimal from s at position i. Returns -1 if invalid.
static so_int parse2(so_String s, so_int i) {
    so_int d1 = (so_int)(so_at(so_byte, s, i) - '0');
    so_int d2 = (so_int)(so_at(so_byte, s, i + 1) - '0');
    if ((d1 | d2) > 9) {
        return -1;
    }
    return d1 * 10 + d2;
}

// parse4 parses a 4-digit decimal from s at position i. Returns -1 if invalid.
static so_int parse4(so_String s, so_int i) {
    so_int d1 = (so_int)(so_at(so_byte, s, i) - '0');
    so_int d2 = (so_int)(so_at(so_byte, s, i + 1) - '0');
    so_int d3 = (so_int)(so_at(so_byte, s, i + 2) - '0');
    so_int d4 = (so_int)(so_at(so_byte, s, i + 3) - '0');
    if ((((d1 | d2) | d3) | d4) > 9) {
        return -1;
    }
    return d1 * 1000 + d2 * 100 + d3 * 10 + d4;
}

// parse9 parses a 9-digit decimal from s at position i. Returns -1 if invalid.
static so_int parse9(so_String s, so_int i) {
    so_int n = 0;
    for (so_int j = 0; j < 9; j++) {
        so_int d = (so_int)(so_at(so_byte, s, i + j) - '0');
        if (d > 9) {
            return -1;
        }
        n = n * 10 + d;
    }
    return n;
}

// -- time.go --

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
time_Time time_Date(so_int year, time_Month month, so_int day, so_int hour, so_int min, so_int sec, so_int nsec, time_Offset offset) {
    // Normalize month, overflowing into year.
    so_int m = (so_int)(month) - 1;
    so_R_int_int _res1 = norm(year, m, 12);
    year = _res1.val;
    m = _res1.val2;
    month = (time_Month)(m) + 1;
    // Normalize nsec, sec, min, hour, overflowing into day.
    so_R_int_int _res2 = norm(sec, nsec, 1000000000);
    sec = _res2.val;
    nsec = _res2.val2;
    so_R_int_int _res3 = norm(min, sec, 60);
    min = _res3.val;
    sec = _res3.val2;
    so_R_int_int _res4 = norm(hour, min, 60);
    hour = _res4.val;
    min = _res4.val2;
    so_R_int_int _res5 = norm(day, hour, 24);
    day = _res5.val;
    hour = _res5.val2;
    // Convert to absolute time and then Unix time.
    int64_t unixSec = (int64_t)(dateToAbsDays((int64_t)(year), month, day)) * secondsPerDay + (int64_t)(hour * secondsPerHour + min * secondsPerMinute + sec) + absoluteToUnix;
    // Adjust to UTC by subtracting the offset.
    if (offset != 0) {
        unixSec -= (int64_t)(offset);
    }
    return unixTime(unixSec, (int32_t)(nsec));
}

// Now returns the current time in UTC.
time_Time time_Now(void) {
    so_R_i64_i32 _res1 = time_wall();
    int64_t sec = _res1.val;
    int32_t nsec = _res1.val2;
    int64_t mono = time_mono();
    if (mono == 0) {
        return (time_Time){(uint64_t)(nsec), sec + unixToInternal};
    }
    mono -= monoStart;
    sec += unixToInternal - minWall;
    if (((uint64_t)(sec) >> 33) != 0) {
        // Seconds field overflowed the 33 bits available when
        // storing a monotonic time. This will be true after
        // March 16, 2157.
        return (time_Time){(uint64_t)(nsec), sec + minWall};
    }
    uint64_t wall = ((hasMonotonic | ((uint64_t)(sec) << nsecShift)) | (uint64_t)(nsec));
    return (time_Time){wall, mono};
}

// These helpers for manipulating the wall and monotonic clock readings
// take pointer receivers, even when they don't modify the time,
// to make them cheaper to call.
// IsZero reports whether t represents the zero time instant,
// January 1, year 1, 00:00:00 UTC.
bool time_Time_IsZero(time_Time t) {
    // If hasMonotonic is set in t.wall, then the time can't be before 1885,
    // so it can't be the year 1.
    // If hasMonotonic is zero, then all the bits in wall other than the
    // nanoseconds field should be 0.
    // So if there are no nanoseconds then t.wall == 0, and if there are
    // no seconds then t.ext == 0.
    // This is equivalent to t.sec() == 0 && t.nsec() == 0, but is more efficient.
    return t.wall == 0 && t.ext == 0;
}

// After reports whether the time instant t is after u.
bool time_Time_After(time_Time t, time_Time u) {
    if (((t.wall & u.wall) & hasMonotonic) != 0) {
        return t.ext > u.ext;
    }
    int64_t ts = time_Time_sec(&t);
    int64_t us = time_Time_sec(&u);
    return ts > us || (ts == us && time_Time_nsec(&t) > time_Time_nsec(&u));
}

// Before reports whether the time instant t is before u.
bool time_Time_Before(time_Time t, time_Time u) {
    if (((t.wall & u.wall) & hasMonotonic) != 0) {
        return t.ext < u.ext;
    }
    int64_t ts = time_Time_sec(&t);
    int64_t us = time_Time_sec(&u);
    return ts < us || (ts == us && time_Time_nsec(&t) < time_Time_nsec(&u));
}

// Compare compares the time instant t with u. If t is before u, it returns -1;
// if t is after u, it returns +1; if they're the same, it returns 0.
so_int time_Time_Compare(time_Time t, time_Time u) {
    int64_t tc = 0, uc = 0;
    if (((t.wall & u.wall) & hasMonotonic) != 0) {
        tc = t.ext;
        uc = u.ext;
    } else {
        tc = time_Time_sec(&t);
        uc = time_Time_sec(&u);
        if (tc == uc) {
            tc = (int64_t)(time_Time_nsec(&t));
            uc = (int64_t)(time_Time_nsec(&u));
        }
    }
    if (tc < uc) {
        return -1;
    } else if (tc > uc) {
        return +1;
    }
    return 0;
}

// Equal reports whether t and u represent the same time instant.
// Unlike the == operator, Equal ignores monotonic clock readings.
bool time_Time_Equal(time_Time t, time_Time u) {
    if (((t.wall & u.wall) & hasMonotonic) != 0) {
        return t.ext == u.ext;
    }
    return time_Time_sec(&t) == time_Time_sec(&u) && time_Time_nsec(&t) == time_Time_nsec(&u);
}

// nsec returns the time's nanoseconds.
static int32_t time_Time_nsec(void* self) {
    time_Time* t = self;
    return (int32_t)(t->wall & nsecMask);
}

// sec returns the time's seconds since Jan 1 year 1.
static int64_t time_Time_sec(void* self) {
    time_Time* t = self;
    if ((t->wall & hasMonotonic) != 0) {
        return wallToInternal + (int64_t)((t->wall << 1) >> (nsecShift + 1));
    }
    return t->ext;
}

// stripMono strips the monotonic clock reading in t.
static void time_Time_stripMono(void* self) {
    time_Time* t = self;
    if ((t->wall & hasMonotonic) != 0) {
        t->ext = time_Time_sec(t);
        t->wall &= nsecMask;
    }
}

// norm returns nhi, nlo such that
//
//	hi * base + lo == nhi * base + nlo
//	0 <= nlo < base
static so_R_int_int norm(so_int hi, so_int lo, so_int base) {
    if (lo < 0) {
        so_int n = (-lo - 1) / base + 1;
        hi -= n;
        lo += n * base;
    }
    if (lo >= base) {
        so_int n = lo / base;
        hi += n;
        lo -= n * base;
    }
    return (so_R_int_int){.val = hi, .val2 = lo};
}

// -- unix.go --

// Unix returns t as a Unix time, the number of seconds elapsed
// since January 1, 1970 UTC.
//
// Unix-like operating systems often record time as a 32-bit
// count of seconds, but since the method here returns a 64-bit
// value it is valid for billions of years into the past or future.
int64_t time_Time_Unix(time_Time t) {
    return time_Time_unixSec(&t);
}

// UnixMilli returns t as a Unix time, the number of milliseconds elapsed since
// January 1, 1970 UTC. The result is undefined if the Unix time in
// milliseconds cannot be represented by an int64 (a date more than 292 million
// years before or after 1970).
int64_t time_Time_UnixMilli(time_Time t) {
    return time_Time_unixSec(&t) * 1000 + (int64_t)(time_Time_nsec(&t)) / 1000000;
}

// UnixMicro returns t as a Unix time, the number of microseconds elapsed since
// January 1, 1970 UTC. The result is undefined if the Unix time in
// microseconds cannot be represented by an int64 (a date before year -290307 or
// after year 294246).
int64_t time_Time_UnixMicro(time_Time t) {
    return time_Time_unixSec(&t) * 1000000 + (int64_t)(time_Time_nsec(&t)) / 1000;
}

// UnixNano returns t as a Unix time, the number of nanoseconds elapsed
// since January 1, 1970 UTC. The result is undefined if the Unix time
// in nanoseconds cannot be represented by an int64 (a date before the year
// 1678 or after 2262). Note that this means the result of calling UnixNano
// on the zero Time is undefined.
int64_t time_Time_UnixNano(time_Time t) {
    return (time_Time_unixSec(&t)) * 1000000000 + (int64_t)(time_Time_nsec(&t));
}

// Unix returns the local Time corresponding to the given Unix time,
// sec seconds and nsec nanoseconds since January 1, 1970 UTC.
// It is valid to pass nsec outside the range [0, 999999999].
// Not all sec values have a corresponding time value. One such
// value is 1<<63-1 (the largest int64 value).
time_Time time_Unix(int64_t sec, int64_t nsec) {
    if (nsec < 0 || nsec >= 1000000000) {
        int64_t n = nsec / 1000000000;
        sec += n;
        nsec -= n * 1000000000;
        if (nsec < 0) {
            nsec += 1000000000;
            sec--;
        }
    }
    return unixTime(sec, (int32_t)(nsec));
}

// UnixMilli returns the local Time corresponding to the given Unix time,
// msec milliseconds since January 1, 1970 UTC.
time_Time time_UnixMilli(int64_t msec) {
    return time_Unix(msec / 1000, (msec % 1000) * 1000000);
}

// UnixMicro returns the local Time corresponding to the given Unix time,
// usec microseconds since January 1, 1970 UTC.
time_Time time_UnixMicro(int64_t usec) {
    return time_Unix(usec / 1000000, (usec % 1000000) * 1000);
}

// unixSec returns the time's seconds since Jan 1 1970 (Unix time).
static int64_t time_Time_unixSec(void* self) {
    time_Time* t = self;
    return time_Time_sec(t) + internalToUnix;
}

static time_Time unixTime(int64_t sec, int32_t nsec) {
    return (time_Time){(uint64_t)(nsec), sec + unixToInternal};
}

// -- zoneinfo.go --

static void __attribute__((constructor)) time_init() {
    monoStart = time_mono() - 1;
}
