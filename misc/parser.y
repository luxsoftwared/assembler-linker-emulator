
%{
  #include <cstdio>
  #include <iostream>
  #include <cstdint>
  #include <string>
 //using namespace std;
      
  // stuff from flex that bison needs to know about:
  extern int yylex();
  extern int yyparse();
  extern FILE *yyin;
  extern int line_num;
 
  void yyerror(const char *s);
%}

%code requires{
#include <cstdio>
  #include <iostream>
  #include <cstdint>
  #include <string>
}

%output "parser.cpp"
%defines "parser.hpp"

%union {
	//const std::string* ident; //std::
	char* ident;
	long num;

}

%token GLOBAL EXTERN SECTION WORD SKIP END
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP
%token NOT LD ST CSRRD CSRWR
%token ENDL COMMA LBRACKET RBRACKET PLUS IMMEDIATE

// program line labeled_line unlabeled_line endls instruction directive

%token <ident> LABEL IDENT 
%token <ident> GPR CSR
%token <ident> OPERATION
%token <num> NUM 



%%

program: program line[l]
		{

		}
		|
		endls
		|
		%empty

line: labeled_line
		|
		unlabeled_line

labeled_line: LABEL endls labeled_line	{ std::cout<< "Labela1 " << $1<<"\n"; }
			  |
			  LABEL labeled_line{ std::cout<< "Labela2 " << $1<<"\n"; }
			  |
			  LABEL endls unlabeled_line{ std::cout<< "Labela3 " << $1<<"\n"; }
			  |
			  LABEL unlabeled_line{ std::cout<< "Labela4 " << $1<<"\n"; }

unlabeled_line: instruction endls 
				|
				directive endls
				|
				END { std::cout<<"end direktiva\n";}

directive: GLOBAL symbol_list { std::cout<<"global direktiva\n";}
		   |
		   EXTERN symbol_list
		   |
		   SECTION IDENT	{ std::cout<<"sekcija: "<< $2<<"\n";}
		   |
		   WORD symbol_or_literal_list { std::cout<<"word: \n";}
		   |
		   SKIP NUM	{ std::cout<<"skip: "<< $2;}
		   |
		   END{ std::cout<<"end direktiva\n";}



instruction: HALT { std::cout<<"halt \n";}
			 |
			 INT  {{ std::cout<<"int \n";}}
			 |
			 IRET	{ std::cout<<"iret \n";}
			 |
			 CALL jmp_operand
			 |
			 RET
			 |
			 JMP jmp_operand
			 |
			 BEQ GPR COMMA GPR COMMA jmp_operand { std::cout<<"beq "<< $2<<$4<<"\n"; }
			 |
			 BNE GPR COMMA GPR COMMA jmp_operand { std::cout<<"beq "<< $2<<$4<<"\n"; }
			 |
			 BGT GPR COMMA GPR COMMA jmp_operand { std::cout<<"beq "<< $2<<$4<<"\n"; }
			 |
			 PUSH GPR
			 |
			 POP GPR
			 |
			 NOT GPR
			 |
			 OPERATION GPR COMMA GPR
			 |
			 LD data_operand COMMA GPR
			 |
			 ST GPR COMMA data_operand
			 |
			 CSRRD CSR COMMA GPR
			 |
			 CSRWR GPR COMMA CSR

jmp_operand: NUM {std::cout<<$1<<" ";}
			 |
			 IDENT {std::cout<<$1<<" ";}

data_operand: NUM {std::cout<<$1<<" ";}
			 |
			 IDENT {std::cout<<$1<<" ";}
			 |
			 IMMEDIATE NUM
			 |
			 IMMEDIATE IDENT
			 |
			 GPR
			 |
			 LBRACKET GPR RBRACKET
			 |
			 LBRACKET GPR PLUS NUM RBRACKET
			 |
			 LBRACKET GPR PLUS IDENT RBRACKET



symbol_list: 	symbol_list COMMA IDENT {std::cout<<$3<<" ";}
				|
				IDENT	{std::cout<<$1<<" ";}

symbol_or_literal_list: symbol_or_literal_list COMMA NUM {std::cout<< "SLL1"<<$3<<" ";}
						|
						symbol_or_literal_list COMMA IDENT {std::cout<<"SLL2"<<$3<<" ";}
						|
						NUM {std::cout<<"SLL3"<<$1<<" ";}
						|
						IDENT {std::cout<<"SLL4"<<$1<<" ";}

endls: endls ENDL | ENDL






/*
snazzle:
  header template body_section footer {
      cout << "done with a snazzle file!" << endl;
    }
  ;
header:
  SNAZZLE FLOAT ENDLS {
      cout << "reading a snazzle file version " << $2 << endl;
    }
  ;
template:
  typelines
  ;
typelines:
  typelines typeline
  | typeline
  ;
typeline:
  TYPE STRING ENDLS {
      cout << "new defined snazzle type: " << $2 << endl;
      free($2);
    }
  ;
body_section:
  body_lines
  ;
body_lines:
  body_lines body_line
  | body_line
  ;
body_line:
  INT INT INT INT STRING ENDLS {
      cout << "new snazzle: " << $1 << $2 << $3 << $4 << $5 << endl;
      free($5);
    }
  ;

footer:
	END ENDLS
  	|
	END
	;
ENDLS:
  ENDLS ENDL
  | ENDL ;

*/
%%

int main(int, char**) {
  // open a file handle to a particular file:
  FILE *myfile = fopen("./tests/handler.s", "r");
  // make sure it's valid:
  if (!myfile) {
    std::cout << "I can't open in.snazzle.file!" << std::endl;
    return -1;
  }
  // set lex to read from it instead of defaulting to STDIN:
  yyin = myfile;

  // parse through the input until there is no more:
  
  do {
    yyparse();
  } while (!feof(yyin));
  
}

void yyerror(const char *s) {
  std::cout << "parse error on line " << line_num << "!  Message: " << s << std::endl;
  // might as well halt now:
  exit(-1);
}
