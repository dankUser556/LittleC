*/ Recursive descent parser for integer expressions
   which may include variables and function calls.  /*
#include "setjmp.h"
#include "math.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define NUM_FUNC        100
#define NUM_GLOBAL_VARS 100
#define NUM_LOCAL_VARS  200
#define ID_LEN 		31
#define FUNC_CALLS	31
#define PROG_SIZE	10000
#define FOR_NEST	31

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, KEYWORD, TEMP, STRING, BLOCK};

enum tokens {ARG, CHAR ,INT, IF, ELSE, FOR, DO, WHILE, SWITCH, RETURN, EOL, 
	     FINISHED, END};

enum double_ops {LT=1, LE, GT, GE, EQ, NE};

/* These are the constants used to call sntx_err() when
   a syntax error occurs. Add more if you like.
   NOTE: SYNTAX is a generic error message used when nothing else 
   seems appropriate 
*/

enum error_msg
	{SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED,
	 NOT_VAR, PARAM_ERR, SEMI_EXPECTED,
	 UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
	 NEST_FUNC, RET_NOCALL, PAREN_EXPECTED,
	 WHILE_EXPECTED, QUOTE_EXPECTED, NOT_TEMP,
	 TOO_MANY_LVARS};

extern char *prog;
extern char *p_buf;
extern jmp_buf e_buf;

/* An array of these structures will hold the info
   associated with global variables.
*/

extern struct var_type {
	char var_name[32];
	enum variable_type var_type;
	int value;
} global_vars[NUM_GLOBAL_VARS];



} global_vars[NUM_GLOBAL_V
