
// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <complex.h>

#include "..\root\mem.h"
#include "port.h"
#include "mtype.h"
#include "init.h"
#include "expression.h"
#include "template.h"

/******************************** Expression **************************/

Expression::Expression(Loc loc, enum TOK op, int size)
    : loc(loc)
{
    this->loc = loc;
    this->op = op;
    this->size = size;
    type = NULL;
}

Expression *Expression::syntaxCopy()
{
    return copy();
}

/*********************************
 * Does *not* do a deep copy.
 */

Expression *Expression::copy()
{
    Expression *e;
    if (!size)
	printf("No expression copy for: %s\n", toChars());
    e = (Expression *)mem.malloc(size);
    return memcpy(e, this, size);
}

/**************************
 * Semantically analyze Expression.
 * Determine types, fold constants, etc.
 */

Expression *Expression::semantic(Scope *sc)
{
    if (type)
	type = type->semantic(loc, sc);
    else
	type = Type::tvoid;
    return this;
}

void Expression::print()
{
    printf("%s\n", toChars());
    fflush(stdout);
}

char *Expression::toChars()
{   OutBuffer *buf;

    buf = new OutBuffer();
    toCBuffer(buf);
    return buf->toChars();
}

void Expression::error(const char *format, ...)
{
    char *p = loc.toChars();

    if (*p)
	printf("%s: ", p);
    mem.free(p);

    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    printf("\n");
    fflush(stdout);

    global.errors++;
    fatal();
}

void Expression::rvalue()
{
    if (type && type->ty == Tvoid)
	error("voids have no value");
}

Expression *Expression::combine(Expression *e1, Expression *e2)
{
    if (e1)
    {
	if (e2)
	{
	    e1 = new CommaExp(e1->loc, e1, e2);
	    e1->type = e2->type;
	}
    }
    else
	e1 = e2;
    return e1;
}

integer_t Expression::toInteger()
{
    printf("Expression %s\n", Token::toChars(op));
    error("Integer constant expression expected instead of %s", toChars());
    return 0;
}

real_t Expression::toReal()
{
    error("Floating point constant expression expected instead of %s", toChars());
    return 0;
}

real_t Expression::toImaginary()
{
    error("Floating point constant expression expected instead of %s", toChars());
    return 0;
}

complex_t Expression::toComplex()
{
    error("Floating point constant expression expected instead of %s", toChars());
    return 0;
}

void Expression::toCBuffer(OutBuffer *buf)
{
    buf->writestring(Token::toChars(op));
}

/*******************************
 * Give error if we're not an lvalue.
 * If we can, convert expression to be an lvalue.
 */

Expression *Expression::toLvalue()
{
    error("'%s' is not an lvalue", toChars());
    return this;
}

Expression *Expression::modifiableLvalue(Scope *sc)
{
    // See if this expression is a modifiable lvalue (i.e. not const)
    return toLvalue();
}

void Expression::checkScalar()
{
    if (!type->isscalar())
	error("'%s' is not a scalar, it is a %s", toChars(), type->toChars());
}

void Expression::checkIntegral()
{
    if (!type->isintegral())
	error("'%s' is not an integral type", toChars());
}

void Expression::checkArithmetic()
{
    if (!type->isintegral() && !type->isfloating())
	error("'%s' is not an arithmetic type", toChars());
}

void Expression::checkDeprecated(Dsymbol *s)
{
    if (!global.params.useDeprecated && s->isDeprecated())
	error("%s %s is deprecated", s->kind(), s->toChars());
}

/*****************************
 * Check that expression can be tested for true or false.
 */

void Expression::checkBoolean()
{
    // Default is 'yes' - do nothing

    if (!type->checkBoolean())
	error("%s does not have a boolean value", type->toChars());
}

/****************************
 */

Expression *Expression::checkToPointer()
{
    Expression *e;

    //printf("Expression::checkToPointer()\n");
    e = this;

    // If C array or function, convert to function pointer
    if (type->ty == Tsarray)
    {
	e = new AddrExp(loc, this);
	e->type = type->next->pointerTo();
    }
    return e;
}

/******************************
 * Take address of expression.
 */

Expression *Expression::addressOf()
{
    Expression *e;

    //printf("Expression::addressOf()\n");
    e = toLvalue();
    e = new AddrExp(loc, e);
    e->type = type->pointerTo();
    return e;
}

/******************************
 * If this is a reference, dereference it.
 */

Expression *Expression::deref()
{
    //printf("Expression::deref()\n");
    if (type->ty == Treference)
    {	Expression *e;

	e = new PtrExp(loc, this);
	e->type = type->next;
	return e;
    }
    return this;
}

/********************************
 * Does this expression statically evaluate to a boolean TRUE or FALSE?
 */

int Expression::isBool(int result)
{
    return FALSE;
}

/********************************
 * Does this expression result in either a 1 or a 0?
 */

int Expression::isBit()
{
    return FALSE;
}

Array *Expression::arraySyntaxCopy(Array *exps)
{   Array *a = NULL;

    if (exps)
    {
	a = new Array();
	a->setDim(exps->dim);
	for (int i = 0; i < a->dim; i++)
	{   Expression *e = (Expression *)exps->data[i];

	    e = e->syntaxCopy();
	    a->data[i] = e;
	}
    }
    return a;
}

/******************************** IntegerExp **************************/

IntegerExp::IntegerExp(Loc loc, integer_t value, Type *type)
	: Expression(loc, TOKint64, sizeof(IntegerExp))
{
    this->type = type;
    this->value = value;
}

int IntegerExp::equals(Object *o)
{   IntegerExp *ne;

    if (this == o ||
	((ne = dynamic_cast<IntegerExp *>(o)) != NULL &&
	 type->equals(ne->type) &&
	 value == ne->value))
	return 1;
    return 0;
}

char *IntegerExp::toChars()
{
    static char buffer[sizeof(value) * 3 + 1];

    sprintf(buffer, "%lld", value);
    return buffer;
}

integer_t IntegerExp::toInteger()
{   Type *t;

    t = type;
    while (t)
    {
	switch (t->ty)
	{
	    case Tbit:		value &= 1;			break;
	    case Tint8:		value = (d_int8)  value;	break;
	    case Tascii:
	    case Tuns8:		value = (d_uns8)  value;	break;
	    case Tint16:	value = (d_int16) value;	break;
	    case Twchar:
	    case Tuns16:	value = (d_uns16) value;	break;
	    case Tint32:	value = (d_int32) value;	break;
	    case Tpointer:
	    case Tuns32:	value = (d_uns32) value;	break;
	    case Tint64:	value = (d_int64) value;	break;
	    case Tuns64:	value = (d_uns64) value;	break;

	    case Tenum:
		TypeEnum *te = (TypeEnum *)t;
		t = te->sym->memtype;
		continue;

	    case Ttypedef:
		TypeTypedef *tt = (TypeTypedef *)t;
		t = tt->sym->basetype;
		continue;

	    default:
		print();
		type->print();
		assert(0);
		break;
	}
	break;
    }
    return value;
}

real_t IntegerExp::toReal()
{
    Type *t;

    toInteger();
    t = type->toBasetype();
    if (t->ty == Tuns64)
	return (real_t)(d_uns64)value;
    else
	return (real_t)(d_int64)value;
}

real_t IntegerExp::toImaginary()
{
    return (real_t) 0;
}

int IntegerExp::isBool(int result)
{
    return result ? value != 0 : value == 0;
}

Expression *IntegerExp::semantic(Scope *sc)
{
    if (!type)
    {
	// Determine what the type of this number is
	integer_t number = value;

	if (number & 0x8000000000000000)
	    type = Type::tuns64;
	else if (number & 0xFFFFFFFF80000000)
	    type = Type::tint64;
	else
	    type = Type::tint32;
    }
    else
	type = type->semantic(loc, sc);
    return this;
}

void IntegerExp::toCBuffer(OutBuffer *buf)
{
    if (type)
    {
	if (type->ty == Tenum)
	{   TypeEnum *te = (TypeEnum *)type;

	    buf->printf("cast(%s)", te->sym->toChars());
	}
    }
    if (value & 0x8000000000000000)
	buf->printf("0x%llx", value);
    else
	buf->printf("%lld", value);
}

/******************************** RealExp **************************/

RealExp::RealExp(Loc loc, real_t value, Type *type)
	: Expression(loc, TOKfloat64, sizeof(RealExp))
{
    //printf("RealExp::RealExp(%Lg)\n", value);
    this->value = value;
    this->type = type;
}

char *RealExp::toChars()
{
    static char buffer[sizeof(value) * 3 + 8 + 1];

    sprintf(buffer, "%Lg", value);
    assert(strlen(buffer) < sizeof(buffer));
    return buffer;
}

integer_t RealExp::toInteger()
{
    return (integer_t) value;
}

real_t RealExp::toReal()
{
    return value;
}

real_t RealExp::toImaginary()
{
    return 0;
}

complex_t RealExp::toComplex()
{
    return value;
}

Expression *RealExp::semantic(Scope *sc)
{
    if (!type)
	type = Type::tfloat64;
    else
	type = type->semantic(loc, sc);
    return this;
}

int RealExp::isBool(int result)
{
    return result ? (value != 0)
		  : (value == 0);
}

void RealExp::toCBuffer(OutBuffer *buf)
{
    buf->printf("%Lg", value);
}


/******************************** ImaginaryExp **************************/

ImaginaryExp::ImaginaryExp(Loc loc, real_t value, Type *type)
	: Expression(loc, TOKimaginary, sizeof(ImaginaryExp))
{
    this->value = value;
    this->type = type;
}

char *ImaginaryExp::toChars()
{
    static char buffer[sizeof(value) * 3 + 8 + 1];

    sprintf(buffer, "%Lgi", value);
    assert(strlen(buffer) < sizeof(buffer));
    return buffer;
}

integer_t ImaginaryExp::toInteger()
{
    return 0;
}

real_t ImaginaryExp::toReal()
{
    return 0;
}

real_t ImaginaryExp::toImaginary()
{
    return value;
}

complex_t ImaginaryExp::toComplex()
{
    return value;
}

Expression *ImaginaryExp::semantic(Scope *sc)
{
    if (!type)
	type = Type::timaginary80;
    else
	type = type->semantic(loc, sc);
    return this;
}

int ImaginaryExp::isBool(int result)
{
    return result ? (value != 0)
		  : (value == 0);
}

