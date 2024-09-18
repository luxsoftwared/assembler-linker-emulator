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
		JMP, // in instr: pc rel to addres or from lit pool
		DATA_OP // in inst: pc rel to data operand
			};
	uint32_t offset;// LC, or where elem should be put (relastive to section start)
	
	std::string* sectionName;
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
		std::cout<<std::setw(10)<<std::left<<(type==RelocType::ABSOLUTE?"ABSOLUTE":"PCRELATIVE");
		std::cout<<std::setw(1)<<"|";
		std::cout<<std::setw(15)<<std::left<<*sectionName;
		std::cout<<std::setw(1)<<"|";
		std::cout<<"\n";
	}
};


struct Section{
	uint32_t id;
	std::string* sectionName;
	uint32_t size;
	uint32_t startAddress;
	uint32_t endAddress;
	uint32_t locationCounter;
	std::vector<uint8_t> code;
	std::vector<uint32_t> relocationTable;
	std::map<std::string, SymbolTableElem> symbolTable;
	//std::map<std::string, uint32_t> symTabNameToId;
	std::vector<LitPoolElem> litPool;

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

struct LitPoolElem{ // addressed pc rel from instruction
	uint32_t value;
	uint32_t addressOfInstruction;
	//bool resolved=true; // if false, when elem is placed in code, add it to relocation table
	// is type always absolute?
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
	void printDebug();
private:
	uint32_t LC;
	uint32_t inputFileLineNum;
	std::map<std::string, SymbolTableElem> symbolTable;
	//std::map<std::string, uint32_t> symTabNameToId;
	std::vector<RelocTableElem> relocationTable;
	std::map<std::string, Section> sections;
	Section* currentSection;
	//std::vector<uint8_t> code;

	void push32bitsToCode(uint32_t word);

	void processJMPoperand(Operand op);
	void processCallInstruction(Instruction* instr);
	void endSection();
	void endFile();

};


enum class InstructionCode{
	HALT=0x00,
	INT=0x10,
	IRET=0x97,
	CALL_L=0x21, //* lit
	RET, //*
	JMP, BEQ, BNE, BGT,
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
	LD, ST,CSRRD,CSRWR, INVALID
}



#endif // __ASSEMBLER_HPP__