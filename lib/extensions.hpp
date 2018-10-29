/*
 * Created by Dmitry Lyssenko
 *
 * Macro definitions for ENUM stringification for in-class enum declarations:
 *
 * ENUM - will declare enum type w/o any possibility for stringification (mimics enum declaration)
 * ENUMSTR - will declare enum type with a possibility for later stringification
 * STRINGIFY - stringification of previously declared enum type with ENUMSTR macro
 *
 * trivial macro definitions making enum stringification easy:
 * declare enums now in a few simple steps:
 *  1. define a macro enlisting all enum items:
 *          #define MY_COLORS \
 *                  Red, \
 *                  Amber, \
 *                  Green
 *
 *  2. declare enums with ENUM macro if non-class declaration, or with ENUMSTR for in-class:
 *          ENUMSTR(trafficLightColors, MY_COLORS)
 *     - ENUM macro only declares trafficLightColors of enums, while ENUMSTR in addition
 *       declares an array of: static const char* trafficLightColors_[]
 *
 *  3. initialize an array of c-strings to generated by STRINGIFY macro (non in-class):
 *          STRINGIFY(SomeClass::trafficLightColors, MY_COLORS)
 *     - declares STRINGIFY macro outside of class, for non in-class declaration drop the
 *     cдass qualifier:
 *          STRINGIFY(trafficLightColors, MY_COLORS)
 *
 *  Now, enum trafficLightColors is defined, as well as its c-string representation:
 *          cout << "All traffic-light colors:";
 *          for(int i=0; i<COUNT_ARGS(MY_COLORS); ++i)
 *           cout << ' ' << SomeClass::trafficLightColors_str[i];
 *           // or equally: cout << ' ' << STREN(trafficLightColors, i);
 *          cout << endl;
 *
 * Obvious caveat: enums declared that way do not allow value re-definition
 * However, consider enum value redefinition a bad programming style.
 * If required, provide enum-remapping facility instead
 */

#pragma once
#include <string>
#include <vector>
#include "macrolib.h"




#define __COMMA_SEPARATED__(x) x,
#define __STR_COMMA_SEPARATED__(x) #x,


#define ENUM(enum_class, enums...) \
  enum enum_class { MACRO_TO_ARGS(__COMMA_SEPARATED__, enums) };


#define ENUMSTR(enum_class, enums...) \
  enum enum_class { MACRO_TO_ARGS(__COMMA_SEPARATED__, enums) }; \
  static const char * enum_class ## _str[];


#define STRINGIFY(enum_class, enums...) \
  const char * enum_class ## _str[] { MACRO_TO_ARGS(__STR_COMMA_SEPARATED__, enums) };

#define ENUMS(enum_class, enum_idx) enum_class ## _str[enum_idx]





/*
 * Following trivial extension facilitates ability to check if value is in enumeration
 * or not (similar to python's "x in [....]" construct)
 *
 * Synopsis:
 *
 * x = 5;
 * if(x AMONG(1,2,3,5,6))
 *  std::cout << "x is in!" << std::endl;
 * else
 *  std::cout << "x is not in!" << std::endl;
 *
 *
 * CAVEAT on usage with c-strings:
 * - first parameter in AMONG macros to be type-casted as:
 *
 * const char *x = "abc";
 * if(x AMONG(static_cast<const char*>("abc"), "def", "xyz")) ...
 */



template<class T>
bool operator==(const T &a, std::vector<T> v) {
 for(auto &x: v)
  if(x == a) return true;
 return false;
}

bool operator==(const std::string &a, std::vector<const char *> b) {
 for(auto x: b)
  if(a == x) return true;
 return false;
}

#define AMONG(first, rest...) \
        ==std::vector<decltype(first)>{first, MACRO_TO_ARGS(__COMMA_SEPARATED__, rest)}





/*
 * A trivial wrapper around std::exception
 * - to be used with enum stringification in classes (ENUMSTR, STRINGIFY)
 *
 * Synopsis:
 * // 1. define ENUMSTR within the class, enumerating exception reasons
 *
 * class myClass {
 *   public:
 *   ...
 *  define THROWREASON
 *          InvalidInput, \
 *          IncorrectUsage, \
 *          WrongType
 *  ENUMSTR(ThrowReason, THROWREASON)
 *
 *   ...
 *  EXCEPTION(ThrowReason)
 * };
 * STRINGIFY(myClass::ThrowReason, THROWREASON)
 * #undef THROWREASON
 *
 *
 * // 2. use in throwing defined exception reasons, e.g.:
 * throw EXP(InvalidInput);
 *
 *
 * // 3. Possible output upon non-handled exception:
 * libc++abi.dylib: terminating with uncaught exception of type myClass::stdException: InvalidInput
 *
 *
 * // 4. define catching exception:
 *  try { ... }     // something producing ThrowReason exception
 *  catch(myClass::stdException & e) {  // or std::exception & e, but then to access code()
 *                                      // and where() down-casting is required
 *   std::cout << "exception string: " << e.what() << std::endl;
 *   std::cout << "exception code: " << e.code() << std::endl;
 *   std::cout << "exception in: " << e.where() << std::endl;
 *  }
 *
 */


// return std::exception from classes;
// upon throwing a copy of the object is made (throwing is by value). In our case
// a shallow copy will suffice despite the pointer: the class is meant to be used
// with ENUMSTR / STRINGIFY, which are static data.
// for in-class declaration:
#define EXCEPTIONS(THROW_ENUM) \
    class stdException: public std::exception { \
     public: \
                            stdException(void) = delete; \
                            stdException(int reason, const char *what, \
                                         const char *func, const char *file, int line): \
                             ec_{reason}, msg_{what}, func_{func}, file_{file}, line_{line} {} \
        const char *        what(void) const noexcept { return msg_; } \
        std::string         where(void) const noexcept { \
                             return std::string{"file: '"} + file_ + \
                                    "', func: '" + func_ + \
                                    "()', line: " + std::to_string(line_); \
                            } \
        const char *        func(void) const noexcept { return func_; } \
        const char *        file(void) const noexcept { return file_; } \
        int                 line(void) const noexcept { return line_; } \
        int                 code(void) const noexcept { return ec_; } \
     private: \
        int                 ec_; \
        const char *        msg_; \
        const char *        func_; \
        const char *        file_; \
        int                 line_; \
    }; \
    stdException __exp__(THROW_ENUM reason, const char *func, const char *file, int line) const \
        { return stdException{reason, ENUMS(THROW_ENUM, reason), func, file, line}; }


// for in-place throw parameter
#define EXP(TROW_REASON) __exp__(TROW_REASON, __func__, __FILE__, __LINE__)




















