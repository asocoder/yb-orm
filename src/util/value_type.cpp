// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include <time.h>
#include <stdio.h>
#include <iomanip>
#include "util/string_utils.h"
#include "util/value_type.h"

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

ValueIsNull::ValueIsNull()
    : ValueError(_T("Trying to get value of null"))
{}

void
Value::init()
{
    switch (type_) {
    case STRING:
        cons_as<String>();
        break;
    case DECIMAL:
        cons_as<Decimal>();
        break;
    case DATETIME:
        cons_as<DateTime>();
        break;
    }
}

void
Value::destroy()
{
    switch (type_) {
    case STRING:
        des_as<String>();
        break;
    case DECIMAL:
        des_as<Decimal>();
        break;
    case DATETIME:
        des_as<DateTime>();
        break;
    }
}

void
Value::assign(const Value &other)
{
    if (type_ != other.type_) {
        destroy();
        type_ = other.type_;
        init();
    }
    switch (type_) {
    case INTEGER:
        get_as<int>() = other.get_as<int>();
        break;
    case LONGINT:
        get_as<LongInt>() = other.get_as<LongInt>();
        break;
    case STRING:
        get_as<String>() = other.get_as<String>();
        break;
    case DECIMAL:
        get_as<Decimal>() = other.get_as<Decimal>();
        break;
    case DATETIME:
        get_as<DateTime>() = other.get_as<DateTime>();
        break;
    case FLOAT:
        get_as<double>() = other.get_as<double>();
        break;
    }
}

Value::Value()
    : type_(INVALID)
{}

Value::Value(const int &x)
    : type_(INTEGER)
{
    copy_as<int>(x);
}

Value::Value(const LongInt &x)
    : type_(LONGINT)
{
    copy_as<LongInt>(x);
}

Value::Value(const double &x)
    : type_(FLOAT)
{
    copy_as<double>(x);
}

Value::Value(const Decimal &x)
    : type_(DECIMAL)
{
    copy_as<Decimal>(x);
}

Value::Value(const DateTime &x)
    : type_(DATETIME)
{
    copy_as<DateTime>(x);
}

Value::Value(const String &x)
    : type_(STRING)
{
    copy_as<String>(x);
}

Value::Value(const Char *x)     : type_(STRING)
    { copy_as<String>(str_from_chars(x)); }
Value::Value(const Value &other)
    : type_(INVALID)
{
    memset(bytes_, 0, sizeof(bytes_));
    assign(other);
}

Value &
Value::operator=(const Value &other)
{
    if (this != &other)
        assign(other);
    return *this;
}

Value::~Value()
{
    destroy();
}

void
Value::swap(Value &other)
{
    if (this == &other)
        return;
    type_ ^= other.type_;
    other.type_ ^= type_;
    type_ ^= other.type_;
    char *p1 = &bytes_[0], *p2 = &other.bytes_[0];
    char *limit1 = p1 + sizeof(bytes_);
    for (; p1 != limit1; ++p1, ++p2) {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }
}

void
Value::fix_type(int type)
{
    if (type_ == type || type_ == INVALID)
        return;
    switch (type) {
    case Value::INVALID:
        {
            destroy();
            type_ = type;
        }
        break;
    case Value::INTEGER:
        {
            int t = as_integer();
            destroy();
            type_ = type;
            get_as<int>() = t;
        }
        break;
    case Value::LONGINT:
        {
            LongInt t = as_longint();
            destroy();
            type_ = type;
            get_as<LongInt>() = t;
        }
        break;
    case Value::STRING:
        {
            String t = as_string();
            destroy();
            type_ = type;
            init();
            get_as<String>() = t;
        }
        break;
    case Value::DECIMAL:
        {
            Decimal t = as_decimal();
            destroy();
            type_ = type;
            init();
            get_as<Decimal>() = t;
        }
        break;
    case Value::DATETIME:
        {
            DateTime t = as_date_time();
            destroy();
            type_ = type;
            init();
            get_as<DateTime>() = t;
        }
        break;
    case Value::FLOAT:
        {
            double t = as_float();
            destroy();
            type_ = type;
            get_as<double>() = t;
        }
        break;
    }
}

int
Value::as_integer() const
{
    if (type_ == INTEGER)
        return get_as<int>();
    if (type_ == LONGINT) {
        LongInt x = get_as<LongInt>();
        LongInt m = 0xFFFF;
        m = (m << 16) | m;
        LongInt h = (x >> 32) & m;
        if (h && h != m)
            throw ValueBadCast(to_string(x) + _T("LL"),
                    _T("Integer"));
        return (int)x;
    }
    String s = as_string();
    try {
        int x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
        throw ValueBadCast(s, _T("Integer"));
    }
}

