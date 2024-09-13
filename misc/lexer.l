
%{
  #include "../inc/parser.hpp"
  int line_num = 1;
%}

%option outfile="lexer.cpp" header-file="lexer.hpp"
%option yylineno
%option noyywrap
%option nounput

%%
[ \t]          ;
sNaZZle        { return SNAZZLE; }
type           { return TYPE; }
end            { return END; }
[0-9]+\.[0-9]+ { yylval.fval = atof(yytext); return FLOAT; }
[0-9]+         { yylval.ival = atoi(yytext); return INT; }
[a-zA-Z0-9]+   {
  yylval.sval = strdup(yytext);
  return STRING;
}
\n             { ++line_num; return ENDL; }
.              ;
%%