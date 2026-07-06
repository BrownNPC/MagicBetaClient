#include "strconv.h"

// -- Types --

typedef struct specialFloat specialFloat;
typedef struct readFloatResult readFloatResult;
typedef struct atof32Result atof32Result;
typedef struct atof64Result atof64Result;
typedef struct decimal decimal;
typedef struct leftCheat leftCheat;
typedef struct floatInfo floatInfo;
typedef struct decimalSlice decimalSlice;
typedef struct phiBeta phiBeta;
typedef struct umul192Result umul192Result;
typedef struct pow10Result pow10Result;

typedef struct specialFloat {
    double f;
    so_int n;
    bool ok;
} specialFloat;

typedef struct readFloatResult {
    uint64_t mantissa;
    so_int exp;
    bool neg;
    bool trunc;
    bool hex;
    so_int n;
    bool ok;
} readFloatResult;

typedef struct atof32Result {
    float f;
    so_int n;
    so_Error err;
} atof32Result;

typedef struct atof64Result {
    double f;
    so_int n;
    so_Error err;
} atof64Result;

typedef struct decimal {
    so_byte d[800];
    so_int nd;
    so_int dp;
    bool neg;
    bool trunc;
} decimal;

// Cheat sheet for left shift: table indexed by shift count giving
// number of new digits that will be introduced by that shift.
//
// For example, leftcheats[4] = {2, "625"}.  That means that
// if we are shifting by 4 (multiplying by 16), it will add 2 digits
// when the string prefix is "625" through "999", and one fewer digit
// if the string prefix is "000" through "624".
//
// Credit for this trick goes to Ken.
typedef struct leftCheat {
    so_int delta;
    so_String cutoff;
} leftCheat;

typedef struct floatInfo {
    so_uint mantbits;
    so_uint expbits;
    so_int bias;
} floatInfo;

typedef struct decimalSlice {
    so_Slice d;
    so_int nd;
    so_int dp;
} decimalSlice;

typedef struct phiBeta {
    so_uint128 phi;
    so_int beta;
} phiBeta;

typedef struct umul192Result {
    uint64_t hi;
    uint64_t mid;
    uint64_t lo;
} umul192Result;

typedef struct pow10Result {
    so_uint128 mant;
    so_int exp;
    bool ok;
} pow10Result;

// -- Forward declarations --
static so_int commonPrefixLenIgnoreCase(so_String s, so_String prefix);
static specialFloat special(so_String s);
static bool decimal_set(void* self, so_String s);
static readFloatResult readFloat(so_String s);
static so_R_u64_bool decimal_floatBits(void* self, floatInfo* flt);
static so_R_f64_bool atof64exact(uint64_t mantissa, so_int exp, bool neg);
static so_R_f32_bool atof32exact(uint64_t mantissa, so_int exp, bool neg);
static so_R_f64_err atofHex(so_String s, floatInfo* flt, uint64_t mantissa, so_int exp, bool neg, bool trunc);
static atof32Result atof32(so_String s);
static atof64Result atof64(so_String s);
static atof64Result parseFloatPrefix(so_String s, so_int bitSize);
static so_R_f64_bool eiselLemire64(uint64_t man, so_int exp10, bool neg);
static so_R_f32_bool eiselLemire32(uint64_t man, so_int exp10, bool neg);
static so_byte lower(so_byte c);
static bool underscoreOK(so_String s);
static void trim(decimal* a);
static void decimal_Assign(void* self, uint64_t v);
static void rightShift(decimal* a, so_uint k);
static bool prefixIsLessThan(so_Slice b, so_String s);
static void leftShift(decimal* a, so_uint k);
static void decimal_Shift(void* self, so_int k);
static bool shouldRoundUp(decimal* a, so_int nd);
static void decimal_Round(void* self, so_int nd);
static void decimal_RoundDown(void* self, so_int nd);
static void decimal_RoundUp(void* self, so_int nd);
static uint64_t decimal_RoundedInteger(void* self);
static double float64frombits(uint64_t b);
static float float32frombits(uint32_t b);
static uint64_t float64bits(double f);
static uint32_t float32bits(float f);
static double floatInf(so_int sign);
static double floatNaN(void);
static so_Slice genericFtoa(so_Slice dst, double val, so_byte fmt, so_int prec, so_int bitSize);
static so_Slice bigFtoa(so_Slice dst, so_int prec, so_byte fmt, bool neg, uint64_t mant, so_int exp, floatInfo* flt);
static so_Slice formatDigits(so_Slice dst, bool shortest, bool neg, decimalSlice digs, so_int prec, so_byte fmt);
static void roundShortest(decimal* d, uint64_t mant, so_int exp, floatInfo* flt);
static so_Slice fmtE(so_Slice dst, bool neg, decimalSlice d, so_int prec, so_byte fmt);
static so_Slice fmtF(so_Slice dst, bool neg, decimalSlice d, so_int prec);
static so_Slice fmtB(so_Slice dst, bool neg, uint64_t mant, so_int exp, floatInfo* flt);
static so_Slice fmtX(so_Slice dst, so_int prec, so_byte fmt, bool neg, uint64_t mant, so_int exp, floatInfo* flt);
static void dboxFtoa(decimalSlice* d, uint64_t mant, so_int exp, bool denorm, so_int bitSize);
static void dboxFtoa64(decimalSlice* d, uint64_t mant, so_int exp, bool denorm);
static void dboxFtoa32(decimalSlice* d, uint32_t mant, so_int exp, bool denorm);
static void dboxDigits(decimalSlice* d, uint64_t mant, so_int exp);
static so_uint128 uadd128(so_uint128 u, uint64_t n);
static uint64_t umul64(uint32_t x, uint32_t y);
static uint64_t umul96Upper64(uint32_t x, uint64_t y);
static uint64_t umul96Lower64(uint32_t x, uint64_t y);
static uint64_t umul128Upper64(uint64_t x, uint64_t y);
static so_uint128 umul192Upper128(uint64_t x, so_uint128 y);
static so_uint128 umul192Lower128(uint64_t x, so_uint128 y);
static so_R_u64_bool dboxMulPow64(uint64_t u, so_uint128 phi);
static so_R_u32_bool dboxMulPow32(uint32_t u, uint64_t phi);
static so_R_bool_bool dboxParity64(uint64_t mant2, so_uint128 phi, so_int beta);
static so_R_bool_bool dboxParity32(uint32_t mant2, uint64_t phi, so_int beta);
static uint32_t dboxDelta64(so_uint128 phi, so_int beta);
static uint32_t dboxDelta32(uint64_t phi, so_int beta);
static so_int mulLog10_2MinusLog10_4Over3(so_int e);
static so_R_u64_u64 dboxRange64(so_uint128 phi, so_int beta);
static so_R_u32_u32 dboxRange32(uint64_t phi, so_int beta);
static uint64_t dboxRoundUp64(so_uint128 phi, so_int beta);
static uint32_t dboxRoundUp32(uint64_t phi, so_int beta);
static phiBeta dboxPow64(so_int k, so_int e);
static so_R_u64_int dboxPow32(so_int k, so_int e);
static void fixedFtoa(decimalSlice* d, uint64_t mant, so_int exp, so_int digits, so_int prec, so_byte fmt);
static so_Slice formatBits(so_Slice dst, uint64_t u, so_int base, bool neg);
static bool isPowerOfTwo(so_int x);
static so_String small(so_int i);
static so_int formatBase10(so_Slice a, uint64_t u);
static so_uint128 umul128(uint64_t x, uint64_t y);
static umul192Result umul192(uint64_t x, so_uint128 y);
static pow10Result intPow10(so_int e);
static so_int mulLog10_2(so_int x);
static so_int mulLog2_10(so_int x);
static so_uint bool2uint(bool b);
static bool divisiblePow5(uint64_t x, so_int p);
static so_R_u64_int trimZeros(uint64_t x);

// -- Variables and constants --

// decimal power of ten to binary power of two.
static so_Slice powtab = (so_Slice){(so_int[9]){1, 3, 6, 9, 13, 16, 19, 23, 26}, 9, 9};

// Exact powers of 10.
static so_Slice float64pow10 = (so_Slice){(double[23]){1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22}, 23, 23};
static so_Slice float32pow10 = (so_Slice){(float[11]){1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10}, 11, 11};
so_Error strconv_ErrRange = errors_New("value out of range");
so_Error strconv_ErrSyntax = errors_New("invalid syntax");
so_Error strconv_ErrBase = errors_New("invalid base");
so_Error strconv_ErrBitSize = errors_New("invalid bit size");
so_Error strconv_ErrUnknown = errors_New("unknown error");

// Maximum shift that we can do in one pass without overflow.
// A uint has 32 or 64 bits, and we have to be able to accommodate 9<<k.
static const int64_t uintSize = ((int64_t)32 << ((uint64_t)(~(so_uint)(0)) >> 63));
static const int64_t maxShift = uintSize - 4;
static so_Slice leftcheats = (so_Slice){(leftCheat[61]){(leftCheat){0, so_str("")}, (leftCheat){1, so_str("5")}, (leftCheat){1, so_str("25")}, (leftCheat){1, so_str("125")}, (leftCheat){2, so_str("625")}, (leftCheat){2, so_str("3125")}, (leftCheat){2, so_str("15625")}, (leftCheat){3, so_str("78125")}, (leftCheat){3, so_str("390625")}, (leftCheat){3, so_str("1953125")}, (leftCheat){4, so_str("9765625")}, (leftCheat){4, so_str("48828125")}, (leftCheat){4, so_str("244140625")}, (leftCheat){4, so_str("1220703125")}, (leftCheat){5, so_str("6103515625")}, (leftCheat){5, so_str("30517578125")}, (leftCheat){5, so_str("152587890625")}, (leftCheat){6, so_str("762939453125")}, (leftCheat){6, so_str("3814697265625")}, (leftCheat){6, so_str("19073486328125")}, (leftCheat){7, so_str("95367431640625")}, (leftCheat){7, so_str("476837158203125")}, (leftCheat){7, so_str("2384185791015625")}, (leftCheat){7, so_str("11920928955078125")}, (leftCheat){8, so_str("59604644775390625")}, (leftCheat){8, so_str("298023223876953125")}, (leftCheat){8, so_str("1490116119384765625")}, (leftCheat){9, so_str("7450580596923828125")}, (leftCheat){9, so_str("37252902984619140625")}, (leftCheat){9, so_str("186264514923095703125")}, (leftCheat){10, so_str("931322574615478515625")}, (leftCheat){10, so_str("4656612873077392578125")}, (leftCheat){10, so_str("23283064365386962890625")}, (leftCheat){10, so_str("116415321826934814453125")}, (leftCheat){11, so_str("582076609134674072265625")}, (leftCheat){11, so_str("2910383045673370361328125")}, (leftCheat){11, so_str("14551915228366851806640625")}, (leftCheat){12, so_str("72759576141834259033203125")}, (leftCheat){12, so_str("363797880709171295166015625")}, (leftCheat){12, so_str("1818989403545856475830078125")}, (leftCheat){13, so_str("9094947017729282379150390625")}, (leftCheat){13, so_str("45474735088646411895751953125")}, (leftCheat){13, so_str("227373675443232059478759765625")}, (leftCheat){13, so_str("1136868377216160297393798828125")}, (leftCheat){14, so_str("5684341886080801486968994140625")}, (leftCheat){14, so_str("28421709430404007434844970703125")}, (leftCheat){14, so_str("142108547152020037174224853515625")}, (leftCheat){15, so_str("710542735760100185871124267578125")}, (leftCheat){15, so_str("3552713678800500929355621337890625")}, (leftCheat){15, so_str("17763568394002504646778106689453125")}, (leftCheat){16, so_str("88817841970012523233890533447265625")}, (leftCheat){16, so_str("444089209850062616169452667236328125")}, (leftCheat){16, so_str("2220446049250313080847263336181640625")}, (leftCheat){16, so_str("11102230246251565404236316680908203125")}, (leftCheat){17, so_str("55511151231257827021181583404541015625")}, (leftCheat){17, so_str("277555756156289135105907917022705078125")}, (leftCheat){17, so_str("1387778780781445675529539585113525390625")}, (leftCheat){18, so_str("6938893903907228377647697925567626953125")}, (leftCheat){18, so_str("34694469519536141888238489627838134765625")}, (leftCheat){18, so_str("173472347597680709441192448139190673828125")}, (leftCheat){19, so_str("867361737988403547205962240695953369140625")}}, 61, 61};
static const so_String lowerhex = so_str("0123456789abcdef");
static const so_String upperhex = so_str("0123456789ABCDEF");
static const int64_t float32MantBits = 23;
static const int64_t float32ExpBits = 8;
static const int64_t float32Bias = -127;
static const int64_t float64MantBits = 52;
static const int64_t float64ExpBits = 11;
static const int64_t float64Bias = -1023;
static floatInfo float32info = (floatInfo){float32MantBits, float32ExpBits, float32Bias};
static floatInfo float64info = (floatInfo){float64MantBits, float64ExpBits, float64Bias};
static const int64_t floatMantBits64 = 52;
static const int64_t floatMantBits32 = 23;
static uint64_t uint64pow10[20] = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19};
static const so_String digits = so_str("0123456789abcdefghijklmnopqrstuvwxyz");
static const int64_t nSmalls = 100;

// smalls is the formatting of 00..99 concatenated.
// It is then padded out with 56 x's to 256 bytes,
// so that smalls[x&0xFF] has no bounds check.
static const so_String smalls = so_str("00010203040506070809" "10111213141516171819" "20212223242526272829" "30313233343536373839" "40414243444546474849" "50515253545556575859" "60616263646566676869" "70717273747576777879" "80818283848586878889" "90919293949596979899");
static const bool host64bit = ((uint64_t)(~(so_uint)(0)) >> 32) != 0;
static const uint64_t maxUint64 = 0xFFFFFFFFFFFFFFFF;

// div5Tab[p-1] is the multiplicative inverse of 5^p and maxUint64/5^p.
static uint64_t div5Tab[22][2] = {{0xcccccccccccccccd, maxUint64 / 5}, {0x8f5c28f5c28f5c29, maxUint64 / 5 / 5}, {0x1cac083126e978d5, maxUint64 / 5 / 5 / 5}, {0xd288ce703afb7e91, maxUint64 / 5 / 5 / 5 / 5}, {0x5d4e8fb00bcbe61d, maxUint64 / 5 / 5 / 5 / 5 / 5}, {0x790fb65668c26139, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5}, {0xe5032477ae8d46a5, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0xc767074b22e90e21, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x8e47ce423a2e9c6d, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x4fa7f60d3ed61f49, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x0fee64690c913975, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x3662e0e1cf503eb1, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0xa47a2cf9f6433fbd, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x54186f653140a659, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x7738164770402145, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0xe4a4d1417cd9a041, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0xc75429d9e5c5200d, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0xc1773b91fac10669, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x26b172506559ce15, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0xd489e3a9addec2d1, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x90e860bb892c8d5d, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}, {0x502e79bf1b6f4f79, maxUint64 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5 / 5}};
static const int64_t pow10Min = -348;
static const int64_t pow10Max = 347;

