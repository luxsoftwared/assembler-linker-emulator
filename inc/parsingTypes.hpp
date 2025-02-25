
#ifndef __PARSINGTYPES_HPP__
#define __PARSINGTYPES_HPP__


#include <cstdint>
#include <iostream>

#include <string>
#include <vector>
#include <map>

using std::uint32_t;

// Enumerations for the different types of instructions
enum class InstructionType {
	R, I, J, INVALID
};

// Enumerations for the different types of registers
enum class GPRType {
	GPR0=0, GPR1, GPR2, GPR3, GPR4, GPR5, GPR6, GPR7, GPR8, GPR9, GPR10, GPR11, GPR12, GPR13, SP, PC, INVALID
};

enum class CSRType {
	STATUS=0, HANDLER=1, CAUSE=2, INVALID
};


struct SymOrLit{
	enum Type{
		SYMBOL, LITERAL
	};
	Type type;
	union {
		std::string* symbol;
		int literal;
	};

	void print(){
		if(type==SYMBOL){
			std::cout<<"Symbol: "<<*symbol<<"\n";
		}else{
			std::cout<<"Literal: "<<literal<<"\n";
		}
	}
};

struct DataOperand{
	enum DataOperandType{
		LIT,// $lit 
		SYM , // $sym
		IMM_LIT, // lit 
		IMM_SYM,// sym
		GPR, REL_GPR, REL_GPR_LIT, REL_GPR_SYM 
	};
	DataOperandType type;
	GPRType gpr;
	union{
		std::string* symbol;
		int literal;
	};
};


struct Operand{
	enum OperandType{
		GPR, CSR, SYM, LIT, DATA_OPERAND, INVALID
	};
	OperandType type;
	union{
		GPRType gpr;
		CSRType csr;
		std::string* symbol;
		int literal;
		DataOperand dataOperand;
	};
};

using SymOrLitList = std::vector<SymOrLit>;
using SymList = std::vector<std::string*>;
using InstructionOperands = std::vector<Operand>;

struct Instruction{
	enum InstructionType {
	HALT, INT, IRET, CALL, RET, JMP, BEQ, BNE, BGT, PUSH, POP, XCHG,
	ADD, SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR, LD, ST,CSRRD,CSRWR, INVALID
};
	InstructionType type;
	InstructionOperands operands;
	static InstructionType getInstructionType(std::string* instruction){
	std::map<std::string, Instruction::InstructionType> instructionMap = {
		{"halt", Instruction::InstructionType::HALT},
		{"int", Instruction::InstructionType::INT},
		{"iret", Instruction::InstructionType::IRET},
		{"call", Instruction::InstructionType::CALL},
		{"ret", Instruction::InstructionType::RET},
		{"jmp", Instruction::InstructionType::JMP},
		{"beq", Instruction::InstructionType::BEQ},
		{"bne", Instruction::InstructionType::BNE},
		{"bgt", Instruction::InstructionType::BGT},
		{"push", Instruction::InstructionType::PUSH},
		{"pop", Instruction::InstructionType::POP},
		{"xchg", Instruction::InstructionType::XCHG},
		{"add", Instruction::InstructionType::ADD},
		{"sub", Instruction::InstructionType::SUB},
		{"mul", Instruction::InstructionType::MUL},
		{"div", Instruction::InstructionType::DIV},
		{"not", Instruction::InstructionType::NOT},
		{"and", Instruction::InstructionType::AND},
		{"or", Instruction::InstructionType::OR},
		{"xor", Instruction::InstructionType::XOR},
		{"shl", Instruction::InstructionType::SHL},
		{"shr", Instruction::InstructionType::SHR},
		{"ld", Instruction::InstructionType::LD},
		{"st", Instruction::InstructionType::ST},
		{"csrrd", Instruction::InstructionType::CSRRD},
		{"csrwr", Instruction::InstructionType::CSRWR}
	};
	auto it = instructionMap.find(*instruction);
	if(it == instructionMap.end()){
		return Instruction::InstructionType::INVALID;
	}
	return it->second;
}
};





struct Directive{
	enum DirectiveType{
		GLOBAL, EXTERN, SECTION, WORD, SKIP, END, INVALID
	};
	DirectiveType type;
	union{
		SymOrLitList* symOrLitList;
		SymList* symList;
		std::string* symbol;
		int literal;
	};
	
};

struct Line{
	enum LineType{
		INSTRUCTION, DIRECTIVE, INVALID
	};
	LineType type;
	uint32_t lineNumber;
	union{
		Instruction* instruction;
		Directive* directive;
	};
	SymList labels;

