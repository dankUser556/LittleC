/* Recursive descent parser	for	integer	expressions
   which may include variables and function	calls. */
#include "setjmp.h"
#include "math.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define NDEBUG //deprecated, remove after metrics
#define	NUM_FUNC		100
#define	NUM_GLOBAL_VARS	100
#define	NUM_LOCAL_VARS	200
#define	ID_LEN 		    31
#define	FUNC_CALLS	    31
#define	PROG_SIZE	    10000
#define	FOR_NEST	    31

#ifdef DEBUG
struct token_types {
	char t_name[11];
} token_id[] = {
	"DELIMITER",
	"IDENTIFIER",
	"NUMBER",
	"KEYWORD",
	"TEMP",
	"STRING",
	"BLOCK"
};
#endif

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, KEYWORD,	TEMP, STRING, BLOCK};

enum tokens	{ARG, CHAR ,INT, IF, ELSE, FOR,	DO,	WHILE, SWITCH, RETURN, EOL,	FINISHED, END};

enum double_ops	{LT=1, LE, GT, GE, EQ, NE};

/* These are the constants used	to call	sntx_err() when
   a syntax	error occurs. Add more if you like.
   NOTE: SYNTAX	is a generic error message used	when 
   nothing else seems appropriate */

enum error_msg {         SYNTAX,   UNBAL_PARENS,         NO_EXP, EQUALS_EXPECTED,
				        NOT_VAR,      PARAM_ERR,  SEMI_EXPECTED, UNBAL_BRACES,
				     FUNC_UNDEF,  TYPE_EXPECTED,      NEST_FUNC, RET_NOCALL,
				 PAREN_EXPECTED, WHILE_EXPECTED, QUOTE_EXPECTED, NOT_TEMP,
				TOO_MANY_LVARS};

int debug_enabled; //deprecated, remove after adding metrics

extern char	*prog;
extern char	*p_buf;
extern jmp_buf e_buf;

extern struct var_type { /* An array of these structures will hold the info associated with global variables.*/
	char var_name[ID_LEN];
	int var_type;
	int value;
} global_vars[NUM_GLOBAL_VARS];

extern struct func_type	{ /* This is the function call stack. */
	char func_name[ID_LEN];
	char loc;	/*location of function entry point in file */
} func_stack[NUM_FUNC];

extern struct commands { /* Keyword table */
	char command[20];
	char tok;
} table[];

/* Internal Library functions are declared here so they can be put into the internal function table that follows. */
int	call_getche(void), call_putch(void), call_puts(void), print(void), getnum(void);

struct intern_func_type	{
	char *f_name; /* function name */
	int (* p)();  /* pointer to the function */
} intern_func[]	= {
	(char *)"getche", call_getche,
	(char *)"putch",  call_putch,
	(char *)"puts",   call_puts,
	(char *)"print",  print,
	(char *)"getnum", getnum,
	(char *)"",       0 /* Practicing null terminators */
};

extern char	token[80];	/* string representation of a token */
extern char	token_type;	/* contains	type of	token */
extern char	tok;		/* internal representation of the token */

extern int ret_value;	/* function return value */

// Forward Declaration of functions defined in this file
void   eval_exp(int*) ,       eval_exp0(int*) ,  eval_exp1(int*)      , eval_exp2(int*);
void  eval_exp3(int*) ,       eval_exp4(int*) ,  eval_exp5(int*)      ,      atom(int*);
void   sntx_err(int)  ,         putback(void) , assign_var(char*, int),      call(void);
int	    isdelim(char) ,         look_up(char*),    iswhite(char)      , find_var(char*); 
int   get_token(void) ,   internal_func(char*),     is_var(char*)     ;
char* find_func(char*);

/* Entry point into	parser.	*/
void eval_exp(int *value)
{
	#ifdef DEBUG
	printf("\neval_exp called with %d %c",*value,*value);
	#endif
	
	get_token();
	if(!*token) {
		sntx_err(NO_EXP);
		return;
	}
	if(*token==';') {
		*value	= 0; /*	empty expression */
		return;
	}
	eval_exp0(value);
	putback(); /*	return last	token read to input	stream */
	
	#ifdef DEBUG
	printf("\nexiting eval_exp");
	#endif
}

