#ifndef __ASSEMBLER_HPP__
#define __ASSEMBLER_HPP__

#include "TableElementTypes.hpp"
#include "parsingTypes.hpp"
#include <iomanip>
#include <vector>



struct UnprocessedInstruction{
	Instruction* instruction;
	//Section* section;
	uint32_t address; // in section
};

class Section : public Serializable{
public:
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
	std::vector<std::vector<LitPoolElem>> oldLitPools;
	// number of times lc for section passed 2000
	uint32_t litPoolThresholdsReached; 
	std::vector<UnprocessedInstruction> unprocessedInstructions;

	/*void addSymbol(SymbolTableElem sym){
		symbolTable.push_back(sym);
	}*/

	virtual void serialize(std::ofstream& out);
	virtual void deserialize(std::ifstream& in);

	void printSection(std::ostream& out = std::cout){
		
		out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<id;
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<*sectionName;
		out<<std::setw(1)<<"|";
		out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<size;
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<startAddress;
		out<<std::setw(1)<<"|";
		out<<std::setw(13)<<std::left<<endAddress;
		out<<std::setw(1)<<"|";
		out<<std::setw(18)<<std::left<<locationCounter;
		out<<std::setw(1)<<"|";
		out<<"\n";
	}
	static void printSectionHeader(std::ostream& out = std::cout){
		out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"ID";
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<"Section name";
		out<<std::setw(1)<<"|";
		out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"Size";
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<"Start address";
		out<<std::setw(1)<<"|";
		out<<std::setw(13)<<std::left<<"End address";
		out<<std::setw(1)<<"|";
		out<<std::setw(18)<<std::left<<"Location counter";
		out<<std::setw(1)<<"|";
		out<<"\n--------------------------------------------------------------------------------------\n";
	}

};






class Assembler {
public:
	Assembler();
	~Assembler();
	void assemble();
	void processLine(Line* line);
private:
	void addLabel(uint32_t address, std::string* label);
	void declareGlobalSymbol(std::string* symbol);
	void declareExternSymbol(std::string* symbol);
	void processDirective(Directive* dir);
	void processInstruction(Instruction* instr);
public:
	std::map<std::string, SymbolTableElem> getSymbolTable(){
		return symbolTable;
	};
	std::map<std::string, Section> getSections(){
		return sections;
	};
	void printSymbolTable();
	void printSymbolTable(std::ostream &out);
	void printRelocTable();
	void printCode();
	void printSections();
	void printLitPools();
	void printDebug();
	void setOutputFiles(std::string outputFilename);
	void closeOutputFile(std::ofstream& out){
		out.close();
	}

	
	std::ofstream& getOutTxt(){
		return outTxt;
	}
	std::ofstream& getOutBin(){
		return outBin;
	}
private:
	uint32_t LC;
	uint32_t inputFileLineNum;
	std::map<std::string, SymbolTableElem> symbolTable;
	//std::map<std::string, uint32_t> symTabNameToId;
	//std::vector<RelocTableElem> relocationTable;
	std::map<std::string, Section> sections;
	Section* currentSection;
	std::ofstream outTxt;
	std::ofstream outBin;
	bool setOutput = false;

	void push32bitsToCode(uint32_t word);

	//void processJMPoperand(Operand op);
	uint32_t processJumpInstructions(Operand op, Section* s,uint32_t addresOfInstruction); //returns instruction
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


class ObjectFile : public Serializable{
	//symbol table
	//sections(wth code and relocation tables)
private:
	std::map<std::string, SymbolTableElem> symbolTable;
	std::map<std::string, Section> sections;
public:
	ObjectFile(Assembler& as){
		symbolTable = as.getSymbolTable();
		sections = as.getSections();
	}
	ObjectFile(){}
	ObjectFile(std::ifstream& in){
		deserialize(in);
	}
	//~ObjectFile();
	virtual void serialize(std::ofstream& out);
	virtual void deserialize(std::ifstream& in);
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