// pow10Tab holds 128-bit mantissas of powers of 10.
// The values are scaled so the high bit is always set; there is no "implicit leading 1 bit".
static so_uint128 pow10Tab[696] = {(so_uint128){0xfa8fd5a0081c0288, 0x1732c869cd60e453}, (so_uint128){0x9c99e58405118195, 0x0e7fbd42205c8eb4}, (so_uint128){0xc3c05ee50655e1fa, 0x521fac92a873b261}, (so_uint128){0xf4b0769e47eb5a78, 0xe6a797b752909ef9}, (so_uint128){0x98ee4a22ecf3188b, 0x9028bed2939a635c}, (so_uint128){0xbf29dcaba82fdeae, 0x7432ee873880fc33}, (so_uint128){0xeef453d6923bd65a, 0x113faa2906a13b3f}, (so_uint128){0x9558b4661b6565f8, 0x4ac7ca59a424c507}, (so_uint128){0xbaaee17fa23ebf76, 0x5d79bcf00d2df649}, (so_uint128){0xe95a99df8ace6f53, 0xf4d82c2c107973dc}, (so_uint128){0x91d8a02bb6c10594, 0x79071b9b8a4be869}, (so_uint128){0xb64ec836a47146f9, 0x9748e2826cdee284}, (so_uint128){0xe3e27a444d8d98b7, 0xfd1b1b2308169b25}, (so_uint128){0x8e6d8c6ab0787f72, 0xfe30f0f5e50e20f7}, (so_uint128){0xb208ef855c969f4f, 0xbdbd2d335e51a935}, (so_uint128){0xde8b2b66b3bc4723, 0xad2c788035e61382}, (so_uint128){0x8b16fb203055ac76, 0x4c3bcb5021afcc31}, (so_uint128){0xaddcb9e83c6b1793, 0xdf4abe242a1bbf3d}, (so_uint128){0xd953e8624b85dd78, 0xd71d6dad34a2af0d}, (so_uint128){0x87d4713d6f33aa6b, 0x8672648c40e5ad68}, (so_uint128){0xa9c98d8ccb009506, 0x680efdaf511f18c2}, (so_uint128){0xd43bf0effdc0ba48, 0x0212bd1b2566def2}, (so_uint128){0x84a57695fe98746d, 0x014bb630f7604b57}, (so_uint128){0xa5ced43b7e3e9188, 0x419ea3bd35385e2d}, (so_uint128){0xcf42894a5dce35ea, 0x52064cac828675b9}, (so_uint128){0x818995ce7aa0e1b2, 0x7343efebd1940993}, (so_uint128){0xa1ebfb4219491a1f, 0x1014ebe6c5f90bf8}, (so_uint128){0xca66fa129f9b60a6, 0xd41a26e077774ef6}, (so_uint128){0xfd00b897478238d0, 0x8920b098955522b4}, (so_uint128){0x9e20735e8cb16382, 0x55b46e5f5d5535b0}, (so_uint128){0xc5a890362fddbc62, 0xeb2189f734aa831d}, (so_uint128){0xf712b443bbd52b7b, 0xa5e9ec7501d523e4}, (so_uint128){0x9a6bb0aa55653b2d, 0x47b233c92125366e}, (so_uint128){0xc1069cd4eabe89f8, 0x999ec0bb696e840a}, (so_uint128){0xf148440a256e2c76, 0xc00670ea43ca250d}, (so_uint128){0x96cd2a865764dbca, 0x380406926a5e5728}, (so_uint128){0xbc807527ed3e12bc, 0xc605083704f5ecf2}, (so_uint128){0xeba09271e88d976b, 0xf7864a44c633682e}, (so_uint128){0x93445b8731587ea3, 0x7ab3ee6afbe0211d}, (so_uint128){0xb8157268fdae9e4c, 0x5960ea05bad82964}, (so_uint128){0xe61acf033d1a45df, 0x6fb92487298e33bd}, (so_uint128){0x8fd0c16206306bab, 0xa5d3b6d479f8e056}, (so_uint128){0xb3c4f1ba87bc8696, 0x8f48a4899877186c}, (so_uint128){0xe0b62e2929aba83c, 0x331acdabfe94de87}, (so_uint128){0x8c71dcd9ba0b4925, 0x9ff0c08b7f1d0b14}, (so_uint128){0xaf8e5410288e1b6f, 0x07ecf0ae5ee44dd9}, (so_uint128){0xdb71e91432b1a24a, 0xc9e82cd9f69d6150}, (so_uint128){0x892731ac9faf056e, 0xbe311c083a225cd2}, (so_uint128){0xab70fe17c79ac6ca, 0x6dbd630a48aaf406}, (so_uint128){0xd64d3d9db981787d, 0x092cbbccdad5b108}, (so_uint128){0x85f0468293f0eb4e, 0x25bbf56008c58ea5}, (so_uint128){0xa76c582338ed2621, 0xaf2af2b80af6f24e}, (so_uint128){0xd1476e2c07286faa, 0x1af5af660db4aee1}, (so_uint128){0x82cca4db847945ca, 0x50d98d9fc890ed4d}, (so_uint128){0xa37fce126597973c, 0xe50ff107bab528a0}, (so_uint128){0xcc5fc196fefd7d0c, 0x1e53ed49a96272c8}, (so_uint128){0xff77b1fcbebcdc4f, 0x25e8e89c13bb0f7a}, (so_uint128){0x9faacf3df73609b1, 0x77b191618c54e9ac}, (so_uint128){0xc795830d75038c1d, 0xd59df5b9ef6a2417}, (so_uint128){0xf97ae3d0d2446f25, 0x4b0573286b44ad1d}, (so_uint128){0x9becce62836ac577, 0x4ee367f9430aec32}, (so_uint128){0xc2e801fb244576d5, 0x229c41f793cda73f}, (so_uint128){0xf3a20279ed56d48a, 0x6b43527578c1110f}, (so_uint128){0x9845418c345644d6, 0x830a13896b78aaa9}, (so_uint128){0xbe5691ef416bd60c, 0x23cc986bc656d553}, (so_uint128){0xedec366b11c6cb8f, 0x2cbfbe86b7ec8aa8}, (so_uint128){0x94b3a202eb1c3f39, 0x7bf7d71432f3d6a9}, (so_uint128){0xb9e08a83a5e34f07, 0xdaf5ccd93fb0cc53}, (so_uint128){0xe858ad248f5c22c9, 0xd1b3400f8f9cff68}, (so_uint128){0x91376c36d99995be, 0x23100809b9c21fa1}, (so_uint128){0xb58547448ffffb2d, 0xabd40a0c2832a78a}, (so_uint128){0xe2e69915b3fff9f9, 0x16c90c8f323f516c}, (so_uint128){0x8dd01fad907ffc3b, 0xae3da7d97f6792e3}, (so_uint128){0xb1442798f49ffb4a, 0x99cd11cfdf41779c}, (so_uint128){0xdd95317f31c7fa1d, 0x40405643d711d583}, (so_uint128){0x8a7d3eef7f1cfc52, 0x482835ea666b2572}, (so_uint128){0xad1c8eab5ee43b66, 0xda3243650005eecf}, (so_uint128){0xd863b256369d4a40, 0x90bed43e40076a82}, (so_uint128){0x873e4f75e2224e68, 0x5a7744a6e804a291}, (so_uint128){0xa90de3535aaae202, 0x711515d0a205cb36}, (so_uint128){0xd3515c2831559a83, 0x0d5a5b44ca873e03}, (so_uint128){0x8412d9991ed58091, 0xe858790afe9486c2}, (so_uint128){0xa5178fff668ae0b6, 0x626e974dbe39a872}, (so_uint128){0xce5d73ff402d98e3, 0xfb0a3d212dc8128f}, (so_uint128){0x80fa687f881c7f8e, 0x7ce66634bc9d0b99}, (so_uint128){0xa139029f6a239f72, 0x1c1fffc1ebc44e80}, (so_uint128){0xc987434744ac874e, 0xa327ffb266b56220}, (so_uint128){0xfbe9141915d7a922, 0x4bf1ff9f0062baa8}, (so_uint128){0x9d71ac8fada6c9b5, 0x6f773fc3603db4a9}, (so_uint128){0xc4ce17b399107c22, 0xcb550fb4384d21d3}, (so_uint128){0xf6019da07f549b2b, 0x7e2a53a146606a48}, (so_uint128){0x99c102844f94e0fb, 0x2eda7444cbfc426d}, (so_uint128){0xc0314325637a1939, 0xfa911155fefb5308}, (so_uint128){0xf03d93eebc589f88, 0x793555ab7eba27ca}, (so_uint128){0x96267c7535b763b5, 0x4bc1558b2f3458de}, (so_uint128){0xbbb01b9283253ca2, 0x9eb1aaedfb016f16}, (so_uint128){0xea9c227723ee8bcb, 0x465e15a979c1cadc}, (so_uint128){0x92a1958a7675175f, 0x0bfacd89ec191ec9}, (so_uint128){0xb749faed14125d36, 0xcef980ec671f667b}, (so_uint128){0xe51c79a85916f484, 0x82b7e12780e7401a}, (so_uint128){0x8f31cc0937ae58d2, 0xd1b2ecb8b0908810}, (so_uint128){0xb2fe3f0b8599ef07, 0x861fa7e6dcb4aa15}, (so_uint128){0xdfbdcece67006ac9, 0x67a791e093e1d49a}, (so_uint128){0x8bd6a141006042bd, 0xe0c8bb2c5c6d24e0}, (so_uint128){0xaecc49914078536d, 0x58fae9f773886e18}, (so_uint128){0xda7f5bf590966848, 0xaf39a475506a899e}, (so_uint128){0x888f99797a5e012d, 0x6d8406c952429603}, (so_uint128){0xaab37fd7d8f58178, 0xc8e5087ba6d33b83}, (so_uint128){0xd5605fcdcf32e1d6, 0xfb1e4a9a90880a64}, (so_uint128){0x855c3be0a17fcd26, 0x5cf2eea09a55067f}, (so_uint128){0xa6b34ad8c9dfc06f, 0xf42faa48c0ea481e}, (so_uint128){0xd0601d8efc57b08b, 0xf13b94daf124da26}, (so_uint128){0x823c12795db6ce57, 0x76c53d08d6b70858}, (so_uint128){0xa2cb1717b52481ed, 0x54768c4b0c64ca6e}, (so_uint128){0xcb7ddcdda26da268, 0xa9942f5dcf7dfd09}, (so_uint128){0xfe5d54150b090b02, 0xd3f93b35435d7c4c}, (so_uint128){0x9efa548d26e5a6e1, 0xc47bc5014a1a6daf}, (so_uint128){0xc6b8e9b0709f109a, 0x359ab6419ca1091b}, (so_uint128){0xf867241c8cc6d4c0, 0xc30163d203c94b62}, (so_uint128){0x9b407691d7fc44f8, 0x79e0de63425dcf1d}, (so_uint128){0xc21094364dfb5636, 0x985915fc12f542e4}, (so_uint128){0xf294b943e17a2bc4, 0x3e6f5b7b17b2939d}, (so_uint128){0x979cf3ca6cec5b5a, 0xa705992ceecf9c42}, (so_uint128){0xbd8430bd08277231, 0x50c6ff782a838353}, (so_uint128){0xece53cec4a314ebd, 0xa4f8bf5635246428}, (so_uint128){0x940f4613ae5ed136, 0x871b7795e136be99}, (so_uint128){0xb913179899f68584, 0x28e2557b59846e3f}, (so_uint128){0xe757dd7ec07426e5, 0x331aeada2fe589cf}, (so_uint128){0x9096ea6f3848984f, 0x3ff0d2c85def7621}, (so_uint128){0xb4bca50b065abe63, 0x0fed077a756b53a9}, (so_uint128){0xe1ebce4dc7f16dfb, 0xd3e8495912c62894}, (so_uint128){0x8d3360f09cf6e4bd, 0x64712dd7abbbd95c}, (so_uint128){0xb080392cc4349dec, 0xbd8d794d96aacfb3}, (so_uint128){0xdca04777f541c567, 0xecf0d7a0fc5583a0}, (so_uint128){0x89e42caaf9491b60, 0xf41686c49db57244}, (so_uint128){0xac5d37d5b79b6239, 0x311c2875c522ced5}, (so_uint128){0xd77485cb25823ac7, 0x7d633293366b828b}, (so_uint128){0x86a8d39ef77164bc, 0xae5dff9c02033197}, (so_uint128){0xa8530886b54dbdeb, 0xd9f57f830283fdfc}, (so_uint128){0xd267caa862a12d66, 0xd072df63c324fd7b}, (so_uint128){0x8380dea93da4bc60, 0x4247cb9e59f71e6d}, (so_uint128){0xa46116538d0deb78, 0x52d9be85f074e608}, (so_uint128){0xcd795be870516656, 0x67902e276c921f8b}, (so_uint128){0x806bd9714632dff6, 0x00ba1cd8a3db53b6}, (so_uint128){0xa086cfcd97bf97f3, 0x80e8a40eccd228a4}, (so_uint128){0xc8a883c0fdaf7df0, 0x6122cd128006b2cd}, (so_uint128){0xfad2a4b13d1b5d6c, 0x796b805720085f81}, (so_uint128){0x9cc3a6eec6311a63, 0xcbe3303674053bb0}, (so_uint128){0xc3f490aa77bd60fc, 0xbedbfc4411068a9c}, (so_uint128){0xf4f1b4d515acb93b, 0xee92fb5515482d44}, (so_uint128){0x991711052d8bf3c5, 0x751bdd152d4d1c4a}, (so_uint128){0xbf5cd54678eef0b6, 0xd262d45a78a0635d}, (so_uint128){0xef340a98172aace4, 0x86fb897116c87c34}, (so_uint128){0x9580869f0e7aac0e, 0xd45d35e6ae3d4da0}, (so_uint128){0xbae0a846d2195712, 0x8974836059cca109}, (so_uint128){0xe998d258869facd7, 0x2bd1a438703fc94b}, (so_uint128){0x91ff83775423cc06, 0x7b6306a34627ddcf}, (so_uint128){0xb67f6455292cbf08, 0x1a3bc84c17b1d542}, (so_uint128){0xe41f3d6a7377eeca, 0x20caba5f1d9e4a93}, (so_uint128){0x8e938662882af53e, 0x547eb47b7282ee9c}, (so_uint128){0xb23867fb2a35b28d, 0xe99e619a4f23aa43}, (so_uint128){0xdec681f9f4c31f31, 0x6405fa00e2ec94d4}, (so_uint128){0x8b3c113c38f9f37e, 0xde83bc408dd3dd04}, (so_uint128){0xae0b158b4738705e, 0x9624ab50b148d445}, (so_uint128){0xd98ddaee19068c76, 0x3badd624dd9b0957}, (so_uint128){0x87f8a8d4cfa417c9, 0xe54ca5d70a80e5d6}, (so_uint128){0xa9f6d30a038d1dbc, 0x5e9fcf4ccd211f4c}, (so_uint128){0xd47487cc8470652b, 0x7647c3200069671f}, (so_uint128){0x84c8d4dfd2c63f3b, 0x29ecd9f40041e073}, (so_uint128){0xa5fb0a17c777cf09, 0xf468107100525890}, (so_uint128){0xcf79cc9db955c2cc, 0x7182148d4066eeb4}, (so_uint128){0x81ac1fe293d599bf, 0xc6f14cd848405530}, (so_uint128){0xa21727db38cb002f, 0xb8ada00e5a506a7c}, (so_uint128){0xca9cf1d206fdc03b, 0xa6d90811f0e4851c}, (so_uint128){0xfd442e4688bd304a, 0x908f4a166d1da663}, (so_uint128){0x9e4a9cec15763e2e, 0x9a598e4e043287fe}, (so_uint128){0xc5dd44271ad3cdba, 0x40eff1e1853f29fd}, (so_uint128){0xf7549530e188c128, 0xd12bee59e68ef47c}, (so_uint128){0x9a94dd3e8cf578b9, 0x82bb74f8301958ce}, (so_uint128){0xc13a148e3032d6e7, 0xe36a52363c1faf01}, (so_uint128){0xf18899b1bc3f8ca1, 0xdc44e6c3cb279ac1}, (so_uint128){0x96f5600f15a7b7e5, 0x29ab103a5ef8c0b9}, (so_uint128){0xbcb2b812db11a5de, 0x7415d448f6b6f0e7}, (so_uint128){0xebdf661791d60f56, 0x111b495b3464ad21}, (so_uint128){0x936b9fcebb25c995, 0xcab10dd900beec34}, (so_uint128){0xb84687c269ef3bfb, 0x3d5d514f40eea742}, (so_uint128){0xe65829b3046b0afa, 0x0cb4a5a3112a5112}, (so_uint128){0x8ff71a0fe2c2e6dc, 0x47f0e785eaba72ab}, (so_uint128){0xb3f4e093db73a093, 0x59ed216765690f56}, (so_uint128){0xe0f218b8d25088b8, 0x306869c13ec3532c}, (so_uint128){0x8c974f7383725573, 0x1e414218c73a13fb}, (so_uint128){0xafbd2350644eeacf, 0xe5d1929ef90898fa}, (so_uint128){0xdbac6c247d62a583, 0xdf45f746b74abf39}, (so_uint128){0x894bc396ce5da772, 0x6b8bba8c328eb783}, (so_uint128){0xab9eb47c81f5114f, 0x066ea92f3f326564}, (so_uint128){0xd686619ba27255a2, 0xc80a537b0efefebd}, (so_uint128){0x8613fd0145877585, 0xbd06742ce95f5f36}, (so_uint128){0xa798fc4196e952e7, 0x2c48113823b73704}, (so_uint128){0xd17f3b51fca3a7a0, 0xf75a15862ca504c5}, (so_uint128){0x82ef85133de648c4, 0x9a984d73dbe722fb}, (so_uint128){0xa3ab66580d5fdaf5, 0xc13e60d0d2e0ebba}, (so_uint128){0xcc963fee10b7d1b3, 0x318df905079926a8}, (so_uint128){0xffbbcfe994e5c61f, 0xfdf17746497f7052}, (so_uint128){0x9fd561f1fd0f9bd3, 0xfeb6ea8bedefa633}, (so_uint128){0xc7caba6e7c5382c8, 0xfe64a52ee96b8fc0}, (so_uint128){0xf9bd690a1b68637b, 0x3dfdce7aa3c673b0}, (so_uint128){0x9c1661a651213e2d, 0x06bea10ca65c084e}, (so_uint128){0xc31bfa0fe5698db8, 0x486e494fcff30a62}, (so_uint128){0xf3e2f893dec3f126, 0x5a89dba3c3efccfa}, (so_uint128){0x986ddb5c6b3a76b7, 0xf89629465a75e01c}, (so_uint128){0xbe89523386091465, 0xf6bbb397f1135823}, (so_uint128){0xee2ba6c0678b597f, 0x746aa07ded582e2c}, (so_uint128){0x94db483840b717ef, 0xa8c2a44eb4571cdc}, (so_uint128){0xba121a4650e4ddeb, 0x92f34d62616ce413}, (so_uint128){0xe896a0d7e51e1566, 0x77b020baf9c81d17}, (so_uint128){0x915e2486ef32cd60, 0x0ace1474dc1d122e}, (so_uint128){0xb5b5ada8aaff80b8, 0x0d819992132456ba}, (so_uint128){0xe3231912d5bf60e6, 0x10e1fff697ed6c69}, (so_uint128){0x8df5efabc5979c8f, 0xca8d3ffa1ef463c1}, (so_uint128){0xb1736b96b6fd83b3, 0xbd308ff8a6b17cb2}, (so_uint128){0xddd0467c64bce4a0, 0xac7cb3f6d05ddbde}, (so_uint128){0x8aa22c0dbef60ee4, 0x6bcdf07a423aa96b}, (so_uint128){0xad4ab7112eb3929d, 0x86c16c98d2c953c6}, (so_uint128){0xd89d64d57a607744, 0xe871c7bf077ba8b7}, (so_uint128){0x87625f056c7c4a8b, 0x11471cd764ad4972}, (so_uint128){0xa93af6c6c79b5d2d, 0xd598e40d3dd89bcf}, (so_uint128){0xd389b47879823479, 0x4aff1d108d4ec2c3}, (so_uint128){0x843610cb4bf160cb, 0xcedf722a585139ba}, (so_uint128){0xa54394fe1eedb8fe, 0xc2974eb4ee658828}, (so_uint128){0xce947a3da6a9273e, 0x733d226229feea32}, (so_uint128){0x811ccc668829b887, 0x0806357d5a3f525f}, (so_uint128){0xa163ff802a3426a8, 0xca07c2dcb0cf26f7}, (so_uint128){0xc9bcff6034c13052, 0xfc89b393dd02f0b5}, (so_uint128){0xfc2c3f3841f17c67, 0xbbac2078d443ace2}, (so_uint128){0x9d9ba7832936edc0, 0xd54b944b84aa4c0d}, (so_uint128){0xc5029163f384a931, 0x0a9e795e65d4df11}, (so_uint128){0xf64335bcf065d37d, 0x4d4617b5ff4a16d5}, (so_uint128){0x99ea0196163fa42e, 0x504bced1bf8e4e45}, (so_uint128){0xc06481fb9bcf8d39, 0xe45ec2862f71e1d6}, (so_uint128){0xf07da27a82c37088, 0x5d767327bb4e5a4c}, (so_uint128){0x964e858c91ba2655, 0x3a6a07f8d510f86f}, (so_uint128){0xbbe226efb628afea, 0x890489f70a55368b}, (so_uint128){0xeadab0aba3b2dbe5, 0x2b45ac74ccea842e}, (so_uint128){0x92c8ae6b464fc96f, 0x3b0b8bc90012929d}, (so_uint128){0xb77ada0617e3bbcb, 0x09ce6ebb40173744}, (so_uint128){0xe55990879ddcaabd, 0xcc420a6a101d0515}, (so_uint128){0x8f57fa54c2a9eab6, 0x9fa946824a12232d}, (so_uint128){0xb32df8e9f3546564, 0x47939822dc96abf9}, (so_uint128){0xdff9772470297ebd, 0x59787e2b93bc56f7}, (so_uint128){0x8bfbea76c619ef36, 0x57eb4edb3c55b65a}, (so_uint128){0xaefae51477a06b03, 0xede622920b6b23f1}, (so_uint128){0xdab99e59958885c4, 0xe95fab368e45eced}, (so_uint128){0x88b402f7fd75539b, 0x11dbcb0218ebb414}, (so_uint128){0xaae103b5fcd2a881, 0xd652bdc29f26a119}, (so_uint128){0xd59944a37c0752a2, 0x4be76d3346f0495f}, (so_uint128){0x857fcae62d8493a5, 0x6f70a4400c562ddb}, (so_uint128){0xa6dfbd9fb8e5b88e, 0xcb4ccd500f6bb952}, (so_uint128){0xd097ad07a71f26b2, 0x7e2000a41346a7a7}, (so_uint128){0x825ecc24c873782f, 0x8ed400668c0c28c8}, (so_uint128){0xa2f67f2dfa90563b, 0x728900802f0f32fa}, (so_uint128){0xcbb41ef979346bca, 0x4f2b40a03ad2ffb9}, (so_uint128){0xfea126b7d78186bc, 0xe2f610c84987bfa8}, (so_uint128){0x9f24b832e6b0f436, 0x0dd9ca7d2df4d7c9}, (so_uint128){0xc6ede63fa05d3143, 0x91503d1c79720dbb}, (so_uint128){0xf8a95fcf88747d94, 0x75a44c6397ce912a}, (so_uint128){0x9b69dbe1b548ce7c, 0xc986afbe3ee11aba}, (so_uint128){0xc24452da229b021b, 0xfbe85badce996168}, (so_uint128){0xf2d56790ab41c2a2, 0xfae27299423fb9c3}, (so_uint128){0x97c560ba6b0919a5, 0xdccd879fc967d41a}, (so_uint128){0xbdb6b8e905cb600f, 0x5400e987bbc1c920}, (so_uint128){0xed246723473e3813, 0x290123e9aab23b68}, (so_uint128){0x9436c0760c86e30b, 0xf9a0b6720aaf6521}, (so_uint128){0xb94470938fa89bce, 0xf808e40e8d5b3e69}, (so_uint128){0xe7958cb87392c2c2, 0xb60b1d1230b20e04}, (so_uint128){0x90bd77f3483bb9b9, 0xb1c6f22b5e6f48c2}, (so_uint128){0xb4ecd5f01a4aa828, 0x1e38aeb6360b1af3}, (so_uint128){0xe2280b6c20dd5232, 0x25c6da63c38de1b0}, (so_uint128){0x8d590723948a535f, 0x579c487e5a38ad0e}, (so_uint128){0xb0af48ec79ace837, 0x2d835a9df0c6d851}, (so_uint128){0xdcdb1b2798182244, 0xf8e431456cf88e65}, (so_uint128){0x8a08f0f8bf0f156b, 0x1b8e9ecb641b58ff}, (so_uint128){0xac8b2d36eed2dac5, 0xe272467e3d222f3f}, (so_uint128){0xd7adf884aa879177, 0x5b0ed81dcc6abb0f}, (so_uint128){0x86ccbb52ea94baea, 0x98e947129fc2b4e9}, (so_uint128){0xa87fea27a539e9a5, 0x3f2398d747b36224}, (so_uint128){0xd29fe4b18e88640e, 0x8eec7f0d19a03aad}, (so_uint128){0x83a3eeeef9153e89, 0x1953cf68300424ac}, (so_uint128){0xa48ceaaab75a8e2b, 0x5fa8c3423c052dd7}, (so_uint128){0xcdb02555653131b6, 0x3792f412cb06794d}, (so_uint128){0x808e17555f3ebf11, 0xe2bbd88bbee40bd0}, (so_uint128){0xa0b19d2ab70e6ed6, 0x5b6aceaeae9d0ec4}, (so_uint128){0xc8de047564d20a8b, 0xf245825a5a445275}, (so_uint128){0xfb158592be068d2e, 0xeed6e2f0f0d56712}, (so_uint128){0x9ced737bb6c4183d, 0x55464dd69685606b}, (so_uint128){0xc428d05aa4751e4c, 0xaa97e14c3c26b886}, (so_uint128){0xf53304714d9265df, 0xd53dd99f4b3066a8}, (so_uint128){0x993fe2c6d07b7fab, 0xe546a8038efe4029}, (so_uint128){0xbf8fdb78849a5f96, 0xde98520472bdd033}, (so_uint128){0xef73d256a5c0f77c, 0x963e66858f6d4440}, (so_uint128){0x95a8637627989aad, 0xdde7001379a44aa8}, (so_uint128){0xbb127c53b17ec159, 0x5560c018580d5d52}, (so_uint128){0xe9d71b689dde71af, 0xaab8f01e6e10b4a6}, (so_uint128){0x9226712162ab070d, 0xcab3961304ca70e8}, (so_uint128){0xb6b00d69bb55c8d1, 0x3d607b97c5fd0d22}, (so_uint128){0xe45c10c42a2b3b05, 0x8cb89a7db77c506a}, (so_uint128){0x8eb98a7a9a5b04e3, 0x77f3608e92adb242}, (so_uint128){0xb267ed1940f1c61c, 0x55f038b237591ed3}, (so_uint128){0xdf01e85f912e37a3, 0x6b6c46dec52f6688}, (so_uint128){0x8b61313bbabce2c6, 0x2323ac4b3b3da015}, (so_uint128){0xae397d8aa96c1b77, 0xabec975e0a0d081a}, (so_uint128){0xd9c7dced53c72255, 0x96e7bd358c904a21}, (so_uint128){0x881cea14545c7575, 0x7e50d64177da2e54}, (so_uint128){0xaa242499697392d2, 0xdde50bd1d5d0b9e9}, (so_uint128){0xd4ad2dbfc3d07787, 0x955e4ec64b44e864}, (so_uint128){0x84ec3c97da624ab4, 0xbd5af13bef0b113e}, (so_uint128){0xa6274bbdd0fadd61, 0xecb1ad8aeacdd58e}, (so_uint128){0xcfb11ead453994ba, 0x67de18eda5814af2}, (so_uint128){0x81ceb32c4b43fcf4, 0x80eacf948770ced7}, (so_uint128){0xa2425ff75e14fc31, 0xa1258379a94d028d}, (so_uint128){0xcad2f7f5359a3b3e, 0x096ee45813a04330}, (so_uint128){0xfd87b5f28300ca0d, 0x8bca9d6e188853fc}, (so_uint128){0x9e74d1b791e07e48, 0x775ea264cf55347d}, (so_uint128){0xc612062576589dda, 0x95364afe032a819d}, (so_uint128){0xf79687aed3eec551, 0x3a83ddbd83f52204}, (so_uint128){0x9abe14cd44753b52, 0xc4926a9672793542}, (so_uint128){0xc16d9a0095928a27, 0x75b7053c0f178293}, (so_uint128){0xf1c90080baf72cb1, 0x5324c68b12dd6338}, (so_uint128){0x971da05074da7bee, 0xd3f6fc16ebca5e03}, (so_uint128){0xbce5086492111aea, 0x88f4bb1ca6bcf584}, (so_uint128){0xec1e4a7db69561a5, 0x2b31e9e3d06c32e5}, (so_uint128){0x9392ee8e921d5d07, 0x3aff322e62439fcf}, (so_uint128){0xb877aa3236a4b449, 0x09befeb9fad487c2}, (so_uint128){0xe69594bec44de15b, 0x4c2ebe687989a9b3}, (so_uint128){0x901d7cf73ab0acd9, 0x0f9d37014bf60a10}, (so_uint128){0xb424dc35095cd80f, 0x538484c19ef38c94}, (so_uint128){0xe12e13424bb40e13, 0x2865a5f206b06fb9}, (so_uint128){0x8cbccc096f5088cb, 0xf93f87b7442e45d3}, (so_uint128){0xafebff0bcb24aafe, 0xf78f69a51539d748}, (so_uint128){0xdbe6fecebdedd5be, 0xb573440e5a884d1b}, (so_uint128){0x89705f4136b4a597, 0x31680a88f8953030}, (so_uint128){0xabcc77118461cefc, 0xfdc20d2b36ba7c3d}, (so_uint128){0xd6bf94d5e57a42bc, 0x3d32907604691b4c}, (so_uint128){0x8637bd05af6c69b5, 0xa63f9a49c2c1b10f}, (so_uint128){0xa7c5ac471b478423, 0x0fcf80dc33721d53}, (so_uint128){0xd1b71758e219652b, 0xd3c36113404ea4a8}, (so_uint128){0x83126e978d4fdf3b, 0x645a1cac083126e9}, (so_uint128){0xa3d70a3d70a3d70a, 0x3d70a3d70a3d70a3}, (so_uint128){0xcccccccccccccccc, 0xcccccccccccccccc}, (so_uint128){0x8000000000000000, 0x0000000000000000}, (so_uint128){0xa000000000000000, 0x0000000000000000}, (so_uint128){0xc800000000000000, 0x0000000000000000}, (so_uint128){0xfa00000000000000, 0x0000000000000000}, (so_uint128){0x9c40000000000000, 0x0000000000000000}, (so_uint128){0xc350000000000000, 0x0000000000000000}, (so_uint128){0xf424000000000000, 0x0000000000000000}, (so_uint128){0x9896800000000000, 0x0000000000000000}, (so_uint128){0xbebc200000000000, 0x0000000000000000}, (so_uint128){0xee6b280000000000, 0x0000000000000000}, (so_uint128){0x9502f90000000000, 0x0000000000000000}, (so_uint128){0xba43b74000000000, 0x0000000000000000}, (so_uint128){0xe8d4a51000000000, 0x0000000000000000}, (so_uint128){0x9184e72a00000000, 0x0000000000000000}, (so_uint128){0xb5e620f480000000, 0x0000000000000000}, (so_uint128){0xe35fa931a0000000, 0x0000000000000000}, (so_uint128){0x8e1bc9bf04000000, 0x0000000000000000}, (so_uint128){0xb1a2bc2ec5000000, 0x0000000000000000}, (so_uint128){0xde0b6b3a76400000, 0x0000000000000000}, (so_uint128){0x8ac7230489e80000, 0x0000000000000000}, (so_uint128){0xad78ebc5ac620000, 0x0000000000000000}, (so_uint128){0xd8d726b7177a8000, 0x0000000000000000}, (so_uint128){0x878678326eac9000, 0x0000000000000000}, (so_uint128){0xa968163f0a57b400, 0x0000000000000000}, (so_uint128){0xd3c21bcecceda100, 0x0000000000000000}, (so_uint128){0x84595161401484a0, 0x0000000000000000}, (so_uint128){0xa56fa5b99019a5c8, 0x0000000000000000}, (so_uint128){0xcecb8f27f4200f3a, 0x0000000000000000}, (so_uint128){0x813f3978f8940984, 0x4000000000000000}, (so_uint128){0xa18f07d736b90be5, 0x5000000000000000}, (so_uint128){0xc9f2c9cd04674ede, 0xa400000000000000}, (so_uint128){0xfc6f7c4045812296, 0x4d00000000000000}, (so_uint128){0x9dc5ada82b70b59d, 0xf020000000000000}, (so_uint128){0xc5371912364ce305, 0x6c28000000000000}, (so_uint128){0xf684df56c3e01bc6, 0xc732000000000000}, (so_uint128){0x9a130b963a6c115c, 0x3c7f400000000000}, (so_uint128){0xc097ce7bc90715b3, 0x4b9f100000000000}, (so_uint128){0xf0bdc21abb48db20, 0x1e86d40000000000}, (so_uint128){0x96769950b50d88f4, 0x1314448000000000}, (so_uint128){0xbc143fa4e250eb31, 0x17d955a000000000}, (so_uint128){0xeb194f8e1ae525fd, 0x5dcfab0800000000}, (so_uint128){0x92efd1b8d0cf37be, 0x5aa1cae500000000}, (so_uint128){0xb7abc627050305ad, 0xf14a3d9e40000000}, (so_uint128){0xe596b7b0c643c719, 0x6d9ccd05d0000000}, (so_uint128){0x8f7e32ce7bea5c6f, 0xe4820023a2000000}, (so_uint128){0xb35dbf821ae4f38b, 0xdda2802c8a800000}, (so_uint128){0xe0352f62a19e306e, 0xd50b2037ad200000}, (so_uint128){0x8c213d9da502de45, 0x4526f422cc340000}, (so_uint128){0xaf298d050e4395d6, 0x9670b12b7f410000}, (so_uint128){0xdaf3f04651d47b4c, 0x3c0cdd765f114000}, (so_uint128){0x88d8762bf324cd0f, 0xa5880a69fb6ac800}, (so_uint128){0xab0e93b6efee0053, 0x8eea0d047a457a00}, (so_uint128){0xd5d238a4abe98068, 0x72a4904598d6d880}, (so_uint128){0x85a36366eb71f041, 0x47a6da2b7f864750}, (so_uint128){0xa70c3c40a64e6c51, 0x999090b65f67d924}, (so_uint128){0xd0cf4b50cfe20765, 0xfff4b4e3f741cf6d}, (so_uint128){0x82818f1281ed449f, 0xbff8f10e7a8921a4}, (so_uint128){0xa321f2d7226895c7, 0xaff72d52192b6a0d}, (so_uint128){0xcbea6f8ceb02bb39, 0x9bf4f8a69f764490}, (so_uint128){0xfee50b7025c36a08, 0x02f236d04753d5b4}, (so_uint128){0x9f4f2726179a2245, 0x01d762422c946590}, (so_uint128){0xc722f0ef9d80aad6, 0x424d3ad2b7b97ef5}, (so_uint128){0xf8ebad2b84e0d58b, 0xd2e0898765a7deb2}, (so_uint128){0x9b934c3b330c8577, 0x63cc55f49f88eb2f}, (so_uint128){0xc2781f49ffcfa6d5, 0x3cbf6b71c76b25fb}, (so_uint128){0xf316271c7fc3908a, 0x8bef464e3945ef7a}, (so_uint128){0x97edd871cfda3a56, 0x97758bf0e3cbb5ac}, (so_uint128){0xbde94e8e43d0c8ec, 0x3d52eeed1cbea317}, (so_uint128){0xed63a231d4c4fb27, 0x4ca7aaa863ee4bdd}, (so_uint128){0x945e455f24fb1cf8, 0x8fe8caa93e74ef6a}, (so_uint128){0xb975d6b6ee39e436, 0xb3e2fd538e122b44}, (so_uint128){0xe7d34c64a9c85d44, 0x60dbbca87196b616}, (so_uint128){0x90e40fbeea1d3a4a, 0xbc8955e946fe31cd}, (so_uint128){0xb51d13aea4a488dd, 0x6babab6398bdbe41}, (so_uint128){0xe264589a4dcdab14, 0xc696963c7eed2dd1}, (so_uint128){0x8d7eb76070a08aec, 0xfc1e1de5cf543ca2}, (so_uint128){0xb0de65388cc8ada8, 0x3b25a55f43294bcb}, (so_uint128){0xdd15fe86affad912, 0x49ef0eb713f39ebe}, (so_uint128){0x8a2dbf142dfcc7ab, 0x6e3569326c784337}, (so_uint128){0xacb92ed9397bf996, 0x49c2c37f07965404}, (so_uint128){0xd7e77a8f87daf7fb, 0xdc33745ec97be906}, (so_uint128){0x86f0ac99b4e8dafd, 0x69a028bb3ded71a3}, (so_uint128){0xa8acd7c0222311bc, 0xc40832ea0d68ce0c}, (so_uint128){0xd2d80db02aabd62b, 0xf50a3fa490c30190}, (so_uint128){0x83c7088e1aab65db, 0x792667c6da79e0fa}, (so_uint128){0xa4b8cab1a1563f52, 0x577001b891185938}, (so_uint128){0xcde6fd5e09abcf26, 0xed4c0226b55e6f86}, (so_uint128){0x80b05e5ac60b6178, 0x544f8158315b05b4}, (so_uint128){0xa0dc75f1778e39d6, 0x696361ae3db1c721}, (so_uint128){0xc913936dd571c84c, 0x03bc3a19cd1e38e9}, (so_uint128){0xfb5878494ace3a5f, 0x04ab48a04065c723}, (so_uint128){0x9d174b2dcec0e47b, 0x62eb0d64283f9c76}, (so_uint128){0xc45d1df942711d9a, 0x3ba5d0bd324f8394}, (so_uint128){0xf5746577930d6500, 0xca8f44ec7ee36479}, (so_uint128){0x9968bf6abbe85f20, 0x7e998b13cf4e1ecb}, (so_uint128){0xbfc2ef456ae276e8, 0x9e3fedd8c321a67e}, (so_uint128){0xefb3ab16c59b14a2, 0xc5cfe94ef3ea101e}, (so_uint128){0x95d04aee3b80ece5, 0xbba1f1d158724a12}, (so_uint128){0xbb445da9ca61281f, 0x2a8a6e45ae8edc97}, (so_uint128){0xea1575143cf97226, 0xf52d09d71a3293bd}, (so_uint128){0x924d692ca61be758, 0x593c2626705f9c56}, (so_uint128){0xb6e0c377cfa2e12e, 0x6f8b2fb00c77836c}, (so_uint128){0xe498f455c38b997a, 0x0b6dfb9c0f956447}, (so_uint128){0x8edf98b59a373fec, 0x4724bd4189bd5eac}, (so_uint128){0xb2977ee300c50fe7, 0x58edec91ec2cb657}, (so_uint128){0xdf3d5e9bc0f653e1, 0x2f2967b66737e3ed}, (so_uint128){0x8b865b215899f46c, 0xbd79e0d20082ee74}, (so_uint128){0xae67f1e9aec07187, 0xecd8590680a3aa11}, (so_uint128){0xda01ee641a708de9, 0xe80e6f4820cc9495}, (so_uint128){0x884134fe908658b2, 0x3109058d147fdcdd}, (so_uint128){0xaa51823e34a7eede, 0xbd4b46f0599fd415}, (so_uint128){0xd4e5e2cdc1d1ea96, 0x6c9e18ac7007c91a}, (so_uint128){0x850fadc09923329e, 0x03e2cf6bc604ddb0}, (so_uint128){0xa6539930bf6bff45, 0x84db8346b786151c}, (so_uint128){0xcfe87f7cef46ff16, 0xe612641865679a63}, (so_uint128){0x81f14fae158c5f6e, 0x4fcb7e8f3f60c07e}, (so_uint128){0xa26da3999aef7749, 0xe3be5e330f38f09d}, (so_uint128){0xcb090c8001ab551c, 0x5cadf5bfd3072cc5}, (so_uint128){0xfdcb4fa002162a63, 0x73d9732fc7c8f7f6}, (so_uint128){0x9e9f11c4014dda7e, 0x2867e7fddcdd9afa}, (so_uint128){0xc646d63501a1511d, 0xb281e1fd541501b8}, (so_uint128){0xf7d88bc24209a565, 0x1f225a7ca91a4226}, (so_uint128){0x9ae757596946075f, 0x3375788de9b06958}, (so_uint128){0xc1a12d2fc3978937, 0x0052d6b1641c83ae}, (so_uint128){0xf209787bb47d6b84, 0xc0678c5dbd23a49a}, (so_uint128){0x9745eb4d50ce6332, 0xf840b7ba963646e0}, (so_uint128){0xbd176620a501fbff, 0xb650e5a93bc3d898}, (so_uint128){0xec5d3fa8ce427aff, 0xa3e51f138ab4cebe}, (so_uint128){0x93ba47c980e98cdf, 0xc66f336c36b10137}, (so_uint128){0xb8a8d9bbe123f017, 0xb80b0047445d4184}, (so_uint128){0xe6d3102ad96cec1d, 0xa60dc059157491e5}, (so_uint128){0x9043ea1ac7e41392, 0x87c89837ad68db2f}, (so_uint128){0xb454e4a179dd1877, 0x29babe4598c311fb}, (so_uint128){0xe16a1dc9d8545e94, 0xf4296dd6fef3d67a}, (so_uint128){0x8ce2529e2734bb1d, 0x1899e4a65f58660c}, (so_uint128){0xb01ae745b101e9e4, 0x5ec05dcff72e7f8f}, (so_uint128){0xdc21a1171d42645d, 0x76707543f4fa1f73}, (so_uint128){0x899504ae72497eba, 0x6a06494a791c53a8}, (so_uint128){0xabfa45da0edbde69, 0x0487db9d17636892}, (so_uint128){0xd6f8d7509292d603, 0x45a9d2845d3c42b6}, (so_uint128){0x865b86925b9bc5c2, 0x0b8a2392ba45a9b2}, (so_uint128){0xa7f26836f282b732, 0x8e6cac7768d7141e}, (so_uint128){0xd1ef0244af2364ff, 0x3207d795430cd926}, (so_uint128){0x8335616aed761f1f, 0x7f44e6bd49e807b8}, (so_uint128){0xa402b9c5a8d3a6e7, 0x5f16206c9c6209a6}, (so_uint128){0xcd036837130890a1, 0x36dba887c37a8c0f}, (so_uint128){0x802221226be55a64, 0xc2494954da2c9789}, (so_uint128){0xa02aa96b06deb0fd, 0xf2db9baa10b7bd6c}, (so_uint128){0xc83553c5c8965d3d, 0x6f92829494e5acc7}, (so_uint128){0xfa42a8b73abbf48c, 0xcb772339ba1f17f9}, (so_uint128){0x9c69a97284b578d7, 0xff2a760414536efb}, (so_uint128){0xc38413cf25e2d70d, 0xfef5138519684aba}, (so_uint128){0xf46518c2ef5b8cd1, 0x7eb258665fc25d69}, (so_uint128){0x98bf2f79d5993802, 0xef2f773ffbd97a61}, (so_uint128){0xbeeefb584aff8603, 0xaafb550ffacfd8fa}, (so_uint128){0xeeaaba2e5dbf6784, 0x95ba2a53f983cf38}, (so_uint128){0x952ab45cfa97a0b2, 0xdd945a747bf26183}, (so_uint128){0xba756174393d88df, 0x94f971119aeef9e4}, (so_uint128){0xe912b9d1478ceb17, 0x7a37cd5601aab85d}, (so_uint128){0x91abb422ccb812ee, 0xac62e055c10ab33a}, (so_uint128){0xb616a12b7fe617aa, 0x577b986b314d6009}, (so_uint128){0xe39c49765fdf9d94, 0xed5a7e85fda0b80b}, (so_uint128){0x8e41ade9fbebc27d, 0x14588f13be847307}, (so_uint128){0xb1d219647ae6b31c, 0x596eb2d8ae258fc8}, (so_uint128){0xde469fbd99a05fe3, 0x6fca5f8ed9aef3bb}, (so_uint128){0x8aec23d680043bee, 0x25de7bb9480d5854}, (so_uint128){0xada72ccc20054ae9, 0xaf561aa79a10ae6a}, (so_uint128){0xd910f7ff28069da4, 0x1b2ba1518094da04}, (so_uint128){0x87aa9aff79042286, 0x90fb44d2f05d0842}, (so_uint128){0xa99541bf57452b28, 0x353a1607ac744a53}, (so_uint128){0xd3fa922f2d1675f2, 0x42889b8997915ce8}, (so_uint128){0x847c9b5d7c2e09b7, 0x69956135febada11}, (so_uint128){0xa59bc234db398c25, 0x43fab9837e699095}, (so_uint128){0xcf02b2c21207ef2e, 0x94f967e45e03f4bb}, (so_uint128){0x8161afb94b44f57d, 0x1d1be0eebac278f5}, (so_uint128){0xa1ba1ba79e1632dc, 0x6462d92a69731732}, (so_uint128){0xca28a291859bbf93, 0x7d7b8f7503cfdcfe}, (so_uint128){0xfcb2cb35e702af78, 0x5cda735244c3d43e}, (so_uint128){0x9defbf01b061adab, 0x3a0888136afa64a7}, (so_uint128){0xc56baec21c7a1916, 0x088aaa1845b8fdd0}, (so_uint128){0xf6c69a72a3989f5b, 0x8aad549e57273d45}, (so_uint128){0x9a3c2087a63f6399, 0x36ac54e2f678864b}, (so_uint128){0xc0cb28a98fcf3c7f, 0x84576a1bb416a7dd}, (so_uint128){0xf0fdf2d3f3c30b9f, 0x656d44a2a11c51d5}, (so_uint128){0x969eb7c47859e743, 0x9f644ae5a4b1b325}, (so_uint128){0xbc4665b596706114, 0x873d5d9f0dde1fee}, (so_uint128){0xeb57ff22fc0c7959, 0xa90cb506d155a7ea}, (so_uint128){0x9316ff75dd87cbd8, 0x09a7f12442d588f2}, (so_uint128){0xb7dcbf5354e9bece, 0x0c11ed6d538aeb2f}, (so_uint128){0xe5d3ef282a242e81, 0x8f1668c8a86da5fa}, (so_uint128){0x8fa475791a569d10, 0xf96e017d694487bc}, (so_uint128){0xb38d92d760ec4455, 0x37c981dcc395a9ac}, (so_uint128){0xe070f78d3927556a, 0x85bbe253f47b1417}, (so_uint128){0x8c469ab843b89562, 0x93956d7478ccec8e}, (so_uint128){0xaf58416654a6babb, 0x387ac8d1970027b2}, (so_uint128){0xdb2e51bfe9d0696a, 0x06997b05fcc0319e}, (so_uint128){0x88fcf317f22241e2, 0x441fece3bdf81f03}, (so_uint128){0xab3c2fddeeaad25a, 0xd527e81cad7626c3}, (so_uint128){0xd60b3bd56a5586f1, 0x8a71e223d8d3b074}, (so_uint128){0x85c7056562757456, 0xf6872d5667844e49}, (so_uint128){0xa738c6bebb12d16c, 0xb428f8ac016561db}, (so_uint128){0xd106f86e69d785c7, 0xe13336d701beba52}, (so_uint128){0x82a45b450226b39c, 0xecc0024661173473}, (so_uint128){0xa34d721642b06084, 0x27f002d7f95d0190}, (so_uint128){0xcc20ce9bd35c78a5, 0x31ec038df7b441f4}, (so_uint128){0xff290242c83396ce, 0x7e67047175a15271}, (so_uint128){0x9f79a169bd203e41, 0x0f0062c6e984d386}, (so_uint128){0xc75809c42c684dd1, 0x52c07b78a3e60868}, (so_uint128){0xf92e0c3537826145, 0xa7709a56ccdf8a82}, (so_uint128){0x9bbcc7a142b17ccb, 0x88a66076400bb691}, (so_uint128){0xc2abf989935ddbfe, 0x6acff893d00ea435}, (so_uint128){0xf356f7ebf83552fe, 0x0583f6b8c4124d43}, (so_uint128){0x98165af37b2153de, 0xc3727a337a8b704a}, (so_uint128){0xbe1bf1b059e9a8d6, 0x744f18c0592e4c5c}, (so_uint128){0xeda2ee1c7064130c, 0x1162def06f79df73}, (so_uint128){0x9485d4d1c63e8be7, 0x8addcb5645ac2ba8}, (so_uint128){0xb9a74a0637ce2ee1, 0x6d953e2bd7173692}, (so_uint128){0xe8111c87c5c1ba99, 0xc8fa8db6ccdd0437}, (so_uint128){0x910ab1d4db9914a0, 0x1d9c9892400a22a2}, (so_uint128){0xb54d5e4a127f59c8, 0x2503beb6d00cab4b}, (so_uint128){0xe2a0b5dc971f303a, 0x2e44ae64840fd61d}, (so_uint128){0x8da471a9de737e24, 0x5ceaecfed289e5d2}, (so_uint128){0xb10d8e1456105dad, 0x7425a83e872c5f47}, (so_uint128){0xdd50f1996b947518, 0xd12f124e28f77719}, (so_uint128){0x8a5296ffe33cc92f, 0x82bd6b70d99aaa6f}, (so_uint128){0xace73cbfdc0bfb7b, 0x636cc64d1001550b}, (so_uint128){0xd8210befd30efa5a, 0x3c47f7e05401aa4e}, (so_uint128){0x8714a775e3e95c78, 0x65acfaec34810a71}, (so_uint128){0xa8d9d1535ce3b396, 0x7f1839a741a14d0d}, (so_uint128){0xd31045a8341ca07c, 0x1ede48111209a050}, (so_uint128){0x83ea2b892091e44d, 0x934aed0aab460432}, (so_uint128){0xa4e4b66b68b65d60, 0xf81da84d5617853f}, (so_uint128){0xce1de40642e3f4b9, 0x36251260ab9d668e}, (so_uint128){0x80d2ae83e9ce78f3, 0xc1d72b7c6b426019}, (so_uint128){0xa1075a24e4421730, 0xb24cf65b8612f81f}, (so_uint128){0xc94930ae1d529cfc, 0xdee033f26797b627}, (so_uint128){0xfb9b7cd9a4a7443c, 0x169840ef017da3b1}, (so_uint128){0x9d412e0806e88aa5, 0x8e1f289560ee864e}, (so_uint128){0xc491798a08a2ad4e, 0xf1a6f2bab92a27e2}, (so_uint128){0xf5b5d7ec8acb58a2, 0xae10af696774b1db}, (so_uint128){0x9991a6f3d6bf1765, 0xacca6da1e0a8ef29}, (so_uint128){0xbff610b0cc6edd3f, 0x17fd090a58d32af3}, (so_uint128){0xeff394dcff8a948e, 0xddfc4b4cef07f5b0}, (so_uint128){0x95f83d0a1fb69cd9, 0x4abdaf101564f98e}, (so_uint128){0xbb764c4ca7a4440f, 0x9d6d1ad41abe37f1}, (so_uint128){0xea53df5fd18d5513, 0x84c86189216dc5ed}, (so_uint128){0x92746b9be2f8552c, 0x32fd3cf5b4e49bb4}, (so_uint128){0xb7118682dbb66a77, 0x3fbc8c33221dc2a1}, (so_uint128){0xe4d5e82392a40515, 0x0fabaf3feaa5334a}, (so_uint128){0x8f05b1163ba6832d, 0x29cb4d87f2a7400e}, (so_uint128){0xb2c71d5bca9023f8, 0x743e20e9ef511012}, (so_uint128){0xdf78e4b2bd342cf6, 0x914da9246b255416}, (so_uint128){0x8bab8eefb6409c1a, 0x1ad089b6c2f7548e}, (so_uint128){0xae9672aba3d0c320, 0xa184ac2473b529b1}, (so_uint128){0xda3c0f568cc4f3e8, 0xc9e5d72d90a2741e}, (so_uint128){0x8865899617fb1871, 0x7e2fa67c7a658892}, (so_uint128){0xaa7eebfb9df9de8d, 0xddbb901b98feeab7}, (so_uint128){0xd51ea6fa85785631, 0x552a74227f3ea565}, (so_uint128){0x8533285c936b35de, 0xd53a88958f87275f}, (so_uint128){0xa67ff273b8460356, 0x8a892abaf368f137}, (so_uint128){0xd01fef10a657842c, 0x2d2b7569b0432d85}, (so_uint128){0x8213f56a67f6b29b, 0x9c3b29620e29fc73}, (so_uint128){0xa298f2c501f45f42, 0x8349f3ba91b47b8f}, (so_uint128){0xcb3f2f7642717713, 0x241c70a936219a73}, (so_uint128){0xfe0efb53d30dd4d7, 0xed238cd383aa0110}, (so_uint128){0x9ec95d1463e8a506, 0xf4363804324a40aa}, (so_uint128){0xc67bb4597ce2ce48, 0xb143c6053edcd0d5}, (so_uint128){0xf81aa16fdc1b81da, 0xdd94b7868e94050a}, (so_uint128){0x9b10a4e5e9913128, 0xca7cf2b4191c8326}, (so_uint128){0xc1d4ce1f63f57d72, 0xfd1c2f611f63a3f0}, (so_uint128){0xf24a01a73cf2dccf, 0xbc633b39673c8cec}, (so_uint128){0x976e41088617ca01, 0xd5be0503e085d813}, (so_uint128){0xbd49d14aa79dbc82, 0x4b2d8644d8a74e18}, (so_uint128){0xec9c459d51852ba2, 0xddf8e7d60ed1219e}, (so_uint128){0x93e1ab8252f33b45, 0xcabb90e5c942b503}, (so_uint128){0xb8da1662e7b00a17, 0x3d6a751f3b936243}, (so_uint128){0xe7109bfba19c0c9d, 0x0cc512670a783ad4}, (so_uint128){0x906a617d450187e2, 0x27fb2b80668b24c5}, (so_uint128){0xb484f9dc9641e9da, 0xb1f9f660802dedf6}, (so_uint128){0xe1a63853bbd26451, 0x5e7873f8a0396973}, (so_uint128){0x8d07e33455637eb2, 0xdb0b487b6423e1e8}, (so_uint128){0xb049dc016abc5e5f, 0x91ce1a9a3d2cda62}, (so_uint128){0xdc5c5301c56b75f7, 0x7641a140cc7810fb}, (so_uint128){0x89b9b3e11b6329ba, 0xa9e904c87fcb0a9d}, (so_uint128){0xac2820d9623bf429, 0x546345fa9fbdcd44}, (so_uint128){0xd732290fbacaf133, 0xa97c177947ad4095}, (so_uint128){0x867f59a9d4bed6c0, 0x49ed8eabcccc485d}, (so_uint128){0xa81f301449ee8c70, 0x5c68f256bfff5a74}, (so_uint128){0xd226fc195c6a2f8c, 0x73832eec6fff3111}, (so_uint128){0x83585d8fd9c25db7, 0xc831fd53c5ff7eab}, (so_uint128){0xa42e74f3d032f525, 0xba3e7ca8b77f5e55}, (so_uint128){0xcd3a1230c43fb26f, 0x28ce1bd2e55f35eb}, (so_uint128){0x80444b5e7aa7cf85, 0x7980d163cf5b81b3}, (so_uint128){0xa0555e361951c366, 0xd7e105bcc332621f}, (so_uint128){0xc86ab5c39fa63440, 0x8dd9472bf3fefaa7}, (so_uint128){0xfa856334878fc150, 0xb14f98f6f0feb951}, (so_uint128){0x9c935e00d4b9d8d2, 0x6ed1bf9a569f33d3}, (so_uint128){0xc3b8358109e84f07, 0x0a862f80ec4700c8}, (so_uint128){0xf4a642e14c6262c8, 0xcd27bb612758c0fa}, (so_uint128){0x98e7e9cccfbd7dbd, 0x8038d51cb897789c}, (so_uint128){0xbf21e44003acdd2c, 0xe0470a63e6bd56c3}, (so_uint128){0xeeea5d5004981478, 0x1858ccfce06cac74}, (so_uint128){0x95527a5202df0ccb, 0x0f37801e0c43ebc8}, (so_uint128){0xbaa718e68396cffd, 0xd30560258f54e6ba}, (so_uint128){0xe950df20247c83fd, 0x47c6b82ef32a2069}, (so_uint128){0x91d28b7416cdd27e, 0x4cdc331d57fa5441}, (so_uint128){0xb6472e511c81471d, 0xe0133fe4adf8e952}, (so_uint128){0xe3d8f9e563a198e5, 0x58180fddd97723a6}, (so_uint128){0x8e679c2f5e44ff8f, 0x570f09eaa7ea7648}, (so_uint128){0xb201833b35d63f73, 0x2cd2cc6551e513da}, (so_uint128){0xde81e40a034bcf4f, 0xf8077f7ea65e58d1}, (so_uint128){0x8b112e86420f6191, 0xfb04afaf27faf782}, (so_uint128){0xadd57a27d29339f6, 0x79c5db9af1f9b563}, (so_uint128){0xd94ad8b1c7380874, 0x18375281ae7822bc}, (so_uint128){0x87cec76f1c830548, 0x8f2293910d0b15b5}, (so_uint128){0xa9c2794ae3a3c69a, 0xb2eb3875504ddb22}, (so_uint128){0xd433179d9c8cb841, 0x5fa60692a46151eb}, (so_uint128){0x849feec281d7f328, 0xdbc7c41ba6bcd333}, (so_uint128){0xa5c7ea73224deff3, 0x12b9b522906c0800}, (so_uint128){0xcf39e50feae16bef, 0xd768226b34870a00}, (so_uint128){0x81842f29f2cce375, 0xe6a1158300d46640}, (so_uint128){0xa1e53af46f801c53, 0x60495ae3c1097fd0}, (so_uint128){0xca5e89b18b602368, 0x385bb19cb14bdfc4}, (so_uint128){0xfcf62c1dee382c42, 0x46729e03dd9ed7b5}, (so_uint128){0x9e19db92b4e31ba9, 0x6c07a2c26a8346d1}, (so_uint128){0xc5a05277621be293, 0xc7098b7305241885}, (so_uint128){0xf70867153aa2db38, 0xb8cbee4fc66d1ea7}, (so_uint128){0x9a65406d44a5c903, 0x737f74f1dc043328}, (so_uint128){0xc0fe908895cf3b44, 0x505f522e53053ff2}, (so_uint128){0xf13e34aabb430a15, 0x647726b9e7c68fef}, (so_uint128){0x96c6e0eab509e64d, 0x5eca783430dc19f5}, (so_uint128){0xbc789925624c5fe0, 0xb67d16413d132072}, (so_uint128){0xeb96bf6ebadf77d8, 0xe41c5bd18c57e88f}, (so_uint128){0x933e37a534cbaae7, 0x8e91b962f7b6f159}, (so_uint128){0xb80dc58e81fe95a1, 0x723627bbb5a4adb0}, (so_uint128){0xe61136f2227e3b09, 0xcec3b1aaa30dd91c}, (so_uint128){0x8fcac257558ee4e6, 0x213a4f0aa5e8a7b1}, (so_uint128){0xb3bd72ed2af29e1f, 0xa988e2cd4f62d19d}, (so_uint128){0xe0accfa875af45a7, 0x93eb1b80a33b8605}, (so_uint128){0x8c6c01c9498d8b88, 0xbc72f130660533c3}, (so_uint128){0xaf87023b9bf0ee6a, 0xeb8fad7c7f8680b4}, (so_uint128){0xdb68c2ca82ed2a05, 0xa67398db9f6820e1}, (so_uint128){0x892179be91d43a43, 0x88083f8943a1148c}, (so_uint128){0xab69d82e364948d4, 0x6a0a4f6b948959b0}, (so_uint128){0xd6444e39c3db9b09, 0x848ce34679abb01c}, (so_uint128){0x85eab0e41a6940e5, 0xf2d80e0c0c0b4e11}, (so_uint128){0xa7655d1d2103911f, 0x6f8e118f0f0e2195}, (so_uint128){0xd13eb46469447567, 0x4b7195f2d2d1a9fb}};