	static void printLine(Line* l){
		for(std::string* sym: l->labels){
			std::cout<<"Label:"<<*sym<<" ";
		}
		std::cout<< "Line type: ";
		
		if(l->type==Line::DIRECTIVE){
			std::cout<<"Directive ";
			switch(l->directive->type){
				case Directive::GLOBAL:{
					std::cout<<"global ";
					for(std::string* sym : *(l->directive->symList) ){
						std::cout<< *sym<<" ";
					}
					std::cout<<"\n";
					break;
				}
				case Directive::EXTERN:{
					std::cout<<"extern ";
					for(std::string* sym : *(l->directive->symList) ){
						std::cout<< *sym<<" ";
					}
					std::cout<<"\n";
					break;
				}
				case Directive::SECTION:{
					std::cout<<"section ";
					std::cout<< *(l->directive->symbol)<<"\n";
					break;
				}
				case Directive::WORD:{
					std::cout<<"word ";
					for(SymOrLit sol : *(l->directive->symOrLitList)){
						if(sol.type==SymOrLit::SYMBOL){
							std::cout<<*(sol.symbol)<<" ";
						}else{
							std::cout<<sol.literal<<" ";
						}
					}
					std::cout<<"\n";
					break;
				}
				case Directive::SKIP:{
					std::cout<<"skip ";
					std::cout<<l->directive->literal<<"\n";
					break;
				}
				case Directive::END:{
					std::cout<<"End\n";
					break;
				}
				default:
					std::cout<<"Error in directive type\n";
			}
		}else if (l->type==Line::INSTRUCTION){
			std::cout<<"Instruction ";
			switch(l->instruction->type){
				case Instruction::HALT:{
					std::cout<<"halt ";
					break;
				}
				case Instruction::INT:{
					std::cout<<"int ";
					break;
				}
				case Instruction::IRET:{
					std::cout<<"iret ";
					break;
				}
				case Instruction::CALL:{
					std::cout<<"call ";
					break;
				}
				case Instruction::RET:{
					std::cout<<"ret ";
					break;
				}
				
				case Instruction::JMP:{
					std::cout<<"jmp ";
					break;
				}
				case Instruction::BEQ:{
					std::cout<<"beq ";
					break;
				}
				case Instruction::BNE:{
					std::cout<<"bne ";
					break;
				}
				case Instruction::BGT:{
					std::cout<<"bgt ";
					break;
				}
				case Instruction::PUSH:{
					std::cout<<"push ";
					break;
				}
				case Instruction::POP:{
					std::cout<<"pop ";
					break;
				}
				case Instruction::XCHG:{
					std::cout<<"xchg ";
					break;
				}
				case Instruction::ADD:{
					std::cout<<"add ";
					break;
				}
				case Instruction::SUB:{
					std::cout<<"sub ";
					break;
				}
				case Instruction::MUL:{
					std::cout<<"mul ";
					break;
				}
				case Instruction::DIV:{
					std::cout<<"div ";
					break;
				}
				case Instruction::NOT:{
					std::cout<<"not ";
					break;
				}
				case Instruction::AND:{
					std::cout<<"and ";
					break;
				}
				case Instruction::OR:{
					std::cout<<"or ";
					break;
				}
				case Instruction::XOR:{
					std::cout<<"xor ";
					break;
				}
				case Instruction::SHL:{
					std::cout<<"shl ";
					break;
				}
				case Instruction::SHR:{
					std::cout<<"shr ";
					break;
				}
				case Instruction::LD:{
					std::cout<<"ld ";
					break;
				}
				case Instruction::ST:{
					std::cout<<"st ";
					break;
				}
				case Instruction::CSRRD:{
					std::cout<<"csrrd ";
					break;
				}
				case Instruction::CSRWR:{
					std::cout<<"csrwr ";
					break;
				}
				default:
					std::cout<<"Error in instr type\n";
			}
			for(Operand op : l->instruction->operands){
				switch(op.type){
					case Operand::GPR:{
						std::cout<<"GPR "<<(int)op.gpr<<" ";
						break;
					}
					case Operand::CSR:{
						std::cout<<"CSR "<<(int)op.csr<<" ";
						break;
					}
					case Operand::SYM:{
						std::cout<<"symbol "<< *(op.symbol) <<" ";
						break;
					}
					case Operand::LIT:{
						std::cout<<"literal "<<op.literal<<" ";
						break;
					}
					case Operand::DATA_OPERAND:
						break;
					default:
						break;
				}
			}
			std::cout<<"\n";
		}
	}

};






#endif // __PARSINGTYPES_HPP__