void eval_exp0(int *value) /* Process an assignment expression	*/
{
	#ifdef DEBUG
	printf("\neval_exp0() called with %d %c",*value,*value);
	#endif
	
	char temp[ID_LEN]; /*	holds name of var receiving the assignment */
	register int temp_tok;
	
	if(token_type==IDENTIFIER) {
		if(is_var(token)) {
			strcpy(temp,token);
			temp_tok=token_type;
			get_token();
			if(*token=='=') { /*	Is an assignment */
				get_token();
				eval_exp0(value);	/* get value to	assign */
				assign_var(temp,*value);
				return;
			} else { /* not an	assignment */
				putback();
				strcpy(token,temp);
				token_type = temp_tok;
	  		}
		}
	}
	eval_exp1(value);
	#ifdef DEBUG
	printf("\neval_exp0() exiting normally");
	#endif
}

char relops[7] = { /* This	array is used by eval_exp(). Because some compilers cannot initialize an array within a the same function in which it is defined, it will be declared here for portability. */
	LT, LE, GT, GE, EQ, NE, 0
};

/* Process relational operators. */
void eval_exp1(int *value)
{
	#ifdef DEBUG
	printf("\nentering eval_exp1 with value %d %c",*value,*value);
	#endif
	int partial_value;
	register char op;

	eval_exp2(value);
	op = *token;
	if (strchr(relops,op)) {
		get_token();
		eval_exp2(&partial_value);
		#ifdef DEBUG
		printf("\ncalling switch in eval_exp2 with value %d",*value);
		#endif
		switch(op)	{
			case LT:
				*value = *value < partial_value;
				break;
			case LE:
				*value = *value <= partial_value;
				break;
			case GT:
				*value = *value > partial_value;
				break;
			case GE:
				*value = *value >= partial_value;
				break;
			case EQ:
				*value = *value == partial_value;
				break;
			case NE:
				*value = *value != partial_value;
				break;
		}
	}
	#ifdef DEBUG
	printf("\nexiting eval_exp1 normally");
	#endif
}

/* Add or subtract two terms. */
void eval_exp2(int *value)
{
	#ifdef DEBUG
	printf("\neval_exp2 now entered with value %d %c",*value,*value);
	#endif
	register char op;
	int partial_value;

	eval_exp3(value);
	while((op = *token) == '+' || op == '-') {
		#ifdef DEBUG
		printf("\neval_exp2 while loop executing with op %c",op);
		#endif
		get_token();
		eval_exp3(&partial_value);
		#ifdef DEBUG
		printf("\newal_exp2 switch with op %c",op);
		#endif
		switch(op)	{
			case '-':
				*value = *value - partial_value;
				break;
			case '+':
				*value = *value + partial_value;
				break;
		}
	}
	#ifdef DEBUG
	printf("\nexiting eval_exp2 normally");
	#endif
}

/* Multiply	or divide two factors. */
void eval_exp3(int *value)
{
	#ifdef DEBUG
	printf("\neval_exp3 entered with value %d",*value);
	#endif

	register char op;
	int partial_value, t;

	eval_exp4(value);
	while((op = *token) == '*' || op == '/' || op == '%') {
		get_token();
		eval_exp4(&partial_value);
		switch(op) {
			case '*':
				#ifdef DEBUG
				printf("\nevaluating multiplication");
				#endif
				*value = *value *	partial_value;
				break;
			case '/':
				#ifdef DEBUG
				printf("\nevaluating division");
				#endif
				*value = *value /	partial_value;
				break;
			case '%':
				#ifdef DEBUG
				printf("\nevaluating modulus");
				#endif
				t = (*value) / partial_value;
				*value = *value-(t*partial_value);
				break;
		}
	}
	#ifdef DEBUG
	printf("\nexiting eval_exp3 normally");
	#endif
}

/* Is a	unary +	or -. */
void eval_exp4(int *value)
{
	#ifdef DEBUG
	printf("\neval_exp4 entered with value %d",*value);
	#endif
	register char op;

	op = '\0';
	if (*token=='+' || *token=='-') {
		#ifdef DEBUG
		printf("\nCheck if unary operator begin");
		#endif
		op = *token;
		get_token();
	}
	eval_exp5(value);
	if(op) {
		if(op=='-') {
			#ifdef DEBUG
			printf("\nUmm..?");
			#endif
			*value = -(*value);
		}
	}
}

/* Process parenthesized expression. */
void eval_exp5(int *value)
{
	#ifdef DEBUG
	printf("\neval_exp5 entered with value %d %c",*value,*value);
	#endif
	if((*token== '(')) {
		get_token();
		eval_exp0(value); /* get subexpression	*/
		if(*token != ')') sntx_err(PAREN_EXPECTED);
		get_token();
	} else {
		atom(value);
	}
	#ifdef DEBUG
	printf("\nexiting eval_exp5 normally");
	#endif
}

