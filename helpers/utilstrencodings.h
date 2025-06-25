#ifndef TINYFORMAT_H_INCLUDED
#define TINYFORMAT_H_INCLUDED

namespace tinyformat {}
//------------------------------------------------------------------------------
// Config section.  Customize to your liking!

// Namespace alias to encourage brevity
namespace tfm = tinyformat;

// Error handling; calls assert() by default.
#define TINYFORMAT_ERROR(reasonString) throw tinyformat::format_error(reasonString)

// Define for C++11 variadic templates which make the code shorter & more
// general.  If you don't define this, C++11 support is autodetected below.
#define TINYFORMAT_USE_VARIADIC_TEMPLATES


//------------------------------------------------------------------------------
// Implementation details.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifndef TINYFORMAT_ERROR
#   define TINYFORMAT_ERROR(reason) assert(0 && reason)
#endif

#if !defined(TINYFORMAT_USE_VARIADIC_TEMPLATES) && !defined(TINYFORMAT_NO_VARIADIC_TEMPLATES)
#   ifdef __GXX_EXPERIMENTAL_CXX0X__
#       define TINYFORMAT_USE_VARIADIC_TEMPLATES
#   endif
#endif

#if defined(__GLIBCXX__) && __GLIBCXX__ < 20080201
//  std::showpos is broken on old libstdc++ as provided with OSX.  See
//  http://gcc.gnu.org/ml/libstdc++/2007-11/msg00075.html
#   define TINYFORMAT_OLD_LIBSTDCPLUSPLUS_WORKAROUND
#endif

#ifdef __APPLE__
// Workaround OSX linker warning: xcode uses different default symbol
// visibilities for static libs vs executables (see issue #25)
#   define TINYFORMAT_HIDDEN __attribute__((visibility("hidden")))
#else
#   define TINYFORMAT_HIDDEN
#endif

