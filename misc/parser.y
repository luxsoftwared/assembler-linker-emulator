
%{
  #include <cstdio>
  #include <iostream>
  #include <cstdint>
  #include <string>
 //using namespace std;
	#include "../inc/parsingTypes.hpp"
      
  // stuff from flex that bison needs to know about:
  extern int yylex();
  //extern int yyparse();
  extern FILE *yyin;
  extern int line_num;
 
 
%}

%code requires{ //ovo vazi i za uniju
#include <cstdio>
  #include <iostream>
  #include <cstdint>
  #include <string>
  #include "../inc/parsingTypes.hpp"
  #include "../inc/Assembler.hpp"
   void yyerror(Assembler* assembler,const char *s);
}

%output "parser.cpp"
%defines "parser.hpp"

%parse-param{Assembler* assembler}

%union {
	//const std::string* ident; //std::
	std::string* ident;
	long num;
	SymList* symList;
	SymOrLitList* symOrLitList;
	Directive* _directive;
	Line*	_line;
	Instruction* instruction;
	Operand operand;
	GPRType gprType;
	CSRType csrType;
	DataOperand dataOperand;
}

%token GLOBAL EXTERN SECTION WORD SKIP END
%token HALT INT IRET CALL RET JMP BEQ BNE BGT PUSH POP
%token NOT LD ST CSRRD CSRWR
%token ENDL COMMA LBRACKET RBRACKET PLUS IMMEDIATE

// program line labeled_line unlabeled_line endls instruction directive

%token <ident> LABEL IDENT 
%token <ident> OPERATION
%token <num> NUM 
%token <gprType> GPR
%token <csrType> CSR

%type <symOrLitList> symbol_or_literal_list
%type <symList> symbol_list
%type <_directive> directive
%type <_line> unlabeled_line labeled_line line
%type <instruction> instruction
%type <operand> jmp_operand
%type <dataOperand> data_operand

%%

program: program line[l]
		{	$l->lineNumber=@$.last_line;
			Line::printLine($l);
			assembler->processLine($l);
			
		}
		|
		endls
		|
		%empty

line: labeled_line{ $$=$1; }
		|
		unlabeled_line{ $$=$1; }

labeled_line: LABEL endls labeled_line[line]	{ //std::cout<< "Labela1 " << $1<<"\n"; 
				$line->labels.push_back($1);
				$$=$line; 
			  }
			  |
			  LABEL labeled_line[line] { //std::cout<< "Labela2 " << $1<<"\n"; 
			  	$line->labels.push_back($1);
				$$=$line; 
			  }
			  |
			  LABEL endls unlabeled_line[line]{ //std::cout<< "Labela3 " << *$1<<"\n"; 
			  	$line->labels.push_back($1);
				$$=$line;
			  }
			  |
			  LABEL unlabeled_line[line]{ //std::cout<< "Labela4 " << $1<<"\n";
				$line->labels.push_back($1);
				$$=$line;
			  }

unlabeled_line: instruction[i] endls {
					Line* line=new Line;
					line->type=Line::INSTRUCTION;
					line->instruction=$i;
					$$=line;
				}
				|
				directive endls{ 
					Line* line=new Line;
					line->type=Line::DIRECTIVE;
					line->directive=$1;
					$$=line; 
				}
				|
				END { 
					Directive* d=new Directive;
					d->type=Directive::END;
					Line* line=new Line;
					line->type=Line::DIRECTIVE;
					line->directive=d;
					$$=line; 
				}

directive: GLOBAL symbol_list[list] { 
				Directive* d=new Directive;
				d->type=Directive::GLOBAL;
				d->symList=$list;
				$$=d;
			}
		   |
		   EXTERN symbol_list[list]{
				Directive* d=new Directive;
				d->type=Directive::EXTERN;
				d->symList=$list;
				$$=d;
		   }
		   |
		   SECTION IDENT[i]	{
				Directive* d=new Directive;
				d->type=Directive::SECTION;
				d->symbol=$i;
				$$=d;
		   }
		   |
		   WORD symbol_or_literal_list[list] {
				Directive* d=new Directive;
				d->type=Directive::WORD;
				d->symOrLitList=$list;
				$$=d;
			}
		   |
		   SKIP NUM[n]	{
				Directive* d=new Directive;
				d->type=Directive::SKIP;
				d->literal=$n;
				$$=d;
		   }
		   |
		   END{ 
		   	Directive* d=new Directive;
			d->type=Directive::END;
			$$=d;
		   }



