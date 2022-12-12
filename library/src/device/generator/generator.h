// Copyright (C) 2021 - 2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include <algorithm>
#include <any>
#include <iostream>
#include <numeric>
#include <optional>
#include <string.h>
#include <string>
#include <variant>
#include <vector>

//
// Helpers
//

template <typename T>
std::string vrender(const T& x)
{
    return std::visit([](const auto& a) { return a.render(); }, x);
}

template <typename T>
unsigned int get_precedence(const T& x)
{
    return std::visit([](const auto& a) { return a.precedence; }, x);
}

// We have a circular dependency here when trying to use std::variant
// for our abstract syntax tree, e.g.:
//
// - Expression variants contains classes like Variable, Literal,
//   Add, Subtract
// - Those classes can themselves contain Expressions
//
// But, std::variant and std::vector can both use incomplete types,
// so long as we don't call any methods on the containers while
// the types are still incomplete.
//
// So we can resolve this:
// - forward-declare all classes that can go into the variant
// - declare the variant type in terms of the classes
// - declare all of the classes
//   - if they require Expression members, put them in a std::vector
//   - don't implement any method bodies that would call std::vector methods
//     (including constructors)
// - after all classes are declared, implement all remaining class methods

enum class Component
{
    REAL,
    IMAG,
    BOTH,
};

//
// Expressions
//

struct ScalarVariable;
class Variable;
class Literal;
class ComplexLiteral;

class Add;
class Subtract;
class Multiply;
class ComplexMultiply;
class Divide;
class Modulus;

class ShiftLeft;
class ShiftRight;
class And;
class BitAnd;
class Or;
class Less;
class LessEqual;
class Greater;
class GreaterEqual;
class Equal;
class NotEqual;

class UnaryMinus;
class Not;
class PreIncrement;
class PreDecrement;

class Ternary;

// FFT expressions

class LoadGlobal;

class TwiddleMultiply;
class TwiddleMultiplyConjugate;

class Parens;

class CallExpr;

class IntrinsicLoad;

using Expression = std::variant<ScalarVariable,
                                Variable,
                                Literal,
                                ComplexLiteral,
                                Add,
                                Subtract,
                                Multiply,
                                ComplexMultiply,
                                Divide,
                                Modulus,
                                ShiftLeft,
                                ShiftRight,
                                And,
                                BitAnd,
                                Or,
                                Less,
                                LessEqual,
                                Greater,
                                GreaterEqual,
                                Equal,
                                NotEqual,
                                UnaryMinus,
                                Not,
                                PreIncrement,
                                PreDecrement,
                                Ternary,
                                LoadGlobal,
                                TwiddleMultiply,
                                TwiddleMultiplyConjugate,
                                Parens,
                                CallExpr,
                                IntrinsicLoad>;

class OptionalExpression
{
    std::any expr;

public:
    OptionalExpression(){};
    OptionalExpression(OptionalExpression&&)      = default;
    OptionalExpression(const OptionalExpression&) = default;
    OptionalExpression& operator=(OptionalExpression&&) = default;
    OptionalExpression& operator=(const OptionalExpression&) = default;
    explicit OptionalExpression(const Expression& expr);
    OptionalExpression& operator=(const Expression& in_expr);
    Expression          operator*() const;
                        operator bool() const;
};

class Literal
{

public:
    static const unsigned int precedence = 0;

    Literal(int num)
        : value(std::to_string(num))

    {
    }
    Literal(unsigned int num)
        : value(std::to_string(num))
    {
    }
    Literal(const std::string& val)
        : value(val)
    {
    }
    Literal(const char* val)
        : value(val)
    {
    }
    Literal(Literal&&)      = default;
    Literal(const Literal&) = default;
    Literal& operator=(Literal&&) = default;
    Literal& operator=(const Literal&) = default;

    std::string value;

    std::string render() const
    {
        return value;
    }
};

struct ScalarVariable
{
    static const unsigned int precedence = 0;
    std::string               name, type;
    Component                 component;
    OptionalExpression        index;

    ScalarVariable(const std::string& name,
                   const std::string& type,
                   Component          component = Component::BOTH)
        : name(name)
        , type(type)
        , component(component){};
    ScalarVariable(ScalarVariable&&)      = default;
    ScalarVariable(const ScalarVariable&) = default;
    ScalarVariable& operator=(const ScalarVariable&) = default;
    ScalarVariable& operator=(ScalarVariable&&) = default;

    std::string render() const;
};

class Variable
{
public:
    static const unsigned int precedence = 0;
    std::string               name, type;
    bool                      pointer = false, restrict = false;
    ScalarVariable            x, y;
    Component                 component;
    // index2d + size2d are set if this is a 2D array variable
    OptionalExpression index;
    OptionalExpression index2D;
    OptionalExpression size;
    OptionalExpression size2D;
    // default value for argument and template declarations
    OptionalExpression decl_default;

    Variable(const std::string& _name,
             const std::string& _type,
             bool               pointer = false,
             bool restrict              = false,
             unsigned int size          = 0);

    Variable(const std::string& _name,
             const std::string& _type,
             bool               pointer,
             bool restrict,
             const Expression& _size);

    Variable(const ScalarVariable& v)
        : name(v.name)
        , type(v.type)
        , x(v.name + ".x", v.type)
        , y(v.name + ".y", v.type){};

    Variable(const Variable& v);
    Variable(Variable&& v) = default;
    Variable(const Variable& v, const Expression& _index);
    Variable(const Variable& v, const Expression& _index, const Expression& _index2D);

    Variable& operator=(const Variable&) = default;
    Variable& operator=(Variable&&) = default;
    Variable  operator[](const Expression& index) const;
    // do a 2D array access
    Variable       at(const Expression& index, const Expression& index2D) const;
    ScalarVariable address() const;

    std::string render() const;
};

class ArgumentList
{
public:
    ArgumentList(){};
    ArgumentList(const std::initializer_list<Variable>& il)
        : arguments(il){};
    ArgumentList(const std::vector<Variable>& arguments)
        : arguments(arguments){};
    ArgumentList(std::vector<Variable>&& arguments)
        : arguments(std::move(arguments)){};
    ArgumentList(const ArgumentList&) = default;
    ArgumentList(ArgumentList&&)      = default;
    ArgumentList& operator=(const ArgumentList&) = default;
    ArgumentList& operator=(ArgumentList&&) = default;

    std::vector<Variable> arguments;
    std::string           render() const;
    std::string           render_decl() const;
                          operator bool() const;
    void                  append(Variable&&);
    void                  append(const Variable&);

    // find an argument with the specified name and set it to the
    // supplied value
    void set_value(const std::string& name, const std::string& value);
};