namespace tinyformat {

class format_error: public std::runtime_error
{
public:
    explicit format_error(const std::string &what): std::runtime_error(what) {
    }
};

//------------------------------------------------------------------------------
namespace detail {

// Test whether type T1 is convertible to type T2
template <typename T1, typename T2>
struct is_convertible
{
    private:
        // two types of different size
        struct fail { char dummy[2]; };
        struct succeed { char dummy; };
        // Try to convert a T1 to a T2 by plugging into tryConvert
        static fail tryConvert(...);
        static succeed tryConvert(const T2&);
        static const T1& makeT1();
    public:
#       ifdef _MSC_VER
        // Disable spurious loss of precision warnings in tryConvert(makeT1())
#       pragma warning(push)
#       pragma warning(disable:4244)
#       pragma warning(disable:4267)
#       endif
        // Standard trick: the (...) version of tryConvert will be chosen from
        // the overload set only if the version taking a T2 doesn't match.
        // Then we compare the sizes of the return types to check which
        // function matched.  Very neat, in a disgusting kind of way :)
        static const bool value =
            sizeof(tryConvert(makeT1())) == sizeof(succeed);
#       ifdef _MSC_VER
#       pragma warning(pop)
#       endif
};


// Detect when a type is not a wchar_t string
template<typename T> struct is_wchar { typedef int tinyformat_wchar_is_not_supported; };
template<> struct is_wchar<wchar_t*> {};
template<> struct is_wchar<const wchar_t*> {};
template<int n> struct is_wchar<const wchar_t[n]> {};
template<int n> struct is_wchar<wchar_t[n]> {};


// Format the value by casting to type fmtT.  This default implementation
// should never be called.
template<typename T, typename fmtT, bool convertible = is_convertible<T, fmtT>::value>
struct formatValueAsType
{
    static void invoke(std::ostream& /*out*/, const T& /*value*/) { assert(0); }
};
// Specialized version for types that can actually be converted to fmtT, as
// indicated by the "convertible" template parameter.
template<typename T, typename fmtT>
struct formatValueAsType<T,fmtT,true>
{
    static void invoke(std::ostream& out, const T& value)
        { out << static_cast<fmtT>(value); }
};

#ifdef TINYFORMAT_OLD_LIBSTDCPLUSPLUS_WORKAROUND
template<typename T, bool convertible = is_convertible<T, int>::value>
struct formatZeroIntegerWorkaround
{
    static bool invoke(std::ostream& /**/, const T& /**/) { return false; }
};
template<typename T>
struct formatZeroIntegerWorkaround<T,true>
{
    static bool invoke(std::ostream& out, const T& value)
    {
        if (static_cast<int>(value) == 0 && out.flags() & std::ios::showpos)
        {
            out << "+0";
            return true;
        }
        return false;
    }
};
#endif // TINYFORMAT_OLD_LIBSTDCPLUSPLUS_WORKAROUND

// Convert an arbitrary type to integer.  The version with convertible=false
// throws an error.
template<typename T, bool convertible = is_convertible<T,int>::value>
struct convertToInt
{
    static int invoke(const T& /*value*/)
    {
        TINYFORMAT_ERROR("tinyformat: Cannot convert from argument type to "
                         "integer for use as variable width or precision");
        return 0;
    }
};
// Specialization for convertToInt when conversion is possible
template<typename T>
struct convertToInt<T,true>
{
    static int invoke(const T& value) { return static_cast<int>(value); }
};

// Format at most ntrunc characters to the given stream.
template<typename T>
inline void formatTruncated(std::ostream& out, const T& value, int ntrunc)
{
    std::ostringstream tmp;
    tmp << value;
    std::string result = tmp.str();
    out.write(result.c_str(), (std::min)(ntrunc, static_cast<int>(result.size())));
}
#define TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR(type)       \
inline void formatTruncated(std::ostream& out, type* value, int ntrunc) \
{                                                           \
    std::streamsize len = 0;                                \
    while(len < ntrunc && value[len] != 0)                  \
        ++len;                                              \
    out.write(value, len);                                  \
}
// Overload for const char* and char*.  Could overload for signed & unsigned
// char too, but these are technically unneeded for printf compatibility.
TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR(const char)
TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR(char)
#undef TINYFORMAT_DEFINE_FORMAT_TRUNCATED_CSTR

} // namespace detail


//------------------------------------------------------------------------------
// Variable formatting functions.  May be overridden for user-defined types if
// desired.


/// Format a value into a stream, delegating to operator<< by default.
///
/// Users may override this for their own types.  When this function is called,
/// the stream flags will have been modified according to the format string.
/// The format specification is provided in the range [fmtBegin, fmtEnd).  For
/// truncating conversions, ntrunc is set to the desired maximum number of
/// characters, for example "%.7s" calls formatValue with ntrunc = 7.
///
/// By default, formatValue() uses the usual stream insertion operator
/// operator<< to format the type T, with special cases for the %c and %p
/// conversions.
template<typename T>
inline void formatValue(std::ostream& out, const char* /*fmtBegin*/,
                        const char* fmtEnd, int ntrunc, const T& value)
{
#ifndef TINYFORMAT_ALLOW_WCHAR_STRINGS
    // Since we don't support printing of wchar_t using "%ls", make it fail at
    // compile time in preference to printing as a void* at runtime.
    typedef typename detail::is_wchar<T>::tinyformat_wchar_is_not_supported DummyType;
    (void) DummyType(); // avoid unused type warning with gcc-4.8
#endif
    // The mess here is to support the %c and %p conversions: if these
    // conversions are active we try to convert the type to a char or const
    // void* respectively and format that instead of the value itself.  For the
    // %p conversion it's important to avoid dereferencing the pointer, which
    // could otherwise lead to a crash when printing a dangling (const char*).
    const bool canConvertToChar = detail::is_convertible<T,char>::value;
    const bool canConvertToVoidPtr = detail::is_convertible<T, const void*>::value;
    if(canConvertToChar && *(fmtEnd-1) == 'c')
        detail::formatValueAsType<T, char>::invoke(out, value);
    else if(canConvertToVoidPtr && *(fmtEnd-1) == 'p')
        detail::formatValueAsType<T, const void*>::invoke(out, value);
#ifdef TINYFORMAT_OLD_LIBSTDCPLUSPLUS_WORKAROUND
    else if(detail::formatZeroIntegerWorkaround<T>::invoke(out, value)) /**/;
#endif
    else if(ntrunc >= 0)
    {
        // Take care not to overread C strings in truncating conversions like
        // "%.4s" where at most 4 characters may be read.
        detail::formatTruncated(out, value, ntrunc);
    }
    else
        out << value;
}


// Overloaded version for char types to support printing as an integer
#define TINYFORMAT_DEFINE_FORMATVALUE_CHAR(charType)                  \
inline void formatValue(std::ostream& out, const char* /*fmtBegin*/,  \
                        const char* fmtEnd, int /**/, charType value) \
{                                                                     \
    switch(*(fmtEnd-1))                                               \
    {                                                                 \
        case 'u': case 'd': case 'i': case 'o': case 'X': case 'x':   \
            out << static_cast<int>(value); break;                    \
        default:                                                      \
            out << value;                   break;                    \
    }                                                                 \
}
// per 3.9.1: char, signed char and unsigned char are all distinct types
TINYFORMAT_DEFINE_FORMATVALUE_CHAR(char)
TINYFORMAT_DEFINE_FORMATVALUE_CHAR(signed char)
TINYFORMAT_DEFINE_FORMATVALUE_CHAR(unsigned char)
#undef TINYFORMAT_DEFINE_FORMATVALUE_CHAR


//------------------------------------------------------------------------------
// Tools for emulating variadic templates in C++98.  The basic idea here is
// stolen from the boost preprocessor metaprogramming library and cut down to
// be just general enough for what we need.

#define TINYFORMAT_ARGTYPES(n) TINYFORMAT_ARGTYPES_ ## n
#define TINYFORMAT_VARARGS(n) TINYFORMAT_VARARGS_ ## n
#define TINYFORMAT_PASSARGS(n) TINYFORMAT_PASSARGS_ ## n
#define TINYFORMAT_PASSARGS_TAIL(n) TINYFORMAT_PASSARGS_TAIL_ ## n

// To keep it as transparent as possible, the macros below have been generated
// using python via the excellent cog.py code generation script.  This avoids
// the need for a bunch of complex (but more general) preprocessor tricks as
// used in boost.preprocessor.
//
// To rerun the code generation in place, use `cog.py -r tinyformat.h`
// (see http://nedbatchelder.com/code/cog).  Alternatively you can just create
// extra versions by hand.

/*[[[cog
maxParams = 16

def makeCommaSepLists(lineTemplate, elemTemplate, startInd=1):
    for j in range(startInd,maxParams+1):
        list = ', '.join([elemTemplate % {'i':i} for i in range(startInd,j+1)])
        cog.outl(lineTemplate % {'j':j, 'list':list})

makeCommaSepLists('#define TINYFORMAT_ARGTYPES_%(j)d %(list)s',
                  'class T%(i)d')

cog.outl()
makeCommaSepLists('#define TINYFORMAT_VARARGS_%(j)d %(list)s',
                  'const T%(i)d& v%(i)d')

cog.outl()
makeCommaSepLists('#define TINYFORMAT_PASSARGS_%(j)d %(list)s', 'v%(i)d')

cog.outl()
cog.outl('#define TINYFORMAT_PASSARGS_TAIL_1')
makeCommaSepLists('#define TINYFORMAT_PASSARGS_TAIL_%(j)d , %(list)s',
                  'v%(i)d', startInd = 2)

cog.outl()
cog.outl('#define TINYFORMAT_FOREACH_ARGNUM(m) \\\n    ' +
         ' '.join(['m(%d)' % (j,) for j in range(1,maxParams+1)]))
]]]*/
#define TINYFORMAT_ARGTYPES_1 class T1
#define TINYFORMAT_ARGTYPES_2 class T1, class T2
#define TINYFORMAT_ARGTYPES_3 class T1, class T2, class T3
#define TINYFORMAT_ARGTYPES_4 class T1, class T2, class T3, class T4
#define TINYFORMAT_ARGTYPES_5 class T1, class T2, class T3, class T4, class T5
#define TINYFORMAT_ARGTYPES_6 class T1, class T2, class T3, class T4, class T5, class T6
#define TINYFORMAT_ARGTYPES_7 class T1, class T2, class T3, class T4, class T5, class T6, class T7
#define TINYFORMAT_ARGTYPES_8 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8
#define TINYFORMAT_ARGTYPES_9 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9
#define TINYFORMAT_ARGTYPES_10 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10
#define TINYFORMAT_ARGTYPES_11 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11
#define TINYFORMAT_ARGTYPES_12 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12
#define TINYFORMAT_ARGTYPES_13 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13
#define TINYFORMAT_ARGTYPES_14 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14
#define TINYFORMAT_ARGTYPES_15 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15
#define TINYFORMAT_ARGTYPES_16 class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16

#define TINYFORMAT_VARARGS_1 const T1& v1
#define TINYFORMAT_VARARGS_2 const T1& v1, const T2& v2
#define TINYFORMAT_VARARGS_3 const T1& v1, const T2& v2, const T3& v3
#define TINYFORMAT_VARARGS_4 const T1& v1, const T2& v2, const T3& v3, const T4& v4
#define TINYFORMAT_VARARGS_5 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5
#define TINYFORMAT_VARARGS_6 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6
#define TINYFORMAT_VARARGS_7 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7
#define TINYFORMAT_VARARGS_8 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8
#define TINYFORMAT_VARARGS_9 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9
#define TINYFORMAT_VARARGS_10 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9, const T10& v10
#define TINYFORMAT_VARARGS_11 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9, const T10& v10, const T11& v11
#define TINYFORMAT_VARARGS_12 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9, const T10& v10, const T11& v11, const T12& v12
#define TINYFORMAT_VARARGS_13 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9, const T10& v10, const T11& v11, const T12& v12, const T13& v13
#define TINYFORMAT_VARARGS_14 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9, const T10& v10, const T11& v11, const T12& v12, const T13& v13, const T14& v14
#define TINYFORMAT_VARARGS_15 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9, const T10& v10, const T11& v11, const T12& v12, const T13& v13, const T14& v14, const T15& v15
#define TINYFORMAT_VARARGS_16 const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6, const T7& v7, const T8& v8, const T9& v9, const T10& v10, const T11& v11, const T12& v12, const T13& v13, const T14& v14, const T15& v15, const T16& v16

#define TINYFORMAT_PASSARGS_1 v1
#define TINYFORMAT_PASSARGS_2 v1, v2
#define TINYFORMAT_PASSARGS_3 v1, v2, v3
#define TINYFORMAT_PASSARGS_4 v1, v2, v3, v4
#define TINYFORMAT_PASSARGS_5 v1, v2, v3, v4, v5
#define TINYFORMAT_PASSARGS_6 v1, v2, v3, v4, v5, v6
#define TINYFORMAT_PASSARGS_7 v1, v2, v3, v4, v5, v6, v7
#define TINYFORMAT_PASSARGS_8 v1, v2, v3, v4, v5, v6, v7, v8
#define TINYFORMAT_PASSARGS_9 v1, v2, v3, v4, v5, v6, v7, v8, v9
#define TINYFORMAT_PASSARGS_10 v1, v2, v3, v4, v5, v6, v7, v8, v9, v10
#define TINYFORMAT_PASSARGS_11 v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11
#define TINYFORMAT_PASSARGS_12 v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12
#define TINYFORMAT_PASSARGS_13 v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13
#define TINYFORMAT_PASSARGS_14 v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14
#define TINYFORMAT_PASSARGS_15 v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15
#define TINYFORMAT_PASSARGS_16 v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16

#define TINYFORMAT_PASSARGS_TAIL_1
#define TINYFORMAT_PASSARGS_TAIL_2 , v2
#define TINYFORMAT_PASSARGS_TAIL_3 , v2, v3
#define TINYFORMAT_PASSARGS_TAIL_4 , v2, v3, v4
#define TINYFORMAT_PASSARGS_TAIL_5 , v2, v3, v4, v5
#define TINYFORMAT_PASSARGS_TAIL_6 , v2, v3, v4, v5, v6
#define TINYFORMAT_PASSARGS_TAIL_7 , v2, v3, v4, v5, v6, v7
#define TINYFORMAT_PASSARGS_TAIL_8 , v2, v3, v4, v5, v6, v7, v8
#define TINYFORMAT_PASSARGS_TAIL_9 , v2, v3, v4, v5, v6, v7, v8, v9
#define TINYFORMAT_PASSARGS_TAIL_10 , v2, v3, v4, v5, v6, v7, v8, v9, v10
#define TINYFORMAT_PASSARGS_TAIL_11 , v2, v3, v4, v5, v6, v7, v8, v9, v10, v11
#define TINYFORMAT_PASSARGS_TAIL_12 , v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12
#define TINYFORMAT_PASSARGS_TAIL_13 , v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13
#define TINYFORMAT_PASSARGS_TAIL_14 , v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14
#define TINYFORMAT_PASSARGS_TAIL_15 , v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15
#define TINYFORMAT_PASSARGS_TAIL_16 , v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16

#define TINYFORMAT_FOREACH_ARGNUM(m) \
    m(1) m(2) m(3) m(4) m(5) m(6) m(7) m(8) m(9) m(10) m(11) m(12) m(13) m(14) m(15) m(16)
//[[[end]]]



namespace detail {

// Type-opaque holder for an argument to format(), with associated actions on
// the type held as explicit function pointers.  This allows FormatArg's for
// each argument to be allocated as a homogenous array inside FormatList
// whereas a naive implementation based on inheritance does not.
class FormatArg
{
    public:
        FormatArg()
             : m_value(nullptr),
             m_formatImpl(nullptr),
             m_toIntImpl(nullptr)
         { }