void ImaginaryExp::toCBuffer(OutBuffer *buf)
{
    buf->printf("%Lgi", value);
}


/******************************** ComplexExp **************************/

ComplexExp::ComplexExp(Loc loc, complex_t value, Type *type)
	: Expression(loc, TOKcomplex, sizeof(ComplexExp))
{
    this->value = value;
    this->type = type;
}

char *ComplexExp::toChars()
{
    static char buffer[sizeof(value) * 3 + 8 + 1];

    sprintf(buffer, "(%Lg+%Lgi)", creall(value), cimagl(value));
    assert(strlen(buffer) < sizeof(buffer));
    return buffer;
}

integer_t ComplexExp::toInteger()
{
    return (integer_t) value;
}

real_t ComplexExp::toReal()
{
    return (real_t) value;
}

real_t ComplexExp::toImaginary()
{
    return cimagl(value);
}

complex_t ComplexExp::toComplex()
{
    return value;
}

Expression *ComplexExp::semantic(Scope *sc)
{
    if (!type)
	type = Type::tcomplex80;
    else
	type = type->semantic(loc, sc);
    return this;
}

int ComplexExp::isBool(int result)
{
    return result ? (value != 0)
		  : (value == 0);
}

void ComplexExp::toCBuffer(OutBuffer *buf)
{
    buf->printf("(%Lg+%Lgi)", creall(value), cimagl(value));
}


/******************************** IdentifierExp **************************/

IdentifierExp::IdentifierExp(Loc loc, Identifier *ident)
	: Expression(loc, TOKidentifier, sizeof(IdentifierExp))
{
    this->ident = ident;
}

Expression *IdentifierExp::semantic(Scope *sc)
{
    Dsymbol *s;
    Dsymbol *scopesym;

    s = sc->search(ident, &scopesym);
    if (s)
    {
	EnumMember *em;
	Expression *e;
	VarDeclaration *v;
	FuncDeclaration *f;
	Declaration *d;
	ClassDeclaration *cd;
	ClassDeclaration *thiscd = NULL;
	WithScopeSymbol *withsym;
	Import *imp;
	Type *t;

	//printf("'%s' is a symbol\n", toChars());
	s = s->toAlias();
	checkDeprecated(s);

	// See if it was a with class
	withsym = dynamic_cast<WithScopeSymbol *>(scopesym);
	if (withsym)
	{
	    // Same as wthis.ident
	    Expression *e = new VarExp(loc, withsym->withstate->wthis);
	    e = new DotIdExp(loc, e, ident);
	    return e->semantic(sc);
	}

	if (sc->func)
	    thiscd = dynamic_cast<ClassDeclaration *>(sc->func->parent);

	if (s->needThis())
	{
	    // Supply an implicit 'this', as in
	    //	  this.ident

	    DotVarExp *de;

	    de = new DotVarExp(loc, new ThisExp(loc), dynamic_cast<Declaration *>(s));
	    return de->semantic(sc);
	}

	em = dynamic_cast<EnumMember *>(s);
	if (em)
	{
	    e = em->value;
	    e = e->semantic(sc);
	    return e;
	}
	v = dynamic_cast<VarDeclaration *>(s);
	if (v)
	{
	    //printf("Identifier '%s' is a variable, type '%s'\n", toChars(), v->type->toChars());
	    type = v->type;
	    if (v->isConst())
	    {
		ExpInitializer *ei = dynamic_cast<ExpInitializer *>(v->init);
		if (ei && ei->exp->type == type)
		{   e = ei->exp->copy();	// make copy so we can change loc
		    e->loc = loc;
		    return e;
		}
	    }
	    e = new VarExp(loc, v);
	    return e->deref();
	}
	f = dynamic_cast<FuncDeclaration *>(s);
	if (f)
	{
	    //printf("it's a function\n");
	    return new VarExp(loc, f);
	}
	cd = dynamic_cast<ClassDeclaration *>(s);
	if (cd && thiscd && cd->isBaseOf(thiscd, NULL) && sc->func->needThis())
	{
	    // We need to add an implicit 'this' if cd is this class or a base class.
	    DotTypeExp *dte;

	    dte = new DotTypeExp(loc, new ThisExp(loc), s);
	    return dte->semantic(sc);
	}
	imp = dynamic_cast<Import *>(s);
	if (imp)
	{
	    ScopeExp *ie;

	    ie = new ScopeExp(loc, imp->pkg);
	    return ie->semantic(sc);
	}

	t = s->getType();
	if (t)
	{
	    return new TypeExp(loc, t);
	}

	TemplateInstance *ti = dynamic_cast<TemplateInstance *>(s);
	if (ti)
	{
	    return new ScopeExp(loc, ti);
	}

	error("%s '%s' is not a variable", s->kind(), s->toChars());
    }
    error("undefined identifier %s", ident->toChars());
    return this;
}

char *IdentifierExp::toChars()
{
    return ident->toChars();
}

void IdentifierExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring(ident->toChars());
}

Expression *IdentifierExp::toLvalue()
{
#if 0
    tym = tybasic(e1->ET->Tty);
    if (!(tyscalar(tym) ||
	  tym == TYstruct ||
	  tym == TYarray && e->Eoper == TOKaddr))
	    synerr(EM_lvalue);	// lvalue expected
#endif
    return this;
}

/******************************** ThisExp **************************/

ThisExp::ThisExp(Loc loc)
	: Expression(loc, TOKthis, sizeof(ThisExp))
{
}

Expression *ThisExp::semantic(Scope *sc)
{   FuncDeclaration *fd;
    Expression *e;

    fd = sc->func;
    if (!fd)
    {	error("'this' is only allowed in member functions");
	e = this;
    }
    else if (!fd->vthis)
    {	error("no 'this' in '%s'", fd->toChars());
	e = this;
    }
    else
    {	e = this;
	e->type = fd->vthis->type;
    }
    sc->callSuper |= CSXthis;
    return e;
}

int ThisExp::isBool(int result)
{
    return result ? TRUE : FALSE;
}

void ThisExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring("this");
}

Expression *ThisExp::toLvalue()
{
    return this;
}

/******************************** SuperExp **************************/

SuperExp::SuperExp(Loc loc)
	: Expression(loc, TOKsuper, sizeof(SuperExp))
{
}

Expression *SuperExp::semantic(Scope *sc)
{   FuncDeclaration *fd;
    Expression *e;
    ClassDeclaration *cd;

    fd = sc->func;
    if (!fd)
    {	error("'super' is only allowed in member functions");
	e = this;
    }
    else if (!fd->vthis)
    {	error("no 'this' in '%s'", fd->toChars());
	e = this;
    }
    else
    {	e = this;
	cd = dynamic_cast<ClassDeclaration *>(fd->parent);
	assert(cd);
	if (!cd->baseClass)
	{
	    error("no base class for %s", cd->toChars());
	    e->type = fd->vthis->type;
	}
	else
	{
	    e->type = cd->baseClass->type;
	}
    }
    sc->callSuper |= CSXsuper;
    return e;
}

int SuperExp::isBool(int result)
{
    return result ? TRUE : FALSE;
}

void SuperExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring("super");
}

/******************************** NullExp **************************/

NullExp::NullExp(Loc loc)
	: Expression(loc, TOKnull, sizeof(NullExp))
{
}

Expression *NullExp::semantic(Scope *sc)
{
    // NULL is the same as (void *)0
    if (!type)
	type = Type::tvoid->pointerTo();
    return this;
}

int NullExp::isBool(int result)
{
    return result ? FALSE : TRUE;
}

void NullExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring("null");
}

/******************************** StringExp **************************/

StringExp::StringExp(Loc loc, wchar_t *string, unsigned len)
	: Expression(loc, TOKstring, sizeof(StringExp))
{
    this->string = string;
    this->len = len;
    this->committed = 0;
}

char *StringExp::toChars()
{
    OutBuffer buf;
    char *p;

    toCBuffer(&buf);
    p = (char *)buf.data;
    buf.data = NULL;
    return p;
}

Expression *StringExp::semantic(Scope *sc)
{
    //printf("StringExp::semantic()\n");
    type = new TypeSArray(Type::twchar, new IntegerExp(loc, len, Type::tindex));
    type = type->semantic(loc, sc);
    assert(type);
    return this;
}

int StringExp::compare(Object *obj)
{
    // Used to sort case statement expressions so we can do an efficient lookup
    StringExp *se2 = dynamic_cast<StringExp *>(obj);
    assert(se2);

    wchar_t *s1 = string;
    wchar_t *s2 = se2->string;

    int len1 = len;
    int len2 = se2->len;

    if (len1 == len2)
	return wcscmp(s1, s2);
    else
	return len1 - len2;
}

int StringExp::isBool(int result)
{
    return result ? TRUE : FALSE;
}

void StringExp::toCBuffer(OutBuffer *buf)
{   wchar_t *s;

    buf->writeByte('"');
    for (s = string; 1; s++)
    {	wchar_t c;

	c = *s;
	switch (c)
	{
	    case 0:
		break;

	    case '"':
		buf->writeByte('\\');
	    default:
		if (isprint(c))
		    buf->writeByte(c);
		else if (c & ~0xFF)
		{
		    buf->printf("\\u%04x", c);
		}
		else
		    buf->printf("\\x%02x", c);
		continue;
	}
	break;
    }
    buf->writeByte('"');
}

/************************************************************/

TypeDotIdExp::TypeDotIdExp(Loc loc, Type *type, Identifier *ident)
    : Expression(loc, TOKtypedot, sizeof(TypeDotIdExp))
{
    this->type = type;
    this->ident = ident;
}

Expression *TypeDotIdExp::semantic(Scope *sc)
{   Expression *e;

    //printf("TypeDotIdExp::semantic()\n");
    type = type->semantic(loc, sc);
    e = type->getProperty(loc, ident);
    e = e->semantic(sc);
    return e;
}

void TypeDotIdExp::toCBuffer(OutBuffer *buf)
{
    buf->writeByte('(');
    type->toCBuffer(buf, NULL);
    buf->writeByte(')');
    buf->writeByte('.');
    buf->writestring(ident->toChars());
}

/************************************************************/

// Mainly just a placeholder

TypeExp::TypeExp(Loc loc, Type *type)
    : Expression(loc, TOKtype, sizeof(TypeExp))
{
    this->type = type;
}

