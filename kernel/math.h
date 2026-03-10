#pragma once
#include "def.h"

#ifndef _MATH_H_
#define _MATH_H_

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Constants
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define DBL_DECIMAL_DIG  17                      // # of decimal digits of rounding precision
#define DBL_DIG          15                      // # of decimal digits of precision
#define DBL_EPSILON      2.2204460492503131e-016 // smallest such that 1.0+DBL_EPSILON != 1.0
#define DBL_HAS_SUBNORM  1                       // type does support subnormal numbers
#define DBL_MANT_DIG     53                      // # of bits in mantissa
#define DBL_MAX          1.7976931348623158e+308 // max value
#define DBL_MAX_10_EXP   308                     // max decimal exponent
#define DBL_MAX_EXP      1024                    // max binary exponent
#define DBL_MIN          2.2250738585072014e-308 // min positive value
#define DBL_MIN_10_EXP   (-307)                  // min decimal exponent
#define DBL_MIN_EXP      (-1021)                 // min binary exponent
#define _DBL_RADIX       2                       // exponent radix
#define DBL_TRUE_MIN     4.9406564584124654e-324 // min positive value

#define FLT_DECIMAL_DIG  9                       // # of decimal digits of rounding precision
#define FLT_DIG          6                       // # of decimal digits of precision
#define FLT_EPSILON      1.192092896e-07F        // smallest such that 1.0+FLT_EPSILON != 1.0
#define FLT_HAS_SUBNORM  1                       // type does support subnormal numbers
#define FLT_GUARD        0
#define FLT_MANT_DIG     24                      // # of bits in mantissa
#define FLT_MAX          3.402823466e+38F        // max value
#define FLT_MAX_10_EXP   38                      // max decimal exponent
#define FLT_MAX_EXP      128                     // max binary exponent
#define FLT_MIN          1.175494351e-38F        // min normalized positive value
#define FLT_MIN_10_EXP   (-37)                   // min decimal exponent
#define FLT_MIN_EXP      (-125)                  // min binary exponent
#define FLT_NORMALIZE    0
#define FLT_RADIX        2                       // exponent radix
#define FLT_TRUE_MIN     1.401298464e-45F        // min positive value

#define LDBL_DIG         DBL_DIG                 // # of decimal digits of precision
#define LDBL_EPSILON     DBL_EPSILON             // smallest such that 1.0+LDBL_EPSILON != 1.0
#define LDBL_HAS_SUBNORM DBL_HAS_SUBNORM         // type does support subnormal numbers
#define LDBL_MANT_DIG    DBL_MANT_DIG            // # of bits in mantissa
#define LDBL_MAX         DBL_MAX                 // max value
#define LDBL_MAX_10_EXP  DBL_MAX_10_EXP          // max decimal exponent
#define LDBL_MAX_EXP     DBL_MAX_EXP             // max binary exponent
#define LDBL_MIN         DBL_MIN                 // min normalized positive value
#define LDBL_MIN_10_EXP  DBL_MIN_10_EXP          // min decimal exponent
#define LDBL_MIN_EXP     DBL_MIN_EXP             // min binary exponent
#define _LDBL_RADIX      _DBL_RADIX              // exponent radix
#define LDBL_TRUE_MIN    DBL_TRUE_MIN            // min positive value

#define DECIMAL_DIG      DBL_DECIMAL_DIG

// 1. 正无穷大 (+inf) : 符号位=0, 指数全1, 尾数全0
#define FLOAT_POS_INF_BITS  0x7F800000  // 0 11111111 00000000000000000000000
#define FLOAT_POS_INF       (*((float*)&FLOAT_POS_INF_BITS))

// 2. 负无穷大 (-inf) : 符号位=1, 指数全1, 尾数全0
#define FLOAT_NEG_INF_BITS  0xFF800000  // 1 11111111 00000000000000000000000
#define FLOAT_NEG_INF       (*((float*)&FLOAT_NEG_INF_BITS))