        template<typename T>
        explicit FormatArg(const T& value)
            : m_value(static_cast<const void*>(&value)),
            m_formatImpl(&formatImpl<T>),
            m_toIntImpl(&toIntImpl<T>)
        { }

        void format(std::ostream& out, const char* fmtBegin,
                    const char* fmtEnd, int ntrunc) const
        {
            assert(m_value);
            assert(m_formatImpl);
            m_formatImpl(out, fmtBegin, fmtEnd, ntrunc, m_value);
        }

        int toInt() const
        {
            assert(m_value);
            assert(m_toIntImpl);
            return m_toIntImpl(m_value);
        }

    private:
        template<typename T>
        TINYFORMAT_HIDDEN static void formatImpl(std::ostream& out, const char* fmtBegin,
                        const char* fmtEnd, int ntrunc, const void* value)
        {
            formatValue(out, fmtBegin, fmtEnd, ntrunc, *static_cast<const T*>(value));
        }

        template<typename T>
        TINYFORMAT_HIDDEN static int toIntImpl(const void* value)
        {
            return convertToInt<T>::invoke(*static_cast<const T*>(value));
        }

        const void* m_value;
        void (*m_formatImpl)(std::ostream& out, const char* fmtBegin,
                             const char* fmtEnd, int ntrunc, const void* value);
        int (*m_toIntImpl)(const void* value);
};


// Parse and return an integer from the string c, as atoi()
// On return, c is set to one past the end of the integer.
inline int parseIntAndAdvance(const char*& c)
{
    int i = 0;
    for(;*c >= '0' && *c <= '9'; ++c)
        i = 10*i + (*c - '0');
    return i;
}

// Print literal part of format string and return next format spec
// position.
//
// Skips over any occurrences of '%%', printing a literal '%' to the
// output.  The position of the first % character of the next
// nontrivial format spec is returned, or the end of string.
inline const char* printFormatStringLiteral(std::ostream& out, const char* fmt)
{
    const char* c = fmt;
    for(;; ++c)
    {
        switch(*c)
        {
            case '\0':
                out.write(fmt, c - fmt);
                return c;
            case '%':
                out.write(fmt, c - fmt);
                if(*(c+1) != '%')
                    return c;
                // for "%%", tack trailing % onto next literal section.
                fmt = ++c;
                break;
            default:
                break;
        }
    }
}


// Parse a format string and set the stream state accordingly.
//
// The format mini-language recognized here is meant to be the one from C99,
// with the form "%[flags][width][.precision][length]type".
//
// Formatting options which can't be natively represented using the ostream
// state are returned in spacePadPositive (for space padded positive numbers)
// and ntrunc (for truncating conversions).  argIndex is incremented if
// necessary to pull out variable width and precision .  The function returns a
// pointer to the character after the end of the current format spec.
inline const char* streamStateFromFormat(std::ostream& out, bool& spacePadPositive,
                                         int& ntrunc, const char* fmtStart,
                                         const detail::FormatArg* formatters,
                                         int& argIndex, int numFormatters)
{
    if(*fmtStart != '%')
    {
        TINYFORMAT_ERROR("tinyformat: Not enough conversion specifiers in format string");
        return fmtStart;
    }
    // Reset stream state to defaults.
    out.width(0);
    out.precision(6);
    out.fill(' ');
    // Reset most flags; ignore irrelevant unitbuf & skipws.
    out.unsetf(std::ios::adjustfield | std::ios::basefield |
               std::ios::floatfield | std::ios::showbase | std::ios::boolalpha |
               std::ios::showpoint | std::ios::showpos | std::ios::uppercase);
    bool precisionSet = false;
    bool widthSet = false;
    int widthExtra = 0;
    const char* c = fmtStart + 1;
    // 1) Parse flags
    for(;; ++c)
    {
        switch(*c)
        {
            case '#':
                out.setf(std::ios::showpoint | std::ios::showbase);
                continue;
            case '0':
                // overridden by left alignment ('-' flag)
                if(!(out.flags() & std::ios::left))
                {
                    // Use internal padding so that numeric values are
                    // formatted correctly, eg -00010 rather than 000-10
                    out.fill('0');
                    out.setf(std::ios::internal, std::ios::adjustfield);
                }
                continue;
            case '-':
                out.fill(' ');
                out.setf(std::ios::left, std::ios::adjustfield);
                continue;
            case ' ':
                // overridden by show positive sign, '+' flag.
                if(!(out.flags() & std::ios::showpos))
                    spacePadPositive = true;
                continue;
            case '+':
                out.setf(std::ios::showpos);
                spacePadPositive = false;
                widthExtra = 1;
                continue;
            default:
                break;
        }
        break;
    }
    // 2) Parse width
    if(*c >= '0' && *c <= '9')
    {
        widthSet = true;
        out.width(parseIntAndAdvance(c));
    }
    if(*c == '*')
    {
        widthSet = true;
        int width = 0;
        if(argIndex < numFormatters)
            width = formatters[argIndex++].toInt();
        else
            TINYFORMAT_ERROR("tinyformat: Not enough arguments to read variable width");
        if(width < 0)
        {
            // negative widths correspond to '-' flag set
            out.fill(' ');
            out.setf(std::ios::left, std::ios::adjustfield);
            width = -width;
        }
        out.width(width);
        ++c;
    }
    // 3) Parse precision
    if(*c == '.')
    {
        ++c;
        int precision = 0;
        if(*c == '*')
        {
            ++c;
            if(argIndex < numFormatters)
                precision = formatters[argIndex++].toInt();
            else
                TINYFORMAT_ERROR("tinyformat: Not enough arguments to read variable precision");
        }
        else
        {
            if(*c >= '0' && *c <= '9')
                precision = parseIntAndAdvance(c);
            else if(*c == '-') // negative precisions ignored, treated as zero.
                parseIntAndAdvance(++c);
        }
        out.precision(precision);
        precisionSet = true;
    }
    // 4) Ignore any C99 length modifier
    while(*c == 'l' || *c == 'h' || *c == 'L' ||
          *c == 'j' || *c == 'z' || *c == 't')
        ++c;
    // 5) We're up to the conversion specifier character.
    // Set stream flags based on conversion specifier (thanks to the
    // boost::format class for forging the way here).
    bool intConversion = false;
    switch(*c)
    {
        case 'u': case 'd': case 'i':
            out.setf(std::ios::dec, std::ios::basefield);
            intConversion = true;
            break;
        case 'o':
            out.setf(std::ios::oct, std::ios::basefield);
            intConversion = true;
            break;
        case 'X':
            out.setf(std::ios::uppercase);
            // Falls through
        case 'x': case 'p':
            out.setf(std::ios::hex, std::ios::basefield);
            intConversion = true;
            break;
        case 'E':
            out.setf(std::ios::uppercase);
            // Falls through
        case 'e':
            out.setf(std::ios::scientific, std::ios::floatfield);
            out.setf(std::ios::dec, std::ios::basefield);
            break;
        case 'F':
            out.setf(std::ios::uppercase);
            // Falls through
        case 'f':
            out.setf(std::ios::fixed, std::ios::floatfield);
            break;
        case 'G':
            out.setf(std::ios::uppercase);
            // Falls through
        case 'g':
            out.setf(std::ios::dec, std::ios::basefield);
            // As in boost::format, let stream decide float format.
            out.flags(out.flags() & ~std::ios::floatfield);
            break;
        case 'a': case 'A':
            TINYFORMAT_ERROR("tinyformat: the %a and %A conversion specs "
                             "are not supported");
            break;
        case 'c':
            // Handled as special case inside formatValue()
            break;
        case 's':
            if(precisionSet)
                ntrunc = static_cast<int>(out.precision());
            // Make %s print booleans as "true" and "false"
            out.setf(std::ios::boolalpha);
            break;
        case 'n':
            // Not supported - will cause problems!
            TINYFORMAT_ERROR("tinyformat: %n conversion spec not supported");
            break;
        case '\0':
            TINYFORMAT_ERROR("tinyformat: Conversion spec incorrectly "
                             "terminated by end of string");
            return c;
        default:
            break;
    }
    if(intConversion && precisionSet && !widthSet)
    {
        // "precision" for integers gives the minimum number of digits (to be
        // padded with zeros on the left).  This isn't really supported by the
        // iostreams, but we can approximately simulate it with the width if
        // the width isn't otherwise used.
        out.width(out.precision() + widthExtra);
        out.setf(std::ios::internal, std::ios::adjustfield);
        out.fill('0');
    }
    return c+1;
}


//------------------------------------------------------------------------------
inline void formatImpl(std::ostream& out, const char* fmt,
                       const detail::FormatArg* formatters,
                       int numFormatters)
{
    // Saved stream state
    std::streamsize origWidth = out.width();
    std::streamsize origPrecision = out.precision();
    std::ios::fmtflags origFlags = out.flags();
    char origFill = out.fill();

    for (int argIndex = 0; argIndex < numFormatters; ++argIndex)
    {
        // Parse the format string
        fmt = printFormatStringLiteral(out, fmt);
        bool spacePadPositive = false;
        int ntrunc = -1;
        const char* fmtEnd = streamStateFromFormat(out, spacePadPositive, ntrunc, fmt,
                                                   formatters, argIndex, numFormatters);
        if (argIndex >= numFormatters)
        {
            // Check args remain after reading any variable width/precision
            TINYFORMAT_ERROR("tinyformat: Not enough format arguments");
            return;
        }
        const FormatArg& arg = formatters[argIndex];
        // Format the arg into the stream.
        if(!spacePadPositive)
            arg.format(out, fmt, fmtEnd, ntrunc);
        else
        {
            // The following is a special case with no direct correspondence
            // between stream formatting and the printf() behaviour.  Simulate
            // it crudely by formatting into a temporary string stream and
            // munging the resulting string.
            std::ostringstream tmpStream;
            tmpStream.copyfmt(out);
            tmpStream.setf(std::ios::showpos);
            arg.format(tmpStream, fmt, fmtEnd, ntrunc);
            std::string result = tmpStream.str(); // allocates... yuck.
            for(size_t i = 0, iend = result.size(); i < iend; ++i)
                if(result[i] == '+') result[i] = ' ';
            out << result;
        }
        fmt = fmtEnd;
    }

    // Print remaining part of format string.
    fmt = printFormatStringLiteral(out, fmt);
    if(*fmt != '\0')
        TINYFORMAT_ERROR("tinyformat: Too many conversion specifiers in format string");

    // Restore stream state
    out.width(origWidth);
    out.precision(origPrecision);
    out.flags(origFlags);
    out.fill(origFill);
}

} // namespace detail


/// List of template arguments format(), held in a type-opaque way.
///
/// A const reference to FormatList (typedef'd as FormatListRef) may be
/// conveniently used to pass arguments to non-template functions: All type
/// information has been stripped from the arguments, leaving just enough of a
/// common interface to perform formatting as required.
class FormatList
{
    public:
        FormatList(detail::FormatArg* formatters, int N)
            : m_formatters(formatters), m_N(N) { }