void TypeExp::toCBuffer(OutBuffer *buf)
{
    type->toCBuffer(buf, NULL);
}

/************************************************************/

// Mainly just a placeholder

ScopeExp::ScopeExp(Loc loc, ScopeDsymbol *pkg)
    : Expression(loc, TOKimport, sizeof(ScopeExp))
{
    this->sds = pkg;
}


void ScopeExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring("import ");
    buf->writestring(sds->toChars());
}

/********************** NewExp **************************************/

NewExp::NewExp(Loc loc, Type *type, Array *arguments)
    : Expression(loc, TOKnew, sizeof(NewExp))
{
    this->type = type;
    this->arguments = arguments;
    member = NULL;
}

Expression *NewExp::syntaxCopy()
{
    return new NewExp(loc, type->syntaxCopy(), arraySyntaxCopy(arguments));
}


Expression *NewExp::semantic(Scope *sc)
{   int i;
    Type *tb;

    //printf("NewExp::semantic()\n");
    //printf("type: %s\n", type->toChars());
    type = type->semantic(loc, sc);
    tb = type->toBasetype();
    //printf("tb: %s\n", tb->toChars());

    if (arguments)
    {
	for (i = 0; i < arguments->dim; i++)
	{   Expression *arg = (Expression *)arguments->data[i];

	    arg = arg->semantic(sc);
	    arguments->data[i] = (void *)arg;
	}
    }

    if (tb->ty == Tclass)
    {	ClassDeclaration *cd;
	TypeClass *tc;
	FuncDeclaration *f;
	TypeFunction *tf;
	unsigned nargs;
	unsigned nproto;
	int i;

	tc = dynamic_cast<TypeClass *>(tb);
	cd = dynamic_cast<ClassDeclaration *>(tc->sym);
	if (cd->isInterface())
	    error("cannot create instance of interface %s", cd->toChars());
	f = cd->ctor;
	if (f)
	{
	    assert(f);
	    f = f->overloadResolve(loc, arguments);
	    assert(f->isConstructor());
	    member = dynamic_cast<CtorDeclaration *>(f);
	    assert(member);

	    tf = (TypeFunction *)f->type;
	    type = tf->next;

	    nargs = arguments ? arguments->dim : 0;
	    nproto = tf->arguments ? tf->arguments->dim : 0;

	    if (nargs != nproto)
	    {
		if (nargs < nproto || !tf->varargs)
		    error("expected %d arguments to constructor, not %d\n", nproto, nargs);
	    }

	    for (i = 0; i < nargs; i++)
	    {   Expression *arg = (Expression *)arguments->data[i];

		if (i < nproto)
		{
		    Argument *p = (Argument *)tf->arguments->data[i];

		    arg = arg->implicitCastTo(p->type);
		    if (p->inout == Out || p->inout == InOut)
		    {
			arg = arg->modifiableLvalue(sc);
		    }
		    // Convert static arrays to pointers
		    if (arg->type->ty == Tsarray)
		    {
			arg = arg->checkToPointer();
		    }
		}
		else
		{
		    // Promote bytes, words, etc., to ints
		    arg = arg->integralPromotions();

		    // If not D linkage, promote floats to doubles
		    if (tf->linkage != LINKd)
		    {
			switch (arg->type->ty)
			{
			    case Tfloat32:
				arg = arg->castTo(Type::tfloat64);
				break;

			    case Timaginary32:
				arg = arg->castTo(Type::timaginary64);
				break;
			}
		    }

		    // Convert static arrays to dynamic arrays
		    if (arg->type->ty == Tsarray)
		    {
			arg = arg->castTo(arg->type->arrayOf());
		    }
		}
		arguments->data[i] = (void *) arg;
	    }
	}
	else
	{
	    if (arguments && arguments->dim)
		error("no constructor for %s", cd->toChars());
	}
    }
    else if (tb->ty == Tarray && (arguments && arguments->dim))
    {	Expression *arg;

	arg = (Expression *)arguments->data[0];
	arg = arg->implicitCastTo(Type::tindex);
	arguments->data[0] = (void *) arg;
    }
    else
    {
	error("new can only create arrays or class objects, not %s's", type->toChars());
	type = type->pointerTo();
    }

//printf("NewExp: '%s'\n", toChars());
//printf("NewExp:type '%s'\n", type->toChars());

    return this;
}

void NewExp::toCBuffer(OutBuffer *buf)
{   int i;

    buf->writestring("new ");
    type->toCBuffer(buf, NULL);
    if (arguments && arguments->dim)
    {
	buf->writeByte('(');
	for (i = 0; i < arguments->dim; i++)
	{   Expression *arg = (Expression *)arguments->data[i];

	    arg->toCBuffer(buf);
	}
	buf->writeByte(')');
    }
}

/********************** SymOffExp **************************************/

SymOffExp::SymOffExp(Loc loc, Declaration *var, unsigned offset)
    : Expression(loc, TOKsymoff, sizeof(SymOffExp))
{
    assert(var);
    this->var = var;
    this->offset = offset;
}

Expression *SymOffExp::semantic(Scope *sc)
{
    var->semantic(sc);
    type = var->type->pointerTo();
    return this;
}

void SymOffExp::toCBuffer(OutBuffer *buf)
{
    if (offset)
	buf->printf("(&%s+%u)", var->toChars(), offset);
    else
	buf->printf("&%s", var->toChars());
}

int SymOffExp::isConst()
{
    return TRUE;
}

/******************************** VarExp **************************/

VarExp::VarExp(Loc loc, Declaration *var)
	: Expression(loc, TOKvar, sizeof(VarExp))
{
    this->var = var;
    this->type = var->type;
}

Expression *VarExp::semantic(Scope *sc)
{
    //printf("VarExp::semantic(%s)\n", toChars());
    if (var->isConst())
    {
	VarDeclaration *v = dynamic_cast<VarDeclaration *>(var);
	if (v)
	{
	    ExpInitializer *ei = dynamic_cast<ExpInitializer *>(v->init);
	    if (ei && ei->exp->type == type)
		return ei->exp;
	}
    }
    return this;
}

char *VarExp::toChars()
{
    return var->toChars();
}

void VarExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring(var->toChars());
}

Expression *VarExp::toLvalue()
{
#if 0
    tym = tybasic(e1->ET->Tty);
    if (!(tyscalar(tym) ||
	  tym == TYstruct ||
	  tym == TYarray && e->Eoper == TOKaddr))
	    synerr(EM_lvalue);	// lvalue expected
#endif
    return this;
}

Expression *VarExp::modifiableLvalue(Scope *sc)
{
    if (sc->incontract && var->isParameter())
	error("cannot modify parameter '%s' in contract", var->toChars());

    if (type && type->ty == Tsarray)
	error("cannot change reference to static array '%s'", var->toChars());

    // See if this expression is a modifiable lvalue (i.e. not const)
    return toLvalue();
}

/******************************** DeclarationExp **************************/

DeclarationExp::DeclarationExp(Loc loc, Dsymbol *declaration)
	: Expression(loc, TOKdeclaration, sizeof(DeclarationExp))
{
    this->declaration = declaration;
}

Expression *DeclarationExp::syntaxCopy()
{
    return new DeclarationExp(loc, declaration->syntaxCopy(NULL));
}

Expression *DeclarationExp::semantic(Scope *sc)
{
    //printf("inserting '%s' %p into sc = %p\n", declaration->toChars(), declaration, sc);
    if (!sc->insert(declaration))
	error("declaration %s.%s is already defined", sc->func->toChars(), declaration->toChars());
    declaration->semantic(sc);

    type = Type::tvoid;
    return this;
}

void DeclarationExp::toCBuffer(OutBuffer *buf)
{
    declaration->toCBuffer(buf);
}


/************************************************************/

UnaExp::UnaExp(Loc loc, enum TOK op, int size, Expression *e1)
	: Expression(loc, op, size)
{
    this->e1 = e1;
}

Expression *UnaExp::syntaxCopy()
{   UnaExp *e;

    e = (UnaExp *)copy();
    e->e1 = e->e1->syntaxCopy();
    return e;
}

Expression *UnaExp::semantic(Scope *sc)
{
    e1 = e1->semantic(sc);
    return this;
}

void UnaExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring(Token::toChars(op));
    e1->toCBuffer(buf);
}

/************************************************************/

BinExp::BinExp(Loc loc, enum TOK op, int size, Expression *e1, Expression *e2)
	: Expression(loc, op, size)
{
    this->e1 = e1;
    this->e2 = e2;
}

Expression *BinExp::syntaxCopy()
{   BinExp *e;

    e = (BinExp *)copy();
    e->e1 = e->e1->syntaxCopy();
    e->e2 = e->e2->syntaxCopy();
    return e;
}

Expression *BinExp::semantic(Scope *sc)
{
    e1 = e1->semantic(sc);
    e2 = e2->semantic(sc);
    return this;
}

/***************************
 * Common semantic routine for some xxxAssignExp's.
 */

Expression *BinExp::commonSemanticAssign(Scope *sc)
{   Expression *e;

    if (!type)
    {
	BinExp::semantic(sc);

	e = op_overload(sc);
	if (e)
	    return e;

	e1 = e1->modifiableLvalue(sc);
	e1->checkScalar();
	type = e1->type;
	typeCombine();
	e1->checkArithmetic();
	e2->checkArithmetic();
    }
    return this;
}

void BinExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writeByte(' ');
    buf->writestring(Token::toChars(op));
    buf->writeByte(' ');
    e2->toCBuffer(buf);
}

int BinExp::isunsigned()
{
    return e1->type->isunsigned() || e2->type->isunsigned();
}

/************************************************************/

AssertExp::AssertExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKassert, sizeof(AssertExp), e)
{
}

Expression *AssertExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    // BUG: see if we can do compile time elimination of the Assert
    e1->checkBoolean();
    type = Type::tvoid;
    return this;
}

void AssertExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring("assert(");
    e1->toCBuffer(buf);
    buf->writeByte(')');
}

/************************************************************/

DotIdExp::DotIdExp(Loc loc, Expression *e, Identifier *ident)
	: UnaExp(loc, TOKdot, sizeof(DotIdExp), e)
{
    this->ident = ident;
}