static ArgumentList get_callback_args()
{
    return {Variable{"load_cb_fn", "void", true, true},
            Variable{"load_cb_data", "void", true, true},
            Variable{"load_cb_lds_bytes", "unsigned int"},
            Variable{"store_cb_fn", "void", true, true},
            Variable{"store_cb_data", "void", true, true}};
}

using TemplateList = ArgumentList;

class CallExpr
{
public:
    static const unsigned int precedence = 0;

    std::string             name;
    TemplateList            templates;
    std::vector<Expression> arguments;

    CallExpr(const std::string& name, const std::vector<Expression>& arguments);
    CallExpr(const std::string&             name,
             const TemplateList&            templates,
             const std::vector<Expression>& arguments);
    CallExpr(CallExpr&&)      = default;
    CallExpr(const CallExpr&) = default;
    CallExpr& operator=(CallExpr&&) = default;
    CallExpr& operator=(const CallExpr&) = default;

    std::string render() const;
};

class ComplexMultiply
{
public:
    static const unsigned int precedence = 5;
    explicit ComplexMultiply(const std::vector<Expression>& args)
        : args(args)
    {
    }
    ComplexMultiply(ComplexMultiply&&)      = default;
    ComplexMultiply(const ComplexMultiply&) = default;
    ComplexMultiply& operator=(ComplexMultiply&&) = default;
    ComplexMultiply& operator=(const ComplexMultiply&) = default;

    std::string render() const;

    std::vector<Expression> args;
};

class Ternary
{
public:
    static const unsigned int precedence = 16;
    Ternary(Expression&& cond, Expression&& true_result, Expression&& false_result);
    explicit Ternary(std::vector<Expression>&& args);
    Ternary(Ternary&&)      = default;
    Ternary(const Ternary&) = default;
    Ternary&    operator=(Ternary&&) = default;
    Ternary&    operator=(const Ternary&) = default;
    std::string render() const;

    std::vector<Expression> args;
};

class LoadGlobal
{
public:
    static const unsigned int precedence = 18;
    LoadGlobal(const Expression& ptr, const Expression& index);
    explicit LoadGlobal(const std::vector<Expression>& args);
    LoadGlobal(LoadGlobal&&)      = default;
    LoadGlobal(const LoadGlobal&) = default;
    LoadGlobal& operator=(LoadGlobal&&) = default;
    LoadGlobal& operator=(const LoadGlobal&) = default;

    std::string render() const;

    std::vector<Expression> args;
};

class TwiddleMultiply
{
public:
    static const unsigned int precedence = 18;
    TwiddleMultiply(const Variable& a, const Variable& b)
        : a(a)
        , b(b)
    {
    }
    TwiddleMultiply(TwiddleMultiply&&)      = default;
    TwiddleMultiply(const TwiddleMultiply&) = default;
    TwiddleMultiply& operator=(TwiddleMultiply&&) = default;
    TwiddleMultiply& operator=(const TwiddleMultiply&) = default;
    Variable         a;
    Variable         b;
    std::string      render() const;
};

class TwiddleMultiplyConjugate
{
public:
    static const unsigned int precedence = 18;
    TwiddleMultiplyConjugate(const Variable& a, const Variable& b)
        : a(a)
        , b(b)
    {
    }
    TwiddleMultiplyConjugate(TwiddleMultiplyConjugate&&)      = default;
    TwiddleMultiplyConjugate(const TwiddleMultiplyConjugate&) = default;
    TwiddleMultiplyConjugate& operator=(TwiddleMultiplyConjugate&&) = default;
    TwiddleMultiplyConjugate& operator=(const TwiddleMultiplyConjugate&) = default;
    Variable                  a;
    Variable                  b;
    std::string               render() const;
};

class Parens
{
public:
    static const unsigned int precedence = 0;
    explicit Parens(Expression&& inside);
    explicit Parens(const Expression& inside);
    explicit Parens(std::vector<Expression>&& args);
    explicit Parens(const std::vector<Expression>& args);
    Parens(Parens&&)      = default;
    Parens(const Parens&) = default;
    Parens& operator=(Parens&&) = default;
    Parens& operator=(const Parens&) = default;

    std::vector<Expression> args;
    std::string             render() const;
};

class IntrinsicLoad
{
public:
    static const unsigned int precedence = 18;
    explicit IntrinsicLoad(const std::vector<Expression>& args);
    IntrinsicLoad(IntrinsicLoad&&)      = default;
    IntrinsicLoad(const IntrinsicLoad&) = default;
    IntrinsicLoad& operator=(IntrinsicLoad&&) = default;
    IntrinsicLoad& operator=(const IntrinsicLoad&) = default;

    // data, voffset, soffset, rw
    std::vector<Expression> args;
    std::string             render() const;
};

#define MAKE_OPER(NAME, OPER, PRECEDENCE)                           \
    class NAME                                                      \
    {                                                               \
        std::string oper{OPER};                                     \
                                                                    \
    public:                                                         \
        static const unsigned int precedence = PRECEDENCE;          \
        std::vector<Expression>   args;                             \
        explicit NAME(const std::initializer_list<Expression>& il); \
        explicit NAME(const std::vector<Expression>& il);           \
        NAME(NAME&&)        = default;                              \
        NAME(const NAME&)   = default;                              \
        NAME&       operator=(NAME&&) = default;                    \
        NAME&       operator=(const NAME&) = default;               \
        std::string render() const;                                 \
    };

MAKE_OPER(Add, " + ", 6);
MAKE_OPER(Subtract, " - ", 6);
MAKE_OPER(Multiply, " * ", 5);
MAKE_OPER(Divide, " / ", 5);
MAKE_OPER(Modulus, " % ", 5);

MAKE_OPER(Less, " < ", 9);
MAKE_OPER(LessEqual, " <= ", 9);
MAKE_OPER(Greater, " > ", 9);
MAKE_OPER(GreaterEqual, " >= ", 9);
MAKE_OPER(Equal, " == ", 10);
MAKE_OPER(NotEqual, " != ", 10);
MAKE_OPER(ShiftLeft, " << ", 7);
MAKE_OPER(ShiftRight, " >> ", 7);
MAKE_OPER(And, " && ", 14);
MAKE_OPER(BitAnd, " & ", 14);
MAKE_OPER(Or, " || ", 15);

MAKE_OPER(UnaryMinus, " -", 3);
MAKE_OPER(Not, " !", 3);
MAKE_OPER(PreIncrement, " ++", 3);
MAKE_OPER(PreDecrement, " --", 3);

MAKE_OPER(ComplexLiteral, ",", 17);