        friend void vformat(std::ostream& out, const char* fmt,
                            const FormatList& list);

    private:
        const detail::FormatArg* m_formatters;
        int m_N;
};

/// Reference to type-opaque format list for passing to vformat()
typedef const FormatList& FormatListRef;


namespace detail {

// Format list subclass with fixed storage to avoid dynamic allocation
template<int N>
class FormatListN : public FormatList
{
    public:
#ifdef TINYFORMAT_USE_VARIADIC_TEMPLATES
        template<typename... Args>
        explicit FormatListN(const Args&... args)
            : FormatList(&m_formatterStore[0], N),
            m_formatterStore { FormatArg(args)... }
        { static_assert(sizeof...(args) == N, "Number of args must be N"); }
#else // C++98 version
        void init(int) {}
#       define TINYFORMAT_MAKE_FORMATLIST_CONSTRUCTOR(n)       \
                                                               \
        template<TINYFORMAT_ARGTYPES(n)>                       \
        explicit FormatListN(TINYFORMAT_VARARGS(n))            \
            : FormatList(&m_formatterStore[0], n)              \
        { assert(n == N); init(0, TINYFORMAT_PASSARGS(n)); }   \
                                                               \
        template<TINYFORMAT_ARGTYPES(n)>                       \
        void init(int i, TINYFORMAT_VARARGS(n))                \
        {                                                      \
            m_formatterStore[i] = FormatArg(v1);               \
            init(i+1 TINYFORMAT_PASSARGS_TAIL(n));             \
        }