instruction: HALT {
			 	Instruction* instr=new Instruction;
				instr->type=Instruction::HALT;
				$$=instr;
			 }
			 |
			 INT  {
				Instruction* instr=new Instruction;
				instr->type=Instruction::INT;
				$$=instr;
			 }
			 |
			 IRET	{
				Instruction* instr=new Instruction;
				instr->type=Instruction::IRET;
				$$=instr;}
			 |
			 CALL jmp_operand[op] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::CALL;
				instr->operands.push_back($op);
				$$=instr;
			 }
			 |
			 RET {
				Instruction* instr=new Instruction;
				instr->type=Instruction::RET;
				$$=instr;
			 }
			 |
			 JMP jmp_operand[op] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::JMP;
				instr->operands.push_back($op);
				$$=instr;
			 }
			 |
			 BEQ GPR[gpr1] COMMA GPR[gpr2] COMMA jmp_operand[op3] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::BEQ;
				Operand op1;
				op1.type=Operand::GPR;
				op1.gpr=$gpr1;
				Operand op2;
				op2.type=Operand::GPR;
				op2.gpr=$gpr2;
				instr->operands.push_back(op1);
				instr->operands.push_back(op2);
				instr->operands.push_back($op3);
				
				$$=instr;
			 }
			 |
			 BNE GPR[gpr1] COMMA GPR[gpr2] COMMA jmp_operand[op3] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::BNE;
				Operand op1;
				op1.type=Operand::GPR;
				op1.gpr=$gpr1;
				Operand op2;
				op2.type=Operand::GPR;
				op2.gpr=$gpr2;
				instr->operands.push_back(op1);
				instr->operands.push_back(op2);
				instr->operands.push_back($op3);


				$$=instr;
				}
			 |
			 BGT GPR[gpr1] COMMA GPR[gpr2] COMMA jmp_operand[op3] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::BGT;
				
				Operand op1;
				op1.type=Operand::GPR;
				op1.gpr=$gpr1;
				Operand op2;
				op2.type=Operand::GPR;
				op2.gpr=$gpr2;
				instr->operands.push_back(op1);
				instr->operands.push_back(op2);
				instr->operands.push_back($op3);

				$$=instr;
			 }
			 |
			 PUSH GPR[gpr] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::PUSH;
				Operand op;
				op.type=Operand::GPR;
				op.gpr=$gpr;

				instr->operands.push_back(op);
				$$=instr;
			 }
			 |
			 POP GPR[gpr] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::POP;
				Operand op;
				op.type=Operand::GPR;
				op.gpr=$gpr;

				instr->operands.push_back(op);

				$$=instr;
			 }
			 |
			 NOT GPR[gpr] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::NOT;

				Operand op;
				op.type=Operand::GPR;
				op.gpr=$gpr;
				instr->operands.push_back(op);

				$$=instr;
			 }
			 |
			 OPERATION[o] GPR[gpr1] COMMA GPR[gpr2] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::getInstructionType($o);
				free($o);

				Operand op1;
				op1.type=Operand::GPR;
				op1.gpr=$gpr1;
				Operand op2;
				op2.type=Operand::GPR;
				op2.gpr=$gpr2;
				instr->operands.push_back(op1);
				instr->operands.push_back(op2);

				$$=instr;
			 }
			 |
			 LD data_operand[dop] COMMA GPR[gpr] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::LD;

				Operand op1;
				op1.type=Operand::DATA_OPERAND;
				op1.dataOperand=$dop;
				instr->operands.push_back(op1);
				Operand op2;
				op2.type=Operand::GPR;
				op2.gpr=$gpr;
				instr->operands.push_back(op2);

				$$=instr;
			 }
			 |
			 ST GPR[gpr] COMMA data_operand[dop] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::ST;
				
				Operand op1;
				op1.type=Operand::GPR;
				op1.gpr=$gpr;
				instr->operands.push_back(op1);
				Operand op2;
				op2.type=Operand::DATA_OPERAND;
				op2.dataOperand=$dop;
				instr->operands.push_back(op2);

				$$=instr;
			 }
			 |
			 CSRRD CSR[csr] COMMA GPR[gpr] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::CSRRD;
				
				Operand op1;
				op1.type=Operand::CSR;
				op1.csr=$csr;
				Operand op2;
				op2.type=Operand::GPR;
				op2.gpr=$gpr;
				instr->operands.push_back(op1);
				instr->operands.push_back(op2);

				$$=instr;
			 }
			 |
			 CSRWR GPR[gpr] COMMA CSR[csr] {
				Instruction* instr=new Instruction;
				instr->type=Instruction::CSRWR;

				Operand op1;
				op1.type=Operand::GPR;
				op1.gpr=$gpr;
				Operand op2;
				op2.type=Operand::CSR;
				op2.csr=$csr;
				instr->operands.push_back(op1);
				instr->operands.push_back(op2);

				$$=instr;
			 }