// end of Expression class declarations

static Add operator+(const Expression& a, const Expression& b)
{
    return Add{a, b};
}

static Subtract operator-(const Expression& a, const Expression& b)
{
    return Subtract{a, b};
}

static Multiply operator*(const Expression& a, const Expression& b)
{
    return Multiply{a, b};
}

static Divide operator/(const Expression& a, const Expression& b)
{
    return Divide{a, b};
}

static Modulus operator%(const Expression& a, const Expression& b)
{
    return Modulus{a, b};
}

static Less operator<(const Expression& a, const Expression& b)
{
    return Less{a, b};
}

static LessEqual operator<=(const Expression& a, const Expression& b)
{
    return LessEqual{a, b};
}

static Greater operator>(const Expression& a, const Expression& b)
{
    return Greater{a, b};
}

static GreaterEqual operator>=(const Expression& a, const Expression& b)
{
    return GreaterEqual{a, b};
}

static Equal operator==(const Expression& a, const Expression& b)
{
    return Equal{a, b};
}

static NotEqual operator!=(const Expression& a, const Expression& b)
{
    return NotEqual{a, b};
}

static ShiftLeft operator<<(const Expression& a, const Expression& b)
{
    return ShiftLeft{a, b};
}

static ShiftRight operator>>(const Expression& a, const Expression& b)
{
    return ShiftRight{a, b};
}

static And operator&&(const Expression& a, const Expression& b)
{
    return And{a, b};
}

static BitAnd operator&(const Expression& a, const Expression& b)
{
    return BitAnd{a, b};
}

static Or operator||(const Expression& a, const Expression& b)
{
    return Or{a, b};
}

static UnaryMinus operator-(const Expression& a)
{
    return UnaryMinus{a};
}

static Not operator!(const Expression& a)
{
    return Not{a};
}

static PreIncrement operator++(const Expression& a)
{
    return PreIncrement{a};
}

static PreDecrement operator--(const Expression& a)
{
    return PreDecrement{a};
}

//
// Statements
//

// Statements also have a circular dependency as described above for
// Expressions, for some classes.

class Assign;
class Call;
class CallbackDeclaration;
class Declaration;
class LDSDeclaration;
class For;
class While;
class If;
class ElseIf;
class Else;
class StoreGlobal;
class StoreGlobalPlanar;
class StatementList;
class Butterfly;
class IntrinsicStore;
class IntrinsicLoadToDest;

struct LineBreak
{
    static std::string render()
    {
        return "\n\n";
    }
};

struct SyncThreads
{
    static std::string render()
    {
        return "__syncthreads();";
    }
};

struct Return
{
    static std::string render()
    {
        return "return;\n";
    }
};

struct Break
{
    static std::string render()
    {
        return "break;\n";
    }
};

struct CommentLines
{
    std::vector<std::string> comments;
    std::string              render() const
    {
        std::string s;

        static const char* NEWLINE   = "\n";
        const char*        separator = "";
        for(auto c : comments)
        {
            s += separator;
            s += "// " + c;
            separator = NEWLINE;
        }
        return s;
    }
    explicit CommentLines(std::initializer_list<std::string> il)
        : comments(il){};
};

using Statement = std::variant<Assign,
                               Call,
                               CallbackDeclaration,
                               CommentLines,
                               Declaration,
                               LDSDeclaration,
                               For,
                               While,
                               If,
                               ElseIf,
                               Else,
                               StoreGlobal,
                               StoreGlobalPlanar,
                               LineBreak,
                               Return,
                               Break,
                               SyncThreads,
                               Butterfly,
                               IntrinsicStore,
                               IntrinsicLoadToDest>;

class Assign
{
public:
    Variable    lhs;
    Expression  rhs;
    std::string oper;

    Assign(const Variable& lhs, const Expression& rhs, const std::string& oper = "=")
        : lhs(lhs)
        , rhs(rhs)
        , oper(oper){};

    std::string render() const
    {
        return lhs.render() + " " + oper + " " + vrender(rhs) + ";";
    }
};

// +=, *= operators are just Assign with another operator
static Assign AddAssign(const Variable& lhs, const Expression& rhs)
{
    return Assign(lhs, rhs, "+=");
}
static Assign MultiplyAssign(const Variable& lhs, const Expression& rhs)
{
    return Assign(lhs, rhs, "*=");
}
static Assign ModulusAssign(const Variable& lhs, const Expression& rhs)
{
    return Assign(lhs, rhs, "%=");
}

//
// Declarations
//

class Declaration
{
public:
    Variable                  var;
    std::optional<Expression> value;
    explicit Declaration(const Variable& v)
        : var(v){};
    Declaration(const Variable& v, Expression&& val)
        : var(v)
        , value(std::move(val)){};
    Declaration(const Variable& v, const Expression& val)
        : var(v)
        , value(val){};
    std::string render() const;
};

class LDSDeclaration
{
public:
    explicit LDSDeclaration(const std::string& scalar_type)
        : scalar_type(scalar_type){};
    std::string scalar_type;
    std::string render() const
    {
        // Declare an LDS buffer whose size is defined at launch time.
        // The declared buffer is of type unsigned char, but is aligned
        // to a complex unit.
        //
        // We then define pointers to that buffer with real and
        // complex types, since the body of the function may look at
        // LDS as real values or as complex values (code for both is
        // generated, and we choose one at compile time via a template
        // parameter).
        //
        // TODO: Ideally we would use C++11 "alignas" and "alignof"
        // for alignment, but they're incompatible with "extern
        // __shared__".  Another alternative would be the __align__
        // HIP macro, but that is not currently present in hipRTC.
        return "extern __shared__ unsigned char __attribute__((aligned(sizeof(" + scalar_type
               + ")))) lds_uchar[];\nreal_type_t<" + scalar_type
               + ">* __restrict__ lds_real = reinterpret_cast<real_type_t<" + scalar_type
               + ">*>(lds_uchar);\n" + scalar_type
               + "* __restrict__ lds_complex = reinterpret_cast<" + scalar_type + "*>(lds_uchar);";
    }
};

class CallbackDeclaration
{
public:
    CallbackDeclaration(const std::string& scalar_type, const std::string& cbtype)
        : scalar_type(scalar_type)
        , cbtype(cbtype){};
    std::string scalar_type;
    std::string cbtype;
    std::string render() const
    {
        return "auto load_cb = get_load_cb<" + scalar_type + ", " + cbtype + ">(load_cb_fn);\n"
               + "auto store_cb = get_store_cb<" + scalar_type + ", " + cbtype
               + ">(store_cb_fn);\n";
    }
};