/* Find	value of number, variable or function. */
void atom(int *value)
{
	int i;

	switch(token_type) {
		case IDENTIFIER:
			i = internal_func(token);
			if(i!= -1) {	/* call	"standard library" function	*/
				*value = (*intern_func[i].p)();
			} else if(find_func(token)){ /*	call user-defined function */
				call();
				*value = ret_value;
	  		} else { 
				*value = find_var(token); /* get var's value */
			}
			get_token();
			return;
		
		case NUMBER: /* is	numeric	constant */
			*value =	atoi(token);
			get_token();
			return;
		
		case DELIMITER: /*	see	if character constant */
			if(*token=='\'')	{
				*value = *prog;
				prog++;
				if(*prog!='\'') {
					sntx_err(QUOTE_EXPECTED);
				}
				prog++;
				get_token();
			}
			return;
		
		default:
			if(*token==')') {
				return; /* process empty	expression */
			} else {
			sntx_err(SYNTAX); /* syntax	error */
		}
	}
}

/* Display an error	message	*/
void sntx_err(int error)
{
  char *p, *temp;
  int linecount	= 0;
  register int i;

  static char *e[]=	{
	(char *)"syntax error",
	(char *)"unbalanced parentheses",
	(char *)"no expression	present",
	(char *)"equals sign expected",
	(char *)"not a variable",
	(char *)"parameter	error",
	(char *)"semicolon	expected",
	(char *)"unbalanced braces",
	(char *)"function undefined",
	(char *)"type specifier expected",
	(char *)"too many nested function calls",
	(char *)"return without call",
	(char *)"parentheses expected",
	(char *)"while	expected",
	(char *)"closing quote	expected",
	(char *)"not a	string",
	(char *)"too many local variables"
  };
  printf("%s", e[error]);
  p	= p_buf;
  while(p != prog) {
	p++;
	if(*p=='\r') {
	  linecount++;
	}
  }
  printf(" in line %d\n", linecount);

  temp = p;
  for(i=0; i<20	&& p>p_buf && *p!='\n';	i++, p--);
  for(i=0; i<30	&& p<=temp;	i++, p++) printf("%c", *p);

		
  longjmp(e_buf, 1);
}

