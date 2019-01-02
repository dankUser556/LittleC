/* A Little C interpreter */
#include<stdio.h>
#include<setjmp.h>
#include<math.h>
#include<ctype.h>
#include<stdlib.h>
#include<string.h>

#define NUM_FUNC	100
#define NUM_GLOBAL_VARS 100
#define NUM_LOCAL_VARS  200
#define NUM_BLOCK	100
#define ID_LEN		31
#define FUNC_CALLS	31
#define NUM_PARAMS	31
#define PROG_SIZE	10000
#define LOOP_NEST	31
#define MAIN_ENTRY	"main"

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, KEYWORD,
		TEMP, STRING, BLOCK};

/* add additional C keywords tokens here */
enum tokens {ARG, CHAR, INT, IF, ELSE, FOR, DO, WHILE,
	     SWITCH, RETURN, EOL, FINISHED, END};

/* add additional double operators her (such as ->) */
enum double_ops {LT=1, LE, GT, GE, EQ, NE};

/* These are the constants used to call sntx_err() when
   a syntax error occurs. Add more if you like.
   NOTE: SYNTAX is a generic error message used when
   nothing else seems appropriate.
*/
enum error_msg
	{SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED,
	 NOT_VAR, PARAM_ERR, SEMI_EXPECTED,
	 UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
	 NEST_FUNC, RET_NOCALL, PAREN_EXPECTED,
	 WHILE_EXPECTED, QUOTE_EXPECTED, NOT_TEMP,
	 TOO_MANY_LVARS};

char *prog; /* current location in source code */
char *p_buf; /* points to start of program buffer */
jmp_buf e_buf; /* hold environment for longjmp() */

/* An array of these structures will hold the info
   associated with global variables.
*/
struct var_type {
	char var_name[ID_LEN];
	int var_type;
	int value;
} global_vars[NUM_GLOBAL_VARS];

struct var_type local_var_stack[NUM_LOCAL_VARS];

struct func_type {
	char func_name[ID_LEN];
	char *loc; /* location of entry point in file */
} func_table[NUM_FUNC];

int call_stack[NUM_FUNC];

struct commands { /* keyword lookup table */
	char command[20];
	char tok;
} table[] = { /* Commands must be entered lowercase */
	"if", IF, /* in this table */
	"else", ELSE,
	"for", FOR,
	"do", DO,
	"while", WHILE,
	"char", CHAR,
	"int", INT,
	"return", RETURN,
	"end", END,
	"", END /* Mark end of table...? */
};

char token[80];
char token_type, tok;

int functos; /* index to top of function call stack */
int func_index; /* index into function table */
int gvar_index; /* index into global variable table */
int lvartos; /* index into local varuable stack */

int ret_value; /* function return value */

void  print(void), prescan(void), decl_global(void);
void   call(void), putback(void), decl_local(void);
void local_push(struct var_type), eval_exp(int*), sntx_err(int);
void    exec_if(void), 	   find_eob(void), exec_for(void);
void get_params(void),	   get_args(void), exec_while(void);
void func_push(int i), 		exec_do(void),  assign_var(char*,int);
void debug_delay(int), interp_block(void), func_ret(void);

int load_program(char*,char*), find_var(char*);
int 		   func_pop(void), is_var(char*), get_token(void);
char *find_func(char *name);

/* Debugging functions */
void dump_buf(char*,int,int);
int  eof_check(char);

int main(int argc, char *argv[])
{
	system("clear");
	if(argc!=2) {
		printf("usage: littlec <filename>\n");
		exit(1);
	}

	/* allocate memory for the program */
	if((p_buf=(char *) malloc(PROG_SIZE))==NULL) {
		printf("\nMemory allocation failure");
		exit(1);
	}
	#ifdef DEBUG
	printf("\nMemory allocation complete");
	#endif
	/* load the program to execute */
	
	if(!load_program(p_buf, argv[1])) exit(1);
	if(setjmp(e_buf)) exit(1);
	
	/* set the program pointer to the start of the program buffer */
	prog = p_buf;
	#ifdef DEBUG
	printf("\ncalling prescan() from main()");
	#endif
	prescan(); /* find the location of all functions 
			and global variables in the program */
	gvar_index = 0; /* initialize global variable index */
	lvartos = 0;	/* initialize local variable stack index */
	functos = 0;	/* initialize the CALL stack index */

	/* setup call to main() */
	prog = find_func((char *)"main"); /* find program starting point */
	prog--; /* back up to opening ( */
	strcpy(token, (char *)"main"); /* cast to (char *) for warning */
	call(); /* call main() to start interpreting */
	#ifdef DEBUG
	printf("\n");
	#endif
	
}

