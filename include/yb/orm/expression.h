// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__EXPRESSION__INCLUDED
#define YB__ORM__EXPRESSION__INCLUDED

#include <vector>
#include <string>
#include <set>
#include <map>
#include "util/utility.h"
#include "util/exception.h"
#include "util/value_type.h"
#include "orm_config.h"

namespace Yb {

typedef std::map<String, int> ParamNums;

class YBORM_DECL ExpressionBackend: public RefCountBase
{
public:
    virtual const String generate_sql(
            Values *params, int *count) const = 0;
    virtual ~ExpressionBackend();
};

typedef IntrusivePtr<ExpressionBackend> ExprBEPtr;

class Column;

class YBORM_DECL Expression
{
protected:
    ExprBEPtr backend_;
    String sql_;
public:
    Expression();
    explicit Expression(const String &sql);
    Expression(const Column &col);
    Expression(ExprBEPtr backend);
    const String generate_sql(
            Values *params, int *count = NULL) const;
    const String get_sql() const { return generate_sql(NULL); }
    bool is_empty() const { return str_empty(sql_) && !backend_.get(); }
    ExpressionBackend *backend() const { return backend_.get(); }
};

YBORM_DECL bool is_number_or_object_name(const String &s);
YBORM_DECL bool is_string_constant(const String &s);
YBORM_DECL bool is_in_parentheses(const String &s);
YBORM_DECL const String sql_parentheses_as_needed(const String &s);
YBORM_DECL const String sql_prefix(const String &s, const String &prefix);
YBORM_DECL const String sql_alias(const String &s, const String &alias);

class YBORM_DECL ColumnExprBackend: public ExpressionBackend
{
    Expression expr_;
    String tbl_name_, col_name_, alias_;
public:
    ColumnExprBackend(const Expression &expr, const String &alias);
    ColumnExprBackend(const String &tbl_name, const String &col_name,
            const String &alias);
    const String generate_sql(Values *params, int *count) const;
    const String &alias() const { return alias_; }
};

class YBORM_DECL ColumnExpr: public Expression
{
public:
    ColumnExpr(const Expression &expr, const String &alias = _T(""));
    ColumnExpr(const String &tbl_name, const String &col_name,
            const String &alias = _T(""));
    const String &alias() const;
};

class YBORM_DECL ConstExprBackend: public ExpressionBackend
{
    Value value_;
public:
    ConstExprBackend(const Value &x);
    const String generate_sql(Values *params, int *count) const;
    const Value &const_value() const { return value_; }
};

class YBORM_DECL ConstExpr: public Expression
{
public:
    ConstExpr();
    ConstExpr(const Value &x);
    const Value &const_value() const;
};

class YBORM_DECL UnaryOpExprBackend: public ExpressionBackend
{
    bool prefix_;
    String op_;
    Expression expr_;
public:
    UnaryOpExprBackend(bool prefix, const String &op, const Expression &expr);
    const String generate_sql(Values *params, int *count) const;
    bool prefix() const { return prefix_; }
    const String &op() const { return op_; }
    const Expression &expr() const { return expr_; }
};

class YBORM_DECL UnaryOpExpr: public Expression
{
public:
    UnaryOpExpr(bool prefix, const String &op, const Expression &expr);
    bool prefix() const;
    const String &op() const;
    const Expression &expr() const;
};

class YBORM_DECL BinaryOpExprBackend: public ExpressionBackend
{
    Expression expr1_, expr2_;
    String op_;
public:
    BinaryOpExprBackend(const Expression &expr1,
            const String &op, const Expression &expr2);
    const String generate_sql(Values *params, int *count) const;
    const String &op() const { return op_; }
    const Expression &expr1() const { return expr1_; }
    const Expression &expr2() const { return expr2_; }
};

class YBORM_DECL BinaryOpExpr: public Expression
{
public:
    BinaryOpExpr(const Expression &expr1,
            const String &op, const Expression &expr2);
    const String &op() const;
    const Expression &expr1() const;
    const Expression &expr2() const;
};

class YBORM_DECL JoinExprBackend: public ExpressionBackend
{
    Expression expr1_, expr2_, cond_;
public:
    JoinExprBackend(const Expression &expr1,
            const Expression &expr2, const Expression &cond)
        : expr1_(expr1), expr2_(expr2), cond_(cond) {}
    const String generate_sql(Values *params, int *count) const;
    const Expression &expr1() const { return expr1_; }
    const Expression &expr2() const { return expr2_; }
    const Expression &cond() const { return cond_; }
};

class YBORM_DECL JoinExpr: public Expression
{
public:
    JoinExpr(const Expression &expr1,
            const Expression &expr2, const Expression &cond);
    const Expression &expr1() const;
    const Expression &expr2() const;
    const Expression &cond() const;
};

class YBORM_DECL ExpressionListBackend: public ExpressionBackend
{
    std::vector<Expression> items_;
public:
    ExpressionListBackend() {}
    void append(const Expression &expr) { items_.push_back(expr); }
    const String generate_sql(Values *params, int *count) const;
    int size() const { return items_.size(); }
    const Expression &item(int n) const {
        YB_ASSERT(n >= 0 && (size_t)n < items_.size());
        return items_[n];
    }
};

class YBORM_DECL ExpressionList: public Expression
{
    template <typename T>
    void fill_from_container(const T &cont)
    {
        typename T::const_iterator it = cont.begin(), end = cont.end();
        for (; it != end; ++it)
            append(Expression(*it));
    }
public:
    ExpressionList();
    ExpressionList(const Expression &expr);
    ExpressionList(const Expression &expr1, const Expression &expr2);
    ExpressionList(const Expression &expr1, const Expression &expr2,
            const Expression &expr3);
    ExpressionList(const Strings &cont)
        : Expression(ExprBEPtr(new ExpressionListBackend))
    {
        fill_from_container<Strings>(cont);
    }
    ExpressionList(const StringSet &cont)
        : Expression(ExprBEPtr(new ExpressionListBackend))
    {
        fill_from_container<StringSet>(cont);
    }
    void append(const Expression &expr);
    ExpressionList &operator << (const Expression &expr) {
        append(expr);
        return *this;
    }
    int size() const;
    const Expression &item(int n) const;
    const Expression &operator [] (int n) const { return item(n); }
};

class YBORM_DECL SelectExprBackend: public ExpressionBackend
{
    Expression select_expr_, from_expr_, where_expr_,
               group_by_expr_, having_expr_, order_by_expr_;
    bool distinct_flag_;
    String lock_mode_;
public:
    SelectExprBackend(const Expression &select_expr)
        : select_expr_(select_expr)
        , distinct_flag_(false)
    {}
    void from_(const Expression &from_expr) { from_expr_ = from_expr; }
    void where_(const Expression &where_expr) { where_expr_ = where_expr; }
    void group_by_(const Expression &group_by_expr) { group_by_expr_ = group_by_expr; }
    void having_(const Expression &having_expr) { having_expr_ = having_expr; }
    void order_by_(const Expression &order_by_expr) { order_by_expr_ = order_by_expr; }
    void distinct(bool flag) { distinct_flag_ = flag; }
    void with_lockmode(const String &lock_mode) { lock_mode_ = lock_mode; }
    const Expression &select_expr() const { return select_expr_; }
    const Expression &from_expr() const { return from_expr_; }
    const Expression &where_expr() const { return where_expr_; }
    const Expression &group_by_expr() const { return group_by_expr_; }
    const Expression &having_expr() const { return having_expr_; }
    const Expression &order_by_expr() const { return order_by_expr_; }
    bool distinct_flag() const { return distinct_flag_; }
    const String &lock_mode() const { return lock_mode_; }
    const String generate_sql(Values *params, int *count) const;
};

class YBORM_DECL SelectExpr: public Expression
{
public:
    SelectExpr(const Expression &select_expr);
    SelectExpr &from_(const Expression &from_expr);
    SelectExpr &where_(const Expression &where_expr);
    SelectExpr &group_by_(const Expression &group_by_expr);
    SelectExpr &having_(const Expression &having_expr);
    SelectExpr &order_by_(const Expression &order_by_expr);
    SelectExpr &distinct(bool flag = true);
    SelectExpr &with_lockmode(const String &lock_mode);
    SelectExpr &for_update(bool flag = true);
    const Expression &select_expr() const;
    const Expression &from_expr() const;
    const Expression &where_expr() const;
    const Expression &group_by_expr() const;
    const Expression &having_expr() const;
    const Expression &order_by_expr() const;
    bool distinct_flag() const;
    const String &lock_mode() const;
    bool for_update_flag() const;
};

YBORM_DECL const Expression operator ! (const Expression &a);
YBORM_DECL const Expression operator && (const Expression &a, const Expression &b);
YBORM_DECL const Expression operator || (const Expression &a, const Expression &b);

YBORM_DECL const Expression operator == (const Expression &a, const Expression &b);
YBORM_DECL const Expression operator == (const Expression &a, const Value &b);
YBORM_DECL const Expression operator == (const Value &a, const Expression &b);
YBORM_DECL const Expression operator == (const Expression &a, const Column &b);
YBORM_DECL const Expression operator == (const Column &a, const Expression &b);
YBORM_DECL const Expression operator == (const Column &a, const Column &b);
YBORM_DECL const Expression operator == (const Column &a, const Value &b);
YBORM_DECL const Expression operator == (const Value &a, const Column &b);
YBORM_DECL const Expression operator != (const Expression &a, const Expression &b);
YBORM_DECL const Expression operator != (const Expression &a, const Value &b);
YBORM_DECL const Expression operator != (const Value &a, const Expression &b);
YBORM_DECL const Expression operator != (const Expression &a, const Column &b);
YBORM_DECL const Expression operator != (const Column &a, const Expression &b);
YBORM_DECL const Expression operator != (const Column &a, const Column &b);
YBORM_DECL const Expression operator != (const Column &a, const Value &b);
YBORM_DECL const Expression operator != (const Value &a, const Column &b);
YBORM_DECL const Expression operator > (const Expression &a, const Expression &b);
YBORM_DECL const Expression operator > (const Expression &a, const Value &b);
YBORM_DECL const Expression operator > (const Value &a, const Expression &b);
YBORM_DECL const Expression operator > (const Expression &a, const Column &b);
YBORM_DECL const Expression operator > (const Column &a, const Expression &b);
YBORM_DECL const Expression operator > (const Column &a, const Column &b);
YBORM_DECL const Expression operator > (const Column &a, const Value &b);
YBORM_DECL const Expression operator > (const Value &a, const Column &b);
YBORM_DECL const Expression operator < (const Expression &a, const Expression &b);
YBORM_DECL const Expression operator < (const Expression &a, const Value &b);
YBORM_DECL const Expression operator < (const Value &a, const Expression &b);
YBORM_DECL const Expression operator < (const Expression &a, const Column &b);
YBORM_DECL const Expression operator < (const Column &a, const Expression &b);
YBORM_DECL const Expression operator < (const Column &a, const Column &b);
YBORM_DECL const Expression operator < (const Column &a, const Value &b);
YBORM_DECL const Expression operator < (const Value &a, const Column &b);
YBORM_DECL const Expression operator >= (const Expression &a, const Expression &b);
YBORM_DECL const Expression operator >= (const Expression &a, const Value &b);
YBORM_DECL const Expression operator >= (const Value &a, const Expression &b);
YBORM_DECL const Expression operator >= (const Expression &a, const Column &b);
YBORM_DECL const Expression operator >= (const Column &a, const Expression &b);
YBORM_DECL const Expression operator >= (const Column &a, const Column &b);
YBORM_DECL const Expression operator >= (const Column &a, const Value &b);
YBORM_DECL const Expression operator >= (const Value &a, const Column &b);
YBORM_DECL const Expression operator <= (const Expression &a, const Expression &b);
YBORM_DECL const Expression operator <= (const Expression &a, const Value &b);
YBORM_DECL const Expression operator <= (const Value &a, const Expression &b);
YBORM_DECL const Expression operator <= (const Expression &a, const Column &b);
YBORM_DECL const Expression operator <= (const Column &a, const Expression &b);
YBORM_DECL const Expression operator <= (const Column &a, const Column &b);
YBORM_DECL const Expression operator <= (const Column &a, const Value &b);
YBORM_DECL const Expression operator <= (const Value &a, const Column &b);

inline const Expression filter_eq(const String &name, const Value &value) {
    return Expression(name) == value;
}
inline const Expression filter_ne(const String &name, const Value &value) {
    return Expression(name) != value;
}
inline const Expression filter_lt(const String &name, const Value &value) {
    return Expression(name) < value;
}
inline const Expression filter_gt(const String &name, const Value &value) {
    return Expression(name) > value;
}
inline const Expression filter_le(const String &name, const Value &value) {
    return Expression(name) <= value;
}
inline const Expression filter_ge(const String &name, const Value &value) {
    return Expression(name) >= value;
}

class YBORM_DECL FilterBackendByPK: public ExpressionBackend
{
    Expression expr_;
    Key key_;
    static const Expression build_expr(const Key &key);
public:
    FilterBackendByPK(const Key &key);
    const String generate_sql(Values *params, int *count) const;
    const Key &key() const { return key_; }
};

class YBORM_DECL KeyFilter: public Expression
{
public:
    KeyFilter(const Key &key);
    const Key &key() const;
};

typedef Expression Filter;

class Schema;

YBORM_DECL void find_all_tables(const Expression &expr, Strings &tables);
YBORM_DECL SelectExpr make_select(const Schema &schema, const Expression &from_where,
        const Expression &filter, const Expression &order_by,
        bool for_update_flag = false);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__EXPRESSION__INCLUDED