/* Get a token */
int	get_token(void)
{
	#ifdef DEBUG
	static int token_counter = 0;
	printf("\nget_token() entered. Token count:\t%d\n\t\t\t   *prog:%c %d %x",++token_counter,*prog,*prog,*prog);
	printf("\nToken is currently %c %d %x",*token,*token,*token);
	if(token_counter >= 20) {
		printf("\nProgram exception must have occured, terminating from function: get_token()");
		printf("\nReason: token_counter is >= 20");
		getchar();
		exit(5);
	}
	#endif
	
	register char *temp;

	token_type = 0; tok =	0;
	temp = token;
	*temp='\0';

	/* skip over white space */
	#ifdef DEBUG
	printf("\nInitializing while loop with call to iswhite. prog: %d %c %x",*prog,*prog,*prog);
	#endif
	while(iswhite(*prog) && *prog) {
		#ifdef DEBUG
		printf("\nget_token iswhite loop (after var init) prog: %d %c %x",*prog,*prog,*prog);
		#endif
		++prog;
	}
	printf("\nchecking for newline with %c %x",*prog,*prog);
	if(*prog=='\n') {
		#ifdef DEBUG
		printf("\nNew line character found: %c %d %x",*prog,*prog,*prog);
		#endif
		prog++;
        #ifdef DEBUG
		printf("\nmid-newline printout: %c,%d,%x",*prog,*prog,*prog);
		#endif
		
		
		#ifdef DEBUG
		printf("\ncalling iswhite loop from if(prog=='\\n')");
		#endif
		while(iswhite(*prog) && *prog) {
			++prog;
		}
	}
	printf("\nskipped newline check, %d",(*prog=='\0'));
	if(*prog=='\0') {
		#ifdef DEBUG
		printf("\n'\0' found, returning FINISHED tok");
		#endif
		*token	= '\0';
		tok = FINISHED;
		
		
	}
	if(tok=FINISHED) token_type=DELIMITER;
	printf("\nskipped \\0 check");
	if(strchr("{}", *prog)) {	/* block delimiters	*/
		#ifdef DEBUG
		printf("\ntemp:\t %c %d %x",*temp,*temp,*temp);
		printf("\nprog:\t %c %d %x\ncalling *temp = *prog",*prog,*prog,*prog);
		#endif
		*temp = *prog;
		#ifdef DEBUG
		printf("\ntemp:\t %c %d %x",*temp,*temp,*temp);
		printf("\nprog:\t %c %d %x\ncalling temp++",*prog,*prog,*prog);
		#endif
		temp++;
		#ifdef DEBUG
		printf("\ntemp after temp++: %c %d %x\ncalling prog++",*temp,*temp,*temp);
		#endif
		*temp = '\0';
		prog++;
		#ifdef DEBUG
		printf("\nprog after prog++: %c %d %x",*prog,*prog,*prog);
		#endif
		#ifdef DEBUG
		printf("\nget_token returning from if(block delimiter) with token_type: %s",token_id[BLOCK].t_name);
		#endif
		return(token_type=BLOCK);
	}
	if(*prog=='/') {  /* look for comments */
		if(*(prog+1)=='*')	{ /* is	a comment, not unlike this one */
			#ifdef DEBUG
			printf("\nComment found, processing end of comment");
			#endif
			prog+=2;
			do {	/* find	end	of comment */
				while(*prog!='*')	prog++;
				prog++;
			} while (*prog!='/');
			#ifdef DEBUG
			printf("\nEnd of comment found, continuing");
			#endif
			prog++;
		}
	}
	if(strchr("!<>=", *prog)) { /* is or might be a relation operator */
		#ifdef DEBUG
		printf("\nChecking for relational operator, prog = %c",*prog);
		#endif
		switch(*prog) {
			case '=': 
				#ifdef DEBUG
				printf("\nRelational = found");
				#endif
				if(*(prog+1)=='=') {
					prog++;
					prog++;
					*temp = EQ;
					temp++; *temp = EQ; temp++;
					*temp =	'\0';
				}
				break;
			case '!':
				#ifdef DEBUG
				printf("\nRelational ! found");
				#endif
				if(*(prog+1)=='='){
					prog++;
					prog++;
					*temp = NE;
					temp++;
					*temp = NE;
					temp++;
					*temp = '\0';
				}
				break;
			case '<':
				#ifdef DEBUG
				printf("\nRelational < found");
				#endif
				if(*(prog+1)=='=') {
					prog++;	prog++;
					*temp=LE;
					temp++;
					*temp=LE;
				} else {
					prog++;
					*temp =	LT;
				}
				temp++;
				*temp='\0';
				break;
			case '>':
				#ifdef DEBUG
				printf("\nRelational > found");
				#endif
				if(*(prog+1)=='='){
					prog++;	prog++;
					*temp=GE;
					temp++;
					*temp=GE;
				} else {
					prog++;
					*temp=GT;
				}
				temp++;
				*temp	= '\0';
				break;
		}
		if(*token) {
			return(token_type =	DELIMITER);
		}
	}
	if(strchr("+-*^/%=;(),'", *prog)) { /* token is delimiter */
		#ifdef DEBUG
		printf("\nget_token() exiting from delimiter check with token_type: %s",token_id[DELIMITER].t_name);
		#endif
		#ifdef DEBUG
		printf("\nprog: %d %c %x",*prog,*prog,*prog);
		#endif
		*temp = *prog;
		prog++;
		temp++;
		*temp = '\0';
		return	(token_type=DELIMITER);
	}
	if(*prog=='"') { /* quoted string	*/
		#ifdef DEBUG
		printf("\nget_token quoted string begin");
		#endif
		prog++;
		while(*prog!='"' && *prog!='\n') {
			*temp++ = *prog++;
		}
		if(*prog=='\n') {
			sntx_err(SYNTAX); 
		}
		prog++; 
		*temp = '\0';
		#ifdef DEBUG
		printf("\nreturning token_type=%s",token_id[STRING].t_name);
		#endif
		return	(token_type=STRING);
	}
	#ifdef DEBUG
	printf("\nchecking if isdigit(*prog)",*prog);
	#endif
	if(isdigit(*prog)) { /* is a digit */
		#ifdef DEBUG
		printf("\nProcessing digit in get_token(), calling while(!isdelim(*prog)) prog: %c %d %x",*prog,*prog,*prog);
		#endif
		while(!isdelim(*prog)) {
			#ifdef DEBUG
			printf("\nisdelim loop, prog: %c %d %x temp: %c %d %x",*prog,*prog,*prog,*temp,*temp,*temp);
			#endif
			*temp++ = *prog++;
		}
		#ifdef DEBUG
		printf("\nsetting temp to '\\0'");
		#endif
		*temp = '\0';
		#ifdef DEBUG
		printf("\nreturning token_type = NUMBER");
		#endif
		return(token_type = NUMBER);
	}
	#ifdef DEBUG
	printf("\npost isdigit if(isalpha(*prog))");
	#endif
	if(isalpha(*prog)) { /* var or command */
		#ifdef DEBUG
		printf("\nProcessing alphanumeric in get_token(), calling while(!isdelim(*prog)) prog: %c %d %x",*prog,*prog,*prog);
		#endif
		while(!isdelim(*prog)) {
			#ifdef DEBUG
			printf("\nisdelim loop prog: %d %c %x",*prog,*prog,*prog);
			
			#endif
			*temp++=*prog++;
		}
		token_type=TEMP;
	}
	#ifdef DEBUG
	printf("\nexited if(isalpha(), changing temp to '\\0', checking if token_type==TEMP");
	#endif
	*temp = '\0';
	if(token_type==TEMP) { 	/* see if a string is a command or a variable */
		#ifdef DEBUG
		printf("\ntoken_type TEMP has been found, calling look_up(token) for enumeration");
		#endif
		tok = look_up(token); /* convert to internal rep */
		if(tok) {
			token_type = KEYWORD; /* is a keyword */
			#ifdef DEBUG
			printf("\nkeyword found, token_type adjusted to %s",token_id[token_type].t_name);
			#endif
		} else {
			token_type = IDENTIFIER;
			#ifdef DEBUG
			printf("\nidentfier found, token_type adjusted to %s",token_id[token_type].t_name);
			#endif
		}
	}
	#ifdef DEBUG
	printf("\nTEMP not found, get_token() exiting from final condition with token_type = %s",token_id[token_type].t_name);
	printf("\nprog: %c %d %x",*prog,*prog,*prog);
	#endif
	return token_type=BLOCK;
}