/* Interpret a single statement or block of code. When
   interp_block() returns from its initial call, the final
   brace (or a return) in main() has been encountered.
*/
void interp_block(void)
{
	int value;
	char block = 0;
	#ifdef DEBUG
	printf("\ninterp_block(void) entered, no significant vars to report");
	#endif
	
	do {
		token_type = get_token();

		/* If interpreting single statement, return on
		   first semicolon.
		*/

		/* see what kind of token is up */
		if(token_type==IDENTIFIER) {
			/* Not a keyword, so a process expression. */
			putback(); /* restore token to input stream for 
					further processing by eval_exp() */
			eval_exp(&value); /* process the expression */
			if(*token!=';') sntx_err(SEMI_EXPECTED);
		}
		else if(token_type==BLOCK) { /*if block delimiter */
			if(*token=='{') /* is a block */
				block = 1;
			else return; /* is a }, so return */
		}
		else /* is a keyword */
			switch(tok) {
				case CHAR:
				case INT: /* declare local variables */
					putback();
					decl_local();
					break;
				case RETURN: /* return from function call */
					func_ret();
					return;
				case IF: /* process an if statement */
					exec_if();
					break;
				case ELSE: /* process an else statement */
					find_eob(); /* find end of else block
							and continue execution */
					break;
				case WHILE: /* process a while loop */
					exec_while();
					break;
				case DO: /* process a do-while loop */
					exec_do();
					break;
				case FOR: /* process for loop */ 
					#ifdef DEBUG
					printf("\ncase FOR found, calling exec_for()");
					#endif
					exec_for();
					break;
				case END:
					exit(0);
			}
	} while (tok != FINISHED && block);
}

/* Load a program */
int load_program(char *p, char *fname)
{
	#ifdef DEBUG
	printf("\nload_program entered using file: %s",fname);
	#endif
	FILE *fp;
	int i=0;
	
	if((fp=fopen(fname, "rb"))==NULL) return 0;
	i = 0;
	do {
		*p = getc(fp);
		p++; i++;
	} while(!feof(fp) && i<PROG_SIZE);
	
	*(p-1) = '\0'; /* '\0' terminate the program */
	fclose(fp);
	#ifdef DEBUG
	dump_buf(p_buf,i,0);
	#endif
	return 1;
}

/* Find the location of all functions in the program and store global variables. */
void prescan(void)
{
	#ifdef DEBUG
	static int prescan_count = 0;
	printf("\nprescan() entered");
	#endif
	
	char *p;
	char temp[32];
	int brace = 0; /* When 0, this var tells us that current source position is outside of any function. */
	p = prog;
	func_index = 0;
	do {
		
		#ifdef DEBUG
		printf("\nprescan do-while loop counter: %d",++prescan_count);
		#endif
		
		while(brace) { /* BRACE LOOP bypasses function codeblocks */				
			#ifdef DEBUG
			static int loop_count = 1;
			printf("\nbrace loop calling get_token(), loop count: %d",loop_count++);
			//if(loo???
			#endif
			get_token();
			if(*token=='{') {
				brace++;
				#ifdef DEBUG
				printf("\nbrace incremented from head of loop.");
				#endif
			}
			#ifdef DEBUG
			printf("\nchecking for end of brace loop with value: %c",*token);
			#endif
			if(*token=='}') {
				brace--;
				#ifdef DEBUG
				printf("\nbrace loop complete. Final loop count: %d",loop_count);
				loop_count = 1;
				#endif
			}
		}
		#ifdef DEBUG
		printf("\nCalling get_token() after skipping brace loop");
		#endif
		get_token();
		if(tok==CHAR || tok==INT) { /* is global var */
			#ifdef DEBUG
			printf("\nGlobal var found, declaring %s", (tok==CHAR) ? "CHAR": "INT");
			#endif
			putback();
			decl_global();
		} else if(token_type==IDENTIFIER) {
			strcpy(temp, token);
			#ifdef DEBUG
			printf("\ncalling get_token() from if(IDENTIFIER) in prescan()");
			#endif
			get_token();
			if(*token=='(') { /* must assume a function */
				#ifdef DEBUG
				printf("\nFunction found by prescan(), adding \"%s()\" to func_table",temp);
				#endif
				func_table[func_index].loc = prog;
				strcpy(func_table[func_index].func_name, temp);
				func_index++;
				while(*prog!=')') prog++;
				prog++;
				/* prog points to opening curly brace of function */
			} else {
				#ifdef DEBUG
				printf("\ncalling putback() from if( token=\"(\" ) else clause");
				#endif
				putback(); 
			  }
		} else if(*token=='{') {
			#ifdef DEBUG
			printf("\nchanging brace value from end of prescan");
			#endif
			brace++;
		}
	} while(tok!=FINISHED);
	prog = p;
	#ifdef DEBUG
	printf("\nexiting prescan()");
	#endif
}