class Call
{
public:
    Call(const std::string& name, const std::vector<Expression>& arguments)
        : expr(name, arguments)
    {
    }
    Call(const std::string&             name,
         const TemplateList&            templates,
         const std::vector<Expression>& arguments)
        : expr(name, templates, arguments)
    {
    }

    CallExpr expr;

    std::string render() const
    {
        return expr.render() + ";";
    }
};

class StatementList
{
public:
    std::vector<Statement> statements;
    StatementList();
    StatementList(const std::initializer_list<Statement>& il);
    std::string render() const;
};

class For
{
public:
    Variable      var;
    Expression    initial;
    Expression    condition;
    Expression    increment;
    StatementList body;
    bool          pragma_unroll;
    For(const Variable&      var,
        const Expression&    initial,
        const Expression&    condition,
        const Expression&    increment,
        const StatementList& body          = {},
        bool                 pragma_unroll = false);
    std::string render() const;
};

class While
{
public:
    Expression    condition;
    StatementList body;
    While(const Expression& condition, const StatementList& body = {});
    std::string render() const;
};

class If
{
public:
    Expression    condition;
    StatementList body;
    If(const Expression& condition, const StatementList& body);
    std::string render() const;
};

class ElseIf
{
public:
    Expression    condition;
    StatementList body;
    ElseIf(const Expression& condition, const StatementList& body);
    std::string render() const;
};

class Else
{
public:
    StatementList body;
    explicit Else(const StatementList& body);
    std::string render() const;
};

class StoreGlobal
{
public:
    StoreGlobal(const Expression& ptr, const Expression& index, const Expression& value)
        : ptr{ptr}
        , index{index}
        , value{value}
    {
    }
    std::string render() const
    {
        return "store_cb(" + vrender(ptr) + "," + vrender(index) + ","
               + vrender(scale_factor ? (value * scale_factor.value()) : value)
               + ", store_cb_data, nullptr);";
    }

    Expression                ptr;
    Expression                index;
    Expression                value;
    std::optional<Expression> scale_factor;
};

// Planar version of StoreGlobal, so we remember the scale factor
// after a conversion to planar kernel
class StoreGlobalPlanar
{
public:
    StoreGlobalPlanar(const Variable&                  realPtr,
                      const Variable&                  imagPtr,
                      const Expression&                index,
                      const Variable&                  value,
                      const std::optional<Expression>& scale_factor)
        : realPtr{realPtr}
        , imagPtr{imagPtr}
        , index{index}
        , value{value}
        , scale_factor{scale_factor}
    {
    }
    std::string render() const
    {
        // Output two assignments
        return Assign{realPtr[index],
                      scale_factor ? Expression{value.x * scale_factor.value()}
                                   : Expression{value.x}}
                   .render()
               + Assign{imagPtr[index],
                        scale_factor ? Expression{value.y * scale_factor.value()}
                                     : Expression{value.y}}
                     .render();
    }

    Variable                  realPtr;
    Variable                  imagPtr;
    Expression                index;
    Variable                  value;
    std::optional<Expression> scale_factor;
};

class Butterfly
{
public:
    static const unsigned int precedence = 0;
    Butterfly(bool forward, const std::vector<Expression>& args)
        : forward(forward)
        , args(args)
    {
    }
    bool                    forward;
    std::vector<Expression> args;
    std::string             render() const;
};

class IntrinsicStore
{
public:
    IntrinsicStore(const Expression& ptr,
                   const Expression& voffset,
                   const Expression& soffset,
                   const Expression& value,
                   const Expression& rw_flag)
        : ptr{ptr}
        , voffset{voffset}
        , soffset{soffset}
        , value{value}
        , rw_flag{rw_flag}
    {
    }
    std::string render() const
    {
        return "store_intrinsic(" + vrender(ptr) + "," + vrender(voffset) + "," + vrender(soffset)
               + "," + vrender(scale_factor ? (value * scale_factor.value()) : value) + ","
               + vrender(rw_flag) + ");";
    }

    Expression                ptr;
    Expression                voffset;
    Expression                soffset;
    Expression                value;
    Expression                rw_flag;
    std::optional<Expression> scale_factor;
};

class IntrinsicLoadToDest
{
public:
    IntrinsicLoadToDest(const Expression& dest,
                        const Expression& data,
                        const Expression& voffset,
                        const Expression& soffset,
                        const Expression& rw_flag)
        : dest{dest}
        , data{data}
        , voffset{voffset}
        , soffset{soffset}
        , rw_flag{rw_flag}
    {
    }
    std::string render() const
    {
        return "intrinsic_load_to_dest(" + vrender(dest) + "," + vrender(data) + ","
               + vrender(voffset) + "," + vrender(soffset) + "," + vrender(rw_flag) + ");";
    }

    Expression dest;
    Expression data;
    Expression voffset;
    Expression soffset;
    Expression rw_flag;
};

// end of Statement class declarations

static void operator+=(StatementList& stmts, const Statement& s)
{
    stmts.statements.emplace_back(s);
}

static void operator+=(StatementList& stmts, Statement&& s)
{
    stmts.statements.emplace_back(std::move(s));
}

static void operator+=(StatementList& stmts, const StatementList& s)
{
    //    stmts.statements.insert(stmts.statements.end(), s.statements.cbegin(),
    //    s.statements.cend());
    for(auto x : s.statements)
    {
        stmts += x;
    }
}

static void operator+=(StatementList& stmts, StatementList&& s)
{
    for(auto&& x : s.statements)
    {
        stmts += std::move(x);
    }
}

//
// Functions
//

class Function
{
public:
    std::string   name;
    StatementList body;
    ArgumentList  arguments;
    TemplateList  templates;
    std::string   qualifier;
    unsigned int  launch_bounds = 0;

    explicit Function(const std::string& name)
        : name(name){};

    std::string render() const;
};

//
// Re-write helpers
//

// Base visitor class that actual visitor implementations can inherit
// from.
struct BaseVisitor
{
    BaseVisitor() = default;

    // Create operator() for each concrete type, so std::visit on a
    // variant will work.  "Statement" types all return a
    // StatementList.  Other types mostly return Expressions.  Each
    // method dispatches to a virtual visit_* method so we can
    // subclass just what we want.
#define MAKE_VISITOR_OPERATOR(RET, CLS) \
    RET operator()(const CLS& x)        \
    {                                   \
        return visit_##CLS(x);          \
    }

