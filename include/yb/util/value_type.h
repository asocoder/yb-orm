// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__VALUE_TYPE__INCLUDED
#define YB__UTIL__VALUE_TYPE__INCLUDED

#include <string.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include "util_config.h"
#include "data_types.h"
#include "nlogger.h"

namespace Yb {

class YBUTIL_DECL ValueIsNull: public ValueError
{
public:
    ValueIsNull();
};

template <class T__> struct ValueTraits {};
template <> struct ValueTraits<int> { enum { TYPE_CODE = 1 }; };
template <> struct ValueTraits<LongInt> { enum { TYPE_CODE = 2 }; };
template <> struct ValueTraits<String> { enum { TYPE_CODE = 3 }; };
template <> struct ValueTraits<Decimal> { enum { TYPE_CODE = 4 }; };
template <> struct ValueTraits<DateTime> { enum { TYPE_CODE = 5 }; };
template <> struct ValueTraits<double> { enum { TYPE_CODE = 6 }; };

#define YB_MAX(x, y) ((x) > (y)? (x): (y))

//! Variant data type for communication to the database layer
/** Value class objects can hold NULL values.  Copying such objects
 * is cheap because of shared pointer.
 * Value class also supports casting to several strict types.
 * 
 * @remark Value class should not be implemented upon boost::any because of massive
 * copying of Value objects here and there.
 */
class YBUTIL_DECL Value
{
    template <class T__>
    T__ &get_as() {
        return *reinterpret_cast<T__ *>(&bytes_[0]);
    }
    template <class T__>
    const T__ &get_as() const {
        return *reinterpret_cast<const T__ *>(&bytes_[0]);
    }
    template <class T__>
    void cons_as() { new (&bytes_[0]) T__(); }
    template <class T__>
    void copy_as(const T__ &x) { new (&bytes_[0]) T__(x); }
    template <class T__>
    void des_as() { get_as<T__>().~T__(); }

    void init();
    void destroy();
    void assign(const Value &other);
public:
    enum Type {
        INVALID = 0, INTEGER, LONGINT, STRING, DECIMAL, DATETIME, FLOAT
    };
    Value();
    Value(const int &x);
    Value(const LongInt &x);
    Value(const double &x);
    Value(const Decimal &x);
    Value(const DateTime &x);
    Value(const String &x);
    Value(const Char *x);
    Value(const Value &other);
    Value &operator=(const Value &other);
    ~Value();
    void swap(Value &other);
    void fix_type(int type);

    int as_integer() const;
    LongInt as_longint() const;
    const String as_string() const;
    const Decimal as_decimal() const;
    const DateTime as_date_time() const;
    double as_float() const;
    const String sql_str() const;
    const Value nvl(const Value &def_value) const;
    int cmp(const Value &x) const;
    bool is_null() const { return type_ == INVALID; }
    int get_type() const { return type_; }
    static const String get_type_name(int type);

    template <class T__>
    const T__ &read_as() const {
        YB_ASSERT((int)type_ == (int)ValueTraits<T__>::TYPE_CODE);
        return get_as<T__>();
    }
private:
    int type_;
    char bytes_[YB_MAX(sizeof(int), YB_MAX(sizeof(LongInt),
                YB_MAX(sizeof(String), YB_MAX(sizeof(Decimal),
                YB_MAX(sizeof(DateTime), sizeof(double))))))];
};

//! @name Overloaded operations for Value class
//! @{
inline bool operator==(const Value &x, const Value &y) { return !x.cmp(y); }
inline bool operator!=(const Value &x, const Value &y) { return x.cmp(y) != 0; }
inline bool operator<(const Value &x, const Value &y) { return x.cmp(y) < 0; }
inline bool operator>=(const Value &x, const Value &y) { return x.cmp(y) >= 0; }
inline bool operator>(const Value &x, const Value &y) { return x.cmp(y) > 0; }
inline bool operator<=(const Value &x, const Value &y) { return x.cmp(y) <= 0; }
//! @}

typedef std::vector<Value> Values;
typedef const Values *RowDataPtr;
typedef std::vector<RowDataPtr> RowsData;
typedef std::vector<std::pair<String, Value> > ValueMap;
typedef std::pair<String, ValueMap> Key;
typedef std::vector<Key> Keys;

YBUTIL_DECL bool empty_key(const Key &key);

//! @name Casting from variant typed to certain type
//! @{
template <class T>
inline T &from_variant(const Value &x, T &t) {
    return from_string(x.as_string(), t);
}

template <>
inline int &from_variant(const Value &x, int &t) {
    t = x.as_integer();
    return t;
}

template <>
inline LongInt &from_variant(const Value &x, LongInt &t) {
    t = x.as_longint();
    return t;
}

template <>
inline Decimal &from_variant(const Value &x, Decimal &t) {
    t = x.as_decimal();
    return t;
}

template <>
inline DateTime &from_variant(const Value &x, DateTime &t) {
    t = x.as_date_time();
    return t;
}

template <>
inline double &from_variant(const Value &x, double &t) {
    t = x.as_float();
    return t;
}
//! @}

} // namespace Yb

namespace std {

template<>
inline void swap(::Yb::Value &x, ::Yb::Value &y) { x.swap(y); }

} // namespace std

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__VALUE_TYPE__INCLUDED