/* Return the entry point of the specified function.
   Return NULL if not found.
*/
char *find_func(char *name)
{
	register int i;

	for(i=0; i<func_index;i++)
		if(!strcmp(name, func_table[i].func_name))
			return func_table[i].loc;
	return NULL;
}

/* Declare a global variable */
void decl_global(void)
{
	#ifdef DEBUG
	printf("\ndecl_global entered, calling get_token()");
	#endif
	get_token(); /* get type */
	global_vars[gvar_index].var_type = tok;
	global_vars[gvar_index].value = 0; /* init to 0 */

	do { /* process comma-separated list */
		#ifdef DEBUG
		printf("calling get_token to process global comma list");
		#endif
		get_token(); /* get name */
		strcpy(global_vars[gvar_index].var_name,token);
		#ifdef DEBUG
		printf("calling get_token again for global comma list");
		#endif
		get_token();
		gvar_index++;
	} while(*token==',');
	if(*token!=';') sntx_err(SEMI_EXPECTED);
}

/* Declare a local variable */
void decl_local(void)
{
	#ifdef DEBUG
	printf("\ndecl_local() entered, calling get_token");
	#endif
	struct var_type i;
	get_token(); /* get type */
	i.var_type = tok;
	i.value = 0; /* init to 0 */

	do { /* process comma-separated list */
		#ifdef DEBUG
		printf("\ncalling get_token() for local comma list");
		#endif
		get_token();
		strcpy(i.var_name, token);
		local_push(i);
		#ifdef DEBUG
		printf("\ncalling get_token() again for local comma list");
		#endif
		get_token();
	} while (*token==',');
	if(*token!=';') sntx_err(SEMI_EXPECTED);
}

/* Call a function */
void call(void)
{
	char *loc, *temp;
	int lvartemp;

	loc = find_func(token); /* find entry point of function */
	if(loc==NULL)
		sntx_err(FUNC_UNDEF); /* Function not defined */
	else {
		lvartemp = lvartos; /* save local var stack index */
		get_args(); /* get function arguments */
		temp = prog; /* save return location */
		func_push(lvartemp); /* save local var stack index */
		prog = loc; /* reset prog to start of function */
		get_params(); /* load the functions parameters with
				the values of the arguments */
		interp_block(); /* interpret the function */
		prog = temp; /* reset the program pointer */
		lvartos = func_pop(); /* reset the local var stack */
	}
}
/* Push the arguments to a function onto the local 
   variable stack */
void get_args(void)
{
	int value, count, temp[NUM_PARAMS];
	struct var_type i;
	count = 0;
	get_token();
	if(*token!='(') sntx_err(PAREN_EXPECTED);

	/* process a comma separated list of values */
	do {
		eval_exp(&value);
		temp[count] = value; /* save temporarily */
		get_token();
		count++;
	} while(*token==',');
	count--;
	/* now, push on local_var_stack in reverse order */
	for(;count >=0; count--) {
		i.value = temp[count];
		i.var_type = ARG;
		local_push(i);
	}
}
/* Get function parameters. */
void get_params(void)
{
	struct var_type *p;
	int i;
	i = lvartos-1;
	do { /* process comma-separated list of parameters */
		get_token();
		p = &local_var_stack[i];
		if(*token!=')') {
			if(tok!=INT && tok!=CHAR) sntx_err(TYPE_EXPECTED);
			p->var_type = token_type;
			get_token();

			/* link parameter name with argument alread on	
				local var stack */
			strcpy(p->var_name, token);
			get_token();
			i--;
		}
		else break;
	} while(*token==',');
	if(*token!=')') sntx_err(PAREN_EXPECTED);
}
/* Return from a function */
void func_ret(void)
{
	int value;
	value = 0;
	/* get return value, if any */
	eval_exp(&value);

	ret_value = value;
}
/* Push local variable */
void local_push(struct var_type i)
{
	if(lvartos>NUM_LOCAL_VARS)
		sntx_err(TOO_MANY_LVARS);
	local_var_stack[lvartos] = i;
	lvartos++;
}
/* Pop index into local variable stack. */
int func_pop(void)
{
	functos--;
	if(functos<0) sntx_err(RET_NOCALL);
	return(call_stack[functos]);
}
/* Push index of local variable stack. */
void func_push(int i)
{
	if(functos>NUM_FUNC)
		sntx_err(NEST_FUNC);
	call_stack[functos]=i;
	functos++;
}
/* Assign a value to a variable */
void assign_var(char *var_name, int value)
{
	register int i;
	/* first see if its a local variable */
	for(i=lvartos-1;i>=call_stack[functos-1];i--) {
		if(!strcmp(local_var_stack[i].var_name, var_name)) {
			local_var_stack[i].value=value;
			return;
		}
	}
	if(i < call_stack[functos-1])	/* if not local, try global var table */
		for(i=0; i<NUM_GLOBAL_VARS; i++)
			if(!strcmp(global_vars[i].var_name,var_name)) {
				global_vars[i].value = value;
				return;
			}
	sntx_err(NOT_VAR); /* variable not found */
}
/* Find the value of a variable */
int find_var(char *s)
{
	register int i;
	/* first see if it's a local variable */
	for(i=lvartos-1;i>=call_stack[functos-1];i--)
		if(!strcmp(local_var_stack[i].var_name, token))
			return local_var_stack[i].value;
	/* otherwise, try global vars */
	for(i=0; i<NUM_GLOBAL_VARS; i++)
		if(!strcmp(global_vars[i].var_name, s))
			return global_vars[i].value;
	sntx_err(NOT_VAR); /* hotdog not found */
}
/* Determine if an identifier is a variable. Return
   1 if a variable is found; 0 otherwise */