    MAKE_VISITOR_OPERATOR(Expression, ScalarVariable);
    MAKE_VISITOR_OPERATOR(Expression, Variable);
    MAKE_VISITOR_OPERATOR(Expression, Literal);
    MAKE_VISITOR_OPERATOR(Expression, ComplexLiteral);
    MAKE_VISITOR_OPERATOR(Expression, Add);
    MAKE_VISITOR_OPERATOR(Expression, Subtract);
    MAKE_VISITOR_OPERATOR(Expression, Multiply);
    MAKE_VISITOR_OPERATOR(Expression, Divide);
    MAKE_VISITOR_OPERATOR(Expression, Modulus);
    MAKE_VISITOR_OPERATOR(Expression, ShiftLeft);
    MAKE_VISITOR_OPERATOR(Expression, ShiftRight);
    MAKE_VISITOR_OPERATOR(Expression, And);
    MAKE_VISITOR_OPERATOR(Expression, BitAnd);
    MAKE_VISITOR_OPERATOR(Expression, Or);
    MAKE_VISITOR_OPERATOR(Expression, Less);
    MAKE_VISITOR_OPERATOR(Expression, LessEqual);
    MAKE_VISITOR_OPERATOR(Expression, Greater);
    MAKE_VISITOR_OPERATOR(Expression, GreaterEqual);
    MAKE_VISITOR_OPERATOR(Expression, Equal);
    MAKE_VISITOR_OPERATOR(Expression, NotEqual);
    MAKE_VISITOR_OPERATOR(Expression, UnaryMinus);
    MAKE_VISITOR_OPERATOR(Expression, Not);
    MAKE_VISITOR_OPERATOR(Expression, PreIncrement);
    MAKE_VISITOR_OPERATOR(Expression, PreDecrement);
    MAKE_VISITOR_OPERATOR(Expression, Ternary);
    MAKE_VISITOR_OPERATOR(Expression, LoadGlobal);
    MAKE_VISITOR_OPERATOR(Expression, ComplexMultiply);
    MAKE_VISITOR_OPERATOR(Expression, TwiddleMultiply);
    MAKE_VISITOR_OPERATOR(Expression, TwiddleMultiplyConjugate);
    MAKE_VISITOR_OPERATOR(Expression, Parens);
    MAKE_VISITOR_OPERATOR(Expression, CallExpr);
    MAKE_VISITOR_OPERATOR(Expression, IntrinsicLoad);

    MAKE_VISITOR_OPERATOR(StatementList, Assign);
    MAKE_VISITOR_OPERATOR(StatementList, Call);
    MAKE_VISITOR_OPERATOR(StatementList, CallbackDeclaration);
    MAKE_VISITOR_OPERATOR(StatementList, CommentLines);
    MAKE_VISITOR_OPERATOR(StatementList, Declaration);
    MAKE_VISITOR_OPERATOR(StatementList, LDSDeclaration);
    MAKE_VISITOR_OPERATOR(StatementList, For);
    MAKE_VISITOR_OPERATOR(StatementList, While);
    MAKE_VISITOR_OPERATOR(StatementList, If);
    MAKE_VISITOR_OPERATOR(StatementList, ElseIf);
    MAKE_VISITOR_OPERATOR(StatementList, Else);
    MAKE_VISITOR_OPERATOR(StatementList, StoreGlobal);
    MAKE_VISITOR_OPERATOR(StatementList, StoreGlobalPlanar);
    MAKE_VISITOR_OPERATOR(StatementList, LineBreak);
    MAKE_VISITOR_OPERATOR(StatementList, Return);
    MAKE_VISITOR_OPERATOR(StatementList, Break);
    MAKE_VISITOR_OPERATOR(StatementList, SyncThreads);
    MAKE_VISITOR_OPERATOR(StatementList, Butterfly);
    MAKE_VISITOR_OPERATOR(StatementList, IntrinsicStore);
    MAKE_VISITOR_OPERATOR(StatementList, IntrinsicLoadToDest);

    MAKE_VISITOR_OPERATOR(ArgumentList, ArgumentList);

    MAKE_VISITOR_OPERATOR(Function, Function);

    // operator for StatementList itself is a bit special - need to
    // visit each statement in the list
    StatementList operator()(const StatementList& x)
    {
        StatementList ret;
        for(const auto& stmt : x.statements)
        {
            StatementList new_stmts = std::visit(*this, stmt);
            for(auto& s : new_stmts.statements)
                ret.statements.emplace_back(std::move(s));
        }
        return ret;
    }

    // "visit" methods know how to visit their children.
    //
    // - TRIVIAL visitors are for types that have no children.
    //
    // - EXPR visitors are for Expression types whose only children
    //   are in a vector<Expression> named "exprs".
    //
    // - STATEMENT types return a StatementList.
    //
    // - Types that have children but don't fit the EXPR mold need
    //   their own hand-written visit function.
#define MAKE_TRIVIAL_VISIT(RET, CLS)      \
    virtual RET visit_##CLS(const CLS& x) \
    {                                     \
        return x;                         \
    }

#define MAKE_TRIVIAL_STATEMENT_VISIT(CLS)           \
    virtual StatementList visit_##CLS(const CLS& x) \
    {                                               \
        StatementList stmts;                        \
        stmts += Statement{x};                      \
        return stmts;                               \
    }

#define MAKE_EXPR_VISIT(CLS)                           \
    virtual Expression visit_##CLS(const CLS& x)       \
    {                                                  \
        std::vector<Expression> args;                  \
        for(auto& arg : x.args)                        \
            args.emplace_back(std::visit(*this, arg)); \
        return CLS{std::move(args)};                   \
    }

    MAKE_EXPR_VISIT(Add);
    MAKE_EXPR_VISIT(And);
    MAKE_EXPR_VISIT(BitAnd);
    MAKE_EXPR_VISIT(Divide);
    MAKE_EXPR_VISIT(Equal);
    MAKE_EXPR_VISIT(Greater);
    MAKE_EXPR_VISIT(GreaterEqual);
    MAKE_EXPR_VISIT(Less);
    MAKE_EXPR_VISIT(LessEqual);
    MAKE_EXPR_VISIT(Modulus);
    MAKE_EXPR_VISIT(Multiply);
    MAKE_EXPR_VISIT(NotEqual);
    MAKE_EXPR_VISIT(Or);
    MAKE_EXPR_VISIT(ShiftLeft);
    MAKE_EXPR_VISIT(ShiftRight);
    MAKE_EXPR_VISIT(Subtract);

    MAKE_EXPR_VISIT(UnaryMinus);
    MAKE_EXPR_VISIT(Not);
    MAKE_EXPR_VISIT(PreIncrement);
    MAKE_EXPR_VISIT(PreDecrement);

    MAKE_EXPR_VISIT(LoadGlobal);

