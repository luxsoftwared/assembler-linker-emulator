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
		ABSOLUTE, PCRELATIVE
	};
	uint32_t offset;//LC
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
	//std::vector<uint32_t> relocationTable;
	std::vector<SymbolTableElem> symbolTable;

	void addSymbol(SymbolTableElem sym){
		symbolTable.push_back(sym);
	}

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
	std::vector<SymbolTableElem> symbolTable;
	std::map<std::string, uint32_t> symTabNameToId;
	std::vector<RelocTableElem> relocationTable;
	std::map<std::string, Section> sections;
	Section* currentSection;
	std::vector<uint8_t> code;

	void push32bitsToCode(uint32_t word);

};


enum class InstructionCode{
	HALT=0x00,
	INT=0x10,
	IRET, CALL, RET, JMP, BEQ, BNE, BGT, PUSH, POP, XCHG,
	ADD, SUB, MUL, DIV, NOT, AND, OR, XOR, SHL, SHR, LD, ST,CSRRD,CSRWR, INVALID
}



#endif // __ASSEMBLER_HPP__