// -- atob.go --

// ParseBool returns the boolean value represented by the string.
// It accepts 1, t, T, TRUE, true, True, 0, f, F, FALSE, false, False.
// Any other value returns an error.
so_R_bool_err strconv_ParseBool(so_String str) {
    if (so_string_eq(str, so_str("1")) || so_string_eq(str, so_str("t")) || so_string_eq(str, so_str("T")) || so_string_eq(str, so_str("true")) || so_string_eq(str, so_str("TRUE")) || so_string_eq(str, so_str("True"))) {
        return (so_R_bool_err){.val = true, .err = (so_Error){0}};
    } else if (so_string_eq(str, so_str("0")) || so_string_eq(str, so_str("f")) || so_string_eq(str, so_str("F")) || so_string_eq(str, so_str("false")) || so_string_eq(str, so_str("FALSE")) || so_string_eq(str, so_str("False"))) {
        return (so_R_bool_err){.val = false, .err = (so_Error){0}};
    }
    return (so_R_bool_err){.val = false, .err = strconv_ErrSyntax};
}

// FormatBool returns "true" or "false" according to the value of b.
so_String strconv_FormatBool(bool b) {
    if (b) {
        return so_str("true");
    }
    return so_str("false");
}

// AppendBool appends "true" or "false", according to the value of b,
// to dst and returns the extended buffer.
so_Slice strconv_AppendBool(so_Slice dst, bool b) {
    if (b) {
        return so_extend(so_byte, dst, so_string_bytes(so_str("true")));
    }
    return so_extend(so_byte, dst, so_string_bytes(so_str("false")));
}