    MAKE_TRIVIAL_VISIT(Expression, ComplexMultiply);
    MAKE_TRIVIAL_VISIT(Expression, TwiddleMultiply);
    MAKE_TRIVIAL_VISIT(Expression, TwiddleMultiplyConjugate);

    MAKE_EXPR_VISIT(IntrinsicLoad);
    MAKE_EXPR_VISIT(Parens);

    MAKE_EXPR_VISIT(Ternary);
    MAKE_EXPR_VISIT(ComplexLiteral)

    MAKE_TRIVIAL_VISIT(Expression, ScalarVariable)
    MAKE_TRIVIAL_STATEMENT_VISIT(CallbackDeclaration)
    MAKE_TRIVIAL_STATEMENT_VISIT(LDSDeclaration)

    MAKE_TRIVIAL_VISIT(Expression, Literal)
    MAKE_TRIVIAL_STATEMENT_VISIT(CommentLines)
    MAKE_TRIVIAL_STATEMENT_VISIT(LineBreak)
    MAKE_TRIVIAL_STATEMENT_VISIT(Return)
    MAKE_TRIVIAL_STATEMENT_VISIT(Break)
    MAKE_TRIVIAL_STATEMENT_VISIT(SyncThreads)
    MAKE_TRIVIAL_STATEMENT_VISIT(Butterfly);
    MAKE_TRIVIAL_STATEMENT_VISIT(IntrinsicLoadToDest);

    MAKE_TRIVIAL_VISIT(Expression, Variable)

    virtual StatementList visit_StatementList(const StatementList& x)
    {
        auto y = StatementList();
        for(auto s : x.statements)
        {
            y += std::visit(*this, s);
        }
        return y;
    }

    virtual ArgumentList visit_ArgumentList(const ArgumentList& x)
    {
        auto y = ArgumentList();
        for(auto s : x.arguments)
        {
            y.append(std::get<Variable>(visit_Variable(s)));
        }
        return y;
    }

    virtual StatementList visit_Assign(const Assign& x)
    {
        auto lhs = std::get<Variable>(visit_Variable(x.lhs));
        auto rhs = std::visit(*this, x.rhs);
        return StatementList{Assign{lhs, rhs, x.oper}};
    }

    virtual Expression visit_CallExpr(const CallExpr& x)
    {
        auto y      = CallExpr(x);
        y.templates = visit_ArgumentList(x.templates);
        y.arguments.clear();
        y.arguments.reserve(x.arguments.size());
        for(const auto& arg : x.arguments)
            y.arguments.emplace_back(std::visit(*this, arg));
        return y;
    }

    virtual StatementList visit_Call(const Call& x)
    {
        auto y = std::get<CallExpr>(visit_CallExpr(x.expr));
        return StatementList{Call{y.name, y.templates, y.arguments}};
    }

    virtual StatementList visit_Declaration(const Declaration& x)
    {
        auto var = std::get<Variable>(visit_Variable(x.var));
        if(x.value)
        {
            return StatementList{Declaration(var, std::visit(*this, *x.value))};
        }
        return StatementList{Declaration(var)};
    }

    virtual StatementList visit_For(const For& x)
    {
        auto var       = std::get<Variable>(visit_Variable(x.var));
        auto initial   = std::visit(*this, x.initial);
        auto condition = std::visit(*this, x.condition);
        auto increment = std::visit(*this, x.increment);
        auto body      = visit_StatementList(x.body);
        return StatementList{For(var, initial, condition, increment, body, x.pragma_unroll)};
    }

    virtual StatementList visit_While(const While& x)
    {
        auto condition = std::visit(*this, x.condition);
        auto body      = visit_StatementList(x.body);
        return StatementList{While(condition, body)};
    }

    virtual StatementList visit_If(const If& x)
    {
        auto condition = std::visit(*this, x.condition);
        auto body      = visit_StatementList(x.body);
        return StatementList{If(condition, body)};
    }

    virtual StatementList visit_ElseIf(const ElseIf& x)
    {
        auto condition = std::visit(*this, x.condition);
        auto body      = visit_StatementList(x.body);
        return StatementList{ElseIf(condition, body)};
    }

    virtual StatementList visit_Else(const Else& x)
    {
        auto body = visit_StatementList(x.body);
        return StatementList{Else(body)};
    }

    virtual StatementList visit_StoreGlobal(const StoreGlobal& x)
    {
        auto ptr   = std::visit(*this, x.ptr);
        auto index = std::visit(*this, x.index);
        auto value = std::visit(*this, x.value);
        return StatementList{StoreGlobal(ptr, index, value)};
    }

    virtual StatementList visit_IntrinsicStore(const IntrinsicStore& x)
    {
        auto ptr     = std::visit(*this, x.ptr);
        auto voffset = std::visit(*this, x.voffset);
        auto soffset = std::visit(*this, x.soffset);
        auto value   = std::visit(*this, x.value);
        auto rw_flag = std::visit(*this, x.rw_flag);
        return StatementList{IntrinsicStore(ptr, voffset, soffset, value, rw_flag)};
    }

    virtual StatementList visit_StoreGlobalPlanar(const StoreGlobalPlanar& x)
    {
        auto                      realPtr = std::get<Variable>(visit_Variable(x.realPtr));
        auto                      imagPtr = std::get<Variable>(visit_Variable(x.imagPtr));
        auto                      index   = std::visit(*this, x.index);
        auto                      value   = std::get<Variable>(visit_Variable(x.value));
        std::optional<Expression> scale_factor;
        if(x.scale_factor)
            scale_factor = std::visit(*this, x.scale_factor.value());
        return StatementList{StoreGlobalPlanar(realPtr, imagPtr, index, value, scale_factor)};
    }

    virtual Function visit_Function(const Function& x)
    {
        auto y          = Function(x.name);
        y.body          = visit_StatementList(x.body);
        y.arguments     = visit_ArgumentList(x.arguments);
        y.templates     = visit_ArgumentList(x.templates);
        y.qualifier     = x.qualifier;
        y.launch_bounds = x.launch_bounds;
        return y;
    }
};

//
// Make planar
//

struct MakePlanarVisitor : public BaseVisitor
{
    std::string varname, rename, imname;

    MakePlanarVisitor(const std::string& varname)
        : varname(varname)
        , rename(varname + "re")
        , imname(varname + "im")
    {
    }

    ArgumentList visit_ArgumentList(const ArgumentList& x) override
    {
        ArgumentList y;
        for(auto a : x.arguments)
        {
            if(a.name == varname)
            {
                auto re = Variable(a);
                re.name = rename;
                re.type = "real_type_t<" + a.type + ">";
                auto im = Variable(a);
                im.name = imname;
                im.type = "real_type_t<" + a.type + ">";
                y.append(re);
                y.append(im);
            }
            else
            {
                y.append(a);
            }
        }
        return y;
    }