// 3. 正无穷小（正的最小非规约数，最接近0的正数）
#define FLOAT_POS_DENORM_MIN_BITS 0x00000001  // 0 00000000 00000000000000000000001
#define FLOAT_POS_DENORM_MIN      (*((float*)&FLOAT_POS_DENORM_MIN_BITS))

// 4. 正的最小规约数（最小的正规化正数）
#define FLOAT_POS_NORM_MIN_BITS   0x00800000  // 0 00000001 00000000000000000000000
#define FLOAT_POS_NORM_MIN        (*((float*)&FLOAT_POS_NORM_MIN_BITS))

// 5. 静默 NaN (Quiet NaN) : 指数全1, 尾数最高位为1
#define FLOAT_QNAN_BITS      0x7FC00000  // 0 11111111 10000000000000000000000
#define FLOAT_QNAN           (*((float*)&FLOAT_QNAN_BITS))

// 6. 信令 NaN (Signaling NaN) : 指数全1, 尾数最高位为0，但至少有一个1
#define FLOAT_SNAN_BITS      0x7F800001  // 0 11111111 00000000000000000000001
#define FLOAT_SNAN           (*((float*)&FLOAT_SNAN_BITS))

// ==================== DOUBLE (64位) 的定义 ====================

// 1. 正无穷大 (+inf) : 符号位=0, 指数全1(11位), 尾数全0(52位)
#define DOUBLE_POS_INF_BITS  0x7FF0000000000000LL  // 0 11111111111 000...000
#define DOUBLE_POS_INF       (*((double*)&DOUBLE_POS_INF_BITS))

// 2. 负无穷大 (-inf) : 符号位=1, 指数全1, 尾数全0
#define DOUBLE_NEG_INF_BITS  0xFFF0000000000000LL  // 1 11111111111 000...000
#define DOUBLE_NEG_INF       (*((double*)&DOUBLE_NEG_INF_BITS))

// 3. 正无穷小（最小的非规约正数）
#define DOUBLE_POS_DENORM_MIN_BITS 0x0000000000000001LL  // 最小的正非规约数
#define DOUBLE_POS_DENORM_MIN      (*((double*)&DOUBLE_POS_DENORM_MIN_BITS))

// 4. 正的最小规约数
#define DOUBLE_POS_NORM_MIN_BITS   0x0010000000000000LL  // 指数为1的最小规约数
#define DOUBLE_POS_NORM_MIN        (*((double*)&DOUBLE_POS_NORM_MIN_BITS))

// 5. 静默 NaN (Quiet NaN)
#define DOUBLE_QNAN_BITS      0x7FF8000000000000LL  // 指数全1, 尾数最高位为1
#define DOUBLE_QNAN           (*((double*)&DOUBLE_QNAN_BITS))

// 6. 信令 NaN (Signaling NaN)
#define DOUBLE_SNAN_BITS      0x7FF0000000000001LL  // 指数全1, 尾数非0且最高位为0
#define DOUBLE_SNAN           (*((double*)&DOUBLE_SNAN_BITS))

// ==================== 辅助宏 ====================

// 检查浮点数类型的特殊值
#define IS_FLOAT_NAN(f)     ((f) != (f))  // NaN 不等于自身
#define IS_FLOAT_INF(f)     (isinf(f))    // 使用 math.h 的 isinf
#define IS_FLOAT_FINITE(f)  (isfinite(f)) // 使用 math.h 的 isfinite

// 获取浮点数的二进制表示
#define FLOAT_TO_BITS(f)    (*((uint32_t*)&(f)))
#define DOUBLE_TO_BITS(d)   (*((uint64_t*)&(d)))

#define SQRT2 					(1.414213562373095145474621858739)

#define PI 						(3.141592653589793238462643)
#define SL_2PI					(PI*2)
#define SL_PI					PI
#define SL_PI_DIV_2				(PI/2)

#define E 						2.7182818284590452353602874713527
#define LN2						0.69314718055994530941723212145818   // ln(2)
#define LN10					2.3025850929940456840179914546844  // ln(10)