// -- atof.go --

// decimal to binary floating point conversion.
// Algorithm:
//   1) Store input in multiprecision decimal.
//   2) Multiply/divide decimal by powers of two until in range [0.5, 1)
//   3) Multiply by 2^precision and round to get mantissa.
// commonPrefixLenIgnoreCase returns the length of the common
// prefix of s and prefix, with the character case of s ignored.
// The prefix argument must be all lower-case.
static so_int commonPrefixLenIgnoreCase(so_String s, so_String prefix) {
    so_int n = so_min(so_len(prefix), so_len(s));
    for (so_int i = 0; i < n; i++) {
        so_byte c = so_at(so_byte, s, i);
        if ('A' <= c && c <= 'Z') {
            c += U'a' - U'A';
        }
        if (c != so_at(so_byte, prefix, i)) {
            return i;
        }
    }
    return n;
}

// special returns the floating-point value for the special,
// possibly signed floating-point representations inf, infinity,
// and NaN. The result is ok if a prefix of s contains one
// of these representations and n is the length of that prefix.
// The character case is ignored.
static specialFloat special(so_String s) {
    if (so_len(s) == 0) {
        return (specialFloat){0, 0, false};
    }
    so_int sign = 1;
    so_int nsign = 0;
    if (so_at(so_byte, s, 0) == ('+') || so_at(so_byte, s, 0) == ('-')) {
        if (so_at(so_byte, s, 0) == '-') {
            sign = -1;
        }
        nsign = 1;
        s = so_string_slice(s, 1, s.len);
        so_int n = commonPrefixLenIgnoreCase(s, so_str("infinity"));
        if (3 < n && n < 8) {
            n = 3;
        }
        if (n == 3 || n == 8) {
            return (specialFloat){floatInf(sign), nsign + n, true};
        }
    } else if (so_at(so_byte, s, 0) == ('i') || so_at(so_byte, s, 0) == ('I')) {
        so_int n = commonPrefixLenIgnoreCase(s, so_str("infinity"));
        // Anything longer than "inf" is ok, but if we
        // don't have "infinity", only consume "inf".
        if (3 < n && n < 8) {
            n = 3;
        }
        if (n == 3 || n == 8) {
            return (specialFloat){floatInf(sign), nsign + n, true};
        }
    } else if (so_at(so_byte, s, 0) == ('n') || so_at(so_byte, s, 0) == ('N')) {
        if (commonPrefixLenIgnoreCase(s, so_str("nan")) == 3) {
            return (specialFloat){floatNaN(), 3, true};
        }
    }
    return (specialFloat){0, 0, false};
}

static bool decimal_set(void* self, so_String s) {
    decimal* b = self;
    so_int i = 0;
    b->neg = false;
    b->trunc = false;
    // optional sign
    if (i >= so_len(s)) {
        return false;
    }
    if (so_at(so_byte, s, i) == ('+')) {
        i++;
    } else if (so_at(so_byte, s, i) == ('-')) {
        i++;
        b->neg = true;
    }
    // digits
    bool sawdot = false;
    bool sawdigits = false;
    for (; i < so_len(s); i++) {
        if (so_at(so_byte, s, i) == '_') {
            // readFloat already checked underscores
            continue;
        } else if (so_at(so_byte, s, i) == '.') {
            if (sawdot) {
                return false;
            }
            sawdot = true;
            b->dp = b->nd;
            continue;
        } else if ('0' <= so_at(so_byte, s, i) && so_at(so_byte, s, i) <= '9') {
            sawdigits = true;
            if (so_at(so_byte, s, i) == '0' && b->nd == 0) {
                // ignore leading zeros
                b->dp--;
                continue;
            }
            if (b->nd < 800) {
                b->d[b->nd] = so_at(so_byte, s, i);
                b->nd++;
            } else if (so_at(so_byte, s, i) != '0') {
                b->trunc = true;
            }
            continue;
        }
        break;
    }
    if (!sawdigits) {
        return false;
    }
    if (!sawdot) {
        b->dp = b->nd;
    }
    // optional exponent moves decimal point.
    // if we read a very large, very long number,
    // just be sure to move the decimal point by
    // a lot (say, 100000).  it doesn't matter if it's
    // not the exact number.
    if (i < so_len(s) && lower(so_at(so_byte, s, i)) == 'e') {
        i++;
        if (i >= so_len(s)) {
            return false;
        }
        so_int esign = 1;
        if (so_at(so_byte, s, i) == ('+')) {
            i++;
        } else if (so_at(so_byte, s, i) == ('-')) {
            i++;
            esign = -1;
        }
        if (i >= so_len(s) || so_at(so_byte, s, i) < '0' || so_at(so_byte, s, i) > '9') {
            return false;
        }
        so_int e = 0;
        for (; i < so_len(s) && (('0' <= so_at(so_byte, s, i) && so_at(so_byte, s, i) <= '9') || so_at(so_byte, s, i) == '_'); i++) {
            if (so_at(so_byte, s, i) == '_') {
                // readFloat already checked underscores
                continue;
            }
            if (e < 10000) {
                e = e * 10 + (so_int)(so_at(so_byte, s, i)) - U'0';
            }
        }
        b->dp += e * esign;
    }
    if (i != so_len(s)) {
        return false;
    }
    return true;
}

// readFloat reads a decimal or hexadecimal mantissa and exponent from a float
// string representation in s; the number may be followed by other characters.
// readFloat reports the number of bytes consumed (i), and whether the number
// is valid (ok).
static readFloatResult readFloat(so_String s) {
    readFloatResult res = {0};
    bool underscores = false;
    // optional sign
    if (res.n >= so_len(s)) {
        return res;
    }
    if (so_at(so_byte, s, res.n) == ('+')) {
        res.n++;
    } else if (so_at(so_byte, s, res.n) == ('-')) {
        res.n++;
        res.neg = true;
    }
    // digits
    uint64_t base = (uint64_t)(10);
    // 10^19 fits in uint64
    so_int maxMantDigits = 19;
    so_byte expChar = (so_byte)('e');
    if (res.n + 2 < so_len(s) && so_at(so_byte, s, res.n) == '0' && lower(so_at(so_byte, s, res.n + 1)) == 'x') {
        base = 16;
        // 16^16 fits in uint64
        maxMantDigits = 16;
        res.n += 2;
        expChar = 'p';
        res.hex = true;
    }
    bool sawdot = false;
    bool sawdigits = false;
    so_int nd = 0;
    so_int ndMant = 0;
    so_int dp = 0;
    loop:;
    for (; res.n < so_len(s); res.n++) {
        so_byte c = so_at(so_byte, s, res.n);
        if (c == '_') {
            underscores = true;
            continue;
        } else if (c == '.') {
            if (sawdot) {
                goto loop_end;
            }
            sawdot = true;
            dp = nd;
            continue;
        } else if ('0' <= c && c <= '9') {
            sawdigits = true;
            if (c == '0' && nd == 0) {
                // ignore leading zeros
                dp--;
                continue;
            }
            nd++;
            if (ndMant < maxMantDigits) {
                res.mantissa *= base;
                res.mantissa += (uint64_t)(c - '0');
                ndMant++;
            } else if (c != '0') {
                res.trunc = true;
            }
            continue;
        } else if (base == 16 && 'a' <= lower(c) && lower(c) <= 'f') {
            sawdigits = true;
            nd++;
            if (ndMant < maxMantDigits) {
                res.mantissa *= 16;
                res.mantissa += (uint64_t)(lower(c) - 'a' + 10);
                ndMant++;
            } else {
                res.trunc = true;
            }
            continue;
        }
        break;
    }
    loop_end:;
    if (!sawdigits) {
        return res;
    }
    if (!sawdot) {
        dp = nd;
    }
    if (base == 16) {
        dp *= 4;
        ndMant *= 4;
    }
    // optional exponent moves decimal point.
    // if we read a very large, very long number,
    // just be sure to move the decimal point by
    // a lot (say, 100000).  it doesn't matter if it's
    // not the exact number.
    if (res.n < so_len(s) && lower(so_at(so_byte, s, res.n)) == expChar) {
        res.n++;
        if (res.n >= so_len(s)) {
            return res;
        }
        so_int esign = 1;
        if (so_at(so_byte, s, res.n) == ('+')) {
            res.n++;
        } else if (so_at(so_byte, s, res.n) == ('-')) {
            res.n++;
            esign = -1;
        }
        if (res.n >= so_len(s) || so_at(so_byte, s, res.n) < '0' || so_at(so_byte, s, res.n) > '9') {
            return res;
        }
        so_int e = 0;
        for (; res.n < so_len(s) && (('0' <= so_at(so_byte, s, res.n) && so_at(so_byte, s, res.n) <= '9') || so_at(so_byte, s, res.n) == '_'); res.n++) {
            if (so_at(so_byte, s, res.n) == '_') {
                underscores = true;
                continue;
            }
            if (e < 10000) {
                e = e * 10 + (so_int)(so_at(so_byte, s, res.n) - '0');
            }
        }
        dp += e * esign;
    } else if (base == 16) {
        // Must have exponent.
        return res;
    }
    if (res.mantissa != 0) {
        res.exp = dp - ndMant;
    }
    if (underscores && !underscoreOK(so_string_slice(s, 0, res.n))) {
        return res;
    }
    res.ok = true;
    return res;
}

static so_R_u64_bool decimal_floatBits(void* self, floatInfo* flt) {
    decimal* d = self;
    bool overflowed = false;
    so_int exp = 0;
    uint64_t mant = 0;
    // Zero is always a special case.
    if (d->nd == 0) {
        mant = 0;
        exp = flt->bias;
        goto out;
    }
    // Obvious overflow/underflow.
    // These bounds are for 64-bit floats.
    // Will have to change if we want to support 80-bit floats in the future.
    if (d->dp > 310) {
        goto overflow;
    }
    if (d->dp < -330) {
        // zero
        mant = 0;
        exp = flt->bias;
        goto out;
    }
    // Scale by powers of two until in range [0.5, 1.0)
    exp = 0;
    for (; d->dp > 0;) {
        so_int n = 0;
        if (d->dp >= so_len(powtab)) {
            n = 27;
        } else {
            n = so_at(so_int, powtab, d->dp);
        }
        decimal_Shift(d, -n);
        exp += n;
    }
    for (; d->dp < 0 || (d->dp == 0 && d->d[0] < '5');) {
        so_int n = 0;
        if (-d->dp >= so_len(powtab)) {
            n = 27;
        } else {
            n = so_at(so_int, powtab, -d->dp);
        }
        decimal_Shift(d, n);
        exp -= n;
    }
    // Our range is [0.5,1) but floating point range is [1,2).
    exp--;
    // Minimum representable exponent is flt.bias+1.
    // If the exponent is smaller, move it up and
    // adjust d accordingly.
    if (exp < flt->bias + 1) {
        so_int n = flt->bias + 1 - exp;
        decimal_Shift(d, -n);
        exp += n;
    }
    if (exp - flt->bias >= ((so_int)1 << flt->expbits) - 1) {
        goto overflow;
    }
    // Extract 1+flt.mantbits bits.
    decimal_Shift(d, (so_int)(1 + flt->mantbits));
    mant = decimal_RoundedInteger(d);
    // Rounding might have added a bit; shift down.
    if (mant == ((uint64_t)2 << flt->mantbits)) {
        mant >>= 1;
        exp++;
        if (exp - flt->bias >= ((so_int)1 << flt->expbits) - 1) {
            goto overflow;
        }
    }
    // Denormalized?
    if ((mant & ((uint64_t)1 << flt->mantbits)) == 0) {
        exp = flt->bias;
    }
    goto out;
    overflow:;
    mant = 0;
    exp = ((so_int)1 << flt->expbits) - 1 + flt->bias;
    overflowed = true;
    out:;
    uint64_t bits = (mant & (((uint64_t)(1) << flt->mantbits) - 1));
    bits |= ((uint64_t)((exp - flt->bias) & (((so_int)1 << flt->expbits) - 1)) << flt->mantbits);
    if (d->neg) {
        bits |= (((uint64_t)1 << flt->mantbits) << flt->expbits);
    }
    return (so_R_u64_bool){.val = bits, .val2 = overflowed};
}

// If possible to convert decimal representation to 64-bit float f exactly,
// entirely in floating-point math, do so, avoiding the expense of decimalToFloatBits.
// Three common cases:
//
//	value is exact integer
//	value is exact integer * exact power of ten
//	value is exact integer / exact power of ten
//
// These all produce potentially inexact but correctly rounded answers.
static so_R_f64_bool atof64exact(uint64_t mantissa, so_int exp, bool neg) {
    if ((mantissa >> float64info.mantbits) != 0) {
        return (so_R_f64_bool){.val = 0, .val2 = false};
    }
    double f = (double)(mantissa);
    if (neg) {
        f = -f;
    }
    if (exp == 0) {
        // an integer.
        return (so_R_f64_bool){.val = f, .val2 = true};
    } else if (exp > 0 && exp <= 15 + 22) {
        // If exponent is big but number of digits is not,
        // can move a few zeros into the integer part.
        if (exp > 22) {
            f *= so_at(double, float64pow10, exp - 22);
            exp = 22;
        }
        if (f > 1e15 || f < -1e15) {
            // the exponent was really too large.
            return (so_R_f64_bool){.val = 0, .val2 = false};
        }
        return (so_R_f64_bool){.val = f * so_at(double, float64pow10, exp), .val2 = true};
    } else if (exp < 0 && exp >= -22) {
        return (so_R_f64_bool){.val = f / so_at(double, float64pow10, -exp), .val2 = true};
    }
    return (so_R_f64_bool){.val = 0, .val2 = false};
}

// If possible to compute mantissa*10^exp to 32-bit float f exactly,
// entirely in floating-point math, do so, avoiding the machinery above.
static so_R_f32_bool atof32exact(uint64_t mantissa, so_int exp, bool neg) {
    if ((mantissa >> float32MantBits) != 0) {
        return (so_R_f32_bool){.val = 0, .val2 = false};
    }
    float f = (float)(mantissa);
    if (neg) {
        f = -f;
    }
    if (exp == 0) {
        return (so_R_f32_bool){.val = f, .val2 = true};
    } else if (exp > 0 && exp <= 7 + 10) {
        // If exponent is big but number of digits is not,
        // can move a few zeros into the integer part.
        if (exp > 10) {
            f *= so_at(float, float32pow10, exp - 10);
            exp = 10;
        }
        if (f > 1e7 || f < -1e7) {
            // the exponent was really too large.
            return (so_R_f32_bool){.val = 0, .val2 = false};
        }
        return (so_R_f32_bool){.val = f * so_at(float, float32pow10, exp), .val2 = true};
    } else if (exp < 0 && exp >= -10) {
        return (so_R_f32_bool){.val = f / so_at(float, float32pow10, -exp), .val2 = true};
    }
    return (so_R_f32_bool){.val = 0, .val2 = false};
}