Expression *DotIdExp::semantic(Scope *sc)
{   Expression *e;

    //printf("DotIdExp::semantic('%s')\n", toChars());
    UnaExp::semantic(sc);
    if (e1->op == TOKimport)	// also used for template alias's
    {
	Dsymbol *s;
	ScopeExp *ie = (ScopeExp *)e1;

	s = ie->sds->search(ident);
	if (s)
	{
	    s = s->toAlias();
	    checkDeprecated(s);

	    EnumMember *em = dynamic_cast<EnumMember *>(s);
	    if (em)
	    {
		e = em->value;
		e = e->semantic(sc);
		return e;
	    }

	    VarDeclaration *v = dynamic_cast<VarDeclaration *>(s);
	    if (v)
	    {
		//printf("Identifier '%s' is a variable, type '%s'\n", toChars(), v->type->toChars());
		type = v->type;
		if (v->isConst())
		{
		    ExpInitializer *ei = dynamic_cast<ExpInitializer *>(v->init);
		    if (ei && ei->exp->type == type)
		    {   e = ei->exp->copy();	// make copy so we can change loc
			e->loc = loc;
			return e;
		    }
		}
		e = new VarExp(loc, v);
		return e->deref();
	    }

	    FuncDeclaration *f = dynamic_cast<FuncDeclaration *>(s);
	    if (f)
	    {
		//printf("it's a function\n");
		return new VarExp(loc, f);
	    }

	    ScopeDsymbol *sds = dynamic_cast<ScopeDsymbol *>(s);
	    if (sds)
	    {
		//printf("it's a ScopeDsymbol\n");
		return new ScopeExp(loc, sds);
	    }

	    // BUG: handle other cases like in IdentifierExp::semantic()
	    assert(0);
	}
	error("undefined identifier %s", toChars());
	return this;
    }
    else if (e1->type->ty == Tpointer && ident != Id::size)
    {
	e = new PtrExp(loc, e1);
	e->type = e1->type->next;
	return e->type->dotExp(sc, e, ident);
    }
    else
    {
	e = e1->type->dotExp(sc, e1, ident);
	e = e->semantic(sc);
	return e;
    }
}

void DotIdExp::toCBuffer(OutBuffer *buf)
{
    //printf("DotIdExp::toCbuffer()\n");
    e1->toCBuffer(buf);
    buf->writeByte('.');
    buf->writestring(ident->toChars());
}

/************************************************************/

DotVarExp::DotVarExp(Loc loc, Expression *e, Declaration *v)
	: UnaExp(loc, TOKdotvar, sizeof(DotVarExp), e)
{
    //printf("DotVarExp()\n");
    this->var = v;
}

Expression *DotVarExp::semantic(Scope *sc)
{
    //printf("DotVarExp::semantic(%p)\n", this);
    if (!type)
    {
	e1 = e1->semantic(sc);
	var = dynamic_cast<Declaration *>(var->toAlias());
	type = var->type;
	assert(type);

	accessCheck(loc, sc, e1, var);
    }
    return this;
}

Expression *DotVarExp::toLvalue()
{
    return this;
}

void DotVarExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writeByte('.');
    buf->writestring(var->toChars());
}

/************************************************************/

DelegateExp::DelegateExp(Loc loc, Expression *e, FuncDeclaration *f)
	: UnaExp(loc, TOKdelegate, sizeof(DelegateExp), e)
{
    this->func = f;
}

Expression *DelegateExp::semantic(Scope *sc)
{
    e1 = e1->semantic(sc);
    type = new TypeDelegate(func->type);
    type = type->semantic(loc, sc);
    return this;
}

void DelegateExp::toCBuffer(OutBuffer *buf)
{
    buf->writeByte('&');
    e1->toCBuffer(buf);
    buf->writeByte('.');
    buf->writestring(func->toChars());
}

/************************************************************/

DotTypeExp::DotTypeExp(Loc loc, Expression *e, Dsymbol *s)
	: UnaExp(loc, TOKdottype, sizeof(DotTypeExp), e)
{
    this->sym = s;
    this->type = s->getType();
}

Expression *DotTypeExp::semantic(Scope *sc)
{
    return this;
}

void DotTypeExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writeByte('.');
    buf->writestring(sym->toChars());
}

/************************************************************/

ArrowExp::ArrowExp(Loc loc, Expression *e, Identifier *ident)
	: UnaExp(loc, TOKarrow, sizeof(ArrowExp), e)
{
    this->ident = ident;
}

Expression *ArrowExp::semantic(Scope *sc)
{   Expression *e;

    UnaExp::semantic(sc);
    e1 = e1->checkToPointer();
    if (e1->type->ty != Tpointer)
	error("pointer expected before ->, not '%s'", e1->type->toChars());
    e = new PtrExp(loc, e1);
    e = new DotIdExp(loc, e, ident);
    e = e->semantic(sc);
    return e;
}

void ArrowExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writestring("->");
    buf->writestring(ident->toChars());
}

/************************************************************/

CallExp::CallExp(Loc loc, Expression *e, Array *arguments)
	: UnaExp(loc, TOKcall, sizeof(CallExp), e)
{
    this->arguments = arguments;
}

Expression *CallExp::syntaxCopy()
{
    return new CallExp(loc, e1->syntaxCopy(), arraySyntaxCopy(arguments));
}


Expression *CallExp::semantic(Scope *sc)
{
    unsigned nargs;
    unsigned nproto;
    TypeFunction *tf;
    FuncDeclaration *f;
    int i;
    Type *t1;

    //printf("CallExp::semantic(): %s\n", toChars());
    if (type)
	return this;		// semantic() already run

    // Transform array.id(args) into id(array,args)
    if (e1->op == TOKdot)
    {
	// BUG: we should handle array.a.b.c.e(args) too

	DotIdExp *dotid = dynamic_cast<DotIdExp *>(e1);
	assert(dotid);
	dotid->e1 = dotid->e1->semantic(sc);
	assert(dotid->e1);
	if (dotid->e1->type)
	{
	    TY e1ty = dotid->e1->type->ty;
	    if (e1ty == Tarray || e1ty == Tsarray || e1ty == Taarray)
	    {
		if (!arguments)
		    arguments = new Array();
		arguments->shift(dotid->e1);
		e1 = new IdentifierExp(dotid->loc, dotid->ident);
	    }
	}
    }

    if (e1->op == TOKcomma)
    {
	CommaExp *ce = (CommaExp *)e1;

	e1 = ce->e2;
	e1->type = ce->type;
	ce->e2 = this;
	ce->type = NULL;
	return ce->semantic(sc);
    }

    if (e1->op == TOKthis || e1->op == TOKsuper)
    {
	// semantic() run later for these
    }
    else
	UnaExp::semantic(sc);

    if (arguments)
    {
	for (i = 0; i < arguments->dim; i++)
	{   Expression *arg = (Expression *)arguments->data[i];

	    arg = arg->semantic(sc);
	    arguments->data[i] = (void *)arg;
	}
    }

    t1 = NULL;
    if (e1->type)
	t1 = e1->type->toBasetype();
    if (e1->op == TOKdotvar && t1->ty == Tfunction)
    {
	// Do overload resolution
	DotVarExp *dve = dynamic_cast<DotVarExp *>(e1);

	f = dynamic_cast<FuncDeclaration *>(dve->var);
	assert(f);
	f = f->overloadResolve(loc, arguments);
	dve->var = f;
	e1->type = f->type;
	t1 = e1->type;
    }
    else if (e1->op == TOKsuper)
    {
	// Base class constructor call
	ClassDeclaration *cd = NULL;

	if (sc->func)
	    cd = dynamic_cast<ClassDeclaration *>(sc->func->parent);
	if (!cd || !cd->baseClass || !sc->func->isConstructor())
	{
	    error("super class constructor call must be in a constructor");
	}
	else
	{
	    f = cd->baseClass->ctor;
	    if (!f)
		error("no super class constructor for %s", cd->baseClass->toChars());
	    else
	    {
#if 0
		if (sc->callSuper & (CSXthis | CSXsuper))
		    error("reference to this before super()");
#endif
		if (sc->noctor || sc->callSuper & CSXlabel)
		    error("constructor calls not allowed in loops or after labels");
		if (sc->callSuper & (CSXsuper_ctor | CSXthis_ctor))
		    error("multiple constructor calls");
		sc->callSuper |= CSXany_ctor | CSXsuper_ctor;

		f = f->overloadResolve(loc, arguments);
		e1 = new DotVarExp(e1->loc, e1, f);
		e1 = e1->semantic(sc);
		t1 = e1->type;
	    }
	}
    }
    else if (e1->op == TOKthis)
    {
	// same class constructor call
	ClassDeclaration *cd = NULL;

	if (sc->func)
	    cd = dynamic_cast<ClassDeclaration *>(sc->func->parent);
	if (!cd || !sc->func->isConstructor())
	{
	    error("class constructor call must be in a constructor");
	}
	else
	{
#if 0
	    if (sc->callSuper & (CSXthis | CSXsuper))
		error("reference to this before super()");
#endif
	    if (sc->noctor || sc->callSuper & CSXlabel)
		error("constructor calls not allowed in loops or after labels");
	    if (sc->callSuper & (CSXsuper_ctor | CSXthis_ctor))
		error("multiple constructor calls");
	    sc->callSuper |= CSXany_ctor | CSXthis_ctor;

	    f = cd->ctor;
	    f = f->overloadResolve(loc, arguments);
	    e1 = new DotVarExp(e1->loc, e1, f);
	    e1 = e1->semantic(sc);
	    t1 = e1->type;

	    // BUG: this should really be done by checking the static
	    // call graph
	    if (f == sc->func)
		error("cyclic constructor call");
	}
    }
    else if (!t1)
    {
	error("function expected before (), not '%s'", e1->toChars());
	return this;
    }
    else if (t1->ty != Tfunction)
    {
	if (t1->ty == Tdelegate)
	{
	    tf = dynamic_cast<TypeFunction *>(t1->next);
	    assert(tf);
	    goto Lcheckargs;
	}
	else if (t1->ty == Tpointer && t1->next->ty == Tfunction)
	{   Expression *e;

	    e = new PtrExp(loc, e1);
	    t1 = t1->next;
	    e->type = t1;
	    e1 = e;
	}
	else
	{   error("function expected before (), not '%s'", e1->type->toChars());
	    return this;
	}
    }
    else if (e1->op == TOKvar)
    {
	// Do overload resolution
	VarExp *ve = (VarExp *)e1;

	f = dynamic_cast<FuncDeclaration *>(ve->var);
	assert(f);
	f = f->overloadResolve(loc, arguments);
	ve->var = f;
	ve->type = f->type;
	t1 = f->type;
    }
    tf = dynamic_cast<TypeFunction *>(t1);
    assert(tf);

Lcheckargs:
    assert(tf->ty == Tfunction);
    type = tf->next;

    nargs = arguments ? arguments->dim : 0;
    nproto = tf->arguments ? tf->arguments->dim : 0;

    if (nargs != nproto)
    {
	if (nargs < nproto || !tf->varargs)
	    error("expected %d arguments to function, not %d\n", nproto, nargs);
    }

    // BUG: combine this with argument processing code in NewExp::semantic()
    // to ensure identical behavior
    for (i = 0; i < nargs; i++)
    {   Expression *arg = (Expression *)arguments->data[i];

	if (i < nproto)
	{
	    Argument *p = (Argument *)tf->arguments->data[i];
#if 1
	    arg = arg->implicitCastTo(p->type);
	    if (p->inout == Out || p->inout == InOut)
	    {
		arg = arg->modifiableLvalue(sc);
	    }
#else
	    if (p->inout == Out || p->inout == InOut)
	    {
		if (!arg->type->equals(p->type))
		{
		    error("argument %d is type %s, while %s parameter is type %s",
			i + 1, arg->type->toChars(),
			p->inout == Out ? "out" : "inout",
			p->type->toChars());
		}
		arg = arg->modifiableLvalue(sc);
	    }
	    else
	    {
		arg = arg->implicitCastTo(p->type);
	    }
#endif
	    // Convert static arrays to pointers
	    if (arg->type->ty == Tsarray)
	    {
		arg = arg->checkToPointer();
	    }
	}
	else
	{
	    // Promote bytes, words, etc., to ints
	    arg = arg->integralPromotions();

	    // If not D linkage, promote floats to doubles
	    if (tf->linkage != LINKd)
	    {
		switch (arg->type->ty)
		{
		    case Tfloat32:
			arg = arg->castTo(Type::tfloat64);
			break;

		    case Timaginary32:
			arg = arg->castTo(Type::timaginary64);
			break;
		}
	    }

	    // Convert static arrays to dynamic arrays
	    if (arg->type->ty == Tsarray)
	    {
		arg = arg->castTo(arg->type->arrayOf());
	    }
	}
	arguments->data[i] = (void *) arg;
    }
    assert(type);
    return this;
}