#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) DWORD __sqrtInteger(DWORD i);
extern "C"  __declspec(dllexport) int __abs(int x);
extern "C"  __declspec(dllexport) double __pown(double x, int n);
extern "C"  __declspec(dllexport) double __cos(double x);
extern "C"  __declspec(dllexport) double __sin(double x);
extern "C"  __declspec(dllexport) double __pow(double a, int b);
extern "C"  __declspec(dllexport) double __sqrt(double x);

extern "C"  __declspec(dllexport) double __acos(double x);
extern "C"  __declspec(dllexport) double __asin(double x);
extern "C"  __declspec(dllexport) double __atan(double y, double x, int infNum);
extern "C"  __declspec(dllexport) double _sin(double x);

extern "C" __declspec(dllexport) float __cosf(float x);
extern "C" __declspec(dllexport) float __sinf(float x);

extern "C"  __declspec(dllexport) double __log(double x);
extern "C"  __declspec(dllexport) double __logn(double base, double exp);
extern "C"  __declspec(dllexport) double __exp(double x);
extern "C" __declspec(dllexport) double __fabs(double x);

extern "C" __declspec(dllexport) float __logf(float x);

extern "C" __declspec(dllexport) float __expf(float x);

extern "C" __declspec(dllexport) float __sqrtf(float x);

extern "C" __declspec(dllexport) float __fabsf(float x);


#ifndef _DEBUG
extern "C" __declspec(dllexport) int abs(int x);
extern "C" __declspec(dllexport) double pown(double x, int n);
extern "C" __declspec(dllexport) double cos(double x);
extern "C" __declspec(dllexport) double sin(double x);
extern "C" __declspec(dllexport) double pow(double a, int b);
extern "C" __declspec(dllexport) double sqrt(double x);

extern "C" __declspec(dllexport) double log(double x);
extern "C" __declspec(dllexport) double exp(double x);

#endif
#else
extern "C" __declspec(dllimport) DWORD __sqrtInteger(DWORD i);
extern "C"  __declspec(dllimport) int __abs(int x);
extern "C"  __declspec(dllimport) double __pown(double x, int n);
extern "C"  __declspec(dllimport) double __cos(double x);
extern "C"  __declspec(dllimport) double __sin(double x);
extern "C"  __declspec(dllimport) double __pow(double a, int b);
extern "C"  __declspec(dllimport) double __sqrt(double a);

extern "C"  __declspec(dllimport) double __acos(double x);
extern "C"  __declspec(dllimport) double __asin(double x);
extern "C"  __declspec(dllimport) double __atan(double y, double x, int infNum);
extern "C"  __declspec(dllimport) double _sin(double x);

extern "C"  __declspec(dllimport) double __log(double x);
extern "C"  __declspec(dllimport) double __logn(double base, double exp);
extern "C"  __declspec(dllimport) double __exp(double x);
extern "C" __declspec(dllimport) float __sqrtf(float x);
extern "C" __declspec(dllimport) float __cosf(float x);
extern "C" __declspec(dllimport) float __sinf(float x);
extern "C" __declspec(dllimport) float __fabsf(float x);
extern "C" __declspec(dllimport) double __fabs(double x);
extern "C" __declspec(dllimport) float __logf(float x);
extern "C" __declspec(dllimport) float __expf(float x);

extern "C" __declspec(dllimport) double abs(double x);
extern "C" __declspec(dllimport) double pown(double x, int n);
extern "C" __declspec(dllimport) double cos(double x);
extern "C" __declspec(dllimport) double sin(double x);
extern "C" __declspec(dllimport) double pow(double a, int b);
extern "C" __declspec(dllimport) double sqrt(double x);
extern "C" __declspec(dllimport) double log(double x);
extern "C" __declspec(dllimport) double exp(double x);


#endif



#endif

int GetCos(int angle);

int GetSin(int angle);

extern "C" int g_sincos[256];