// atofHex converts the hex floating-point string s
// to a rounded float32 or float64 value (depending on flt==&float32info or flt==&float64info)
// and returns it as a float64.
// The string s has already been parsed into a mantissa, exponent, and sign (neg==true for negative).
// If trunc is true, trailing non-zero bits have been omitted from the mantissa.
static so_R_f64_err atofHex(so_String s, floatInfo* flt, uint64_t mantissa, so_int exp, bool neg, bool trunc) {
    (void)s;
    so_int maxExp = ((so_int)1 << flt->expbits) + flt->bias - 2;
    so_int minExp = flt->bias + 1;
    // mantissa now implicitly divided by 2^mantbits.
    exp += (so_int)(flt->mantbits);
    // Shift mantissa and exponent to bring representation into float range.
    // Eventually we want a mantissa with a leading 1-bit followed by mantbits other bits.
    // For rounding, we need two more, where the bottom bit represents
    // whether that bit or any later bit was non-zero.
    // (If the mantissa has already lost non-zero bits, trunc is true,
    // and we OR in a 1 below after shifting left appropriately.)
    for (; mantissa != 0 && (mantissa >> (flt->mantbits + 2)) == 0;) {
        mantissa <<= 1;
        exp--;
    }
    if (trunc) {
        mantissa |= 1;
    }
    for (; (mantissa >> (1 + flt->mantbits + 2)) != 0;) {
        mantissa = ((mantissa >> 1) | (mantissa & 1));
        exp++;
    }
    // If exponent is too negative,
    // denormalize in hopes of making it representable.
    // (The -2 is for the rounding bits.)
    for (; mantissa > 1 && exp < minExp - 2;) {
        mantissa = ((mantissa >> 1) | (mantissa & 1));
        exp++;
    }
    // Round using two bottom bits.
    uint64_t round = (mantissa & 3);
    mantissa >>= 2;
    // round to even (round up if mantissa is odd)
    round |= (mantissa & 1);
    exp += 2;
    if (round == 3) {
        mantissa++;
        if (mantissa == ((uint64_t)1 << (1 + flt->mantbits))) {
            mantissa >>= 1;
            exp++;
        }
    }
    if ((mantissa >> flt->mantbits) == 0) {
        // Denormal or zero.
        exp = flt->bias;
    }
    so_Error err = {0};
    if (exp > maxExp) {
        // infinity and range error
        mantissa = ((uint64_t)1 << flt->mantbits);
        exp = maxExp + 1;
        err = strconv_ErrRange;
    }
    uint64_t bits = (mantissa & (((uint64_t)1 << flt->mantbits) - 1));
    bits |= ((uint64_t)((exp - flt->bias) & (((so_int)1 << flt->expbits) - 1)) << flt->mantbits);
    if (neg) {
        bits |= (((uint64_t)1 << flt->mantbits) << flt->expbits);
    }
    if (flt == &float32info) {
        return (so_R_f64_err){.val = (double)(float32frombits((uint32_t)(bits))), .err = err};
    }
    return (so_R_f64_err){.val = float64frombits(bits), .err = err};
}

static atof32Result atof32(so_String s) {
    so_Error err = {0};
    {
        specialFloat spec = special(s);
        if (spec.ok) {
            return (atof32Result){.f = (float)(spec.f), .n = spec.n, .err = (so_Error){0}};
        }
    }
    readFloatResult flo = readFloat(s);
    if (!flo.ok) {
        return (atof32Result){.f = 0, .n = flo.n, .err = strconv_ErrSyntax};
    }
    if (flo.hex) {
        so_R_f64_err _res1 = atofHex(so_string_slice(s, 0, flo.n), &float32info, flo.mantissa, flo.exp, flo.neg, flo.trunc);
        double f = _res1.val;
        so_Error err = _res1.err;
        return (atof32Result){.f = (float)(f), .n = flo.n, .err = err};
    }
    // Try pure floating-point arithmetic conversion, and if that fails,
    // the Eisel-Lemire algorithm.
    if (!flo.trunc) {
        {
            so_R_f32_bool _res2 = atof32exact(flo.mantissa, flo.exp, flo.neg);
            float f = _res2.val;
            bool ok = _res2.val2;
            if (ok) {
                return (atof32Result){.f = f, .n = flo.n, .err = (so_Error){0}};
            }
        }
    }
    so_R_f32_bool _res3 = eiselLemire32(flo.mantissa, flo.exp, flo.neg);
    float f = _res3.val;
    bool ok = _res3.val2;
    if (ok) {
        if (!flo.trunc) {
            return (atof32Result){.f = f, .n = flo.n, .err = (so_Error){0}};
        }
        // Even if the mantissa was truncated, we may
        // have found the correct result. Confirm by
        // converting the upper mantissa bound.
        so_R_f32_bool _res4 = eiselLemire32(flo.mantissa + 1, flo.exp, flo.neg);
        float fUp = _res4.val;
        bool ok = _res4.val2;
        if (ok && f == fUp) {
            return (atof32Result){.f = f, .n = flo.n, .err = (so_Error){0}};
        }
    }
    // Slow fallback.
    decimal d = {0};
    if (!decimal_set(&d, so_string_slice(s, 0, flo.n))) {
        return (atof32Result){.f = 0, .n = flo.n, .err = strconv_ErrSyntax};
    }
    so_R_u64_bool _res5 = decimal_floatBits(&d, &float32info);
    uint64_t b = _res5.val;
    bool ovf = _res5.val2;
    f = float32frombits((uint32_t)(b));
    if (ovf) {
        err = strconv_ErrRange;
    }
    return (atof32Result){.f = f, .n = flo.n, .err = err};
}

static atof64Result atof64(so_String s) {
    so_Error err = {0};
    {
        specialFloat spec = special(s);
        if (spec.ok) {
            return (atof64Result){.f = spec.f, .n = spec.n, .err = (so_Error){0}};
        }
    }
    readFloatResult flo = readFloat(s);
    if (!flo.ok) {
        return (atof64Result){.f = 0, .n = flo.n, .err = strconv_ErrSyntax};
    }
    if (flo.hex) {
        so_R_f64_err _res1 = atofHex(so_string_slice(s, 0, flo.n), &float64info, flo.mantissa, flo.exp, flo.neg, flo.trunc);
        double f = _res1.val;
        so_Error err = _res1.err;
        return (atof64Result){.f = f, .n = flo.n, .err = err};
    }
    // Try pure floating-point arithmetic conversion, and if that fails,
    // the Eisel-Lemire algorithm.
    if (!flo.trunc) {
        {
            so_R_f64_bool _res2 = atof64exact(flo.mantissa, flo.exp, flo.neg);
            double f = _res2.val;
            bool ok = _res2.val2;
            if (ok) {
                return (atof64Result){.f = f, .n = flo.n, .err = (so_Error){0}};
            }
        }
    }
    so_R_f64_bool _res3 = eiselLemire64(flo.mantissa, flo.exp, flo.neg);
    double f = _res3.val;
    bool ok = _res3.val2;
    if (ok) {
        if (!flo.trunc) {
            return (atof64Result){.f = f, .n = flo.n, .err = (so_Error){0}};
        }
        // Even if the mantissa was truncated, we may
        // have found the correct result. Confirm by
        // converting the upper mantissa bound.
        so_R_f64_bool _res4 = eiselLemire64(flo.mantissa + 1, flo.exp, flo.neg);
        double fUp = _res4.val;
        bool ok = _res4.val2;
        if (ok && f == fUp) {
            return (atof64Result){.f = f, .n = flo.n, .err = (so_Error){0}};
        }
    }
    // Slow fallback.
    decimal d = {0};
    if (!decimal_set(&d, so_string_slice(s, 0, flo.n))) {
        return (atof64Result){.f = 0, .n = flo.n, .err = strconv_ErrSyntax};
    }
    so_R_u64_bool _res5 = decimal_floatBits(&d, &float64info);
    uint64_t b = _res5.val;
    bool ovf = _res5.val2;
    f = float64frombits(b);
    if (ovf) {
        err = strconv_ErrRange;
    }
    return (atof64Result){.f = f, .n = flo.n, .err = err};
}

// ParseFloat converts the string s to a floating-point number
// with the precision specified by bitSize: 32 for float32, or 64 for float64.
// When bitSize=32, the result still has type float64, but it will be
// convertible to float32 without changing its value.
//
// ParseFloat accepts decimal and hexadecimal floating-point numbers
// as defined by the Go syntax for [floating-point literals].
// If s is well-formed and near a valid floating-point number,
// ParseFloat returns the nearest floating-point number rounded
// using IEEE754 unbiased rounding.
// (Parsing a hexadecimal floating-point value only rounds when
// there are more bits in the hexadecimal representation than
// will fit in the mantissa.)
//
// The errors that ParseFloat returns have concrete type *NumError
// and include err.Num = s.
//
// If s is not syntactically well-formed, ParseFloat returns err.Err = ErrSyntax.
//
// If s is syntactically well-formed but is more than 1/2 ULP
// away from the largest floating point number of the given size,
// ParseFloat returns f = ±Inf, err.Err = ErrRange.
//
// ParseFloat recognizes the string "NaN", and the (possibly signed) strings "Inf" and "Infinity"
// as their respective special floating point values. It ignores case when matching.
//
// [floating-point literals]: https://go.dev/ref/spec#Floating-point_literals
so_R_f64_err strconv_ParseFloat(so_String s, so_int bitSize) {
    atof64Result res = parseFloatPrefix(s, bitSize);
    if (res.n != so_len(s)) {
        return (so_R_f64_err){.val = 0, .err = strconv_ErrSyntax};
    }
    return (so_R_f64_err){.val = res.f, .err = res.err};
}

static atof64Result parseFloatPrefix(so_String s, so_int bitSize) {
    if (bitSize == 32) {
        atof32Result res = atof32(s);
        return (atof64Result){.f = (double)(res.f), .n = res.n, .err = res.err};
    }
    return atof64(s);
}

// -- atofeisel.go --

static so_R_f64_bool eiselLemire64(uint64_t man, so_int exp10, bool neg) {
    // The terse comments in this function body refer to sections of the
    // https://nigeltao.github.io/blog/2020/eisel-lemire.html blog post.
    double f = 0;
    // Exp10 Range.
    if (man == 0) {
        if (neg) {
            // Negative zero.
            f = float64frombits(0x8000000000000000);
        }
        return (so_R_f64_bool){.val = f, .val2 = true};
    }
    pow10Result powr = intPow10(exp10);
    so_uint128 pow = powr.mant;
    so_int exp2 = powr.exp;
    if (!powr.ok) {
        return (so_R_f64_bool){.val = 0, .val2 = false};
    }
    // Normalization.
    so_int clz = bits_LeadingZeros64(man);
    man <<= (so_uint)(clz);
    uint64_t retExp2 = (uint64_t)(exp2 + 63 - float64Bias) - (uint64_t)(clz);
    // Multiplication.
    so_R_u64_u64 _res1 = bits_Mul64(man, pow.hi);
    uint64_t xHi = _res1.val;
    uint64_t xLo = _res1.val2;
    // Wider Approximation.
    if ((xHi & 0x1FF) == 0x1FF && xLo + man < man) {
        so_R_u64_u64 _res2 = bits_Mul64(man, pow.lo);
        uint64_t yHi = _res2.val;
        uint64_t yLo = _res2.val2;
        uint64_t mergedHi = xHi, mergedLo = xLo + yHi;
        if (mergedLo < xLo) {
            mergedHi++;
        }
        if ((mergedHi & 0x1FF) == 0x1FF && mergedLo + 1 == 0 && yLo + man < man) {
            return (so_R_f64_bool){.val = 0, .val2 = false};
        }
        xHi = mergedHi;
        xLo = mergedLo;
    }
    // Shifting to 54 Bits.
    uint64_t msb = (xHi >> 63);
    uint64_t retMantissa = (xHi >> (msb + 9));
    retExp2 -= (1 ^ msb);
    // Half-way Ambiguity.
    if (xLo == 0 && (xHi & 0x1FF) == 0 && (retMantissa & 3) == 1) {
        return (so_R_f64_bool){.val = 0, .val2 = false};
    }
    // From 54 to 53 Bits.
    retMantissa += (retMantissa & 1);
    retMantissa >>= 1;
    if ((retMantissa >> 53) > 0) {
        retMantissa >>= 1;
        retExp2 += 1;
    }
    // retExp2 is a uint64. Zero or underflow means that we're in subnormal
    // float64 space. 0x7FF or above means that we're in Inf/NaN float64 space.
    //
    // The if block is equivalent to (but has fewer branches than):
    //   if retExp2 <= 0 || retExp2 >= 0x7FF { etc }
    if (retExp2 - 1 >= 0x7FF - 1) {
        return (so_R_f64_bool){.val = 0, .val2 = false};
    }
    uint64_t retBits = ((retExp2 << float64MantBits) | (retMantissa & (((int64_t)1 << float64MantBits) - 1)));
    if (neg) {
        retBits |= 0x8000000000000000;
    }
    return (so_R_f64_bool){.val = float64frombits(retBits), .val2 = true};
}

static so_R_f32_bool eiselLemire32(uint64_t man, so_int exp10, bool neg) {
    // The terse comments in this function body refer to sections of the
    // https://nigeltao.github.io/blog/2020/eisel-lemire.html blog post.
    //
    // That blog post discusses the float64 flavor (11 exponent bits with a
    // -1023 bias, 52 mantissa bits) of the algorithm, but the same approach
    // applies to the float32 flavor (8 exponent bits with a -127 bias, 23
    // mantissa bits). The computation here happens with 64-bit values (e.g.
    // man, xHi, retMantissa) before finally converting to a 32-bit float.
    float f = 0;
    // Exp10 Range.
    if (man == 0) {
        if (neg) {
            // Negative zero.
            f = float32frombits(0x80000000);
        }
        return (so_R_f32_bool){.val = f, .val2 = true};
    }
    pow10Result powr = intPow10(exp10);
    so_uint128 pow = powr.mant;
    so_int exp2 = powr.exp;
    if (!powr.ok) {
        return (so_R_f32_bool){.val = 0, .val2 = false};
    }
    // Normalization.
    so_int clz = bits_LeadingZeros64(man);
    man <<= (so_uint)(clz);
    uint64_t retExp2 = (uint64_t)(exp2 + 63 - float32Bias) - (uint64_t)(clz);
    // Multiplication.
    so_R_u64_u64 _res1 = bits_Mul64(man, pow.hi);
    uint64_t xHi = _res1.val;
    uint64_t xLo = _res1.val2;
    // Wider Approximation.
    if ((xHi & 0x3FFFFFFFFF) == 0x3FFFFFFFFF && xLo + man < man) {
        so_R_u64_u64 _res2 = bits_Mul64(man, pow.lo);
        uint64_t yHi = _res2.val;
        uint64_t yLo = _res2.val2;
        uint64_t mergedHi = xHi, mergedLo = xLo + yHi;
        if (mergedLo < xLo) {
            mergedHi++;
        }
        if ((mergedHi & 0x3FFFFFFFFF) == 0x3FFFFFFFFF && mergedLo + 1 == 0 && yLo + man < man) {
            return (so_R_f32_bool){.val = 0, .val2 = false};
        }
        xHi = mergedHi;
        xLo = mergedLo;
    }
    // Shifting to 54 Bits (and for float32, it's shifting to 25 bits).
    uint64_t msb = (xHi >> 63);
    uint64_t retMantissa = (xHi >> (msb + 38));
    retExp2 -= (1 ^ msb);
    // Half-way Ambiguity.
    if (xLo == 0 && (xHi & 0x3FFFFFFFFF) == 0 && (retMantissa & 3) == 1) {
        return (so_R_f32_bool){.val = 0, .val2 = false};
    }
    // From 54 to 53 Bits (and for float32, it's from 25 to 24 bits).
    retMantissa += (retMantissa & 1);
    retMantissa >>= 1;
    if ((retMantissa >> 24) > 0) {
        retMantissa >>= 1;
        retExp2 += 1;
    }
    // retExp2 is a uint64. Zero or underflow means that we're in subnormal
    // float32 space. 0xFF or above means that we're in Inf/NaN float32 space.
    //
    // The if block is equivalent to (but has fewer branches than):
    //   if retExp2 <= 0 || retExp2 >= 0xFF { etc }
    if (retExp2 - 1 >= 0xFF - 1) {
        return (so_R_f32_bool){.val = 0, .val2 = false};
    }
    uint64_t retBits = ((retExp2 << float32MantBits) | (retMantissa & (((int64_t)1 << float32MantBits) - 1)));
    if (neg) {
        retBits |= 0x80000000;
    }
    return (so_R_f32_bool){.val = float32frombits((uint32_t)(retBits)), .val2 = true};
}

// -- atoi.go --

// lower(c) is a lower-case letter if and only if
// c is either that lower-case letter or the equivalent upper-case letter.
// Instead of writing c == 'x' || c == 'X' one can write lower(c) == 'x'.
// Note that lower of non-letters can produce other non-letters.
static so_byte lower(so_byte c) {
    return (c | (U'x' - U'X'));
}

// ParseUint is like [ParseInt] but for unsigned numbers.
//
// A sign prefix is not permitted.
so_R_u64_err strconv_ParseUint(so_String s, so_int base, so_int bitSize) {
    if (so_string_eq(s, so_str(""))) {
        return (so_R_u64_err){.val = 0, .err = strconv_ErrSyntax};
    }
    bool base0 = base == 0;
    so_String s0 = s;
    if (2 <= base && base <= 36) {
    } else if (base == 0) {
        // Look for octal, hex prefix.
        base = 10;
        if (so_at(so_byte, s, 0) == '0') {
            if (so_len(s) >= 3 && lower(so_at(so_byte, s, 1)) == 'b') {
                base = 2;
                s = so_string_slice(s, 2, s.len);
            } else if (so_len(s) >= 3 && lower(so_at(so_byte, s, 1)) == 'o') {
                base = 8;
                s = so_string_slice(s, 2, s.len);
            } else if (so_len(s) >= 3 && lower(so_at(so_byte, s, 1)) == 'x') {
                base = 16;
                s = so_string_slice(s, 2, s.len);
            } else {
                base = 8;
                s = so_string_slice(s, 1, s.len);
            }
        }
    } else {
        return (so_R_u64_err){.val = 0, .err = strconv_ErrBase};
    }
    if (bitSize == 0) {
        bitSize = strconv_IntSize;
    } else if (bitSize < 0 || bitSize > 64) {
        return (so_R_u64_err){.val = 0, .err = strconv_ErrBitSize};
    }
    // Cutoff is the smallest number such that cutoff*base > maxUint64.
    // Use compile-time constants for common cases.
    uint64_t cutoff = 0;
    if (base == (10)) {
        cutoff = maxUint64 / 10 + 1;
    } else if (base == (16)) {
        cutoff = maxUint64 / 16 + 1;
    } else {
        cutoff = maxUint64 / (uint64_t)(base) + 1;
    }
    uint64_t maxVal = (((uint64_t)(1) << (so_uint)(bitSize - 1)) << 1) - 1;
    bool underscores = false;
    uint64_t n = 0;
    for (so_int _ = 0; _ < so_len(so_string_bytes(s)); _++) {
        so_byte c = so_at(so_byte, so_string_bytes(s), _);
        so_byte d = 0;
        if (c == '_' && base0) {
            underscores = true;
            continue;
        } else if ('0' <= c && c <= '9') {
            d = c - '0';
        } else if ('a' <= lower(c) && lower(c) <= 'z') {
            d = lower(c) - 'a' + 10;
        } else {
            return (so_R_u64_err){.val = 0, .err = strconv_ErrSyntax};
        }
        if (d >= (so_byte)(base)) {
            return (so_R_u64_err){.val = 0, .err = strconv_ErrSyntax};
        }
        if (n >= cutoff) {
            // n*base overflows
            return (so_R_u64_err){.val = maxVal, .err = strconv_ErrRange};
        }
        n *= (uint64_t)(base);
        uint64_t n1 = n + (uint64_t)(d);
        if (n1 < n || n1 > maxVal) {
            // n+d overflows
            return (so_R_u64_err){.val = maxVal, .err = strconv_ErrRange};
        }
        n = n1;
    }
    if (underscores && !underscoreOK(s0)) {
        return (so_R_u64_err){.val = 0, .err = strconv_ErrSyntax};
    }
    return (so_R_u64_err){.val = n, .err = (so_Error){0}};
}

// ParseInt interprets a string s in the given base (0, 2 to 36) and
// bit size (0 to 64) and returns the corresponding value i.
//
// The string may begin with a leading sign: "+" or "-".
//
// If the base argument is 0, the true base is implied by the string's
// prefix following the sign (if present): 2 for "0b", 8 for "0" or "0o",
// 16 for "0x", and 10 otherwise. Also, for argument base 0 only,
// underscore characters are permitted as defined by the Go syntax for
// [integer literals].
//
// The bitSize argument specifies the integer type
// that the result must fit into. Bit sizes 0, 8, 16, 32, and 64
// correspond to int, int8, int16, int32, and int64.
// If bitSize is below 0 or above 64, an error is returned.
//
// The errors that ParseInt returns have concrete type [*NumError]
// and include err.Num = s. If s is empty or contains invalid
// digits, err.Err = [ErrSyntax] and the returned value is 0;
// if the value corresponding to s cannot be represented by a
// signed integer of the given size, err.Err = [ErrRange] and the
// returned value is the maximum magnitude integer of the
// appropriate bitSize and sign.
//
// [integer literals]: https://go.dev/ref/spec#Integer_literals
so_R_i64_err strconv_ParseInt(so_String s, so_int base, so_int bitSize) {
    so_Error err = {0};
    if (so_string_eq(s, so_str(""))) {
        return (so_R_i64_err){.val = 0, .err = strconv_ErrSyntax};
    }
    // Pick off leading sign.
    bool neg = false;
    if (so_at(so_byte, s, 0) == ('+')) {
        s = so_string_slice(s, 1, s.len);
    } else if (so_at(so_byte, s, 0) == ('-')) {
        s = so_string_slice(s, 1, s.len);
        neg = true;
    }
    // Convert unsigned and check range.
    uint64_t un = 0;
    so_R_u64_err _res1 = strconv_ParseUint(s, base, bitSize);
    un = _res1.val;
    err = _res1.err;
    if (err.self != NULL && err.self != strconv_ErrRange.self) {
        return (so_R_i64_err){.val = 0, .err = err};
    }
    if (bitSize == 0) {
        bitSize = strconv_IntSize;
    }
    uint64_t cutoff = ((uint64_t)(1) << (so_uint)(bitSize - 1));
    if (!neg && un >= cutoff) {
        return (so_R_i64_err){.val = (int64_t)(cutoff - 1), .err = strconv_ErrRange};
    }
    if (neg && un > cutoff) {
        return (so_R_i64_err){.val = -(int64_t)(cutoff), .err = strconv_ErrRange};
    }
    int64_t n = (int64_t)(un);
    if (neg) {
        n = -n;
    }
    return (so_R_i64_err){.val = n, .err = (so_Error){0}};
}