    StatementList visit_Assign(const Assign& x) override
    {
        StatementList stmts;
        if(x.lhs.name == varname && std::holds_alternative<Variable>(x.rhs))
        {
            // on lhs, lhs needs to be split; use .x and .y on rhs

            auto rhs = std::get<Variable>(x.rhs);

            auto re = Variable(x.lhs);
            re.name = rename;
            auto im = Variable(x.lhs);
            im.name = imname;

            // FIXME- Not every rhs is complex
            // stmts += Assign(re, rhs.x, x.oper);
            // stmts += Assign(im, rhs.y, x.oper);
            stmts += Assign(re, rhs, x.oper);
            stmts += Assign(im, rhs, x.oper);
            return stmts;
        }
        else if(std::holds_alternative<Variable>(x.rhs)
                && std::get<Variable>(x.rhs).name == varname)
        {
            // on rhs, rhs needs to be joined as a complex literal

            auto rhs = std::get<Variable>(x.rhs);
            auto re  = Variable(rhs);
            re.name  = rename;
            auto im  = Variable(rhs);
            im.name  = imname;
            stmts += Assign{x.lhs, ComplexLiteral{re, im}, x.oper};
            return stmts;
        }
        // callbacks don't support planar, so loads are just direct
        // memory accesses
        else if(std::holds_alternative<LoadGlobal>(x.rhs))
        {
            auto load = std::get<LoadGlobal>(x.rhs);
            auto ptr  = std::get<Variable>(load.args[0]);
            if(ptr.name == varname)
            {
                auto& idx = load.args[1];

                auto re = ptr;
                re.name = rename;
                auto im = ptr;
                im.name = imname;

                stmts += Assign{x.lhs, ComplexLiteral{re[idx], im[idx]}, x.oper};
                return stmts;
            }
        }
        // callbacks don't support planar
        else if(std::holds_alternative<IntrinsicLoad>(x.rhs))
        {
            auto load = std::get<IntrinsicLoad>(x.rhs);
            auto ptr  = std::get<Variable>(load.args[0]);
            if(ptr.name == varname)
            {
                auto& voffset = load.args[1];
                auto& soffset = load.args[2];
                auto& rw_flag = load.args[3];

                auto re = ptr;
                re.name = rename;
                auto im = ptr;
                im.name = imname;

                stmts += Assign{x.lhs,
                                ComplexLiteral{IntrinsicLoad({re, voffset, soffset, rw_flag}),
                                               IntrinsicLoad({im, voffset, soffset, rw_flag})},
                                x.oper};
                return stmts;
            }
        }

        return StatementList{x};
    }

    StatementList visit_StoreGlobal(const StoreGlobal& x) override
    {
        // callbacks don't support planar, so stores are just direct
        // memory accesses
        auto var = std::get<Variable>(x.ptr);

        if(var.name == varname)
        {
            auto re = var;
            re.name = rename;
            auto im = var;
            im.name = imname;

            auto value = std::get<Variable>(x.value);
            return {StoreGlobalPlanar{re, im, x.index, value, x.scale_factor}};
        }
        return StatementList{x};
    }

    StatementList visit_IntrinsicStore(const IntrinsicStore& x) override
    {
        // callbacks don't support planar
        auto var = std::get<Variable>(x.ptr);

        if(var.name == varname)
        {
            auto re = var;
            re.name = rename;
            auto im = var;
            im.name = imname;

            StatementList stmts;
            stmts += Call{
                "store_intrinsic",
                {re,
                 x.voffset,
                 x.soffset,
                 Literal{"real_type_t<scalar_type>("
                         + vrender(x.scale_factor ? (x.scale_factor.value() * x.value) : x.value)
                         + ".x)"},
                 x.rw_flag}};
            stmts += Call{
                "store_intrinsic",
                {im,
                 x.voffset,
                 x.soffset,
                 Literal{"real_type_t<scalar_type>("
                         + vrender(x.scale_factor ? (x.scale_factor.value() * x.value) : x.value)
                         + ".y)"},
                 x.rw_flag}};
            return stmts;
        }
        return StatementList{x};
    }
};

static Function make_planar(const Function& f, const std::string& varname)
{
    auto visitor = MakePlanarVisitor(varname);
    return visitor(f);
}

//
// Make out of place
//
struct MakeOutOfPlaceVisitor : public BaseVisitor
{
    const std::vector<std::string> op_names;
    bool                           rename_function;
    MakeOutOfPlaceVisitor(std::vector<std::string>&& op_names, bool rename_function = true)
        : op_names(op_names)
        , rename_function(rename_function)
    {
    }

    bool op_name_match(const std::string& s)
    {
        return std::find(op_names.begin(), op_names.end(), s) != op_names.end();
    }

    enum class ExpressionVisitMode
    {
        INPUT,
        OUTPUT,
    };
    ExpressionVisitMode mode = ExpressionVisitMode::INPUT;

    Expression visit_LoadGlobal(const LoadGlobal& x) override
    {
        mode = ExpressionVisitMode::INPUT;
        std::vector<Expression> args;
        for(const auto& arg : x.args)
            args.emplace_back(std::visit(*this, arg));
        return LoadGlobal{args};
    }
    Expression visit_IntrinsicLoad(const IntrinsicLoad& x) override
    {
        mode = ExpressionVisitMode::INPUT;
        std::vector<Expression> args;
        for(const auto& arg : x.args)
            args.emplace_back(std::visit(*this, arg));
        return IntrinsicLoad{args};
    }
    StatementList visit_StoreGlobal(const StoreGlobal& x) override
    {
        StatementList stmts;
        mode       = ExpressionVisitMode::OUTPUT;
        auto ptr   = std::visit(*this, x.ptr);
        auto index = std::visit(*this, x.index);
        auto value = std::visit(*this, x.value);
        stmts += StoreGlobal{ptr, index, value};
        return stmts;
    }
    StatementList visit_IntrinsicStore(const IntrinsicStore& x) override
    {
        StatementList stmts;
        mode         = ExpressionVisitMode::OUTPUT;
        auto ptr     = std::visit(*this, x.ptr);
        auto voffset = std::visit(*this, x.voffset);
        auto soffset = std::visit(*this, x.soffset);
        auto value   = std::visit(*this, x.value);
        auto rw_flag = std::visit(*this, x.rw_flag);
        stmts += IntrinsicStore{ptr, voffset, soffset, value, rw_flag};
        return stmts;
    }

