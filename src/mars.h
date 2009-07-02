
// Copyright (c) 1999-2002 by Digital Mars
// All Rights Reserved
// written by Walter Bright
// www.digitalmars.com
// License for redistribution is by either the Artistic License
// in artistic.txt, or the GNU General Public License in gnu.txt.
// See the included readme.txt for details.

#pragma once

struct Array;

// Put command line switches in here
struct Param
{
    char link;		// perform link
    char trace;		// insert profiling hooks
    char verbose;	// verbose compile
    char symdebug;	// insert debug symbolic information
    char optimize;	// run optimizer
    char cpu;		// target CPU
    char scheduler;	// which scheduler to use
    char useDeprecated;	// allow use of deprecated features
    char useAssert;	// generate runtime code for assert()'s
    char useInvariants;	// generate class invariant checks
    char useIn;		// generate precondition checks
    char useOut;	// generate postcondition checks
    char useArrayBounds; // generate array bounds checks
    char useSwitchError; // check for switches without a default
    char useUnitTests;	// generate unittest code
    char useInline;	// inline expand functions
    char release;	// build release version

    char *argv0;	// program name
    Array *imppath;	// array of char*'s of where to look for import modules
    char *objdir;	// .obj file output directory
    char *objname;	// .obj file output name

    unsigned debuglevel;	// debug level
    Array *debugids;		// debug identifiers

    unsigned versionlevel;	// version level
    Array *versionids;		// version identifiers

    // Hidden debug switches
    char debuga;
    char debugb;
    char debugc;
    char debugr;
    char debugw;
    char debugx;

    // Linker stuff
    Array *objfiles;
    Array *linkswitches;
    Array *libfiles;
    char *deffile;
    char *resfile;
    char *exefile;
};

struct Global
{
    char *mars_ext;
    char *sym_ext;
    char *copyright;
    char *written;
    Array *path;	// Array of char*'s which form the import lookup path
    int structalign;
    char *version;

    Param params;
    unsigned errors;	// number of errors reported so far

    Global();
};

extern Global global;

typedef unsigned long long integer_t;
typedef long double real_t;
typedef _Complex long double complex_t;

typedef signed char		d_int8;
typedef unsigned char		d_uns8;
typedef short			d_int16;
typedef unsigned short		d_uns16;
typedef int			d_int32;
typedef unsigned		d_uns32;
typedef long long		d_int64;
typedef unsigned long long	d_uns64;

typedef float			d_float32;
typedef double			d_float64;
typedef long double		d_float80;

// Note: this will be 2 bytes on Win32 systems, and 4 bytes under linux.
typedef wchar_t			d_wchar;

// Modify OutBuffer::writewchar to write the correct size of wchar
#if _WIN32
#define writewchar writeword
#endif

#if linux
#define writewchar write4
#endif


struct Module;

//typedef unsigned Loc;		// file location
struct Loc
{
    Module *mod;
    unsigned linnum;

    Loc()
    {
	linnum = 0;
	mod = NULL;
    }

    Loc(int x)
    {
	linnum = 0;
	mod = NULL;
    }

    Loc(Module *mod, unsigned linnum)
    {
	this->linnum = linnum;
	this->mod = mod;
    }

    char *toChars();
};

#define TRUE	1
#define FALSE	0

#define INTERFACE_OFFSET	0	// if 1, put classinfo as first entry
					// in interface vtbl[]'s
#define INTERFACE_VIRTUAL	0	// 1 means if an interface appears
					// in the inheritance graph multiple
					// times, only one is used

enum LINK
{
    LINKdefault,
    LINKd,
    LINKc,
    LINKcpp,
    LINKwindows,
    LINKpascal,
};

void error(Loc loc, const char *format, ...);
void fatal();
int runLINK();
void inifile(char *argv0, char *inifile);