// Atoi is equivalent to ParseInt(s, 10, 0), converted to type int.
so_R_int_err strconv_Atoi(so_String s) {
    so_int sLen = so_len(s);
    if ((strconv_IntSize == 32 && (0 < sLen && sLen < 10)) || (strconv_IntSize == 64 && (0 < sLen && sLen < 19))) {
        // Fast path for small integers that fit int type.
        so_String s0 = s;
        if (so_at(so_byte, s, 0) == '-' || so_at(so_byte, s, 0) == '+') {
            s = so_string_slice(s, 1, s.len);
            if (so_len(s) < 1) {
                return (so_R_int_err){.val = 0, .err = strconv_ErrSyntax};
            }
        }
        so_int n = 0;
        for (so_int _ = 0; _ < so_len(so_string_bytes(s)); _++) {
            so_byte ch = so_at(so_byte, so_string_bytes(s), _);
            ch -= '0';
            if (ch > 9) {
                return (so_R_int_err){.val = 0, .err = strconv_ErrSyntax};
            }
            n = n * 10 + (so_int)(ch);
        }
        if (so_at(so_byte, s0, 0) == '-') {
            n = -n;
        }
        return (so_R_int_err){.val = n, .err = (so_Error){0}};
    }
    // Slow path for invalid, big, or underscored integers.
    so_R_i64_err _res1 = strconv_ParseInt(s, 10, 0);
    int64_t i64 = _res1.val;
    so_Error err = _res1.err;
    return (so_R_int_err){.val = (so_int)(i64), .err = err};
}

// underscoreOK reports whether the underscores in s are allowed.
// Checking them in this one function lets all the parsers skip over them simply.
// Underscore must appear only between digits or between a base prefix and a digit.
static bool underscoreOK(so_String s) {
    // saw tracks the last character (class) we saw:
    // ^ for beginning of number,
    // 0 for a digit or base prefix,
    // _ for an underscore,
    // ! for none of the above.
    so_rune saw = U'^';
    so_int i = 0;
    // Optional sign.
    if (so_len(s) >= 1 && (so_at(so_byte, s, 0) == '-' || so_at(so_byte, s, 0) == '+')) {
        s = so_string_slice(s, 1, s.len);
    }
    // Optional base prefix.
    bool hex = false;
    if (so_len(s) >= 2 && so_at(so_byte, s, 0) == '0' && (lower(so_at(so_byte, s, 1)) == 'b' || lower(so_at(so_byte, s, 1)) == 'o' || lower(so_at(so_byte, s, 1)) == 'x')) {
        i = 2;
        // base prefix counts as a digit for "underscore as digit separator"
        saw = U'0';
        hex = lower(so_at(so_byte, s, 1)) == 'x';
    }
    // Number proper.
    for (; i < so_len(s); i++) {
        // Digits are always okay.
        if (('0' <= so_at(so_byte, s, i) && so_at(so_byte, s, i) <= '9') || (hex && 'a' <= lower(so_at(so_byte, s, i)) && lower(so_at(so_byte, s, i)) <= 'f')) {
            saw = U'0';
            continue;
        }
        // Underscore must follow digit.
        if (so_at(so_byte, s, i) == '_') {
            if (saw != U'0') {
                return false;
            }
            saw = U'_';
            continue;
        }
        // Underscore must also be followed by digit.
        if (saw == U'_') {
            return false;
        }
        // Saw non-digit, non-underscore.
        saw = U'!';
    }
    return saw != U'_';
}

// -- decimal.go --

// trim trailing zeros from number.
// (They are meaningless; the decimal point is tracked
// independent of the number of digits.)
static void trim(decimal* a) {
    for (; a->nd > 0 && a->d[a->nd - 1] == '0';) {
        a->nd--;
    }
    if (a->nd == 0) {
        a->dp = 0;
    }
}

// Assign v to a.
static void decimal_Assign(void* self, uint64_t v) {
    decimal* a = self;
    so_byte buf[24] = {0};
    // Write reversed decimal in buf.
    so_int n = 0;
    for (; v > 0;) {
        uint64_t v1 = v / 10;
        v -= 10 * v1;
        buf[n] = (so_byte)(v + U'0');
        n++;
        v = v1;
    }
    // Reverse again to produce forward decimal in a.d.
    a->nd = 0;
    for (n--; n >= 0; n--) {
        a->d[a->nd] = buf[n];
        a->nd++;
    }
    a->dp = a->nd;
    trim(a);
}

// Binary shift right (/ 2) by k bits.  k <= maxShift to avoid overflow.
static void rightShift(decimal* a, so_uint k) {
    // read pointer
    so_int r = 0;
    // write pointer
    so_int w = 0;
    // Pick up enough leading digits to cover first shift.
    so_uint n = 0;
    for (; (n >> k) == 0; r++) {
        if (r >= a->nd) {
            if (n == 0) {
                // a == 0; shouldn't get here, but handle anyway.
                a->nd = 0;
                return;
            }
            for (; (n >> k) == 0;) {
                n = n * 10;
                r++;
            }
            break;
        }
        so_uint c = (so_uint)(a->d[r]);
        n = n * 10 + c - U'0';
    }
    a->dp -= r - 1;
    so_uint mask = ((so_uint)1 << k) - 1;
    // Pick up a digit, put down a digit.
    for (; r < a->nd; r++) {
        so_uint c = (so_uint)(a->d[r]);
        so_uint dig = (n >> k);
        n &= mask;
        a->d[w] = (so_byte)(dig + U'0');
        w++;
        n = n * 10 + c - U'0';
    }
    // Put down extra digits.
    for (; n > 0;) {
        so_uint dig = (n >> k);
        n &= mask;
        if (w < 800) {
            a->d[w] = (so_byte)(dig + U'0');
            w++;
        } else if (dig > 0) {
            a->trunc = true;
        }
        n = n * 10;
    }
    a->nd = w;
    trim(a);
}

// Is the leading prefix of b lexicographically less than s?
static bool prefixIsLessThan(so_Slice b, so_String s) {
    for (so_int i = 0; i < so_len(s); i++) {
        if (i >= so_len(b)) {
            return true;
        }
        if (so_at(so_byte, b, i) != so_at(so_byte, s, i)) {
            return so_at(so_byte, b, i) < so_at(so_byte, s, i);
        }
    }
    return false;
}

// Binary shift left (* 2) by k bits.  k <= maxShift to avoid overflow.
static void leftShift(decimal* a, so_uint k) {
    so_int delta = so_at(leftCheat, leftcheats, k).delta;
    if (prefixIsLessThan(so_array_slice(so_byte, a->d, 0, a->nd, 800), so_at(leftCheat, leftcheats, k).cutoff)) {
        delta--;
    }
    // read index
    so_int r = a->nd;
    // write index
    so_int w = a->nd + delta;
    // Pick up a digit, put down a digit.
    so_uint n = 0;
    for (r--; r >= 0; r--) {
        n += (((so_uint)(a->d[r]) - U'0') << k);
        so_uint quo = n / 10;
        so_uint rem = n - 10 * quo;
        w--;
        if (w < 800) {
            a->d[w] = (so_byte)(rem + U'0');
        } else if (rem != 0) {
            a->trunc = true;
        }
        n = quo;
    }
    // Put down extra digits.
    for (; n > 0;) {
        so_uint quo = n / 10;
        so_uint rem = n - 10 * quo;
        w--;
        if (w < 800) {
            a->d[w] = (so_byte)(rem + U'0');
        } else if (rem != 0) {
            a->trunc = true;
        }
        n = quo;
    }
    a->nd += delta;
    if (a->nd >= 800) {
        a->nd = 800;
    }
    a->dp += delta;
    trim(a);
}

// Binary shift left (k > 0) or right (k < 0).
static void decimal_Shift(void* self, so_int k) {
    decimal* a = self;
    if (a->nd == 0) {
    } else if (k > 0) {
        for (; k > maxShift;) {
            leftShift(a, maxShift);
            k -= maxShift;
        }
        leftShift(a, (so_uint)(k));
    } else if (k < 0) {
        for (; k < -maxShift;) {
            rightShift(a, maxShift);
            k += maxShift;
        }
        rightShift(a, (so_uint)(-k));
    }
}

// If we chop a at nd digits, should we round up?
static bool shouldRoundUp(decimal* a, so_int nd) {
    if (nd < 0 || nd >= a->nd) {
        return false;
    }
    if (a->d[nd] == '5' && nd + 1 == a->nd) {
        // exactly halfway - round to even
        // if we truncated, a little higher than what's recorded - always round up
        if (a->trunc) {
            return true;
        }
        return nd > 0 && (a->d[nd - 1] - '0') % 2 != 0;
    }
    // not halfway - digit tells all
    return a->d[nd] >= '5';
}

// Round a to nd digits (or fewer).
// If nd is zero, it means we're rounding
// just to the left of the digits, as in
// 0.09 -> 0.1.
static void decimal_Round(void* self, so_int nd) {
    decimal* a = self;
    if (nd < 0 || nd >= a->nd) {
        return;
    }
    if (shouldRoundUp(a, nd)) {
        decimal_RoundUp(a, nd);
    } else {
        decimal_RoundDown(a, nd);
    }
}

// Round a down to nd digits (or fewer).
static void decimal_RoundDown(void* self, so_int nd) {
    decimal* a = self;
    if (nd < 0 || nd >= a->nd) {
        return;
    }
    a->nd = nd;
    trim(a);
}

// Round a up to nd digits (or fewer).
static void decimal_RoundUp(void* self, so_int nd) {
    decimal* a = self;
    if (nd < 0 || nd >= a->nd) {
        return;
    }
    // round up
    for (so_int i = nd - 1; i >= 0; i--) {
        so_byte c = a->d[i];
        if (c < '9') {
            // can stop after this digit
            a->d[i]++;
            a->nd = i + 1;
            return;
        }
    }
    // Number is all 9s.
    // Change to single 1 with adjusted decimal point.
    a->d[0] = '1';
    a->nd = 1;
    a->dp++;
}

// Extract integer part, rounded appropriately.
// No guarantees about overflow.
static uint64_t decimal_RoundedInteger(void* self) {
    decimal* a = self;
    if (a->dp > 20) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    so_int i = 0;
    uint64_t n = (uint64_t)(0);
    for (i = 0; i < a->dp && i < a->nd; i++) {
        n = n * 10 + (uint64_t)(a->d[i] - '0');
    }
    for (; i < a->dp; i++) {
        n *= 10;
    }
    if (shouldRoundUp(a, a->dp)) {
        n++;
    }
    return n;
}

// -- deps.go --

// Implementations to avoid importing other dependencies.
// package math
static double float64frombits(uint64_t b) {
    return *(double*)((void*)(&b));
}

static float float32frombits(uint32_t b) {
    return *(float*)((void*)(&b));
}

static uint64_t float64bits(double f) {
    return *(uint64_t*)((void*)(&f));
}

static uint32_t float32bits(float f) {
    return *(uint32_t*)((void*)(&f));
}

static double floatInf(so_int sign) {
    uint64_t v = 0;
    if (sign >= 0) {
        v = 0x7FF0000000000000;
    } else {
        v = 0xFFF0000000000000;
    }
    return float64frombits(v);
}

static double floatNaN(void) {
    return float64frombits(0x7FF8000000000001);
}

// -- doc.go --

// -- ftoa.go --

// FormatFloat converts the floating-point number f to a string,
// according to the format fmt and precision prec. It rounds the
// result assuming that the original was obtained from a floating-point
// value of bitSize bits (32 for float32, 64 for float64).
//
// The format fmt is one of
//   - 'b' (-ddddp±ddd, a binary exponent),
//   - 'e' (-d.dddde±dd, a decimal exponent),
//   - 'E' (-d.ddddE±dd, a decimal exponent),
//   - 'f' (-ddd.dddd, no exponent),
//   - 'g' ('e' for large exponents, 'f' otherwise),
//   - 'G' ('E' for large exponents, 'f' otherwise),
//   - 'x' (-0xd.ddddp±ddd, a hexadecimal fraction and binary exponent), or
//   - 'X' (-0Xd.ddddP±ddd, a hexadecimal fraction and binary exponent).
//
// The precision prec controls the number of digits (excluding the exponent)
// printed by the 'e', 'E', 'f', 'g', 'G', 'x', and 'X' formats.
// For 'e', 'E', 'f', 'x', and 'X', it is the number of digits after the decimal point.
// For 'g' and 'G' it is the maximum number of significant digits (trailing
// zeros are removed).
// The special precision -1 uses the smallest number of digits
// necessary such that ParseFloat will return f exactly.
// The exponent is written as a decimal integer;
// for all formats other than 'b', it will be at least two digits.
//
// dst length must be at least prec+4 bytes when prec >= 0,
// and at least [MaxFloat64Len] bytes when prec < 0.
so_String strconv_FormatFloat(so_Slice dst, double f, so_byte fmt, so_int prec, so_int bitSize) {
    so_Slice buf = so_slice(so_byte, dst, 0, 0);
    so_Slice res = genericFtoa(buf, f, fmt, prec, bitSize);
    return so_bytes_string(res);
}

// AppendFloat appends the string form of the floating-point number f,
// as generated by [FormatFloat], to dst and returns the extended buffer.
//
// dst free capacity must be at least prec+4 bytes when prec >= 0,
// and at least [MaxFloat64Len] bytes when prec < 0.
so_Slice strconv_AppendFloat(so_Slice dst, double f, so_byte fmt, so_int prec, so_int bitSize) {
    return genericFtoa(dst, f, fmt, prec, bitSize);
}

static so_Slice genericFtoa(so_Slice dst, double val, so_byte fmt, so_int prec, so_int bitSize) {
    uint64_t bits = 0;
    floatInfo* flt = NULL;
    if (bitSize == (32)) {
        bits = (uint64_t)(float32bits((float)(val)));
        flt = &float32info;
    } else if (bitSize == (64)) {
        bits = float64bits(val);
        flt = &float64info;
    } else {
        so_panic("strconv: illegal AppendFloat/FormatFloat bitSize");
    }
    bool neg = (bits >> (flt->expbits + flt->mantbits)) != 0;
    so_int exp = ((so_int)(bits >> flt->mantbits) & (((so_int)1 << flt->expbits) - 1));
    uint64_t mant = (bits & (((uint64_t)(1) << flt->mantbits) - 1));
    bool denorm = false;
    if (exp == (((so_int)1 << flt->expbits) - 1)) {
        // Inf, NaN
        so_String s = so_str("");
        if (mant != 0) {
            s = so_str("NaN");
        } else if (neg) {
            s = so_str("-Inf");
        } else {
            s = so_str("+Inf");
        }
        return so_extend(so_byte, dst, so_string_bytes(s));
    } else if (exp == (0)) {
        // denormalized
        exp++;
        denorm = true;
    } else {
        // add implicit top bit
        mant |= ((uint64_t)(1) << flt->mantbits);
    }
    exp += flt->bias;
    // Pick off easy binary, hex formats.
    if (fmt == 'b') {
        return fmtB(dst, neg, mant, exp, flt);
    }
    if (fmt == 'x' || fmt == 'X') {
        return fmtX(dst, prec, fmt, neg, mant, exp, flt);
    }
    // Negative precision means "only as much as needed to be exact."
    bool shortest = prec < 0;
    decimalSlice digs = {0};
    if (mant == 0) {
        return formatDigits(dst, shortest, neg, digs, prec, fmt);
    }
    if (shortest) {
        // Use the Dragonbox algorithm.
        so_byte buf[32] = {0};
        digs.d = so_array_slice(so_byte, buf, 0, 32, 32);
        dboxFtoa(&digs, mant, exp - (so_int)(flt->mantbits), denorm, bitSize);
        // Precision for shortest representation mode.
        if (fmt == ('e') || fmt == ('E')) {
            prec = so_max(digs.nd - 1, 0);
        } else if (fmt == ('f')) {
            prec = so_max(digs.nd - digs.dp, 0);
        } else if (fmt == ('g') || fmt == ('G')) {
            prec = digs.nd;
        }
        return formatDigits(dst, shortest, neg, digs, prec, fmt);
    }
    // Fixed number of digits.
    so_int digits = prec;
    if (fmt == ('f')) {
        // %f precision specifies digits after the decimal point.
        // Estimate an upper bound on the total number of digits needed.
        // ftoaFixed will shorten as needed according to prec.
        if (exp >= 0) {
            digits = 1 + mulLog10_2(1 + exp) + prec;
        } else {
            digits = 1 + prec - mulLog10_2(-exp);
        }
    } else if (fmt == ('e') || fmt == ('E')) {
        digits++;
    } else if (fmt == ('g') || fmt == ('G')) {
        if (prec == 0) {
            prec = 1;
        }
        digits = prec;
    } else {
        // Invalid mode.
        digits = 1;
    }
    if (digits <= 18) {
        // digits <= 0 happens for %f on very small numbers
        // and means that we're guaranteed to print all zeros.
        if (digits > 0) {
            so_byte buf[24] = {0};
            digs.d = so_array_slice(so_byte, buf, 0, 24, 24);
            fixedFtoa(&digs, mant, exp - (so_int)(flt->mantbits), digits, prec, fmt);
        }
        return formatDigits(dst, false, neg, digs, prec, fmt);
    }
    return bigFtoa(dst, prec, fmt, neg, mant, exp, flt);
}

// bigFtoa uses multiprecision computations to format a float.
static so_Slice bigFtoa(so_Slice dst, so_int prec, so_byte fmt, bool neg, uint64_t mant, so_int exp, floatInfo* flt) {
    decimal* d = &(decimal){0};
    decimal_Assign(d, mant);
    decimal_Shift(d, exp - (so_int)(flt->mantbits));
    decimalSlice digs = {0};
    bool shortest = prec < 0;
    if (shortest) {
        roundShortest(d, mant, exp, flt);
        digs = (decimalSlice){.d = so_array_slice(so_byte, d->d, 0, 800, 800), .nd = d->nd, .dp = d->dp};
        // Precision for shortest representation mode.
        if (fmt == ('e') || fmt == ('E')) {
            prec = digs.nd - 1;
        } else if (fmt == ('f')) {
            prec = so_max(digs.nd - digs.dp, 0);
        } else if (fmt == ('g') || fmt == ('G')) {
            prec = digs.nd;
        }
    } else {
        // Round appropriately.
        if (fmt == ('e') || fmt == ('E')) {
            decimal_Round(d, prec + 1);
        } else if (fmt == ('f')) {
            decimal_Round(d, d->dp + prec);
        } else if (fmt == ('g') || fmt == ('G')) {
            if (prec == 0) {
                prec = 1;
            }
            decimal_Round(d, prec);
        }
        digs = (decimalSlice){.d = so_array_slice(so_byte, d->d, 0, 800, 800), .nd = d->nd, .dp = d->dp};
    }
    return formatDigits(dst, shortest, neg, digs, prec, fmt);
}

static so_Slice formatDigits(so_Slice dst, bool shortest, bool neg, decimalSlice digs, so_int prec, so_byte fmt) {
    if (fmt == ('e') || fmt == ('E')) {
        return fmtE(dst, neg, digs, prec, fmt);
    } else if (fmt == ('f')) {
        return fmtF(dst, neg, digs, prec);
    } else if (fmt == ('g') || fmt == ('G')) {
        // trailing fractional zeros in 'e' form will be trimmed.
        so_int eprec = prec;
        if (eprec > digs.nd && digs.nd >= digs.dp) {
            eprec = digs.nd;
        }
        // %e is used if the exponent from the conversion
        // is less than -4 or greater than or equal to the precision.
        // if precision was the shortest possible, use precision 6 for this decision.
        if (shortest) {
            eprec = 6;
        }
        so_int exp = digs.dp - 1;
        if (exp < -4 || exp >= eprec) {
            if (prec > digs.nd) {
                prec = digs.nd;
            }
            return fmtE(dst, neg, digs, prec - 1, fmt + 'e' - 'g');
        }
        if (prec > digs.dp) {
            prec = digs.nd;
        }
        return fmtF(dst, neg, digs, so_max(prec - digs.dp, 0));
    }
    // unknown format
    return so_append(so_byte, dst, '%', fmt);
}

// roundShortest rounds d (= mant * 2^exp) to the shortest number of digits
// that will let the original floating point value be precisely reconstructed.
static void roundShortest(decimal* d, uint64_t mant, so_int exp, floatInfo* flt) {
    // If mantissa is zero, the number is zero; stop now.
    if (mant == 0) {
        d->nd = 0;
        return;
    }
    // Compute upper and lower such that any decimal number
    // between upper and lower (possibly inclusive)
    // will round to the original floating point number.
    // We may see at once that the number is already shortest.
    //
    // Suppose d is not denormal, so that 2^exp <= d < 10^dp.
    // The closest shorter number is at least 10^(dp-nd) away.
    // The lower/upper bounds computed below are at distance
    // at most 2^(exp-mantbits).
    //
    // So the number is already shortest if 10^(dp-nd) > 2^(exp-mantbits),
    // or equivalently log2(10)*(dp-nd) > exp-mantbits.
    // It is true if 332/100*(dp-nd) >= exp-mantbits (log2(10) > 3.32).
    // minimum possible exponent
    so_int minexp = flt->bias + 1;
    if (exp > minexp && 332 * (d->dp - d->nd) >= 100 * (exp - (so_int)(flt->mantbits))) {
        // The number is already shortest.
        return;
    }
    // d = mant << (exp - mantbits)
    // Next highest floating point number is mant+1 << exp-mantbits.
    // Our upper bound is halfway between, mant*2+1 << exp-mantbits-1.
    decimal* upper = &(decimal){0};
    decimal_Assign(upper, mant * 2 + 1);
    decimal_Shift(upper, exp - (so_int)(flt->mantbits) - 1);
    // d = mant << (exp - mantbits)
    // Next lowest floating point number is mant-1 << exp-mantbits,
    // unless mant-1 drops the significant bit and exp is not the minimum exp,
    // in which case the next lowest is mant*2-1 << exp-mantbits-1.
    // Either way, call it mantlo << explo-mantbits.
    // Our lower bound is halfway between, mantlo*2+1 << explo-mantbits-1.
    uint64_t mantlo = 0;
    so_int explo = 0;
    if (mant > ((uint64_t)1 << flt->mantbits) || exp == minexp) {
        mantlo = mant - 1;
        explo = exp;
    } else {
        mantlo = mant * 2 - 1;
        explo = exp - 1;
    }
    decimal* lower = &(decimal){0};
    decimal_Assign(lower, mantlo * 2 + 1);
    decimal_Shift(lower, explo - (so_int)(flt->mantbits) - 1);
    // The upper and lower bounds are possible outputs only if
    // the original mantissa is even, so that IEEE round-to-even
    // would round to the original mantissa and not the neighbors.
    bool inclusive = mant % 2 == 0;
    // As we walk the digits we want to know whether rounding up would fall
    // within the upper bound. This is tracked by upperdelta:
    //
    // If upperdelta == 0, the digits of d and upper are the same so far.
    //
    // If upperdelta == 1, we saw a difference of 1 between d and upper on a
    // previous digit and subsequently only 9s for d and 0s for upper.
    // (Thus rounding up may fall outside the bound, if it is exclusive.)
    //
    // If upperdelta == 2, then the difference is greater than 1
    // and we know that rounding up falls within the bound.
    uint8_t upperdelta = 0;
    // Now we can figure out the minimum number of digits required.
    // Walk along until d has distinguished itself from upper and lower.
    for (so_int ui = 0;; ui++) {
        // lower, d, and upper may have the decimal points at different
        // places. In this case upper is the longest, so we iterate from
        // ui==0 and start li and mi at (possibly) -1.
        so_int mi = ui - upper->dp + d->dp;
        if (mi >= d->nd) {
            break;
        }
        so_int li = ui - upper->dp + lower->dp;
        // lower digit
        so_byte l = (so_byte)('0');
        if (li >= 0 && li < lower->nd) {
            l = lower->d[li];
        }
        // middle digit
        so_byte m = (so_byte)('0');
        if (mi >= 0) {
            m = d->d[mi];
        }
        // upper digit
        so_byte u = (so_byte)('0');
        if (ui < upper->nd) {
            u = upper->d[ui];
        }
        // Okay to round down (truncate) if lower has a different digit
        // or if lower is inclusive and is exactly the result of rounding
        // down (i.e., and we have reached the final digit of lower).
        bool okdown = l != m || (inclusive && li + 1 == lower->nd);
        if (upperdelta == 0 && m + 1 < u) {
            // Example:
            // m = 12345xxx
            // u = 12347xxx
            upperdelta = 2;
        } else if (upperdelta == 0 && m != u) {
            // Example:
            // m = 12345xxx
            // u = 12346xxx
            upperdelta = 1;
        } else if (upperdelta == 1 && (m != '9' || u != '0')) {
            // Example:
            // m = 1234598x
            // u = 1234600x
            upperdelta = 2;
        }
        // Okay to round up if upper has a different digit and either upper
        // is inclusive or upper is bigger than the result of rounding up.
        bool okup = upperdelta > 0 && (inclusive || upperdelta > 1 || ui + 1 < upper->nd);
        // If it's okay to do either, then round to the nearest one.
        // If it's okay to do only one, do it.
        if (okdown && okup) {
            decimal_Round(d, mi + 1);
            return;
        } else if (okdown) {
            decimal_RoundDown(d, mi + 1);
            return;
        } else if (okup) {
            decimal_RoundUp(d, mi + 1);
            return;
        }
    }
}