/* Return a	token to input stream. */
void putback(void)
{
	#ifdef DEBUG
	printf("\nputback() entered",*prog,*prog,*prog);
	#endif
	char *t;
	t = token;
	for(; *t; t++) {
		#ifdef DEBUG
		printf("\nputback() prog: %d %c %x",*prog,*prog,*prog);
		#endif
		prog--;
	}
}

/* Look	up a token's internal representation in	the
   token table 
*/
int	look_up(char *s)
{
	register int i;
	char *p;
	
	p = s;
	#ifdef DEBUG
	printf("\nlook_up() entered. Cannot display s yet");
	#endif
	while(*p) { /* convert to lower case */
		*p = tolower(*p);
		p++;
	}
	/* see if token is in table */
	for(i=0; *table[i].command; i++) {
		if(!strcmp(table[i].command, s)) {
			#ifdef DEBUG
			printf("\nreturning table[i].tok: %d %c %x %s",table[i].tok,table[i].tok,table[i].tok,table[i].tok);
			#endif
			return table[i].tok;
		}
	}
	return 0;
}

/* Return index	of internal	library	function or	-l if
   not found.
*/
int	internal_func(char *s)
{
	int i;

	#ifdef DEBUG
	printf("\ninternal_func entered with s = %s",s);
	#endif
	for(i=0; intern_func[i].f_name[0]; i++) {
		if(!strcmp(intern_func[i].f_name, s)) {
			#ifdef DEBUG
			printf("\ninternal_func found, returning %s",intern_func[i].f_name);
			#endif
			return i;
		}
	}
  return -1;
}

/* Return true if c	is a delimiter.	*/
int	isdelim(char c)
{
	#ifdef DEBUG
	printf("\nisdelim() called with c = %c",c);
	#endif
	if(strchr(" !;,+-<>'/*%&^=()", c) || c==9 || c=='\r' || c==0) {
		#ifdef DEBUG
		printf("\nisdelim() returning True");
		#endif
		return 1;
	}
	#ifdef DEBUG
	printf("\nisdelim() returning false");
	#endif
	return 0;
}

/* Return a	1 if c is space	or tab */
int	iswhite(char c)
{
	#ifdef DEBUG
		printf("\niswhite entered with c = %c, %d, %x",c,c,c);
	#endif
	
	if(c==' ' || c=='\t') {
		#ifdef DEBUG
		printf("\niswhite returning True");
		#endif
		return 1;
	} else {
		#ifdef DEBUG
		printf("\niswhite returning false");
		#endif
		return 0;
	}
}