    StatementList visit_Assign(const Assign& x) override
    {
        if(!op_name_match(x.lhs.name))
            return BaseVisitor::visit_Assign(x);
        mode         = ExpressionVisitMode::INPUT;
        auto in_lhs  = std::get<Variable>(visit_Variable(x.lhs));
        auto in_rhs  = std::visit(*this, x.rhs);
        mode         = ExpressionVisitMode::OUTPUT;
        auto out_lhs = std::get<Variable>(visit_Variable(x.lhs));
        auto out_rhs = std::visit(*this, x.rhs);

        StatementList ret;
        ret += Assign{in_lhs, in_rhs, x.oper};
        ret += Assign{out_lhs, out_rhs, x.oper};
        return ret;
    }

    ArgumentList visit_ArgumentList(const ArgumentList& x) override
    {
        ArgumentList ret;
        for(const auto& arg : x.arguments)
        {
            if(op_name_match(arg.name))
            {
                mode = ExpressionVisitMode::INPUT;
                ret.append(std::get<Variable>(visit_Variable(arg)));
                mode = ExpressionVisitMode::OUTPUT;
                ret.append(std::get<Variable>(visit_Variable(arg)));
            }
            else
                ret.append(arg);
        }
        return ret;
    }

    Expression visit_Variable(const Variable& x) override
    {
        if(!op_name_match(x.name))
            return x;

        Variable y{x};
        y.name += mode == ExpressionVisitMode::INPUT ? "_in" : "_out";
        return y;
    }

    StatementList visit_Declaration(const Declaration& x) override
    {
        if(!op_name_match(x.var.name))
            return BaseVisitor::visit_Declaration(x);

        StatementList ret;
        mode        = ExpressionVisitMode::INPUT;
        auto in_var = std::get<Variable>(visit_Variable(x.var));
        if(x.value)
            ret += Declaration{in_var, std::visit(*this, *x.value)};
        else
            ret += Declaration{in_var};
        mode         = ExpressionVisitMode::OUTPUT;
        auto out_var = std::get<Variable>(visit_Variable(x.var));
        if(x.value)
            ret += Declaration{out_var, std::visit(*this, *x.value)};
        else
            ret += Declaration{out_var};
        return ret;
    }

    Function visit_Function(const Function& x) override
    {
        Function y{x};
        if(rename_function)
            y.name = "op_" + y.name;
        return BaseVisitor::visit_Function(y);
    }
};

static Function make_outofplace(const Function&    f,
                                const std::string& bufName         = "buf",
                                bool               rename_function = true)
{
    auto visitor = MakeOutOfPlaceVisitor({bufName, "stride", "stride0", "offset"}, rename_function);
    return visitor(f);
}

//
// Make in-place
//
struct MakeInPlaceVisitor : public BaseVisitor
{
    MakeInPlaceVisitor() = default;

    Function visit_Function(const Function& x) override
    {
        Function y{x};
        y.name = "ip_" + y.name;
        return BaseVisitor::visit_Function(y);
    }
};

static Function make_inplace(const Function& f)
{
    auto visitor = MakeInPlaceVisitor();
    return visitor(f);
}

//
// Make inverse
//
static const char*  FORWARD_PREFIX     = "forward_";
static const size_t FORWARD_PREFIX_LEN = strlen(FORWARD_PREFIX);
static const char*  INVERSE_PREFIX     = "inverse_";

struct MakeInverseVisitor : public BaseVisitor
{
    Expression visit_TwiddleMultiply(const TwiddleMultiply& x) override
    {
        return TwiddleMultiplyConjugate{x.a, x.b};
    }

    StatementList visit_Butterfly(const Butterfly& x) override
    {
        return {Butterfly{false, x.args}};
    }

    Expression visit_CallExpr(const CallExpr& x) override
    {
        auto pos = x.name.rfind(FORWARD_PREFIX, 0);
        if(pos == 0)
        {
            CallExpr y{x};
            y.name.replace(0, FORWARD_PREFIX_LEN, INVERSE_PREFIX);
            return y;
        }
        return BaseVisitor::visit_CallExpr(x);
    }

    Function visit_Function(const Function& x) override
    {
        auto pos = x.name.rfind(FORWARD_PREFIX, 0);
        if(pos != 0)
            return x;
        Function y{x};
        y.name.replace(0, FORWARD_PREFIX_LEN, INVERSE_PREFIX);
        return BaseVisitor::visit_Function(y);
    }
};

static Function make_inverse(const Function& f)
{
    auto visitor = MakeInverseVisitor();
    return visitor(f);
}

//
// Make runtime-compileable
//

struct MakeRTCVisitor : public BaseVisitor
{
    MakeRTCVisitor(const std::string& kernel_name, bool enable_scaling)
        : kernel_name(kernel_name)
        , enable_scaling(enable_scaling)
        , scale_factor("scale_factor", "const real_type_t<scalar_type>")
    {
    }
    Function visit_Function(const Function& x) override
    {
        if(x.qualifier != "__global__")
            return x;
        // give function C linkage so caller doesn't have to do C++ name
        // mangling
        Function y{x};
        y.qualifier = "extern \"C\" __global__";
        // rocfft constructed a name for the function
        y.name = kernel_name;
        // assume some global-scope typedefs + consts have removed
        // the need for template args.
        y.templates.arguments.clear();

        // scaling requires an extra argument with the scale factor
        if(enable_scaling)
            y.arguments.append(scale_factor);

        return BaseVisitor::visit_Function(y);
    }

    StatementList visit_StoreGlobal(const StoreGlobal& x) override
    {
        StoreGlobal y{x};
        // multiply by scale factor when storing to global, if requested
        if(enable_scaling)
            y.scale_factor = scale_factor;
        return StatementList{y};
    }

    StatementList visit_StoreGlobalPlanar(const StoreGlobalPlanar& x) override
    {
        StoreGlobalPlanar y{x};
        // multiply by scale factor when storing to global, if requested
        if(enable_scaling)
            y.scale_factor = scale_factor;
        return StatementList{y};
    }

    StatementList visit_IntrinsicStore(const IntrinsicStore& x) override
    {
        IntrinsicStore y{x};
        // multiply by scale factor when storing to global, if requested
        if(enable_scaling)
            y.scale_factor = scale_factor;
        return StatementList{y};
    }

    std::string kernel_name;
    bool        enable_scaling;
    Variable    scale_factor;
};

static Function make_rtc(const Function& f, const std::string& kernel_name, bool enable_scaling)
{
    auto visitor = MakeRTCVisitor(kernel_name, enable_scaling);
    return visitor(f);
}