jmp_operand: NUM[num] {
				Operand op;
				op.type=Operand::LIT;
				op.literal= $num;
				$$=op;
			 }
			 |
			 IDENT[i] {
				Operand op;
				op.type=Operand::SYM;
				op.symbol= $i;
				$$=op;
			 }

data_operand: NUM[num] {
				DataOperand dop;
				dop.type=DataOperand::LIT;
				dop.literal = $num;
				$$ = dop;
			}
			 |
			 IDENT[i] {
				DataOperand dop;
				dop.type=DataOperand::SYM;
				dop.symbol = $i;
				$$ = dop;
			 }
			 |
			 IMMEDIATE NUM[num] {
				DataOperand dop;
				dop.type=DataOperand::IMM_LIT;
				dop.literal = $num;
				$$ = dop;
			 }
			 |
			 IMMEDIATE IDENT[i] {
				DataOperand dop;
				dop.type=DataOperand::IMM_SYM;
				dop.symbol = $i;
				$$ = dop;
			 }
			 |
			 GPR[gpr] {
				DataOperand dop;
				dop.type=DataOperand::GPR;
				dop.gpr = $gpr;
				$$ = dop;
			 } 
			 |
			 LBRACKET GPR[gpr] RBRACKET {
				DataOperand dop;
				dop.type=DataOperand::REL_GPR;
				dop.gpr = $gpr;
				$$ = dop;
			 }
			 |
			 LBRACKET GPR[gpr] PLUS NUM[num] RBRACKET {
				DataOperand dop;
				dop.type=DataOperand::REL_GPR_LIT;
				dop.gpr = $gpr;
				dop.literal= $num;
				$$ = dop;
			 }
			 |
			 LBRACKET GPR[gpr] PLUS IDENT[i] RBRACKET {
				DataOperand dop;
				dop.type=DataOperand::REL_GPR_SYM;
				dop.gpr = $gpr;
				dop.symbol= $i;
				$$ = dop;
			 }



symbol_list: 	symbol_list[list] COMMA IDENT[idnt] {
					$list->push_back($idnt);
					$$=$list;
				}
				|
				IDENT[idnt]	{
					SymList* list= new SymList();
					list->push_back($idnt);
					$$=list;
				}

symbol_or_literal_list: symbol_or_literal_list[list] COMMA NUM[num] {
							SymOrLit sl;
							sl.type= SymOrLit::LITERAL;
							sl.literal=$num;
							$list->push_back(sl);
							$$=$list;
						}
						|
						symbol_or_literal_list[list] COMMA IDENT[idnt] {
							SymOrLit sl;
							sl.type= SymOrLit::SYMBOL;
							sl.symbol=$idnt;
							$list->push_back(sl);
							$$=$list;
						}
						|
						NUM[num] {
							SymOrLitList* list= new SymOrLitList();
							SymOrLit sl;
							sl.type= SymOrLit::LITERAL;
							sl.literal=$num;
							list->push_back(sl);
							$$=list;
						}
						|
						IDENT[idnt] {
							SymOrLitList* list= new SymOrLitList();
							SymOrLit sl;
							sl.type= SymOrLit::SYMBOL;
							sl.symbol=$idnt;
							list->push_back(sl);
							$$=list;
						}

endls: endls ENDL | ENDL





%%