LongInt
Value::as_longint() const
{
    if (type_ == LONGINT)
        return get_as<LongInt>();
    if (type_ == INTEGER)
        return get_as<int>();
    String s = as_string();
    try {
        LongInt x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
        throw ValueBadCast(s, _T("LongInt"));
    }
}

const Decimal
Value::as_decimal() const
{
    if (type_ == DECIMAL)
        return get_as<Decimal>();
    String s = as_string();
    try {
        return Decimal(s);
    }
    catch (const DecimalException &) {
        double f = 0.0;
        try {
            from_string(s, f);
        }
        catch (const std::exception &) {
            throw ValueBadCast(s, _T("Decimal"));
        }
        std::ostringstream o;
        o << setprecision(10) << fixed << f;
        try {
            return Decimal(WIDEN(o.str()));
        }
        catch (const DecimalException &) {
            throw ValueBadCast(s, _T("Decimal"));
        }
    }
}

const DateTime
Value::as_date_time() const
{
    if (type_ == DATETIME)
        return get_as<DateTime>();
    String s = as_string();
    try {
        DateTime x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
        time_t t;
        from_string(s, t);
        tm stm;
        if (!localtime_safe(&t, &stm))
            throw ValueBadCast(s, _T("struct tm"));
        return dt_make(stm.tm_year + 1900,
                stm.tm_mon + 1, stm.tm_mday,
                stm.tm_hour, stm.tm_min, stm.tm_sec);
    }
}

double
Value::as_float() const
{
    if (type_ == FLOAT)
        return get_as<double>();
    String s = as_string();
    try {
        double x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
        throw ValueBadCast(s, _T("Float"));
    }
}

const String
Value::as_string() const
{
    switch (type_) {
    case INVALID:
        throw ValueIsNull();
    case INTEGER:
        return to_string(get_as<int>());
    case LONGINT:
        return to_string(get_as<LongInt>());
    case STRING:
        return get_as<String>();
    case DECIMAL:
        return to_string(get_as<Decimal>());
    case DATETIME:
        return to_string(get_as<DateTime>());
    case FLOAT:
        return to_string(get_as<double>());
    }
    throw ValueBadCast(_T("UnknownType"), _T("Value::as_string()"));
}

const String
Value::sql_str() const
{
    if (is_null())
        return _T("NULL");
    if (type_ == STRING)
        return quote(sql_string_escape(get_as<String>()));
    else if (type_ == DATETIME) {
        String t(to_string(get_as<DateTime>()));
        int pos = str_find(t, _T('T'));
        if (pos != -1)
            t[pos] = _T(' ');
        return _T("'") + t + _T("'");
    }
    return as_string();
}

const Value
Value::nvl(const Value &def_value) const
{
    return is_null()? def_value: *this;
}

int
Value::cmp(const Value &x) const
{
    if (is_null() && x.is_null())
        return 0;
    if (is_null())
        return -1;
    if (x.is_null())
        return 1;
    if (type_ == STRING && x.type_ == STRING)
        return get_as<String>().compare(x.get_as<String>());
    if ((type_ == LONGINT || type_ == INTEGER) &&
        (x.type_ == LONGINT || x.type_ == INTEGER))
    {
        LongInt a, b;
        if (type_ == INTEGER)
            a = get_as<int>();
        else
            a = get_as<LongInt>();
        if (x.type_ == INTEGER)
            b = x.get_as<int>();
        else
            b = x.get_as<LongInt>();
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == DATETIME && x.type_ == DATETIME)
    {
        const DateTime &a = get_as<DateTime>(), &b = x.get_as<DateTime>();
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == DECIMAL && x.type_ == DECIMAL)
        return get_as<Decimal>().cmp(x.get_as<Decimal>());
    if (type_ == FLOAT && x.type_ == FLOAT)
    {
        const double &a = get_as<double>(), &b = x.get_as<double>();
        return a < b? -1: (a > b? 1: 0);
    }
    return as_string().compare(x.as_string());
}

const String
Value::get_type_name(int type)
{
    static const char *type_names[] =
        { "Invalid", "Integer", "LongInt",
          "String", "Decimal", "DateTime", "Float" };
    if (type < 0 || type >= sizeof(type_names)/sizeof(const char *))
        return WIDEN("UnknownType");
    return WIDEN(type_names[type]);
}

YBUTIL_DECL bool
empty_key(const Key &key)
{
    ValueMap::const_iterator i = key.second.begin(), iend = key.second.end();
    for (; i != iend; ++i)
        if (i->second.is_null())
            return false;
    return true;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