void CallExp::toCBuffer(OutBuffer *buf)
{   int i;

    e1->toCBuffer(buf);
    buf->writeByte('(');
    if (arguments)
    {
	for (i = 0; i < arguments->dim; i++)
	{   Expression *arg = (Expression *)arguments->data[i];

	    if (i)
		buf->writeByte(',');
	    arg->toCBuffer(buf);
	    // BUG: handle varargs
	}
    }
    buf->writeByte(')');
}


/************************************************************/

AddrExp::AddrExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKaddress, sizeof(AddrExp), e)
{
}

Expression *AddrExp::semantic(Scope *sc)
{
    if (!type)
    {
	UnaExp::semantic(sc);
	e1 = e1->toLvalue();
	type = e1->type->pointerTo();

	// See if this should really be a delegate
	if (e1->op == TOKdotvar)
	{
	    DotVarExp *dve = (DotVarExp *)e1;
	    FuncDeclaration *f = dynamic_cast<FuncDeclaration *>(dve->var);

	    if (f)
	    {	Expression *e;

		e = new DelegateExp(loc, dve->e1, f);
		e = e->semantic(sc);
		return e;
	    }
	}
    }
    return this;
}

/************************************************************/

PtrExp::PtrExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKstar, sizeof(PtrExp), e)
{
    if (e->type)
	type = e->type->next;
}

PtrExp::PtrExp(Loc loc, Expression *e, Type *t)
	: UnaExp(loc, TOKstar, sizeof(PtrExp), e)
{
    type = t;
}

Expression *PtrExp::semantic(Scope *sc)
{   Type *tb;

    UnaExp::semantic(sc);
    if (type)
	return this;
    if (!e1->type)
	printf("PtrExp::semantic('%s')\n", toChars());
    tb = e1->type->toBasetype();
    switch (tb->ty)
    {
	case Tpointer:
	    type = tb->next;
	    break;

	case Tsarray:
	case Tarray:
	    type = tb->next;
	    e1 = e1->castTo(type->pointerTo());
	    break;

	default:
	    error("can only * a pointer, not a '%s'", e1->type->toChars());
	    type = Type::tint32;
	    break;
    }
    rvalue();
    return this;
}

Expression *PtrExp::toLvalue()
{
#if 0
    tym = tybasic(e1->ET->Tty);
    if (!(tyscalar(tym) ||
	  tym == TYstruct ||
	  tym == TYarray && e->Eoper == TOKaddr))
	    synerr(EM_lvalue);	// lvalue expected
#endif
    return this;
}

void PtrExp::toCBuffer(OutBuffer *buf)
{
    buf->writeByte('*');
    buf->writeByte('(');
    e1->toCBuffer(buf);
    buf->writeByte(')');
}

/************************************************************/

NegExp::NegExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKneg, sizeof(NegExp), e)
{
}

Expression *NegExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {
	UnaExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;

	e1->checkArithmetic();
	type = e1->type;
    }
    return this;
}

/************************************************************/

ComExp::ComExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKtilde, sizeof(ComExp), e)
{
}

Expression *ComExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {
	UnaExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;

	e1->checkIntegral();
	type = e1->type;
    }
    return this;
}

/************************************************************/

NotExp::NotExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKnot, sizeof(NotExp), e)
{
}

Expression *NotExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    e1->checkBoolean();
    type = Type::tboolean;
    return this;
}

int NotExp::isBit()
{
    return TRUE;
}



/************************************************************/

BoolExp::BoolExp(Loc loc, Expression *e, Type *t)
	: UnaExp(loc, TOKnot, sizeof(BoolExp), e)
{
    type = t;
}

Expression *BoolExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    e1->checkBoolean();
    type = Type::tboolean;
    return this;
}

int BoolExp::isBit()
{
    return TRUE;
}

/************************************************************/

DeleteExp::DeleteExp(Loc loc, Expression *e)
	: UnaExp(loc, TOKdelete, sizeof(DeleteExp), e)
{
}

Expression *DeleteExp::semantic(Scope *sc)
{
    UnaExp::semantic(sc);
    e1 = e1->toLvalue();
    type = Type::tvoid;
    return this;
}

void DeleteExp::checkBoolean()
{
    error("delete does not give a boolean result");
}


/************************************************************/

CastExp::CastExp(Loc loc, Expression *e, Type *t)
	: UnaExp(loc, TOKcast, sizeof(CastExp), e)
{
    to = t;
}

Expression *CastExp::syntaxCopy()
{
    return new CastExp(loc, e1->syntaxCopy(), to->syntaxCopy());
}


Expression *CastExp::semantic(Scope *sc)
{   Expression *e;
    BinExp *b;
    UnaExp *u;

    //printf("CastExp::semantic()\n");
    if (type)
	return this;
    UnaExp::semantic(sc);
    to = to->semantic(loc, sc);

    return e1->castTo(to);
#if 0
    type = to;

    // Do (type *) cast of (type [])
    if (to->ty == Tpointer &&
	e1->type->ty == Tarray
       )
    {
	return this;
#if 0
	// e1 -> *(&e1 + 4)
	//printf("Converting [] to *\n");

	e = new AddrExp(loc, e1);
	e->type = e1->type->next->pointerTo()->pointerTo();

	b = new AddExp(loc, e, new IntegerExp(loc, 4, Type::tint32));
	b->type = e->type;

	u = new PtrExp(loc, b);
	u->type = type;

	return u;
#endif
    }

    if (e1->op == TOKstring)
    {
	return e1->castTo(to);
    }

    // Do (type *) cast of (type [dim])
    if (to->ty == Tpointer &&
	e1->type->ty == Tsarray
       )
    {
	//printf("Converting [dim] to *\n");

	e = new AddrExp(loc, e1);
	e->type = type;

	return e;
    }


    if (e1->op == TOKnull)
    {
	return e1->castTo(to);
    }

    return this;
#endif
}

void CastExp::toCBuffer(OutBuffer *buf)
{
    buf->writestring("cast(");
    to->toCBuffer(buf, NULL);
    buf->writestring(")(");
    e1->toCBuffer(buf);
    buf->writeByte(')');
}


/************************************************************/

ArrayRangeExp::ArrayRangeExp(Loc loc, Expression *e1, Expression *lwr, Expression *upr)
	: UnaExp(loc, TOKrange, sizeof(ArrayRangeExp), e1)
{
    this->upr = upr;
    this->lwr = lwr;
}

Expression *ArrayRangeExp::syntaxCopy()
{
    Expression *lwr = NULL;
    if (this->lwr)
	lwr = this->lwr->syntaxCopy();

    Expression *upr = NULL;
    if (this->upr)
	upr = this->upr->syntaxCopy();

    return new ArrayRangeExp(loc, e1->syntaxCopy(), lwr, upr);
}

Expression *ArrayRangeExp::semantic(Scope *sc)
{   Expression *e;

    //printf("ArrayRangeExp::semantic(%p)\n", sc);
    UnaExp::semantic(sc);

    if (lwr)
    {	lwr = lwr->semantic(sc);
	lwr = lwr->castTo(Type::tindex);	// BUG: implicitCast?
    }
    if (upr)
    {	upr = upr->semantic(sc);
	upr = upr->castTo(Type::tindex);
    }
    e = this;

    Type *t = e1->type->toBasetype();
    if (t->ty == Tpointer)
    {
    }
    else if (t->ty == Tarray)
    {
    }
    else if (t->ty == Tsarray)
    {
    }
    else
	error("incompatible types for array[range], had %s[]", e1->type->toChars());

    type = t->next->arrayOf();
    return e;
}

Expression *ArrayRangeExp::toLvalue()
{
    return this;
}

Expression *ArrayRangeExp::modifiableLvalue(Scope *sc)
{
    error("cannot modify range expression %s", toChars());
    return this;
}

void ArrayRangeExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writeByte('[');
    if (upr || lwr)
    {
	if (lwr)
	    lwr->toCBuffer(buf);
	else
	    buf->writeByte('0');
	buf->writestring("..");
	if (upr)
	    upr->toCBuffer(buf);
	else
	    buf->writestring("length");		// BUG: should be array.length
    }
    buf->writeByte(']');
}

/********************** ArrayLength **************************************/

ArrayLengthExp::ArrayLengthExp(Loc loc, Expression *e1)
	: UnaExp(loc, TOKarraylength, sizeof(ArrayLengthExp), e1)
{
}

Expression *ArrayLengthExp::semantic(Scope *sc)
{   Expression *e;

    //printf("ArrayLengthExp::semantic(%p)\n", sc);
    UnaExp::semantic(sc);

    type = Type::tindex;
    return this;
}

void ArrayLengthExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writestring(".length");
}

/************************* CommaExp ***********************************/

CommaExp::CommaExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKcomma, sizeof(CommaExp), e1, e2)
{
}

Expression *CommaExp::semantic(Scope *sc)
{
    BinExp::semantic(sc);
    type = e2->type;
    return this;
}

Expression *CommaExp::toLvalue()
{
    e2 = e2->toLvalue();
    return this;
}

int CommaExp::isBool(int result)
{
    return e2->isBool(result);
}

/************************************************************/

ArrayExp::ArrayExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKarray, sizeof(ArrayExp), e1, e2)
{
}

Expression *ArrayExp::semantic(Scope *sc)
{   Expression *e;
    BinExp *b;
    UnaExp *u;
    Type *t1;

    //printf("ArrayExp::semantic(): "); print();
    BinExp::semantic(sc);
    e = this;

    // Note that unlike C we do not implement the int[ptr]

    t1 = e1->type->toBasetype();
    switch (t1->ty)
    {
	case Tpointer:
	case Tarray:
	    e2 = e2->implicitCastTo(Type::tindex);
	    e->type = t1->next;
	    break;

	case Tsarray:
	    e2 = e2->implicitCastTo(Type::tindex);
	    if (t1->next->toBasetype()->ty == Tbit)
	    {
		e->type = t1->next;
		break;
	    }

	    TypeSArray *tsa = (TypeSArray *)t1;

	    // Do compile time array bounds checking if possible
	    e2 = e2->optimize(WANTvalue);
	    if (e2->op == TOKint64)
	    {
		integer_t index = e2->toInteger();
		integer_t length = tsa->dim->toInteger();
		if (index < 0 || index >= length)
		    error("array index [%lld] is outside array bounds [0 .. %lld]",
			    index, length);
	    }
	    e->type = t1->next;
	    break;

	case Taarray:
	{   TypeAArray *taa = (TypeAArray *)t1;

	    e1 = e1->modifiableLvalue(sc);
	    e2 = e2->implicitCastTo(taa->index);	// type checking
	    e2 = e2->implicitCastTo(taa->key);		// actual argument type
	    type = taa->next;
	    break;
	}

	default:
	    error("%s must be an array or pointer type, not %s",
		e1->toChars(), e1->type->toChars());
	    break;
    }
    return e;
}

Expression *ArrayExp::toLvalue()
{
    return this;
}

void ArrayExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writeByte('[');
    e2->toCBuffer(buf);
    buf->writeByte(']');
}

/************************************************************/

PostIncExp::PostIncExp(Loc loc, Expression *e)
	: BinExp(loc, TOKplusplus, sizeof(PostIncExp), e, new IntegerExp(loc, 1, Type::tint32))
{
}

Expression *PostIncExp::semantic(Scope *sc)
{   Expression *e = this;

    if (!type)
    {
	BinExp::semantic(sc);

	e = op_overload(sc);
	if (e)
	    return e;

	e = this;
	e1 = e1->modifiableLvalue(sc);
	e1->checkScalar();
	if (e1->type->ty == Tpointer)
	    e = scaleFactor();
	else
	    e2 = e2->castTo(e1->type);
	e->type = e1->type;
    }
    return e;
}

void PostIncExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writestring("++");
}

/************************************************************/

PostDecExp::PostDecExp(Loc loc, Expression *e)
	: BinExp(loc, TOKminusminus, sizeof(PostDecExp), e, new IntegerExp(loc, 1, Type::tint32))
{
}

Expression *PostDecExp::semantic(Scope *sc)
{   Expression *e = this;

    if (!type)
    {
	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;

	e = this;
	e1 = e1->modifiableLvalue(sc);
	e1->checkScalar();
	if (e1->type->ty == Tpointer)
	    e = scaleFactor();
	else
	    e2 = e2->castTo(e1->type);
	e->type = e1->type;
    }
    return e;
}

void PostDecExp::toCBuffer(OutBuffer *buf)
{
    e1->toCBuffer(buf);
    buf->writestring("--");
}

/************************************************************/

AssignExp::AssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKassign, sizeof(AssignExp), e1, e2)
{
}

Expression *AssignExp::semantic(Scope *sc)
{
    //printf("AssignExp::semantic() "); print();
    BinExp::semantic(sc);
    if (e1->op == TOKarraylength)
    {
	// e1 is not an lvalue, but we let code generator handle it
	ArrayLengthExp *ale = (ArrayLengthExp *)e1;

	ale->e1 = ale->e1->modifiableLvalue(sc);
    }
    else
	e1 = e1->toLvalue();

    if (e1->op == TOKrange &&
	!(e1->type->next->equals(e2->type->next) ||
	  (e1->type->next->ty == Tchar && e2->op == TOKstring))
       )
    {	// memset
	e2 = e2->implicitCastTo(e1->type->next);
    }
    else if (e1->op == TOKrange &&
	     e2->op == TOKstring &&
	     ((StringExp *)e2)->len == 1)
    {	// memset
	e2 = e2->implicitCastTo(e1->type->next);
    }
    else
    {
	e2 = e2->implicitCastTo(e1->type);
    }
    type = e1->type;
    return this;
}

void AssignExp::checkBoolean()
{
    // Things like:
    //	if (a = b) ...
    // are usually mistakes.

    error("'=' does not give a boolean result");
}

/************************************************************/

AddAssignExp::AddAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKaddass, sizeof(AddAssignExp), e1, e2)
{
}

Expression *AddAssignExp::semantic(Scope *sc)
{   Expression *e;

    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    if ((e1->type->ty == Tarray || e1->type->ty == Tsarray) &&
	(e2->type->ty == Tarray || e2->type->ty == Tsarray) &&
	e1->type->next->equals(e2->type->next)
       )
    {
	type = e1->type;
	e = this;
    }
    else
    {
	e1->checkScalar();
	if (e1->type->ty == Tpointer && e2->type->isintegral())
	    e = scaleFactor();
	else
	{
	    type = e1->type;
	    typeCombine();
	    e1->checkArithmetic();
	    e2->checkArithmetic();
	    if (type->isreal() || type->isimaginary())
	    {
		assert(e2->type->isfloating());
		e2 = e2->castTo(type);
	    }
	    e = this;
	}
    }
    return e;
}

/************************************************************/

MinAssignExp::MinAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKminass, sizeof(MinAssignExp), e1, e2)
{
}

Expression *MinAssignExp::semantic(Scope *sc)
{   Expression *e;

    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    e1->checkScalar();
    if (e1->type->ty == Tpointer && e2->type->isintegral())
	e = scaleFactor();
    else
    {
	type = e1->type;
	typeCombine();
	e1->checkArithmetic();
	e2->checkArithmetic();
	if (type->isreal() || type->isimaginary())
	{
	    assert(e2->type->isfloating());
	    e2 = e2->castTo(type);
	}
	e = this;
    }
    return e;
}

/************************************************************/

CatAssignExp::CatAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKcatass, sizeof(CatAssignExp), e1, e2)
{
}

Expression *CatAssignExp::semantic(Scope *sc)
{   Expression *e;

    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    if ((e1->type->ty == Tarray || e1->type->ty == Tsarray) &&
	(e2->type->ty == Tarray || e2->type->ty == Tsarray) &&
	e2->implicitConvTo(e1->type)
	//e1->type->next->equals(e2->type->next)
       )
    {	// Append array
	e2 = e2->castTo(e1->type);
	type = e1->type;
	e = this;
    }
    else if ((e1->type->ty == Tarray || e1->type->ty == Tsarray) &&
	e2->implicitConvTo(e1->type->next)
       )
    {	// Append element
	e2 = e2->castTo(e1->type->next);
	type = e1->type;
	e = this;
    }
    else
    {
	error("Can only concatenate arrays");
	type = Type::tint32;
	e = this;
    }
    return e;
}

/************************************************************/

MulAssignExp::MulAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKmulass, sizeof(MulAssignExp), e1, e2)
{
}

Expression *MulAssignExp::semantic(Scope *sc)
{   Expression *e;

    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    e1->checkScalar();
    type = e1->type;
    typeCombine();
    e1->checkArithmetic();
    e2->checkArithmetic();
    if (e2->type->isfloating())
    {	Type *t1;
	Type *t2;

	t1 = e1->type;
	t2 = e2->type;
	if (t1->isreal())
	{
	    if (t2->isimaginary() || t2->iscomplex())
	    {
		e2 = e2->castTo(t1);
	    }
	}
	else if (t1->isimaginary())
	{
	    if (t2->isimaginary() || t2->iscomplex())
	    {
		switch (t1->ty)
		{
		    case Timaginary32: t2 = Type::tfloat32; break;
		    case Timaginary64: t2 = Type::tfloat64; break;
		    case Timaginary80: t2 = Type::tfloat80; break;
		    default:
			assert(0);
		}
		e2 = e2->castTo(t2);
	    }
	}
    }
    return this;
}

/************************************************************/

DivAssignExp::DivAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKdivass, sizeof(DivAssignExp), e1, e2)
{
}

Expression *DivAssignExp::semantic(Scope *sc)
{   Expression *e;

    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    e1->checkScalar();
    type = e1->type;
    typeCombine();
    e1->checkArithmetic();
    e2->checkArithmetic();
    if (e2->type->isimaginary())
    {	Type *t1;
	Type *t2;

	t1 = e1->type;
	if (t1->isreal() || t1->isimaginary())
	{   Expression *e;

	    switch (t1->ty)
	    {
		case Timaginary32: t2 = Type::tfloat32; break;
		case Timaginary64: t2 = Type::tfloat64; break;
		case Timaginary80: t2 = Type::tfloat80; break;
		default:
		    assert(0);
	    }
	    e2 = e2->castTo(t2);
	    e = new AssignExp(loc, e1, e2);
	    e->type = t1;
	    return e;
	}
    }
    return this;
}