        TINYFORMAT_FOREACH_ARGNUM(TINYFORMAT_MAKE_FORMATLIST_CONSTRUCTOR)
#       undef TINYFORMAT_MAKE_FORMATLIST_CONSTRUCTOR
#endif

    private:
        FormatArg m_formatterStore[N];
};

// Special 0-arg version - MSVC says zero-sized C array in struct is nonstandard
template<> class FormatListN<0> : public FormatList
{
    public: FormatListN() : FormatList(0, 0) {}
};

} // namespace detail


//------------------------------------------------------------------------------
// Primary API functions

#ifdef TINYFORMAT_USE_VARIADIC_TEMPLATES

/// Make type-agnostic format list from list of template arguments.
///
/// The exact return type of this function is an implementation detail and
/// shouldn't be relied upon.  Instead it should be stored as a FormatListRef:
///
///   FormatListRef formatList = makeFormatList( /*...*/ );
template<typename... Args>
detail::FormatListN<sizeof...(Args)> makeFormatList(const Args&... args)
{
    return detail::FormatListN<sizeof...(args)>(args...);
}

#else // C++98 version

inline detail::FormatListN<0> makeFormatList()
{
    return detail::FormatListN<0>();
}
#define TINYFORMAT_MAKE_MAKEFORMATLIST(n)                     \
template<TINYFORMAT_ARGTYPES(n)>                              \
detail::FormatListN<n> makeFormatList(TINYFORMAT_VARARGS(n))  \
{                                                             \
    return detail::FormatListN<n>(TINYFORMAT_PASSARGS(n));    \
}
TINYFORMAT_FOREACH_ARGNUM(TINYFORMAT_MAKE_MAKEFORMATLIST)
#undef TINYFORMAT_MAKE_MAKEFORMATLIST

#endif

/// Format list of arguments to the stream according to the given format string.
///
/// The name vformat() is chosen for the semantic similarity to vprintf(): the
/// list of format arguments is held in a single function argument.
inline void vformat(std::ostream& out, const char* fmt, FormatListRef list)
{
    detail::formatImpl(out, fmt, list.m_formatters, list.m_N);
}


#ifdef TINYFORMAT_USE_VARIADIC_TEMPLATES

/// Format list of arguments to the stream according to given format string.
template<typename... Args>
void format(std::ostream& out, const char* fmt, const Args&... args)
{
    vformat(out, fmt, makeFormatList(args...));
}

/// Format list of arguments according to the given format string and return
/// the result as a string.
template<typename... Args>
std::string format(const char* fmt, const Args&... args)
{
    std::ostringstream oss;
    format(oss, fmt, args...);
    return oss.str();
}

/// Format list of arguments to std::cout, according to the given format string
template<typename... Args>
void printf(const char* fmt, const Args&... args)
{
    format(std::cout, fmt, args...);
}

template<typename... Args>
void printfln(const char* fmt, const Args&... args)
{
    format(std::cout, fmt, args...);
    std::cout << '\n';
}

#else // C++98 version

inline void format(std::ostream& out, const char* fmt)
{
    vformat(out, fmt, makeFormatList());
}

inline std::string format(const char* fmt)
{
    std::ostringstream oss;
    format(oss, fmt);
    return oss.str();
}

inline void printf(const char* fmt)
{
    format(std::cout, fmt);
}

inline void printfln(const char* fmt)
{
    format(std::cout, fmt);
    std::cout << '\n';
}

#define TINYFORMAT_MAKE_FORMAT_FUNCS(n)                                   \
                                                                          \
template<TINYFORMAT_ARGTYPES(n)>                                          \
void format(std::ostream& out, const char* fmt, TINYFORMAT_VARARGS(n))    \
{                                                                         \
    vformat(out, fmt, makeFormatList(TINYFORMAT_PASSARGS(n)));            \
}                                                                         \
                                                                          \
template<TINYFORMAT_ARGTYPES(n)>                                          \
std::string format(const char* fmt, TINYFORMAT_VARARGS(n))                \
{                                                                         \
    std::ostringstream oss;                                               \
    format(oss, fmt, TINYFORMAT_PASSARGS(n));                             \
    return oss.str();                                                     \
}                                                                         \
                                                                          \
template<TINYFORMAT_ARGTYPES(n)>                                          \
void printf(const char* fmt, TINYFORMAT_VARARGS(n))                       \
{                                                                         \
    format(std::cout, fmt, TINYFORMAT_PASSARGS(n));                       \
}                                                                         \
                                                                          \
template<TINYFORMAT_ARGTYPES(n)>                                          \
void printfln(const char* fmt, TINYFORMAT_VARARGS(n))                     \
{                                                                         \
    format(std::cout, fmt, TINYFORMAT_PASSARGS(n));                       \
    std::cout << '\n';                                                    \
}

TINYFORMAT_FOREACH_ARGNUM(TINYFORMAT_MAKE_FORMAT_FUNCS)
#undef TINYFORMAT_MAKE_FORMAT_FUNCS

#endif

// Added for Bitcoin Core
template<typename... Args>
std::string format(const std::string &fmt, const Args&... args)
{
    std::ostringstream oss;
    format(oss, fmt.c_str(), args...);
    return oss.str();
}

} // namespace tinyformat

#define strprintf tfm::format

#endif // TINYFORMAT_H_INCLUDED

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Utilities for converting data from/to strings.
 */
#ifndef BITCOIN_UTILSTRENCODINGS_H
#define BITCOIN_UTILSTRENCODINGS_H

#include <stdint.h>
#include <string>
#include <vector>

#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))
#define UBEGIN(a)           ((unsigned char*)&(a))
#define UEND(a)             ((unsigned char*)&((&(a))[1]))
#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))

/** Used by SanitizeString() */
enum SafeChars
{
    SAFE_CHARS_DEFAULT, //!< The full set of allowed chars
    SAFE_CHARS_UA_COMMENT, //!< BIP-0014 subset
    SAFE_CHARS_FILENAME, //!< Chars allowed in filenames
};

/**
* Remove unsafe chars. Safe chars chosen to allow simple messages/URLs/email
* addresses, but avoid anything even possibly remotely dangerous like & or >
* @param[in] str    The string to sanitize
* @param[in] rule   The set of safe chars to choose (default: least restrictive)
* @return           A new string without unsafe chars
*/
std::string SanitizeString(const std::string& str, int rule = SAFE_CHARS_DEFAULT);
std::vector<unsigned char> ParseHex(const char* psz);
std::vector<unsigned char> ParseHex(const std::string& str);
signed char HexDigit(char c);
/* Returns true if each character in str is a hex character, and has an even
 * number of hex digits.*/
bool IsHex(const std::string& str);
/**
* Return true if the string is a hex number, optionally prefixed with "0x"
*/
bool IsHexNumber(const std::string& str);
std::vector<unsigned char> DecodeBase64(const char* p, bool* pfInvalid = nullptr);
std::string DecodeBase64(const std::string& str);
std::string EncodeBase64(const unsigned char* pch, size_t len);
std::string EncodeBase64(const std::string& str);
std::vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid = nullptr);
std::string DecodeBase32(const std::string& str);
std::string EncodeBase32(const unsigned char* pch, size_t len);
std::string EncodeBase32(const std::string& str);

