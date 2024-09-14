
%{
  #include "../inc/parser.hpp"
  #include <string.h>
  #include <stdlib.h>
  int line_num = 1;

%}

%x COMMENT

%option outfile="lexer.cpp" header-file="lexer.hpp"
%option yylineno
%option noyywrap
%option nounput

GPR		(r([0-9]|1[0-5])|sp|SP|pc|PC)
CSR		status|handler|cause
IDENT	[A-Za-z_][A-Za-z0-9_]*
DEC 	(-?[0-9]+)
HEX		(-?0[xX][0-9a-fA-F]+)
BIN 	(-?0[bB][01]+)
	



/*
[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }
[0-9]+         { yylval.ival = atoi(yytext); return INT; }
[a-zA-Z0-9]+   {
  yylval.sval = strdup(yytext);
  return STRING;
}
*/





%%

[ \t]          ;


\.global		{ return GLOBAL; }
\.extern		{ return EXTERN; }
\.section		{ return SECTION; }
\.word			{ return WORD; }
\.skip 			{ return SKIP; }
\.end			{ return END; }


halt		{ return HALT; }
int			{ return INT; }
iret		{ return IRET; }
call		{ return CALL; }
ret			{ return RET; }
jmp			{ return JMP; }
beq			{ return BEQ; }
bne			{ return BNE; }
bgt			{ return BGT; }
push 		{ return PUSH; }
pop 		{ return POP; }
xchg|add|sub|mul|div|and|or|xor|shl|shr {return OPERATION;}
not			{return NOT;}
ld			{ return LD;}
st 			{return ST;}
csrrd		{return CSRRD;}
csrwr		{return CSRWR;}

"%"{GPR}	{ // TODO obradi br registra
			yylval.ident = strdup(yytext); // FIX: MEMORY LEAK
			return GPR;
			}
"%"{CSR}	{ // TODO obradi br registra
			yylval.ident = strdup(yytext); // FIX: MEMORY LEAK
			return CSR;
			}
{IDENT}:	{ 
			yylval.ident = strdup(yytext); // FIX: MEMORY LEAK
			return LABEL;
			}
{IDENT}		{ 
			yylval.ident = strdup(yytext); // FIX: MEMORY LEAK
			return IDENT;
			}
{DEC}|{BIN}|{HEX}		{
						yylval.num = strtol(yytext,NULL,0); 
						return NUM;
						}		
,	{ return COMMA; }	
"["	{return LBRACKET;}
"]" {return RBRACKET;}
\+ {return PLUS;}
"$"	{return IMMEDIATE;}




#			   { BEGIN(COMMENT); }
<COMMENT>\n	   { BEGIN(INITIAL); ++line_num; return ENDL; }
<COMMENT>.		;

\n             { ++line_num; return ENDL; }
.              ;







%%