int is_var(char *s)
{
	register int i;
	for(i=lvartos-1;i>=call_stack[functos-1];i--)
		if(!strcmp(local_var_stack[i].var_name, token))
			return 1;
	for(i=0; i<NUM_GLOBAL_VARS; i++)
		if(!strcmp(global_vars[i].var_name,s))
			return 1;
	return 0;
}

/* execute if statement */
void exec_if(void)
{
	int cond;
	eval_exp(&cond);
	if(cond) {
		interp_block();
	}
	else {
		find_eob();
		get_token();
		if(tok!=ELSE) {
			putback();
			return;
		}
		interp_block();
	}
}
void exec_while(void)
{
	int cond;
	char *temp;
	putback();
	temp = prog;
	get_token();
	eval_exp(&cond);
	if(cond) interp_block();
	else {
		find_eob();
		return;
	}
	prog = temp;
}
void exec_do(void)
{
	int cond;
	char *temp;
	putback();
	temp = prog;
	get_token();
	interp_block();
	get_token();
	if(tok!=WHILE) sntx_err(WHILE_EXPECTED);
	eval_exp(&cond);
	if(cond) prog = temp;
}
void find_eob(void)
{
	int brace;
	get_token();	
	brace = 1;
	do {
		get_token();
		if(*token=='{') brace++;
		else if(*token=='}') brace--;
	} while(brace);
}
void exec_for(void)
{
	int cond;
	char *temp, *temp2;
	int brace;
	get_token();
	eval_exp(&cond);
	if(*token!=';') sntx_err(SEMI_EXPECTED);
	prog++;
	temp = prog;
	for(;;) {
		eval_exp(&cond);
		if(*token!=';') sntx_err(SEMI_EXPECTED);
		prog++;
		temp2 = prog;
		brace = 1;
		while(brace) {
			get_token();
			if(*token=='(') brace++;
			if(*token==')') brace--;
		}
		if(cond) interp_block();
		else {
			find_eob();
			return;
		}
		prog = temp2;
		eval_exp(&cond);
		prog = temp;
	}
}
void debug_delay(int var)
{	//Here is a function I wrote to slow down loops during debug. I hope it works
	int i, d;	
	for (i=0;i<var;i++)
		for (d=0;d<var;d++) {}
}

void dump_buf(char *buf,int prog_size,int dump_mode)
{
	int i = 0;
	char *buf_origin = buf;
	printf("\ndump_buf entered, dumping buffer contents in ascii...\n");
	if((dump_mode = 0 )|| (dump_mode =1)) {	
		while ((i<prog_size) && (!eof_check(*buf))) {
			printf("%c",*buf++);
			i++;
		}
	}
	i = 0;
	buf = buf_origin;
	printf("\nbuf reinitialized, dumping in hex mode...\n");
	if((dump_mode = 0) || (dump_mode = 2)) {
		while ((i<prog_size) && (!eof_check(*buf))) {
			printf("%x",*buf++);
			i++;
		}
	}
}

int eof_check(char ch) 
{
	if(ch = '\0') {
		return 1;
	}
	return 0;
}