void SplitHostPort(std::string in, int &portOut, std::string &hostOut);
std::string i64tostr(int64_t n);
std::string itostr(int n);
int64_t atoi64(const char* psz);
int64_t atoi64(const std::string& str);
int atoi(const std::string& str);

/**
 * Convert string to signed 32-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
bool ParseInt32(const std::string& str, int32_t *out);

/**
 * Convert string to signed 64-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
bool ParseInt64(const std::string& str, int64_t *out);

/**
 * Convert decimal string to unsigned 32-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
bool ParseUInt32(const std::string& str, uint32_t *out);

/**
 * Convert decimal string to unsigned 64-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
bool ParseUInt64(const std::string& str, uint64_t *out);

/**
 * Convert string to double with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid double,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
bool ParseDouble(const std::string& str, double *out);

template<typename T>
std::string HexStr(const T itbegin, const T itend, bool fSpaces=false)
{
    std::string rv;
    static const char hexmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    rv.reserve((itend-itbegin)*3);
    for(T it = itbegin; it < itend; ++it)
    {
        unsigned char val = (unsigned char)(*it);
        if(fSpaces && it != itbegin)
            rv.push_back(' ');
        rv.push_back(hexmap[val>>4]);
        rv.push_back(hexmap[val&15]);
    }

    return rv;
}

template<typename T>
inline std::string HexStr(const T& vch, bool fSpaces=false)
{
    return HexStr(vch.begin(), vch.end(), fSpaces);
}

/**
 * Format a paragraph of text to a fixed width, adding spaces for
 * indentation to any added line.
 */
std::string FormatParagraph(const std::string& in, size_t width = 79, size_t indent = 0);

/**
 * Timing-attack-resistant comparison.
 * Takes time proportional to length
 * of first argument.
 */
template <typename T>
bool TimingResistantEqual(const T& a, const T& b)
{
    if (b.size() == 0) return a.size() == 0;
    size_t accumulator = a.size() ^ b.size();
    for (size_t i = 0; i < a.size(); i++)
        accumulator |= a[i] ^ b[i%b.size()];
    return accumulator == 0;
}

/** Parse number as fixed point according to JSON number syntax.
 * See http://json.org/number.gif
 * @returns true on success, false on error.
 * @note The result must be in the range (-10^18,10^18), otherwise an overflow error will trigger.
 */
bool ParseFixedPoint(const std::string &val, int decimals, int64_t *amount_out);