/************************************************************/

ModAssignExp::ModAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKmodass, sizeof(ModAssignExp), e1, e2)
{
}

Expression *ModAssignExp::semantic(Scope *sc)
{
    return commonSemanticAssign(sc);
}

/************************************************************/

ShlAssignExp::ShlAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKshlass, sizeof(ShlAssignExp), e1, e2)
{
}

Expression *ShlAssignExp::semantic(Scope *sc)
{   Expression *e;

    //printf("ShlAssignExp::semantic()\n");
    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    e1->checkScalar();
    type = e1->type;
    typeCombine();
    e1->checkIntegral();
    e2->checkIntegral();
    e2 = e2->castTo(Type::tshiftcnt);
    return this;
}

/************************************************************/

ShrAssignExp::ShrAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKshrass, sizeof(ShrAssignExp), e1, e2)
{
}

Expression *ShrAssignExp::semantic(Scope *sc)
{   Expression *e;

    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    e1->checkScalar();
    type = e1->type;
    typeCombine();
    e1->checkIntegral();
    e2->checkIntegral();
    e2 = e2->castTo(Type::tshiftcnt);
    return this;
}

/************************************************************/

UshrAssignExp::UshrAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKushrass, sizeof(UshrAssignExp), e1, e2)
{
}

Expression *UshrAssignExp::semantic(Scope *sc)
{   Expression *e;

    BinExp::semantic(sc);

    e = op_overload(sc);
    if (e)
	return e;

    e1 = e1->modifiableLvalue(sc);
    e1->checkScalar();
    type = e1->type;
    typeCombine();
    e1->checkIntegral();
    e2->checkIntegral();
    e2 = e2->castTo(Type::tshiftcnt);
    return this;
}

/************************************************************/

AndAssignExp::AndAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKandass, sizeof(AndAssignExp), e1, e2)
{
}

Expression *AndAssignExp::semantic(Scope *sc)
{
    return commonSemanticAssign(sc);
}

/************************************************************/

OrAssignExp::OrAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKorass, sizeof(OrAssignExp), e1, e2)
{
}

Expression *OrAssignExp::semantic(Scope *sc)
{
    return commonSemanticAssign(sc);
}

/************************************************************/

XorAssignExp::XorAssignExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKxorass, sizeof(XorAssignExp), e1, e2)
{
}

Expression *XorAssignExp::semantic(Scope *sc)
{
    return commonSemanticAssign(sc);
}

/************************* AddExp *****************************/

AddExp::AddExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKadd, sizeof(AddExp), e1, e2)
{
}

Expression *AddExp::semantic(Scope *sc)
{   Expression *e;

    //printf("AddExp::semantic()\n");
    if (!type)
    {
	BinExp::semantic(sc);

	e = op_overload(sc);
	if (e)
	    return e;

        if ((e1->type->ty == Tarray || e1->type->ty == Tsarray) &&
            (e2->type->ty == Tarray || e2->type->ty == Tsarray) &&
            e1->type->next->equals(e2->type->next)
           )
        {
            type = e1->type;
            e = this;
        }
	else if (e1->type->ty == Tpointer && e2->type->isintegral() ||
	    e2->type->ty == Tpointer && e1->type->isintegral())
	    e = scaleFactor();
	else
	{
	    typeCombine();
	    if ((e1->type->isreal() && e2->type->isimaginary()) ||
		(e1->type->isimaginary() && e2->type->isreal()))
	    {
		switch (type->ty)
		{
		    case Tfloat32:
		    case Timaginary32:
			type = Type::tcomplex32;
			break;

		    case Tfloat64:
		    case Timaginary64:
			type = Type::tcomplex64;
			break;

		    case Tfloat80:
		    case Timaginary80:
			type = Type::tcomplex80;
			break;

		    default:
			assert(0);
		}
	    }
	    e = this;
	}
	return e;
    }
    return this;
}

/************************************************************/

MinExp::MinExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKmin, sizeof(MinExp), e1, e2)
{
}

Expression *MinExp::semantic(Scope *sc)
{   Expression *e;

    if (type)
	return this;

    BinExp::semantic(sc);
    e = op_overload(sc);
    if (e)
	return e;

    e = this;
    if (e1->type->ty == Tpointer)
    {
	if (e2->type->ty == Tpointer)
	{   // Need to divide the result by the stride
	    // Replace (ptr - ptr) with (ptr - ptr) / stride
	    d_int32 stride;
	    Expression *e;

	    typeCombine();		// make sure pointer types are compatible
	    type = Type::tint32;
	    stride = e2->type->next->size();
	    e = new DivExp(loc, this, new IntegerExp(0, stride, Type::tint32));
	    e->type = Type::tint32;
	    return e;
	}
	else if (e2->type->isintegral())
	    e = scaleFactor();
	else
	    error("incompatible types for -");
    }
    else if (e2->type->ty == Tpointer)
    {
	type = e2->type;
	error("can't subtract pointer from %s", e1->type->toChars());
    }
    else
    {	typeCombine();
	if ((e1->type->isreal() && e2->type->isimaginary()) ||
	    (e1->type->isimaginary() && e2->type->isreal()))
	{
	    switch (type->ty)
	    {
		case Tfloat32:
		case Timaginary32:
		    type = Type::tcomplex32;
		    break;

		case Tfloat64:
		case Timaginary64:
		    type = Type::tcomplex64;
		    break;

		case Tfloat80:
		case Timaginary80:
		    type = Type::tcomplex80;
		    break;

		default:
		    assert(0);
	    }
	}
    }
    return e;
}

/************************* CatExp *****************************/

CatExp::CatExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKcat, sizeof(CatExp), e1, e2)
{
}

Expression *CatExp::semantic(Scope *sc)
{   Expression *e;

    //printf("CatExp::semantic()\n");
    if (!type)
    {
	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;

	if (e1->type->ty == Tsarray)
	    e1 = e1->castTo(e1->type->next->arrayOf());
	if (e2->type->ty == Tsarray)
	    e2 = e2->castTo(e2->type->next->arrayOf());

	typeCombine();
#if 0
	e1->type->print();
	e2->type->print();
	type->print();
	print();
#endif
	if (e1->op == TOKstring && e2->op == TOKstring)
	    e = optimize(WANTvalue);
	else if (e1->type-equals(e2->type))
	{
	    e = this;
	}
	else
	{
	    error("Can only concatenate arrays, not (%s ~ %s)",
		e1->type->toChars(), e2->type->toChars());
	    type = Type::tint32;
	    e = this;
	}
	return e;
    }
    return this;
}

/************************************************************/

MulExp::MulExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKmul, sizeof(MulExp), e1, e2)
{
}

Expression *MulExp::semantic(Scope *sc)
{   Expression *e;

    if (type)
	return this;

    BinExp::semantic(sc);
    e = op_overload(sc);
    if (e)
	return e;

    typeCombine();
    e1->checkArithmetic();
    e2->checkArithmetic();
    if (type->isfloating())
    {	Type *t1 = e1->type;
	Type *t2 = e2->type;

	if (t1->isreal())
	{
	    type = t2;
	}
	else if (t2->isreal())
	{
	    type = t1;
	}
	else if (t1->isimaginary())
	{
	    if (t2->isimaginary())
	    {	Expression *e;

		switch (t1->ty)
		{
		    case Timaginary32:	type = Type::tfloat32;	break;
		    case Timaginary64:	type = Type::tfloat64;	break;
		    case Timaginary80:	type = Type::tfloat80;	break;
		    default:		assert(0);
		}
		// iy * iv = -yv
		e1->type = type;
		e2->type = type;
		e = new NegExp(loc, this);
		e = e->semantic(sc);
		return e;
	    }
	    else
		type = t2;	// t2 is complex
	}
	else if (t2->isimaginary())
	{
	    type = t1;	// t1 is complex
	}
    }
    return this;
}

/************************************************************/

DivExp::DivExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKdiv, sizeof(DivExp), e1, e2)
{
}

Expression *DivExp::semantic(Scope *sc)
{   Expression *e;

    if (type)
	return this;

    BinExp::semantic(sc);
    e = op_overload(sc);
    if (e)
	return e;

    typeCombine();
    e1->checkArithmetic();
    e2->checkArithmetic();
    if (type->isfloating())
    {	Type *t1 = e1->type;
	Type *t2 = e2->type;

	if (t1->isreal())
	{
	    type = t2;
	    if (t2->isimaginary())
	    {	Expression *e;

		// x/iv = i(-x/v)
		e2->type = t1;
		e = new NegExp(loc, this);
		e = e->semantic(sc);
		return e;
	    }
	}
	else if (t2->isreal())
	{
	    type = t1;
	}
	else if (t1->isimaginary())
	{
	    if (t2->isimaginary())
	    {
		switch (t1->ty)
		{
		    case Timaginary32:	type = Type::tfloat32;	break;
		    case Timaginary64:	type = Type::tfloat64;	break;
		    case Timaginary80:	type = Type::tfloat80;	break;
		    default:		assert(0);
		}
	    }
	    else
		type = t2;	// t2 is complex
	}
	else if (t2->isimaginary())
	{
	    type = t1;	// t1 is complex
	}
    }
    return this;
}

/************************************************************/

ModExp::ModExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKmod, sizeof(ModExp), e1, e2)
{
}

Expression *ModExp::semantic(Scope *sc)
{   Expression *e;

    if (type)
	return this;

    BinExp::semantic(sc);
    e = op_overload(sc);
    if (e)
	return e;

    typeCombine();
    e1->checkArithmetic();
    e2->checkArithmetic();
    return this;
}

/************************************************************/

ShlExp::ShlExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKshl, sizeof(ShlExp), e1, e2)
{
}

Expression *ShlExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;
	e1->checkIntegral();
	e2->checkIntegral();
	e1 = e1->integralPromotions();
	e2 = e2->castTo(Type::tshiftcnt);
	type = e1->type;
    }
    return this;
}

/************************************************************/

ShrExp::ShrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKshr, sizeof(ShrExp), e1, e2)
{
}

