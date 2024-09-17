
%{
  #include "../inc/parser.hpp"
  #include <string.h>
  #include <stdlib.h>
  int line_num = 1;

%}

%x COMMENT
%x ENDING

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
	


%%

[ \t]          ;


\.global		{ return GLOBAL; }
\.extern		{ return EXTERN; }
\.section		{ return SECTION; }
\.word			{ return WORD; }
\.skip 			{ return SKIP; }
\.end			{ BEGIN(ENDING); return END; }


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
xchg|add|sub|mul|div|and|or|xor|shl|shr {
				yylval.ident = new std::string(yytext);
				return OPERATION;
			}
not			{return NOT;}
ld			{ return LD;}
st 			{return ST;}
csrrd		{return CSRRD;}
csrwr		{return CSRWR;}

"%"{GPR}	{ 
			switch(yytext[1]){
				case 'r':{
					yylval.gprType = (GPRType)strtol(yytext+2,NULL,0);
					break;
				}
				case 'p':{
					yylval.gprType = GPRType::PC;
					break;
				}
				case 's':{
					yylval.gprType=GPRType::SP;
				}
			}
			return GPR;
			}
"%"{CSR}	{ 
				switch(yytext[1]){
					case 's':{
						yylval.csrType = CSRType::STATUS;
						break;
					}
					case 'h':{
						yylval.csrType = CSRType::HANDLER;
						break;
					}
					case 'c':{
						yylval.csrType= CSRType::CAUSE;
					}
				}
				return CSR;
			}
{IDENT}:	{ 
			yylval.ident = new std::string(yytext);//strdup(yytext); // FIX: MEMORY LEAK
			yylval.ident->pop_back();
			return LABEL;
			}
{IDENT}		{ 
			yylval.ident = new std::string(yytext);
			//strdup(yytext); // FIX: MEMORY LEAK
			return IDENT;
			}
{DEC}|{HEX}		{
					yylval.num = strtol(yytext,NULL,0); 
					return NUM;
				}	

{BIN}			{
					yylval.num = strtol(yytext+2,NULL,2);
					return NUM;
				}	
,	{ return COMMA; }	
"["	{return LBRACKET;}
"]" {return RBRACKET;}
\+ {return PLUS;}
"$"	{return IMMEDIATE;}



<ENDING>.		;
<ENDING>\n		;
#			   { BEGIN(COMMENT); }
<COMMENT>\n	   { BEGIN(INITIAL); ++line_num; return ENDL; }
<COMMENT>.		;

\n             { ++line_num; return ENDL; }
.              ;







%%