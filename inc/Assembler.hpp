#ifndef __ASSEMBLER_HPP__
#define __ASSEMBLER_HPP__

#include <cstdint>
#include "parsingTypes.hpp"
#include <iomanip>
#include <vector>

using std::uint32_t;


enum class SymbolType{
	LOCAL, GLOBAL, EXTERN, SECTION
};


#define TABLE_FIELD_WIDTH 10
struct SymbolTableElem{
	static uint32_t idCounter;
	uint32_t id;
	std::string* symbolName;
	uint32_t value;
	SymbolType type;
	union{
		std::string* sectionName;
		uint32_t size;
	};

	void printSymbolTableElem(){
		std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<id;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<*symbolName;
		std::cout<<std::setw(1)<<"|";
		switch(type){
			case SymbolType::LOCAL:{
				std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"LOCAL";
				break;
			}
			case SymbolType::GLOBAL:{
				std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"GLOBAL";
				break;
			}
			case SymbolType::EXTERN:{
				std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"EXTERN";
				break;
			}
			case SymbolType::SECTION:{
				std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"SECTION";
				break;
			}
		}
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<value<<"";
		std::cout<<std::setw(1)<<"|";
		if(type==SymbolType::SECTION){
			std::cout<<std::setw(18)<<std::left<<size<<"";
		}
		else{
			std::cout<<std::setw(18)<<std::left<<(sectionName!=NULL?(*sectionName):"0");
		}
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
	}
	static void printSymbolTableHeader(){
		std::cout<<std::setw(10)<<std::left<<"ID";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<"Symbol name";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<"Type";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<"Value";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(18)<<std::left<<"Size/Section name";
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
		std::cout<<"-------------------------------------------------------------------\n";

	}
};

struct RelocTableElem{
	enum RelocType{
		VALUE,
		JMP_OP, // in instr: pc rel to addres or from lit pool
		PCREL,
		DATA_OP, 
		INVALID
			};
	uint32_t offset;// LC, or where elem should be put (relastive to section start)
	std::string* sectionName; // section where elem should be put

	RelocType type;
	std::string* symbolName;

	static void printRelocTableHeader(){
		std::cout<<std::setw(15)<<std::left<<"Symbol name";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<"Offset";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<"Type";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<"Section name";
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
		std::cout<<"-----------------------------------------------------\n";
	}

	void printRelocTableElem(){
		std::cout<<std::setw(15)<<std::left<<*symbolName;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<offset;
		std::cout<<std::setw(1)<<"|";
		switch(type){
			case RelocType::VALUE:{
				std::cout<<std::setw(10)<<std::left<<"VALUE";
				break;
			}
			case RelocType::JMP_OP:{
				std::cout<<std::setw(10)<<std::left<<"JMP_OP";
				break;
			}
			case RelocType::DATA_OP:{
				std::cout<<std::setw(10)<<std::left<<"DATA_OP";
				break;
			}
		}
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<*sectionName;
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
	}
};

struct LitPoolElem{ // addressed pc rel from instruction
	int32_t value;
	uint32_t addressOfInstruction;
	bool resolved; // if false, reloc
	RelocTableElem* reloc;// 

	LitPoolElem(int32_t val,uint32_t adr, bool res=true, RelocTableElem* rel=NULL){
		value=val;
		addressOfInstruction=adr;
		resolved=res;
		reloc=rel;
	}

	static void printLitPoolHeader(){
		std::cout<<std::setw(10)<<std::left<<"Value";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<"I Address";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<"Resolved";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<"Reloc Sym name";
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
		std::cout<<"-------------------------------------------------------------\n";
	}

	void printLitPoolElem(){
		std::cout<<std::setw(10)<<std::left<<value;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<addressOfInstruction;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(10)<<std::left<<(resolved?"YES":"NOT");
		std::cout<<std::setw(1)<<"|";
		if(resolved || reloc==NULL){
			std::cout<<std::setw(15)<<std::left<<"";
		}else{
			std::cout<<std::setw(15)<<std::left<<*(reloc->symbolName);
		}
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
	}
};