Expression *ShrExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;
	e1->checkIntegral();
	e2->checkIntegral();
	e1 = e1->integralPromotions();
	e2 = e2->castTo(Type::tshiftcnt);
	type = e1->type;
    }
    return this;
}

/************************************************************/

UshrExp::UshrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKushr, sizeof(UshrExp), e1, e2)
{
}

Expression *UshrExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;
	e1->checkIntegral();
	e2->checkIntegral();
	e1 = e1->integralPromotions();
	e2 = e2->castTo(Type::tshiftcnt);
	type = e1->type;
    }
    return this;
}

/************************************************************/

AndExp::AndExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKand, sizeof(AndExp), e1, e2)
{
}

Expression *AndExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;
	typeCombine();
	e1->checkIntegral();
	e2->checkIntegral();
    }
    return this;
}

/************************************************************/

OrExp::OrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKor, sizeof(OrExp), e1, e2)
{
}

Expression *OrExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;
	typeCombine();
	e1->checkIntegral();
	e2->checkIntegral();
    }
    return this;
}

/************************************************************/

XorExp::XorExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKxor, sizeof(XorExp), e1, e2)
{
}

Expression *XorExp::semantic(Scope *sc)
{   Expression *e;

    if (!type)
    {	BinExp::semantic(sc);
	e = op_overload(sc);
	if (e)
	    return e;
	typeCombine();
	e1->checkIntegral();
	e2->checkIntegral();
    }
    return this;
}


/************************************************************/

OrOrExp::OrOrExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKoror, sizeof(OrOrExp), e1, e2)
{
}

Expression *OrOrExp::semantic(Scope *sc)
{
    unsigned cs1;

    // same as for AndAnd
    e1 = e1->semantic(sc);
    cs1 = sc->callSuper;
    e2 = e2->semantic(sc);
    sc->mergeCallSuper(loc, cs1);

    e1 = e1->checkToPointer();
    e2 = e2->checkToPointer();
    e1->checkBoolean();
    type = Type::tboolean;
    if (e1->type->ty == Tvoid)
	type = Type::tvoid;
    return this;
}

void OrOrExp::checkBoolean()
{
    e2->checkBoolean();
}

int OrOrExp::isBit()
{
    return TRUE;
}


/************************************************************/

AndAndExp::AndAndExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKandand, sizeof(AndAndExp), e1, e2)
{
}

Expression *AndAndExp::semantic(Scope *sc)
{
    unsigned cs1;

    // same as for OrOr
    e1 = e1->semantic(sc);
    cs1 = sc->callSuper;
    e2 = e2->semantic(sc);
    sc->mergeCallSuper(loc, cs1);

    e1 = e1->checkToPointer();
    e2 = e2->checkToPointer();
    e1->checkBoolean();
    type = Type::tboolean;
    if (e1->type->ty == Tvoid)
	type = Type::tvoid;
    return this;
}

void AndAndExp::checkBoolean()
{
    e2->checkBoolean();
}

int AndAndExp::isBit()
{
    return TRUE;
}


/************************************************************/

InExp::InExp(Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, TOKin, sizeof(InExp), e1, e2)
{
}

Expression *InExp::semantic(Scope *sc)
{
    BinExp::semantic(sc);
    type = Type::tboolean;
    if (e2->type->ty != Taarray)
    {
	error("rvalue of in expression must be an associative array, not %s", e2->type->toChars());
    }
    else
    {
	TypeAArray *ta = (TypeAArray *)e2->type;

	// Convert key to type of key
	e1 = e1->implicitCastTo(ta->index);
    }
    return this;
}

int InExp::isBit()
{
    return TRUE;
}


/************************************************************/

CmpExp::CmpExp(enum TOK op, Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, op, sizeof(CmpExp), e1, e2)
{
}

Expression *CmpExp::semantic(Scope *sc)
{   Expression *e;
    Type *t1;
    Type *t2;

    //printf("CmpExp::semantic()\n");
    if (type)
	return this;

    BinExp::semantic(sc);
    e = op_overload(sc);
    if (e)
    {
	e = new CmpExp(op, loc, e, new IntegerExp(loc, 0, Type::tint32));
	e = e->semantic(sc);
	return e;
    }

    typeCombine();
    type = Type::tboolean;

    // Special handling for array comparisons
    t1 = e1->type->toBasetype();
    t2 = e2->type->toBasetype();
    if ((t1->ty == Tarray || t1->ty == Tsarray) &&
	(t2->ty == Tarray || t2->ty == Tsarray) &&
	t1->next->equals(t2->next))
    {
	//printf("array, t1='%s', t2='%s'\n", t1->toChars(), t2->toChars());
	Expression *ec;
	FuncDeclaration *fd;
	Array *arguments;
	Expression *a1;
	Expression *a2;
	Type *telement = t1->next->toBasetype();

	a1 = e1->castTo(t1->next->arrayOf());
	a2 = e2->castTo(t2->next->arrayOf());
	arguments = new Array();
	arguments->push(a1);
	arguments->push(a2);

	if (telement->ty == Tchar)
	{
	    fd = FuncDeclaration::genCfunc(type, "_adCmpChar");
	    ec = new VarExp(loc, fd);
	}
	else
	{
	    fd = FuncDeclaration::genCfunc(type, (telement->ty == Tbit ? "_adCmpBit" : "_adCmp"));
	    ec = new VarExp(loc, fd);
	    if (telement->ty != Tbit)
		arguments->push(t1->next->toBasetype()->getProperty(loc, Id::typeinfo));
	}
	e = new CallExp(loc, ec, arguments);
	e->type = Type::tint32;
	e1 = e;
	e2 = new IntegerExp(loc, 0, e->type);
	e = this;
    }
    else
	e = this;
    return e;
}

int CmpExp::isBit()
{
    return TRUE;
}


/************************************************************/

EqualExp::EqualExp(enum TOK op, Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, op, sizeof(EqualExp), e1, e2)
{
}

Expression *EqualExp::semantic(Scope *sc)
{   Expression *e;
    Type *t1;
    Type *t2;

    if (type)
	return this;

    BinExp::semantic(sc);

    //if (e2->op != TOKnull)
    {
	e = op_overload(sc);
	if (e)
	{
	    if (op == TOKnotequal)
	    {
		e = new NotExp(e->loc, e);
		e = e->semantic(sc);
	    }
	    return e;
	}
    }

    e = typeCombine();
    type = Type::tboolean;

    // Special handling for array comparisons
    t1 = e1->type->toBasetype();
    t2 = e2->type->toBasetype();
    if ((t1->ty == Tarray || t1->ty == Tsarray) &&
	(t2->ty == Tarray || t2->ty == Tsarray) &&
	t1->next->equals(t2->next))
    {
	Expression *ec;
	FuncDeclaration *fd;
	Array *arguments;
	Expression *a1;
	Expression *a2;
	Type *telement = t1->next->toBasetype();

	fd = FuncDeclaration::genCfunc(type, (telement->ty == Tbit ? "_adEqBit" : "_adEq"));
	ec = new VarExp(loc, fd);
	a1 = e1->castTo(t1->next->arrayOf());
	a2 = e2->castTo(t2->next->arrayOf());
	arguments = new Array();
	arguments->push(a1);
	arguments->push(a2);
	if (telement->ty != Tbit)
	    arguments->push(telement->getProperty(loc, Id::typeinfo));
	e = new CallExp(loc, ec, arguments);
	e->type = type;

	if (op == TOKnotequal)
	{   e = new NotExp(loc, e);
	    e->type = type;
	}
    }
    else
    {
	if (e1->type != e2->type && e1->type->isfloating() && e2->type->isfloating())
	{
	    // Cast both to complex
	    e1 = e1->castTo(Type::tcomplex80);
	    e2 = e2->castTo(Type::tcomplex80);
	}
    }
    return e;
}

int EqualExp::isBit()
{
    return TRUE;
}



/************************************************************/

IdentityExp::IdentityExp(enum TOK op, Loc loc, Expression *e1, Expression *e2)
	: BinExp(loc, op, sizeof(IdentityExp), e1, e2)
{
}

Expression *IdentityExp::semantic(Scope *sc)
{
    BinExp::semantic(sc);
    type = Type::tboolean;
    typeCombine();
    if (e1->type != e2->type && e1->type->isfloating() && e2->type->isfloating())
    {
	// Cast both to complex
	e1 = e1->castTo(Type::tcomplex80);
	e2 = e2->castTo(Type::tcomplex80);
    }
    return this;
}

int IdentityExp::isBit()
{
    return TRUE;
}


/****************************************************************/

CondExp::CondExp(Loc loc, Expression *econd, Expression *e1, Expression *e2)
	: BinExp(loc, TOKquestion, sizeof(CondExp), e1, e2)
{
    this->econd = econd;
}

Expression *CondExp::syntaxCopy()
{
    return new CondExp(loc, econd->syntaxCopy(), e1->syntaxCopy(), e2->syntaxCopy());
}


Expression *CondExp::semantic(Scope *sc)
{   Type *t1;
    Type *t2;
    unsigned cs0;
    unsigned cs1;

    econd = econd->semantic(sc);


    cs0 = sc->callSuper;
    e1 = e1->semantic(sc);
    cs1 = sc->callSuper;
    sc->callSuper = cs0;
    e2 = e2->semantic(sc);
    sc->mergeCallSuper(loc, cs1);


    econd = econd->checkToPointer();
    econd->checkBoolean();
    // If either operand is void, the result is void
    t1 = e1->type;
    t2 = e2->type;
    if (t1->ty == Tvoid || t2->ty == Tvoid)
	type = Type::tvoid;
    else if (t1 == t2)
	type = t1;
    else
    {	typeCombine();
    }
    return this;
}

Expression *CondExp::toLvalue()
{
    PtrExp *e;

    // convert (econd ? e1 : e2) to *(econd ? &e1 : &e2)
    e = new PtrExp(loc, this, type);

    e1 = e1->addressOf();
    e1 = e1->toLvalue();

    e2 = e2->addressOf();
    e2 = e2->toLvalue();

    typeCombine();

    type = e2->type;
    return e;
}

void CondExp::checkBoolean()
{
    e1->checkBoolean();
    e2->checkBoolean();
}

void CondExp::toCBuffer(OutBuffer *buf)
{
    econd->toCBuffer(buf);
    buf->writestring(" ? ");
    e1->toCBuffer(buf);
    buf->writestring(" : ");
    e2->toCBuffer(buf);
}