// %e: -d.ddddde±dd
static so_Slice fmtE(so_Slice dst, bool neg, decimalSlice d, so_int prec, so_byte fmt) {
    // sign
    if (neg) {
        dst = so_append(so_byte, dst, '-');
    }
    // first digit
    so_byte ch = (so_byte)('0');
    if (d.nd != 0) {
        ch = so_at(so_byte, d.d, 0);
    }
    dst = so_append(so_byte, dst, ch);
    // .moredigits
    if (prec > 0) {
        dst = so_append(so_byte, dst, '.');
        so_int i = 1;
        so_int m = so_min(d.nd, prec + 1);
        if (i < m) {
            dst = so_extend(so_byte, dst, (so_slice(so_byte, d.d, i, m)));
            i = m;
        }
        for (; i <= prec; i++) {
            dst = so_append(so_byte, dst, '0');
        }
    }
    // e±
    dst = so_append(so_byte, dst, fmt);
    so_int exp = d.dp - 1;
    if (d.nd == 0) {
        // special case: 0 has exponent 0
        exp = 0;
    }
    if (exp < 0) {
        ch = '-';
        exp = -exp;
    } else {
        ch = '+';
    }
    dst = so_append(so_byte, dst, ch);
    // dd or ddd
    if (exp < 10) {
        dst = so_append(so_byte, dst, '0', (so_byte)(exp) + '0');
    } else if (exp < 100) {
        dst = so_append(so_byte, dst, (so_byte)(exp / 10) + '0', (so_byte)(exp % 10) + '0');
    } else {
        dst = so_append(so_byte, dst, (so_byte)(exp / 100) + '0', (so_byte)(exp / 10) % 10 + '0', (so_byte)(exp % 10) + '0');
    }
    return dst;
}

// %f: -ddddddd.ddddd
static so_Slice fmtF(so_Slice dst, bool neg, decimalSlice d, so_int prec) {
    // sign
    if (neg) {
        dst = so_append(so_byte, dst, '-');
    }
    // integer, padded with zeros as needed.
    if (d.dp > 0) {
        so_int m = so_min(d.nd, d.dp);
        dst = so_extend(so_byte, dst, (so_slice(so_byte, d.d, 0, m)));
        for (; m < d.dp; m++) {
            dst = so_append(so_byte, dst, '0');
        }
    } else {
        dst = so_append(so_byte, dst, '0');
    }
    // fraction
    if (prec > 0) {
        dst = so_append(so_byte, dst, '.');
        for (so_int i = 0; i < prec; i++) {
            so_byte ch = (so_byte)('0');
            {
                so_int j = d.dp + i;
                if (0 <= j && j < d.nd) {
                    ch = so_at(so_byte, d.d, j);
                }
            }
            dst = so_append(so_byte, dst, ch);
        }
    }
    return dst;
}

// %b: -ddddddddp±ddd
static so_Slice fmtB(so_Slice dst, bool neg, uint64_t mant, so_int exp, floatInfo* flt) {
    // sign
    if (neg) {
        dst = so_append(so_byte, dst, '-');
    }
    // mantissa
    dst = strconv_AppendUint(dst, mant, 10);
    // p
    dst = so_append(so_byte, dst, 'p');
    // ±exponent
    exp -= (so_int)(flt->mantbits);
    if (exp >= 0) {
        dst = so_append(so_byte, dst, '+');
    }
    dst = strconv_AppendInt(dst, (int64_t)(exp), 10);
    return dst;
}

// %x: -0x1.yyyyyyyyp±ddd or -0x0p+0. (y is hex digit, d is decimal digit)
static so_Slice fmtX(so_Slice dst, so_int prec, so_byte fmt, bool neg, uint64_t mant, so_int exp, floatInfo* flt) {
    if (mant == 0) {
        exp = 0;
    }
    // Shift digits so leading 1 (if any) is at bit 1<<60.
    mant <<= 60 - flt->mantbits;
    for (; mant != 0 && (mant & ((uint64_t)1 << 60)) == 0;) {
        mant <<= 1;
        exp--;
    }
    // Round if requested.
    if (prec >= 0 && prec < 15) {
        so_uint shift = (so_uint)(prec * 4);
        uint64_t extra = ((mant << shift) & (((int64_t)1 << 60) - 1));
        mant >>= 60 - shift;
        if ((extra | (mant & 1)) > ((uint64_t)1 << 59)) {
            mant++;
        }
        mant <<= 60 - shift;
        if ((mant & ((uint64_t)1 << 61)) != 0) {
            // Wrapped around.
            mant >>= 1;
            exp++;
        }
    }
    so_String hex = lowerhex;
    if (fmt == 'X') {
        hex = upperhex;
    }
    // sign, 0x, leading digit
    if (neg) {
        dst = so_append(so_byte, dst, '-');
    }
    dst = so_append(so_byte, dst, '0', fmt, '0' + (so_byte)((mant >> 60) & 1));
    // .fraction
    // remove leading 0 or 1
    mant <<= 4;
    if (prec < 0 && mant != 0) {
        dst = so_append(so_byte, dst, '.');
        for (; mant != 0;) {
            dst = so_append(so_byte, dst, so_at(so_byte, hex, ((mant >> 60) & 15)));
            mant <<= 4;
        }
    } else if (prec > 0) {
        dst = so_append(so_byte, dst, '.');
        for (so_int i = 0; i < prec; i++) {
            dst = so_append(so_byte, dst, so_at(so_byte, hex, ((mant >> 60) & 15)));
            mant <<= 4;
        }
    }
    // p±
    so_byte ch = (so_byte)('P');
    if (fmt == lower(fmt)) {
        ch = 'p';
    }
    dst = so_append(so_byte, dst, ch);
    if (exp < 0) {
        ch = '-';
        exp = -exp;
    } else {
        ch = '+';
    }
    dst = so_append(so_byte, dst, ch);
    // dd or ddd or dddd
    if (exp < 100) {
        dst = so_append(so_byte, dst, (so_byte)(exp / 10) + '0', (so_byte)(exp % 10) + '0');
    } else if (exp < 1000) {
        dst = so_append(so_byte, dst, (so_byte)(exp / 100) + '0', (so_byte)((exp / 10) % 10) + '0', (so_byte)(exp % 10) + '0');
    } else {
        dst = so_append(so_byte, dst, (so_byte)(exp / 1000) + '0', (so_byte)(exp / 100) % 10 + '0', (so_byte)((exp / 10) % 10) + '0', (so_byte)(exp % 10) + '0');
    }
    return dst;
}

// -- ftoadbox.go --

// Binary to decimal conversion using the Dragonbox algorithm by Junekey Jeon.
//
// Fixed precision format is not supported by the Dragonbox algorithm
// so we continue to use Ryū-printf for this purpose.
// See https://github.com/jk-jeon/dragonbox/issues/38 for more details.
//
// For binary to decimal rounding, uses round to nearest, tie to even.
// For decimal to binary rounding, assumes round to nearest, tie to even.
//
// The original paper by Junekey Jeon can be found at:
// https://github.com/jk-jeon/dragonbox/blob/d5dc40ae6a3f1a4559cda816738df2d6255b4e24/other_files/Dragonbox.pdf
//
// The reference implementation in C++ by Junekey Jeon can be found at:
// https://github.com/jk-jeon/dragonbox/blob/6c7c925b571d54486b9ffae8d9d18a822801cbda/subproject/simple/include/simple_dragonbox.h
// dragonboxFtoa computes the decimal significand and exponent
// from the binary significand and exponent using the Dragonbox algorithm
// and formats the decimal floating point number in d.
static void dboxFtoa(decimalSlice* d, uint64_t mant, so_int exp, bool denorm, so_int bitSize) {
    if (bitSize == 32) {
        dboxFtoa32(d, (uint32_t)(mant), exp, denorm);
        return;
    }
    dboxFtoa64(d, mant, exp, denorm);
}

static void dboxFtoa64(decimalSlice* d, uint64_t mant, so_int exp, bool denorm) {
    if (mant == ((uint64_t)1 << float64MantBits) && !denorm) {
        // Algorithm 5.6 (page 24).
        so_int k0 = -mulLog10_2MinusLog10_4Over3(exp);
        phiBeta pb = dboxPow64(k0, exp);
        so_R_u64_u64 _res1 = dboxRange64(pb.phi, pb.beta);
        uint64_t xi = _res1.val;
        uint64_t zi = _res1.val2;
        if (exp != 2 && exp != 3) {
            xi++;
        }
        uint64_t q = zi / 10;
        if (xi <= q * 10) {
            so_R_u64_int _res2 = trimZeros(q);
            uint64_t q = _res2.val;
            so_int zeros = _res2.val2;
            dboxDigits(d, q, -k0 + 1 + zeros);
            return;
        }
        uint64_t yru = dboxRoundUp64(pb.phi, pb.beta);
        if (exp == -77 && yru % 2 != 0) {
            yru--;
        } else if (yru < xi) {
            yru++;
        }
        dboxDigits(d, yru, -k0);
        return;
    }
    // κ = 2 for float64 (section 5.1.3)
    const int64_t κ = 2;
    const int64_t p10κ = 100;
    const int64_t p10κ1 = p10κ * 10;
    // Algorithm 5.2 (page 15).
    so_int k0 = -mulLog10_2(exp);
    phiBeta pb = dboxPow64(κ + k0, exp);
    so_R_u64_bool _res3 = dboxMulPow64(((uint64_t)(mant * 2 + 1) << pb.beta), pb.phi);
    uint64_t zi = _res3.val;
    bool exact = _res3.val2;
    uint64_t s = zi / p10κ1;
    uint32_t r = (uint32_t)(zi % p10κ1);
    uint32_t δi = dboxDelta64(pb.phi, pb.beta);
    if (r < δi) {
        if (r != 0 || !exact || mant % 2 == 0) {
            so_R_u64_int _res4 = trimZeros(s);
            uint64_t s = _res4.val;
            so_int zeros = _res4.val2;
            dboxDigits(d, s, -k0 + 1 + zeros);
            return;
        }
        s--;
        r = p10κ * 10;
    } else if (r == δi) {
        so_R_bool_bool _res5 = dboxParity64((uint64_t)(mant * 2 - 1), pb.phi, pb.beta);
        bool parity = _res5.val;
        bool exact = _res5.val2;
        if (parity || (exact && mant % 2 == 0)) {
            so_R_u64_int _res6 = trimZeros(s);
            uint64_t s = _res6.val;
            so_int zeros = _res6.val2;
            dboxDigits(d, s, -k0 + 1 + zeros);
            return;
        }
    }
    // Algorithm 5.4 (page 18).
    uint32_t D = r + p10κ / 2 - δi / 2;
    uint32_t t = D / p10κ, rho = D % p10κ;
    uint64_t yru = 10 * s + (uint64_t)(t);
    if (rho == 0) {
        so_R_bool_bool _res7 = dboxParity64(mant * 2, pb.phi, pb.beta);
        bool parity = _res7.val;
        bool exact = _res7.val2;
        if (parity != ((D - p10κ / 2) % 2 != 0) || (exact && yru % 2 != 0)) {
            yru--;
        }
    }
    dboxDigits(d, yru, -k0);
}

// Almost identical to dragonboxFtoa64.
// This is kept as a separate copy to minimize runtime overhead.
static void dboxFtoa32(decimalSlice* d, uint32_t mant, so_int exp, bool denorm) {
    if (mant == ((uint32_t)1 << float32MantBits) && !denorm) {
        // Algorithm 5.6 (page 24).
        so_int k0 = -mulLog10_2MinusLog10_4Over3(exp);
        so_R_u64_int _res1 = dboxPow32(k0, exp);
        uint64_t phi = _res1.val;
        so_int beta = _res1.val2;
        so_R_u32_u32 _res2 = dboxRange32(phi, beta);
        uint32_t xi = _res2.val;
        uint32_t zi = _res2.val2;
        if (exp != 2 && exp != 3) {
            xi++;
        }
        uint32_t q = zi / 10;
        if (xi <= q * 10) {
            so_R_u64_int _res3 = trimZeros((uint64_t)(q));
            uint64_t q = _res3.val;
            so_int zeros = _res3.val2;
            dboxDigits(d, q, -k0 + 1 + zeros);
            return;
        }
        uint32_t yru = dboxRoundUp32(phi, beta);
        if (exp == -77 && yru % 2 != 0) {
            yru--;
        } else if (yru < xi) {
            yru++;
        }
        dboxDigits(d, (uint64_t)(yru), -k0);
        return;
    }
    // κ = 1 for float32 (section 5.1.3)
    const int64_t κ = 1;
    const int64_t p10κ = 10;
    const int64_t p10κ1 = p10κ * 10;
    // Algorithm 5.2 (page 15).
    so_int k0 = -mulLog10_2(exp);
    so_R_u64_int _res4 = dboxPow32(κ + k0, exp);
    uint64_t phi = _res4.val;
    so_int beta = _res4.val2;
    so_R_u32_bool _res5 = dboxMulPow32(((uint32_t)(mant * 2 + 1) << beta), phi);
    uint32_t zi = _res5.val;
    bool exact = _res5.val2;
    uint32_t s = zi / p10κ1, r = (uint32_t)(zi % p10κ1);
    uint32_t δi = dboxDelta32(phi, beta);
    if (r < δi) {
        if (r != 0 || !exact || mant % 2 == 0) {
            so_R_u64_int _res6 = trimZeros((uint64_t)(s));
            uint64_t s = _res6.val;
            so_int zeros = _res6.val2;
            dboxDigits(d, s, -k0 + 1 + zeros);
            return;
        }
        s--;
        r = p10κ * 10;
    } else if (r == δi) {
        so_R_bool_bool _res7 = dboxParity32((uint32_t)(mant * 2 - 1), phi, beta);
        bool parity = _res7.val;
        bool exact = _res7.val2;
        if (parity || (exact && mant % 2 == 0)) {
            so_R_u64_int _res8 = trimZeros((uint64_t)(s));
            uint64_t s = _res8.val;
            so_int zeros = _res8.val2;
            dboxDigits(d, s, -k0 + 1 + zeros);
            return;
        }
    }
    // Algorithm 5.4 (page 18).
    uint32_t D = r + p10κ / 2 - δi / 2;
    uint32_t t = D / p10κ, rho = D % p10κ;
    uint32_t yru = 10 * s + (uint32_t)(t);
    if (rho == 0) {
        so_R_bool_bool _res9 = dboxParity32(mant * 2, phi, beta);
        bool parity = _res9.val;
        bool exact = _res9.val2;
        if (parity != ((D - p10κ / 2) % 2 != 0) || (exact && yru % 2 != 0)) {
            yru--;
        }
    }
    dboxDigits(d, (uint64_t)(yru), -k0);
}

// dboxDigits emits decimal digits of mant in d for float64
// and adjusts the decimal point based on exp.
static void dboxDigits(decimalSlice* d, uint64_t mant, so_int exp) {
    so_int i = formatBase10(d->d, mant);
    d->d = so_slice(so_byte, d->d, i, d->d.len);
    d->nd = so_len(d->d);
    d->dp = d->nd + exp;
}

// uadd128 returns the full 128 bits of u + n.
static so_uint128 uadd128(so_uint128 u, uint64_t n) {
    uint64_t sum = (uint64_t)(u.lo + n);
    // Check if lo is wrapped around.
    if (sum < u.lo) {
        u.hi++;
    }
    u.lo = sum;
    return u;
}

// umul64 returns the full 64 bits of x * y.
static uint64_t umul64(uint32_t x, uint32_t y) {
    return (uint64_t)(x) * (uint64_t)(y);
}

// umul96Upper64 returns the upper 64 bits (out of 96 bits) of x * y.
static uint64_t umul96Upper64(uint32_t x, uint64_t y) {
    uint32_t yh = (uint32_t)(y >> 32);
    uint32_t yl = (uint32_t)(y);
    uint64_t xyh = umul64(x, yh);
    uint64_t xyl = umul64(x, yl);
    return xyh + (xyl >> 32);
}

// umul96Lower64 returns the lower 64 bits (out of 96 bits) of x * y.
static uint64_t umul96Lower64(uint32_t x, uint64_t y) {
    return (uint64_t)((uint64_t)(x) * y);
}

// umul128Upper64 returns the upper 64 bits (out of 128 bits) of x * y.
static uint64_t umul128Upper64(uint64_t x, uint64_t y) {
    uint32_t a = (uint32_t)(x >> 32);
    uint32_t b = (uint32_t)(x);
    uint32_t c = (uint32_t)(y >> 32);
    uint32_t d = (uint32_t)(y);
    uint64_t ac = umul64(a, c);
    uint64_t bc = umul64(b, c);
    uint64_t ad = umul64(a, d);
    uint64_t bd = umul64(b, d);
    uint64_t intermediate = (bd >> 32) + (uint64_t)((uint32_t)(ad)) + (uint64_t)((uint32_t)(bc));
    return ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32);
}

// umul192Upper128 returns the upper 128 bits (out of 192 bits) of x * y.
static so_uint128 umul192Upper128(uint64_t x, so_uint128 y) {
    so_uint128 r = umul128(x, y.hi);
    uint64_t t = umul128Upper64(x, y.lo);
    return uadd128(r, t);
}

// umul192Lower128 returns the lower 128 bits (out of 192 bits) of x * y.
static so_uint128 umul192Lower128(uint64_t x, so_uint128 y) {
    uint64_t high = x * y.hi;
    so_uint128 highLow = umul128(x, y.lo);
    return (so_uint128){(uint64_t)(high + highLow.hi), highLow.lo};
}

// dboxMulPow64 computes x^(i), y^(i), z^(i)
// from the precomputed value of φ̃k for float64
// and also checks if x^(f), y^(f), z^(f) == 0 (section 5.2.1).
static so_R_u64_bool dboxMulPow64(uint64_t u, so_uint128 phi) {
    so_uint128 r = umul192Upper128(u, phi);
    uint64_t intPart = r.hi;
    bool isInt = r.lo == 0;
    return (so_R_u64_bool){.val = intPart, .val2 = isInt};
}

// dboxMulPow32 computes x^(i), y^(i), z^(i)
// from the precomputed value of φ̃k for float32
// and also checks if x^(f), y^(f), z^(f) == 0 (section 5.2.1).
static so_R_u32_bool dboxMulPow32(uint32_t u, uint64_t phi) {
    uint64_t r = umul96Upper64(u, phi);
    uint32_t intPart = (uint32_t)(r >> 32);
    bool isInt = (uint32_t)(r) == 0;
    return (so_R_u32_bool){.val = intPart, .val2 = isInt};
}

// dboxParity64 computes only the parity of x^(i), y^(i), z^(i)
// from the precomputed value of φ̃k for float64
// and also checks if x^(f), y^(f), z^(f) = 0 (section 5.2.1).
static so_R_bool_bool dboxParity64(uint64_t mant2, so_uint128 phi, so_int beta) {
    so_uint128 r = umul192Lower128(mant2, phi);
    bool parity = ((r.hi >> (64 - beta)) & 1) != 0;
    bool isInt = (((uint64_t)(r.hi << beta)) | (r.lo >> (64 - beta))) == 0;
    return (so_R_bool_bool){.val = parity, .val2 = isInt};
}

// dboxParity32 computes only the parity of x^(i), y^(i), z^(i)
// from the precomputed value of φ̃k for float32
// and also checks if x^(f), y^(f), z^(f) = 0 (section 5.2.1).
static so_R_bool_bool dboxParity32(uint32_t mant2, uint64_t phi, so_int beta) {
    uint64_t r = umul96Lower64(mant2, phi);
    bool parity = ((r >> (64 - beta)) & 1) != 0;
    bool isInt = (uint32_t)(r >> (32 - beta)) == 0;
    return (so_R_bool_bool){.val = parity, .val2 = isInt};
}

// dboxDelta64 returns δ^(i) from the precomputed value of φ̃k for float64.
static uint32_t dboxDelta64(so_uint128 phi, so_int beta) {
    return (uint32_t)(phi.hi >> (64 - 1 - beta));
}

// dboxDelta32 returns δ^(i) from the precomputed value of φ̃k for float32.
static uint32_t dboxDelta32(uint64_t phi, so_int beta) {
    return (uint32_t)(phi >> (64 - 1 - beta));
}

// mulLog10_2MinusLog10_4Over3 computes
// ⌊e*log10(2)-log10(4/3)⌋ = ⌊log10(2^e)-log10(4/3)⌋ (section 6.3).
static so_int mulLog10_2MinusLog10_4Over3(so_int e) {
    // e should be in the range [-2985, 2936].
    return ((e * 631305 - 261663) >> 21);
}

// dboxRange64 returns the left and right float64 endpoints.
static so_R_u64_u64 dboxRange64(so_uint128 phi, so_int beta) {
    uint64_t left = ((phi.hi - (phi.hi >> (floatMantBits64 + 2))) >> (64 - floatMantBits64 - 1 - beta));
    uint64_t right = ((phi.hi + (phi.hi >> (floatMantBits64 + 1))) >> (64 - floatMantBits64 - 1 - beta));
    return (so_R_u64_u64){.val = left, .val2 = right};
}

// dboxRange32 returns the left and right float32 endpoints.
static so_R_u32_u32 dboxRange32(uint64_t phi, so_int beta) {
    uint32_t left = (uint32_t)((phi - (phi >> (floatMantBits32 + 2))) >> (64 - floatMantBits32 - 1 - beta));
    uint32_t right = (uint32_t)((phi + (phi >> (floatMantBits32 + 1))) >> (64 - floatMantBits32 - 1 - beta));
    return (so_R_u32_u32){.val = left, .val2 = right};
}

