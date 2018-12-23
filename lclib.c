

#include"stdio.h"
#include"stdlib.h"

extern char *prog;
extern char token[80];
extern char token_type;
extern char tok;

enum tok_types {DELIMITER, IDENTIFIER, NUMBER, COMMAND, STRING,
		QUOTE, VARIABLE, BLOCK, FUNCTION};

enum error_msg
	{SYNTAX, UNBAL_PARENS, NO_EXP, EQUALS_EXPECTED,
	 NOT_VAR, PARAM_ERR, SEMI_EXPECTED,
	 UNBAL_BRACES, FUNC_UNDEF, TYPE_EXPECTED,
	 NEST_FUNC, RET_NOCALL, PAREN_EXPECTED,
	 WHILE_EXPECTED, QUOTE_EXPECTED, NOT_STRING,
	 TOO_MANY_LVARS};

int get_token(void);
void sntx_err(int error), eval_exp(int *result);
void putback(void);

int call_getche(void)
{
	char ch;
	ch = getchar();
	while(*prog!=')') prog++;
	prog++;
	return ch;
}
int call_putch(void)
{
	int value;
	eval_exp(&value);
	printf("%c",value);
	return value;
}
int call_puts(void)
{
	get_token();
	if(*token!='(') sntx_err(PAREN_EXPECTED);
	get_token();
	if(token_type!=QUOTE) sntx_err(QUOTE_EXPECTED);
	puts(token);
	get_token();
	if(*token!=')') sntx_err(PAREN_EXPECTED);
	get_token();
	if(*token!=';') sntx_err(SEMI_EXPECTED);
	putback();
	return 0;
}
int print(void)
{
	int i;
	get_token();
	if(*token!='(') sntx_err(PAREN_EXPECTED);
	get_token();
	if(token_type==QUOTE) {
		printf("%s ", token);
	}
	else {
		putback();
		eval_exp(&i);
		printf("%d ", i);
	}
	get_token();
	if(*token!=')') sntx_err(PAREN_EXPECTED);
	get_token();
	if(*token!=';') sntx_err(SEMI_EXPECTED);
	putback();
	return 0;
}
int getnum(void)
{
	char s[80];
	fgets(s, 80, stdin);
	while(*prog!=')') prog++;
	prog++;
	return atoi(s);
}