struct UnprocessedInstruction{
	Instruction* instruction;
	//Section* section;
	uint32_t address; // in section
};

struct Section{
	uint32_t id;
	std::string* sectionName;
	uint32_t size;
	uint32_t startAddress;
	uint32_t endAddress;
	uint32_t locationCounter;
	std::vector<uint8_t> code;
	std::vector<RelocTableElem> relocationTable;
	std::map<std::string, SymbolTableElem> symbolTable;
	//std::map<std::string, uint32_t> symTabNameToId;
	std::vector<LitPoolElem> litPool;
	std::vector<UnprocessedInstruction> unprocessedInstructions;

	/*void addSymbol(SymbolTableElem sym){
		symbolTable.push_back(sym);
	}*/

	void printSection(){
		std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<id;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<*sectionName;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<size;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<startAddress;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(13)<<std::left<<endAddress;
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(18)<<std::left<<locationCounter;
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
	}
	static void printSectionHeader(){
		std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"ID";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<"Section name";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"Size";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<"Start address";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(13)<<std::left<<"End address";
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(18)<<std::left<<"Location counter";
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n--------------------------------------------------------------------------------------\n";
	}

};



struct LitPool{
	std::vector<LitPoolElem> pool;
	uint32_t startAddress = 0;
};





class Assembler{
public:
	Assembler();
	//~Assembler();
	void assemble();
	void processLine(Line* line);
	void addLabel(uint32_t address, std::string* label);
	void declareGlobalSymbol(std::string* symbol);
	void declareExternSymbol(std::string* symbol);
	void processDirective(Directive* dir);
	void processInstruction(Instruction* instr);
	void printSymbolTable();
	void printRelocTable();
	void printCode();
	void printSections();
	void printLitPools();
	void printDebug();
private:
	uint32_t LC;
	uint32_t inputFileLineNum;
	std::map<std::string, SymbolTableElem> symbolTable;
	//std::map<std::string, uint32_t> symTabNameToId;
	//std::vector<RelocTableElem> relocationTable;
	std::map<std::string, Section> sections;
	Section* currentSection;
	//std::vector<uint8_t> code;

	void push32bitsToCode(uint32_t word);

	//void processJMPoperand(Operand op);
	uint32_t processJumpInstructions(Operand op); //returns instruction
	void addToLitPool(SymOrLit sol, Section* sec, uint32_t addressOfInstr);
	void insertLitPool(); // insert lit pool to code
	void endSection();
	void endFile();

	void resolveSymbol(RelocTableElem& el);
	void edit32bitsOfCode(Section& s,uint32_t adress, uint32_t word);
	void addDispToInstruction(Section& s, uint32_t address, int32_t disp);
	void postProccessInstructions();
	uint32_t processDataOperand(DataOperand op, Section* sec, uint32_t address);
};


enum class InstructionCode{
	HALT=0x00,
	INT=0x10,
	IRET=0x97,
	CALL=0x20, // pc+D
	CALL_M=0x21, // lit pool m[pc+D]
	RET, //* nema kod
	JMP=0x30,
	JMP_M=0x38,
	BEQ=0x31,
	BEQ_M=0x39,
	BNE=0x32,
	BNE_M=0x3A,
	BGT=0x33,
	BGT_M=0x3B,
	PUSH=0x81,
	POP=0x93,
	XCHG=0x40,
	ADD=0x50,
	SUB=0x51,
	MUL=0x52,
	DIV=0x53,
	NOT=0x60,
	AND=0x61,
	OR=0x62,
	XOR=0x63,
	SHL=0x70,
	SHR=0x71,
	LD=0x91,
	LD_M=0x92,
	ST=0x80,
	ST_M=0x82,
	CSRRD=0x90,
	CSRWR=0x94, INVALID,
	DIRECT_ADDRESSING=0x00,
	INDIRECT_ADDRESSING=0x01,
};



#endif // __ASSEMBLER_HPP__