// dboxRoundUp64 computes the round up of y (i.e., y^(ru)).
static uint64_t dboxRoundUp64(so_uint128 phi, so_int beta) {
    return ((phi.hi >> (128 / 2 - floatMantBits64 - 2 - beta)) + 1) / 2;
}

// dboxRoundUp32 computes the round up of y (i.e., y^(ru)).
static uint32_t dboxRoundUp32(uint64_t phi, so_int beta) {
    return (uint32_t)((phi >> (64 - floatMantBits32 - 2 - beta)) + 1) / 2;
}

// dboxPow64 gets the precomputed value of φ̃̃k for float64.
static phiBeta dboxPow64(so_int k, so_int e) {
    pow10Result powr = intPow10(k);
    so_uint128 phi = powr.mant;
    so_int e1 = powr.exp;
    if (k < 0 || k > 55) {
        phi.lo++;
    }
    so_int beta = e + e1 - 1;
    return (phiBeta){phi, beta};
}

// dboxPow32 gets the precomputed value of φ̃̃k for float32.
static so_R_u64_int dboxPow32(so_int k, so_int e) {
    pow10Result powr = intPow10(k);
    so_uint128 m = powr.mant;
    so_int e1 = powr.exp;
    if (k < 0 || k > 27) {
        m.hi++;
    }
    so_int exp = e + e1 - 1;
    return (so_R_u64_int){.val = m.hi, .val2 = exp};
}

// -- ftoafixed.go --

// fixedFtoa formats a number of decimal digits of mant*(2^exp) into d,
// where mant > 0 and 1 ≤ digits ≤ 18.
// If fmt == 'f', digits is a conservative overestimate, and the final
// number of digits is prec past the decimal point.
static void fixedFtoa(decimalSlice* d, uint64_t mant, so_int exp, so_int digits, so_int prec, so_byte fmt) {
    // The strategy here is to multiply (mant * 2^exp) by a power of 10
    // to make the resulting integer be the number of digits we want.
    //
    // Adams proved in the Ryu paper that 128-bit precision in the
    // power-of-10 constant is sufficient to produce correctly
    // rounded output for all float64s, up to 18 digits.
    // https://dl.acm.org/doi/10.1145/3192366.3192369
    //
    // TODO(rsc): The paper is not focused on, nor terribly clear about,
    // this fact in this context, and the proof seems too complicated.
    // Post a shorter, more direct proof and link to it here.
    if (digits > 18) {
        so_panic("fixedFtoa called with digits > 18");
    }
    // Shift mantissa to have 64 bits,
    // so that the 192-bit product below will
    // have at least 63 bits in its top word.
    so_int b = 64 - bits_Len64(mant);
    mant <<= b;
    exp -= b;
    // We have f = mant * 2^exp ≥ 2^(63+exp)
    // and we want to multiply it by some 10^p
    // to make it have the number of digits plus one rounding bit:
    //
    //	2 * 10^(digits-1) ≤ f * 10^p < ~2 * 10^digits
    //
    // The lower bound is required, but the upper bound is approximate:
    // we must not have too few digits, but we can round away extra ones.
    //
    //	f * 10^p ≥ 2 * 10^(digits-1)
    //	10^p ≥ 2 * 10^(digits-1) / f                         [dividing by f]
    //	p ≥ (log₁₀ 2) + (digits-1) - log₁₀ f                 [taking log₁₀]
    //	p ≥ (log₁₀ 2) + (digits-1) - log₁₀ (mant * 2^exp)    [expanding f]
    //	p ≥ (log₁₀ 2) + (digits-1) - (log₁₀ 2) * (64 + exp)  [mant < 2⁶⁴]
    //	p ≥ (digits - 1) - (log₁₀ 2) * (63 + exp)            [refactoring]
    //
    // Once we have p, we can compute the scaled value:
    //
    //	dm * 2^de = mant * 2^exp * 10^p
    //	          = mant * 2^exp * pow/2^128 * 2^exp2.
    //	          = (mant * pow/2^128) * 2^(exp+exp2).
    so_int p = (digits - 1) - mulLog10_2(63 + exp);
    pow10Result powr = intPow10(p);
    so_uint128 pow = powr.mant;
    so_int exp2 = powr.exp;
    if (!powr.ok) {
        // This never happens due to the range of float32/float64 exponent
        so_panic("fixedFtoa: intPow10 out of range");
    }
    if (-22 <= p && p < 0) {
        // Special case: Let q=-p. q is in [1,22]. We are dividing by 10^q
        // and the mantissa may be a multiple of 5^q (5^22 < 2^53),
        // in which case the division must be computed exactly and
        // recorded as exact for correct rounding. Our normal computation is:
        //
        //	dm = floor(mant * floor(10^p * 2^s))
        //
        // for some scaling shift s. To make this an exact division,
        // it suffices to change the inner floor to a ceil:
        //
        //	dm = floor(mant * ceil(10^p * 2^s))
        //
        // In the range of values we are using, the floor and ceil
        // cancel each other out and the high 64 bits of the product
        // come out exactly right.
        // (This is the same trick compilers use for division by constants.
        // See Hacker's Delight, 2nd ed., Chapter 10.)
        pow.lo++;
    }
    umul192Result umulr = umul192(mant, pow);
    uint64_t dm = umulr.hi, lo1 = umulr.mid, lo0 = umulr.lo;
    so_int de = exp + exp2;
    // Check whether any bits have been truncated from dm.
    // If so, set dt != 0. If not, leave dt == 0 (meaning dm is exact).
    so_uint dt = 0;
    if (0 <= p && p <= 55) {
        // Small positive powers of 10 (up to 10⁵⁵) can be represented
        // precisely in a 128-bit mantissa (5⁵⁵ ≤ 2¹²⁸), so the only truncation
        // comes from discarding the low bits of the 192-bit product.
        //
        // TODO(rsc): The new proof mentioned above should also
        // prove that we can't have lo1 == 0 and lo0 != 0.
        // After proving that, drop computation and use of lo0 here.
        dt = bool2uint((lo1 | lo0) != 0);
    } else if (-22 <= p && p < 0 && divisiblePow5(mant, -p)) {
        // If the original mantissa was a multiple of 5^p,
        // the result is exact. (See comment above for pow.Lo++.)
        dt = 0;
    } else {
        // Most powers of 10 use a truncated constant,
        // meaning the result is also truncated.
        dt = 1;
    }
    // The value we want to format is dm * 2^de, where de < 0.
    // Multply by 2^de by shifting, but leave one extra bit for rounding.
    // After the shift, the "integer part" of dm is dm>>1,
    // the "rounding bit" (the first fractional bit) is dm&1,
    // and the "truncated bit" (have any bits been discarded?) is dt.
    so_int shift = -de - 1;
    dt |= bool2uint((dm & (((uint64_t)1 << shift) - 1)) != 0);
    dm >>= shift;
    // Set decimal point in eventual formatted digits,
    // so we can update it as we adjust the digits.
    d->dp = digits - p;
    // Trim excess digit if any, updating truncation and decimal point.
    // The << 1 is leaving room for the rounding bit.
    uint64_t max = (uint64pow10[digits] << 1);
    if (dm >= max) {
        so_uint r = (so_uint)(dm % 10);
        dm = dm / 10;
        dt |= bool2uint(r != 0);
        d->dp++;
    }
    // If this is %.*f we may have overestimated the digits needed.
    // Now that we know where the decimal point is,
    // trim to the actual number of digits, which is d.dp+prec.
    if (fmt == 'f' && digits != d->dp + prec) {
        for (; digits > d->dp + prec;) {
            so_uint r = (so_uint)(dm % 10);
            dm = dm / 10;
            dt |= bool2uint(r != 0);
            digits--;
        }
        // Dropping those digits can create a new leftmost
        // non-zero digit, like if we are formatting %.1f and
        // convert 0.09 -> 0.1. Detect and adjust for that.
        if (digits <= 0) {
            digits = 1;
            d->dp++;
        }
        max = (uint64pow10[digits] << 1);
    }
    // Round and shift away rounding bit.
    // We want to round up when
    // (a) the fractional part is > 0.5 (dm&1 != 0 and dt == 1)
    // (b) or the fractional part is ≥ 0.5 and the integer part is odd
    //     (dm&1 != 0 and dm&2 != 0).
    // The bitwise expression encodes that logic.
    dm += (uint64_t)(((so_uint)(dm) & (dt | ((so_uint)(dm) >> 1))) & 1);
    dm >>= 1;
    if (dm == (max >> 1)) {
        // 999... rolled over to 1000...
        dm = uint64pow10[digits - 1];
        d->dp++;
    }
    // Format digits into d.
    if (dm != 0) {
        if (formatBase10(so_slice(so_byte, d->d, 0, digits), dm) != 0) {
            so_panic("formatBase10");
        }
        d->nd = digits;
        for (; so_at(so_byte, d->d, d->nd - 1) == '0';) {
            d->nd--;
        }
    }
}

// -- itoa.go --

// FormatUint returns the string representation of i in the given base,
// for 2 <= base <= 36. The result uses the lower-case letters 'a' to 'z'
// for digit values >= 10.
// dst must have enough length to hold the result (see MaxUintBase*Len constants).
so_String strconv_FormatUint(so_Slice dst, uint64_t i, so_int base) {
    so_Slice buf = so_slice(so_byte, dst, 0, 0);
    if (base == 10) {
        if (i < nSmalls) {
            so_String s = small((so_int)(i));
            buf = so_extend(so_byte, buf, so_string_bytes(s));
            return so_bytes_string(buf);
        }
        so_byte a[24] = {0};
        so_int j = formatBase10(so_array_slice(so_byte, a, 0, 24, 24), i);
        buf = so_extend(so_byte, buf, (so_array_slice(so_byte, a, j, 24, 24)));
        return so_bytes_string(buf);
    }
    buf = formatBits(buf, i, base, false);
    return so_bytes_string(buf);
}

// FormatInt returns the string representation of i in the given base,
// for 2 <= base <= 36. The result uses the lower-case letters 'a' to 'z'
// for digit values >= 10.
// dst must have enough length to hold the result (see MaxIntBase*Len constants).
so_String strconv_FormatInt(so_Slice dst, int64_t i, so_int base) {
    so_Slice buf = so_slice(so_byte, dst, 0, 0);
    if (base == 10) {
        if (0 <= i && i < nSmalls) {
            so_String s = small((so_int)(i));
            buf = so_extend(so_byte, buf, so_string_bytes(s));
            return so_bytes_string(buf);
        }
        so_byte a[24] = {0};
        uint64_t u = (uint64_t)(i);
        if (i < 0) {
            u = -u;
        }
        so_int j = formatBase10(so_array_slice(so_byte, a, 0, 24, 24), u);
        if (i < 0) {
            j--;
            a[j] = '-';
        }
        buf = so_extend(so_byte, buf, (so_array_slice(so_byte, a, j, 24, 24)));
        return so_bytes_string(buf);
    }
    buf = formatBits(buf, (uint64_t)(i), base, i < 0);
    return so_bytes_string(buf);
}

// Itoa is equivalent to [FormatInt](int64(i), 10).
// dst length must be at least [MaxIntBase10Len] bytes.
so_String strconv_Itoa(so_Slice dst, so_int i) {
    return strconv_FormatInt(dst, (int64_t)(i), 10);
}

// AppendInt appends the string form of the integer i,
// as generated by [FormatInt], to dst and returns the extended buffer.
// dst must have enough capacity to hold the result (see MaxIntBase*Len constants).
so_Slice strconv_AppendInt(so_Slice dst, int64_t i, so_int base) {
    uint64_t u = (uint64_t)(i);
    if (i < 0) {
        dst = so_append(so_byte, dst, '-');
        u = -u;
    }
    return strconv_AppendUint(dst, u, base);
}

// AppendUint appends the string form of the unsigned integer i,
// as generated by [FormatUint], to dst and returns the extended buffer.
// dst must have enough capacity to hold the result (see MaxUintBase*Len constants).
so_Slice strconv_AppendUint(so_Slice dst, uint64_t i, so_int base) {
    if (base == 10) {
        if (i < nSmalls) {
            return so_extend(so_byte, dst, so_string_bytes(small((so_int)(i))));
        }
        so_byte a[24] = {0};
        so_int j = formatBase10(so_array_slice(so_byte, a, 0, 24, 24), i);
        return so_extend(so_byte, dst, (so_array_slice(so_byte, a, j, 24, 24)));
    }
    dst = formatBits(dst, i, base, false);
    return dst;
}

// formatBits computes the string representation of u in the given base.
// Appends the string to dst and returns the resulting byte slice.
// dst must have enough capacity to hold the result.
// If neg is set, u is treated as negative int64 value.
// The caller is expected to have handled base 10 separately for speed.
static so_Slice formatBits(so_Slice dst, uint64_t u, so_int base, bool neg) {
    // 2 <= base && base <= len(digits)
    if (base < 2 || base == 10 || base > so_len(digits)) {
        so_panic("strconv: illegal AppendInt/FormatInt base");
    }
    // +1 for sign of 64bit value in base 2
    so_byte a[65] = {0};
    so_int i = 65;
    if (neg) {
        u = -u;
    }
    // convert bits
    // We use uint values where we can because those will
    // fit into a single register even on a 32bit machine.
    if (isPowerOfTwo(base)) {
        // Use shifts and masks instead of / and %.
        so_uint shift = (so_uint)(bits_TrailingZeros((so_uint)(base)));
        uint64_t b = (uint64_t)(base);
        // == 1<<shift - 1
        so_uint m = (so_uint)(base) - 1;
        for (; u >= b;) {
            i--;
            a[i] = so_at(so_byte, digits, ((so_uint)(u) & m));
            u >>= shift;
        }
        // u < base
        i--;
        a[i] = so_at(so_byte, digits, (so_uint)(u));
    } else {
        // general case
        uint64_t b = (uint64_t)(base);
        for (; u >= b;) {
            i--;
            // Avoid using r = a%b in addition to q = a/b
            // since 64bit division and modulo operations
            // are calculated by runtime functions on 32bit machines.
            uint64_t q = u / b;
            a[i] = so_at(so_byte, digits, (so_uint)(u - q * b));
            u = q;
        }
        // u < base
        i--;
        a[i] = so_at(so_byte, digits, (so_uint)(u));
    }
    // add sign, if any
    if (neg) {
        i--;
        a[i] = '-';
    }
    so_Slice d = so_extend(so_byte, dst, (so_array_slice(so_byte, a, i, 65, 65)));
    return d;
}

static bool isPowerOfTwo(so_int x) {
    return (x & (x - 1)) == 0;
}

// small returns the string for an i with 0 <= i < nSmalls.
static so_String small(so_int i) {
    if (i < 10) {
        return so_string_slice(digits, i, i + 1);
    }
    return so_string_slice(smalls, i * 2, i * 2 + 2);
}

// formatBase10 formats the decimal representation of u into the tail of a
// and returns the offset of the first byte written to a. That is, after
//
//	i := formatBase10(a, u)
//
// the decimal representation is in a[i:].
static so_int formatBase10(so_Slice a, uint64_t u) {
    // Split into 9-digit chunks that fit in uint32s
    // and convert each chunk using uint32 math instead of uint64 math.
    // The obvious way to write the outer loop is "for u >= 1000000000", but most numbers are small,
    // so the setup for the comparison u >= 1000000000 is usually pure overhead.
    // Instead, we approximate it by u>>29 != 0, which is usually faster and good enough.
    so_int i = so_len(a);
    for (; (host64bit && (u >> 29) != 0) || (!host64bit && (((uint32_t)(u) >> 29) | (uint32_t)(u >> 32)) != 0);) {
        uint32_t lo = (uint32_t)(u % 1000000000);
        u = u / 1000000000;
        // Convert 9 digits.
        for (so_int _i = 0; _i < 4; _i++) {
            uint32_t dd = (lo % 100) * 2;
            lo = lo / 100;
            i -= 2;
            so_at(so_byte, a, i + 0) = so_at(so_byte, smalls, dd + 0);
            so_at(so_byte, a, i + 1) = so_at(so_byte, smalls, dd + 1);
        }
        i--;
        so_at(so_byte, a, i) = so_at(so_byte, smalls, lo * 2 + 1);
        // If we'd been using u >= 1000000000 then we would be guaranteed that u/1000000000 > 0,
        // but since we used u>>29 != 0, u/1000000000 might be 0, so we might be done.
        // (If u is now 0, then at the start we had 2²⁹ ≤ u < 10⁹, so it was still correct
        // to write 9 digits; we have not accidentally written any leading zeros.)
        if (u == 0) {
            return i;
        }
    }
    // Convert final chunk, at most 8 digits.
    uint32_t lo = (uint32_t)(u);
    for (; lo >= 100;) {
        uint32_t dd = (lo % 100) * 2;
        lo = lo / 100;
        i -= 2;
        so_at(so_byte, a, i + 0) = so_at(so_byte, smalls, dd + 0);
        so_at(so_byte, a, i + 1) = so_at(so_byte, smalls, dd + 1);
    }
    i--;
    uint32_t dd = lo * 2;
    so_at(so_byte, a, i) = so_at(so_byte, smalls, dd + 1);
    if (lo >= 10) {
        i--;
        so_at(so_byte, a, i) = so_at(so_byte, smalls, dd + 0);
    }
    return i;
}

// -- math.go --

// umul128 returns the 128-bit product x*y.
static so_uint128 umul128(uint64_t x, uint64_t y) {
    so_R_u64_u64 _res1 = bits_Mul64(x, y);
    uint64_t hi = _res1.val;
    uint64_t lo = _res1.val2;
    return (so_uint128){hi, lo};
}

// umul192 returns the 192-bit product x*y in three uint64s.
static umul192Result umul192(uint64_t x, so_uint128 y) {
    so_R_u64_u64 _res1 = bits_Mul64(x, y.lo);
    uint64_t mid1 = _res1.val;
    uint64_t lo = _res1.val2;
    so_R_u64_u64 _res2 = bits_Mul64(x, y.hi);
    uint64_t hi = _res2.val;
    uint64_t mid2 = _res2.val2;
    so_R_u64_u64 _res3 = bits_Add64(mid1, mid2, 0);
    uint64_t mid = _res3.val;
    uint64_t carry = _res3.val2;
    return (umul192Result){hi + carry, mid, lo};
}

// intPow10 returns the 128-bit mantissa and binary exponent of 10**e.
// That is, 10^e = mant/2^128 * 2**exp.
// If e is out of range, intPow10 returns ok=false.
static pow10Result intPow10(so_int e) {
    if (e < pow10Min || e > pow10Max) {
        return (pow10Result){};
    }
    so_uint128 mant = pow10Tab[e - pow10Min];
    so_int exp = 1 + mulLog2_10(e);
    return (pow10Result){mant, exp, true};
}

// mulLog10_2 returns math.Floor(x * log(2)/log(10)) for an integer x in
// the range -1600 <= x && x <= +1600.
//
// The range restriction lets us work in faster integer arithmetic instead of
// slower floating point arithmetic. Correctness is verified by unit tests.
static so_int mulLog10_2(so_int x) {
    // log(2)/log(10) ≈ 0.30102999566 ≈ 78913 / 2^18
    return ((x * 78913) >> 18);
}

// mulLog2_10 returns math.Floor(x * log(10)/log(2)) for an integer x in
// the range -500 <= x && x <= +500.
//
// The range restriction lets us work in faster integer arithmetic instead of
// slower floating point arithmetic. Correctness is verified by unit tests.
static so_int mulLog2_10(so_int x) {
    // log(10)/log(2) ≈ 3.32192809489 ≈ 108853 / 2^15
    return ((x * 108853) >> 15);
}

static so_uint bool2uint(bool b) {
    if (b) {
        return 1;
    }
    return 0;
}

// Exact Division and Remainder Checking
//
// An exact division x/c (exact means x%c == 0)
// can be implemented by x*m where m is the multiplicative inverse of c (m*c == 1).
//
// Since c is also the multiplicative inverse of m, x*m is lossless,
// and all the exact multiples of c map to all of [0, maxUint64/c].
// The non-multiples are forced to map to larger values.
// This also gives a quick test for whether x is an exact multiple of c:
// compute the exact division and check whether it's at most maxUint64/c:
//	x%c == 0 => x*m <= maxUint64/c.
//
// Only odd c have multiplicative inverses mod powers of two.
// To do an exact divide x / (c<<s) we can use (x/c)>>s instead.
// And to check for remainder, we need to check that those low s
// bits are all zero before we shift them away. We can merge that
// with the <= for the exact odd remainder check by rotating the
// shifted bits into the high part instead:
// 	x%(c<<s) == 0 => bits.RotateLeft64(x*m, -s) <= maxUint64/c.
//
// The compiler does this transformation automatically in general,
// but we apply it here by hand in a few ways that the compiler can't help with.
//
// For a more detailed explanation, see
// Henry S. Warren, Jr., Hacker's Delight, 2nd ed., sections 10-16 and 10-17.
// divisiblePow5 reports whether x is divisible by 5^p.
// It returns false for p not in [1, 22],
// because we only care about float64 mantissas, and 5^23 > 2^53.
static bool divisiblePow5(uint64_t x, so_int p) {
    return 1 <= p && p <= 22 && x * div5Tab[p - 1][0] <= div5Tab[p - 1][1];
}

// trimZeros trims trailing zeros from x.
// It finds the largest p such that x % 10^p == 0
// and then returns x / 10^p, p.
//
// This is here for reference and tested, because it is an optimization
// used by other ftoa algorithms, but in our implementations it has
// never been benchmarked to be faster than trimming zeros after
// formatting into decimal bytes.
static so_R_u64_int trimZeros(uint64_t x) {
    const uint64_t div1e8m = 0xc767074b22e90e21;
    const uint64_t div1e8le = maxUint64 / 100000000;
    const uint64_t div1e4m = 0xd288ce703afb7e91;
    const uint64_t div1e4le = maxUint64 / 10000;
    const uint64_t div1e2m = 0x8f5c28f5c28f5c29;
    const uint64_t div1e2le = maxUint64 / 100;
    const uint64_t div1e1m = 0xcccccccccccccccd;
    const uint64_t div1e1le = maxUint64 / 10;
    // Cut 8 zeros, then 4, then 2, then 1.
    so_int p = 0;
    for (uint64_t d = bits_RotateLeft64(x * div1e8m, -8); d <= div1e8le; d = bits_RotateLeft64(x * div1e8m, -8)) {
        x = d;
        p += 8;
    }
    {
        uint64_t d = bits_RotateLeft64(x * div1e4m, -4);
        if (d <= div1e4le) {
            x = d;
            p += 4;
        }
    }
    {
        uint64_t d = bits_RotateLeft64(x * div1e2m, -2);
        if (d <= div1e2le) {
            x = d;
            p += 2;
        }
    }
    {
        uint64_t d = bits_RotateLeft64(x * div1e1m, -1);
        if (d <= div1e1le) {
            x = d;
            p += 1;
        }
    }
    return (so_R_u64_int){.val = x, .val2 = p};
}

// -- pow10tab.go --