/** Convert from one power-of-2 number base to another. */
template<int frombits, int tobits, bool pad, typename O, typename I>
bool ConvertBits(O& out, I it, I end) {
    size_t acc = 0;
    size_t bits = 0;
    constexpr size_t maxv = (1 << tobits) - 1;
    constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
    while (it != end) {
        acc = ((acc << frombits) | *it) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            out.push_back((acc >> bits) & maxv);
        }
        ++it;
    }
    if (pad) {
        if (bits) out.push_back((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}


#endif // BITCOIN_UTILSTRENCODINGS_H

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TINYFORMAT_CPP_INCLUDED
#define TINYFORMAT_CPP_INCLUDED


#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <limits>

static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const std::string SAFE_CHARS[] =
{
    CHARS_ALPHA_NUM + " .,;-_/:?@()", // SAFE_CHARS_DEFAULT
    CHARS_ALPHA_NUM + " .,;-_?@", // SAFE_CHARS_UA_COMMENT
    CHARS_ALPHA_NUM + ".-_", // SAFE_CHARS_FILENAME
};

std::string SanitizeString(const std::string& str, int rule)
{
    std::string strResult;
    for (std::string::size_type i = 0; i < str.size(); i++)
    {
        if (SAFE_CHARS[rule].find(str[i]) != std::string::npos)
            strResult.push_back(str[i]);
    }
    return strResult;
}

const signed char p_util_hexdigit[256] =
{ -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

signed char HexDigit(char c)
{
    return p_util_hexdigit[(unsigned char)c];
}

bool IsHex(const std::string& str)
{
    for(std::string::const_iterator it(str.begin()); it != str.end(); ++it)
    {
        if (HexDigit(*it) < 0)
            return false;
    }
    return (str.size() > 0) && (str.size()%2 == 0);
}

bool IsHexNumber(const std::string& str)
{
    size_t starting_location = 0;
    if (str.size() > 2 && *str.begin() == '0' && *(str.begin()+1) == 'x') {
        starting_location = 2;
    }
    for (auto c : str.substr(starting_location)) {
        if (HexDigit(c) < 0) return false;
    }
    // Return false for empty string or "0x".
    return (str.size() > starting_location);
}

std::vector<unsigned char> ParseHex(const char* psz)
{
    // convert hex dump to vector
    std::vector<unsigned char> vch;
    while (true)
    {
        while (isspace(*psz))
            psz++;
        signed char c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        unsigned char n = (c << 4);
        c = HexDigit(*psz++);
        if (c == (signed char)-1)
            break;
        n |= c;
        vch.push_back(n);
    }
    return vch;
}

std::vector<unsigned char> ParseHex(const std::string& str)
{
    return ParseHex(str.c_str());
}

void SplitHostPort(std::string in, int &portOut, std::string &hostOut) {
    size_t colon = in.find_last_of(':');
    // if a : is found, and it either follows a [...], or no other : is in the string, treat it as port separator
    bool fHaveColon = colon != in.npos;
    bool fBracketed = fHaveColon && (in[0]=='[' && in[colon-1]==']'); // if there is a colon, and in[0]=='[', colon is not 0, so in[colon-1] is safe
    bool fMultiColon = fHaveColon && (in.find_last_of(':',colon-1) != in.npos);
    if (fHaveColon && (colon==0 || fBracketed || !fMultiColon)) {
        int32_t n;
        if (ParseInt32(in.substr(colon + 1), &n) && n > 0 && n < 0x10000) {
            in = in.substr(0, colon);
            portOut = n;
        }
    }
    if (in.size()>0 && in[0] == '[' && in[in.size()-1] == ']')
        hostOut = in.substr(1, in.size()-2);
    else
        hostOut = in;
}

std::string EncodeBase64(const unsigned char* pch, size_t len)
{
    static const char *pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string strRet = "";
    strRet.reserve((len+2)/3*4);

    int mode=0, left=0;
    const unsigned char *pchEnd = pch+len;

    while (pch<pchEnd)
    {
        int enc = *(pch++);
        switch (mode)
        {
            case 0: // we have no bits
                strRet += pbase64[enc >> 2];
                left = (enc & 3) << 4;
                mode = 1;
                break;

            case 1: // we have two bits
                strRet += pbase64[left | (enc >> 4)];
                left = (enc & 15) << 2;
                mode = 2;
                break;

            case 2: // we have four bits
                strRet += pbase64[left | (enc >> 6)];
                strRet += pbase64[enc & 63];
                mode = 0;
                break;
        }
    }

    if (mode)
    {
        strRet += pbase64[left];
        strRet += '=';
        if (mode == 1)
            strRet += '=';
    }

    return strRet;
}

std::string EncodeBase64(const std::string& str)
{
    return EncodeBase64((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase64(const char* p, bool* pfInvalid)
{
    static const int decode64_table[256] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    if (pfInvalid)
        *pfInvalid = false;

    std::vector<unsigned char> vchRet;
    vchRet.reserve(strlen(p)*3/4);

    int mode = 0;
    int left = 0;

    while (1)
    {
         int dec = decode64_table[(unsigned char)*p];
         if (dec == -1) break;
         p++;
         switch (mode)
         {
             case 0: // we have no bits and get 6
                 left = dec;
                 mode = 1;
                 break;

              case 1: // we have 6 bits and keep 4
                  vchRet.push_back((left<<2) | (dec>>4));
                  left = dec & 15;
                  mode = 2;
                  break;

             case 2: // we have 4 bits and get 6, we keep 2
                 vchRet.push_back((left<<4) | (dec>>2));
                 left = dec & 3;
                 mode = 3;
                 break;

             case 3: // we have 2 bits and get 6
                 vchRet.push_back((left<<6) | dec);
                 mode = 0;
                 break;
         }
    }

    if (pfInvalid)
        switch (mode)
        {
            case 0: // 4n base64 characters processed: ok
                break;

            case 1: // 4n+1 base64 character processed: impossible
                *pfInvalid = true;
                break;

            case 2: // 4n+2 base64 characters processed: require '=='
                if (left || p[0] != '=' || p[1] != '=' || decode64_table[(unsigned char)p[2]] != -1)
                    *pfInvalid = true;
                break;

            case 3: // 4n+3 base64 characters processed: require '='
                if (left || p[0] != '=' || decode64_table[(unsigned char)p[1]] != -1)
                    *pfInvalid = true;
                break;
        }

    return vchRet;
}

std::string DecodeBase64(const std::string& str)
{
    std::vector<unsigned char> vchRet = DecodeBase64(str.c_str());
    return std::string((const char*)vchRet.data(), vchRet.size());
}

std::string EncodeBase32(const unsigned char* pch, size_t len)
{
    static const char *pbase32 = "abcdefghijklmnopqrstuvwxyz234567";

    std::string strRet="";
    strRet.reserve((len+4)/5*8);

    int mode=0, left=0;
    const unsigned char *pchEnd = pch+len;

    while (pch<pchEnd)
    {
        int enc = *(pch++);
        switch (mode)
        {
            case 0: // we have no bits
                strRet += pbase32[enc >> 3];
                left = (enc & 7) << 2;
                mode = 1;
                break;

            case 1: // we have three bits
                strRet += pbase32[left | (enc >> 6)];
                strRet += pbase32[(enc >> 1) & 31];
                left = (enc & 1) << 4;
                mode = 2;
                break;

            case 2: // we have one bit
                strRet += pbase32[left | (enc >> 4)];
                left = (enc & 15) << 1;
                mode = 3;
                break;

            case 3: // we have four bits
                strRet += pbase32[left | (enc >> 7)];
                strRet += pbase32[(enc >> 2) & 31];
                left = (enc & 3) << 3;
                mode = 4;
                break;

            case 4: // we have two bits
                strRet += pbase32[left | (enc >> 5)];
                strRet += pbase32[enc & 31];
                mode = 0;
        }
    }

    static const int nPadding[5] = {0, 6, 4, 3, 1};
    if (mode)
    {
        strRet += pbase32[left];
        for (int n=0; n<nPadding[mode]; n++)
             strRet += '=';
    }

    return strRet;
}

std::string EncodeBase32(const std::string& str)
{
    return EncodeBase32((const unsigned char*)str.c_str(), str.size());
}

std::vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid)
{
    static const int decode32_table[256] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1,
        -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1,  0,  1,  2,
         3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    if (pfInvalid)
        *pfInvalid = false;

    std::vector<unsigned char> vchRet;
    vchRet.reserve((strlen(p))*5/8);

    int mode = 0;
    int left = 0;

    while (1)
    {
         int dec = decode32_table[(unsigned char)*p];
         if (dec == -1) break;
         p++;
         switch (mode)
         {
             case 0: // we have no bits and get 5
                 left = dec;
                 mode = 1;
                 break;

              case 1: // we have 5 bits and keep 2
                  vchRet.push_back((left<<3) | (dec>>2));
                  left = dec & 3;
                  mode = 2;
                  break;

             case 2: // we have 2 bits and keep 7
                 left = left << 5 | dec;
                 mode = 3;
                 break;

             case 3: // we have 7 bits and keep 4
                 vchRet.push_back((left<<1) | (dec>>4));
                 left = dec & 15;
                 mode = 4;
                 break;

             case 4: // we have 4 bits, and keep 1
                 vchRet.push_back((left<<4) | (dec>>1));
                 left = dec & 1;
                 mode = 5;
                 break;

             case 5: // we have 1 bit, and keep 6
                 left = left << 5 | dec;
                 mode = 6;
                 break;

             case 6: // we have 6 bits, and keep 3
                 vchRet.push_back((left<<2) | (dec>>3));
                 left = dec & 7;
                 mode = 7;
                 break;

             case 7: // we have 3 bits, and keep 0
                 vchRet.push_back((left<<5) | dec);
                 mode = 0;
                 break;
         }
    }

    if (pfInvalid)
        switch (mode)
        {
            case 0: // 8n base32 characters processed: ok
                break;

            case 1: // 8n+1 base32 characters processed: impossible
            case 3: //   +3
            case 6: //   +6
                *pfInvalid = true;
                break;

            case 2: // 8n+2 base32 characters processed: require '======'
                if (left || p[0] != '=' || p[1] != '=' || p[2] != '=' || p[3] != '=' || p[4] != '=' || p[5] != '=' || decode32_table[(unsigned char)p[6]] != -1)
                    *pfInvalid = true;
                break;

            case 4: // 8n+4 base32 characters processed: require '===='
                if (left || p[0] != '=' || p[1] != '=' || p[2] != '=' || p[3] != '=' || decode32_table[(unsigned char)p[4]] != -1)
                    *pfInvalid = true;
                break;

            case 5: // 8n+5 base32 characters processed: require '==='
                if (left || p[0] != '=' || p[1] != '=' || p[2] != '=' || decode32_table[(unsigned char)p[3]] != -1)
                    *pfInvalid = true;
                break;

            case 7: // 8n+7 base32 characters processed: require '='
                if (left || p[0] != '=' || decode32_table[(unsigned char)p[1]] != -1)
                    *pfInvalid = true;
                break;
        }

    return vchRet;
}

std::string DecodeBase32(const std::string& str)
{
    std::vector<unsigned char> vchRet = DecodeBase32(str.c_str());
    return std::string((const char*)vchRet.data(), vchRet.size());
}

static bool ParsePrechecks(const std::string& str)
{
    if (str.empty()) // No empty string allowed
        return false;
    if (str.size() >= 1 && (isspace(str[0]) || isspace(str[str.size()-1]))) // No padding allowed
        return false;
    if (str.size() != strlen(str.c_str())) // No embedded NUL characters allowed
        return false;
    return true;
}

bool ParseInt32(const std::string& str, int32_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    char *endp = nullptr;
    errno = 0; // strtol will not set errno if valid
    long int n = strtol(str.c_str(), &endp, 10);
    if(out) *out = (int32_t)n;
    // Note that strtol returns a *long int*, so even if strtol doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *int32_t*. On 64-bit
    // platforms the size of these types may be different.
    return endp && *endp == 0 && !errno &&
        n >= std::numeric_limits<int32_t>::min() &&
        n <= std::numeric_limits<int32_t>::max();
}

bool ParseInt64(const std::string& str, int64_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    char *endp = nullptr;
    errno = 0; // strtoll will not set errno if valid
    long long int n = strtoll(str.c_str(), &endp, 10);
    if(out) *out = (int64_t)n;
    // Note that strtoll returns a *long long int*, so even if strtol doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *int64_t*.
    return endp && *endp == 0 && !errno &&
        n >= std::numeric_limits<int64_t>::min() &&
        n <= std::numeric_limits<int64_t>::max();
}

bool ParseUInt32(const std::string& str, uint32_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    if (str.size() >= 1 && str[0] == '-') // Reject negative values, unfortunately strtoul accepts these by default if they fit in the range
        return false;
    char *endp = nullptr;
    errno = 0; // strtoul will not set errno if valid
    unsigned long int n = strtoul(str.c_str(), &endp, 10);
    if(out) *out = (uint32_t)n;
    // Note that strtoul returns a *unsigned long int*, so even if it doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *uint32_t*. On 64-bit
    // platforms the size of these types may be different.
    return endp && *endp == 0 && !errno &&
        n <= std::numeric_limits<uint32_t>::max();
}

bool ParseUInt64(const std::string& str, uint64_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    if (str.size() >= 1 && str[0] == '-') // Reject negative values, unfortunately strtoull accepts these by default if they fit in the range
        return false;
    char *endp = nullptr;
    errno = 0; // strtoull will not set errno if valid
    unsigned long long int n = strtoull(str.c_str(), &endp, 10);
    if(out) *out = (uint64_t)n;
    // Note that strtoull returns a *unsigned long long int*, so even if it doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *uint64_t*.
    return endp && *endp == 0 && !errno &&
        n <= std::numeric_limits<uint64_t>::max();
}


bool ParseDouble(const std::string& str, double *out)
{
    if (!ParsePrechecks(str))
        return false;
    if (str.size() >= 2 && str[0] == '0' && str[1] == 'x') // No hexadecimal floats allowed
        return false;
    std::istringstream text(str);
    text.imbue(std::locale::classic());
    double result;
    text >> result;
    if(out) *out = result;
    return text.eof() && !text.fail();
}

std::string FormatParagraph(const std::string& in, size_t width, size_t indent)
{
    std::stringstream out;
    size_t ptr = 0;
    size_t indented = 0;
    while (ptr < in.size())
    {
        size_t lineend = in.find_first_of('\n', ptr);
        if (lineend == std::string::npos) {
            lineend = in.size();
        }
        const size_t linelen = lineend - ptr;
        const size_t rem_width = width - indented;
        if (linelen <= rem_width) {
            out << in.substr(ptr, linelen + 1);
            ptr = lineend + 1;
            indented = 0;
        } else {
            size_t finalspace = in.find_last_of(" \n", ptr + rem_width);
            if (finalspace == std::string::npos || finalspace < ptr) {
                // No place to break; just include the entire word and move on
                finalspace = in.find_first_of("\n ", ptr);
                if (finalspace == std::string::npos) {
                    // End of the string, just add it and break
                    out << in.substr(ptr);
                    break;
                }
            }
            out << in.substr(ptr, finalspace - ptr) << "\n";
            if (in[finalspace] == '\n') {
                indented = 0;
            } else if (indent) {
                out << std::string(indent, ' ');
                indented = indent;
            }
            ptr = finalspace + 1;
        }
    }
    return out.str();
}

std::string i64tostr(int64_t n)
{
    return strprintf("%d", n);
}

std::string itostr(int n)
{
    return strprintf("%d", n);
}

int64_t atoi64(const char* psz)
{
#ifdef _MSC_VER
    return _atoi64(psz);
#else
    return strtoll(psz, nullptr, 10);
#endif
}

int64_t atoi64(const std::string& str)
{
#ifdef _MSC_VER
    return _atoi64(str.c_str());
#else
    return strtoll(str.c_str(), nullptr, 10);
#endif
}

int atoi(const std::string& str)
{
    return atoi(str.c_str());
}

/** Upper bound for mantissa.
 * 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
 * Larger integers cannot consist of arbitrary combinations of 0-9:
 *
 *   999999999999999999  1^18-1
 *  9223372036854775807  (1<<63)-1  (max int64_t)
 *  9999999999999999999  1^19-1     (would overflow)
 */
static const int64_t UPPER_BOUND = 1000000000000000000LL - 1LL;

/** Helper function for ParseFixedPoint */
static inline bool ProcessMantissaDigit(char ch, int64_t &mantissa, int &mantissa_tzeros)
{
    if(ch == '0')
        ++mantissa_tzeros;
    else {
        for (int i=0; i<=mantissa_tzeros; ++i) {
            if (mantissa > (UPPER_BOUND / 10LL))
                return false; /* overflow */
            mantissa *= 10;
        }
        mantissa += ch - '0';
        mantissa_tzeros = 0;
    }
    return true;
}

bool ParseFixedPoint(const std::string &val, int decimals, int64_t *amount_out)
{
    int64_t mantissa = 0;
    int64_t exponent = 0;
    int mantissa_tzeros = 0;
    bool mantissa_sign = false;
    bool exponent_sign = false;
    int ptr = 0;
    int end = val.size();
    int point_ofs = 0;

    if (ptr < end && val[ptr] == '-') {
        mantissa_sign = true;
        ++ptr;
    }
    if (ptr < end)
    {
        if (val[ptr] == '0') {
            /* pass single 0 */
            ++ptr;
        } else if (val[ptr] >= '1' && val[ptr] <= '9') {
            while (ptr < end && val[ptr] >= '0' && val[ptr] <= '9') {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
            }
        } else return false; /* missing expected digit */
    } else return false; /* empty string or loose '-' */
    if (ptr < end && val[ptr] == '.')
    {
        ++ptr;
        if (ptr < end && val[ptr] >= '0' && val[ptr] <= '9')
        {
            while (ptr < end && val[ptr] >= '0' && val[ptr] <= '9') {
                if (!ProcessMantissaDigit(val[ptr], mantissa, mantissa_tzeros))
                    return false; /* overflow */
                ++ptr;
                ++point_ofs;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr < end && (val[ptr] == 'e' || val[ptr] == 'E'))
    {
        ++ptr;
        if (ptr < end && val[ptr] == '+')
            ++ptr;
        else if (ptr < end && val[ptr] == '-') {
            exponent_sign = true;
            ++ptr;
        }
        if (ptr < end && val[ptr] >= '0' && val[ptr] <= '9') {
            while (ptr < end && val[ptr] >= '0' && val[ptr] <= '9') {
                if (exponent > (UPPER_BOUND / 10LL))
                    return false; /* overflow */
                exponent = exponent * 10 + val[ptr] - '0';
                ++ptr;
            }
        } else return false; /* missing expected digit */
    }
    if (ptr != end)
        return false; /* trailing garbage */

    /* finalize exponent */
    if (exponent_sign)
        exponent = -exponent;
    exponent = exponent - point_ofs + mantissa_tzeros;

    /* finalize mantissa */
    if (mantissa_sign)
        mantissa = -mantissa;

    /* convert to one 64-bit fixed-point value */
    exponent += decimals;
    if (exponent < 0)
        return false; /* cannot represent values smaller than 10^-decimals */
    if (exponent >= 18)
        return false; /* cannot represent values larger than or equal to 10^(18-decimals) */

    for (int i=0; i < exponent; ++i) {
        if (mantissa > (UPPER_BOUND / 10LL) || mantissa < -(UPPER_BOUND / 10LL))
            return false; /* overflow */
        mantissa *= 10;
    }
    if (mantissa > UPPER_BOUND || mantissa < -UPPER_BOUND)
        return false; /* overflow */

    if (amount_out)
        *amount_out = mantissa;

    return true;
}

#endif