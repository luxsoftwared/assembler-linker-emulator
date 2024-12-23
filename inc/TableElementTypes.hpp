#ifndef TABLE_ELEMENT_TYPES_HPP
#define TABLE_ELEMENT_TYPES_HPP

#include <cstdint>
//#include "parsingTypes.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>



using std::uint32_t;


enum class SymbolType{
	LOCAL, GLOBAL, EXTERN, SECTION
};


#define TABLE_FIELD_WIDTH 10


class Serializable{
public:
	virtual void serialize(std::ofstream& out) = 0;
	virtual void deserialize(std::ifstream& in) = 0;
};

class SymbolTableElem : public Serializable{
public:
	static uint32_t idCounter;
	uint32_t id;
	std::string* symbolName;
	uint32_t value;
	SymbolType type;
	union{
		std::string* sectionName;
		uint32_t size;
	};

	void printSymbolTableElem(std::ostream& out = std::cout){
		out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<id;

		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<*symbolName;
		out<<std::setw(1)<<"|";
		switch(type){
			case SymbolType::LOCAL:{
				out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"LOCAL";
				break;
			}
			case SymbolType::GLOBAL:{
				out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"GLOBAL";
				break;
			}
			case SymbolType::EXTERN:{
				out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"EXTERN";
				break;
			}
			case SymbolType::SECTION:{
				out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<"SECTION";
				break;
			}
			default:
				out<<"ERROR switch(type) in printSymbolTabelElem\n";
		}
		out<<std::setw(1)<<"|";
		out<<std::setw(TABLE_FIELD_WIDTH)<<std::left<<value<<"";
		out<<std::setw(1)<<"|";
		if(type==SymbolType::SECTION){
			out<<std::setw(18)<<std::left<<size<<"";
		}
		else{
			out<<std::setw(18)<<std::left<<(sectionName!=NULL?(*sectionName):"0");
		}
		out<<std::setw(1)<<"|";
		out<<"\n";
	}
	static void printSymbolTableHeader(std::ostream& out = std::cout){
		out<<std::setw(10)<<std::left<<"ID";
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<"Symbol name";
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<"Type";
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<"Value";
		out<<std::setw(1)<<"|";
		out<<std::setw(18)<<std::left<<"Size/Section name";
		out<<std::setw(1)<<"|";
		out<<"\n";
		out<<"-------------------------------------------------------------------\n";

	}

	void serialize(std::ofstream& out){
		out.write((char*)&id,sizeof(id));
		uint32_t len = symbolName->length();
		out.write((char*)&len,sizeof(len));
		out.write(symbolName->c_str(),len);
		out.write((char*)&value,sizeof(value));
		out.write((char*)&type,sizeof(type));
		if(type==SymbolType::SECTION){ // then field size is active, othervise section name
			out.write((char*)&size,sizeof(size));
		}else{
			if(sectionName==NULL){
				len = 0;
				out.write((char*)&len,sizeof(len));
			}else{
				len = sectionName->length();
				out.write((char*)&len,sizeof(len));
				out.write(sectionName->c_str(),len);
			}
		}
	}

	void deserialize(std::ifstream& in){
		in.read((char*)&id,sizeof(id));
		uint32_t len;
		in.read((char*)&len,sizeof(len));
		char* buff = new char[len];
		in.read(buff,len);
		symbolName = new std::string(buff,len);
		delete[] buff;
		in.read((char*)&value,sizeof(value));
		in.read((char*)&type,sizeof(type));
		if(type==SymbolType::SECTION){
			in.read((char*)&size,sizeof(size));
		}else{
			in.read((char*)&len,sizeof(len));
			if(len>0){ // section name is not null
				buff = new char[len];
				in.read(buff,len);
				sectionName = new std::string(buff,len);
				delete[] buff;
			}else{
				sectionName = NULL;
			}
			
		}
	}
};





class RelocTableElem : public Serializable {
public:
	enum RelocType{
		VALUE,
		JMP_OP, // in instr: pc rel to addres or from lit pool
		PCREL,
		DATA_OP, 
		INVALID
			};
	uint32_t offset;// LC, or where elem should be put (relastive to section start)
	std::string* sectionName; // section where elem should be put

	RelocType type; //?
	std::string* symbolName;

	RelocTableElem(uint32_t off, std::string* secName, RelocType t, std::string* symName){
		offset=off;
		sectionName=secName;
		type=t;
		symbolName=symName;
	};

	RelocTableElem(){
		offset=0;
		sectionName=NULL;
		type=RelocType::INVALID;
		symbolName=NULL;
	}

	static void printRelocTableHeader(std::ostream& out = std::cout){
		out<<std::setw(15)<<std::left<<"Symbol name";
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<"Offset";
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<"Type";
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<"Section name";
		out<<std::setw(1)<<"|";
		out<<"\n";
		out<<"-----------------------------------------------------\n";
	}

	void printRelocTableElem(std::ostream& out = std::cout){
		out<<std::setw(15)<<std::left<<*symbolName;
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<offset;
		out<<std::setw(1)<<"|";
		switch(type){
			case RelocType::VALUE:{
				out<<std::setw(10)<<std::left<<"VALUE";
				break;
			}
			case RelocType::JMP_OP:{
				out<<std::setw(10)<<std::left<<"JMP_OP";
				break;
			}
			case RelocType::DATA_OP:{
				out<<std::setw(10)<<std::left<<"DATA_OP";
				break;
			}
			default:
				out<<std::setw(10)<<std::left<<"Error in printRelocTableElem\n";
		}
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<*sectionName;
		out<<std::setw(1)<<"|";
		out<<"\n";
	}


	void serialize(std::ofstream& out){
		uint32_t len = symbolName->length();
		out.write((char*)&len,sizeof(len));
		out.write(symbolName->c_str(),len);
		out.write((char*)&offset,sizeof(offset));
		len = sectionName->length();
		out.write((char*)&len,sizeof(len));
		out.write(sectionName->c_str(),len);
		out.write((char*)&type,sizeof(type));
	}

	void deserialize(std::ifstream& in){
		uint32_t len;
		in.read((char*)&len,sizeof(len));
		char* buff = new char[len];
		in.read(buff,len);
		symbolName = new std::string(buff,len);
		delete[] buff;
		in.read((char*)&offset,sizeof(offset));
		in.read((char*)&len,sizeof(len));
		buff = new char[len];
		in.read(buff,len);
		sectionName = new std::string(buff,len);
		delete[] buff;
		in.read((char*)&type,sizeof(type));
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

	static void printLitPoolHeader(std::ostream& out = std::cout){
		out<<std::setw(10)<<std::left<<"Value";
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<"I Address";
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<"Resolved";
		out<<std::setw(1)<<"|";
		out<<std::setw(15)<<std::left<<"Reloc Sym name";
		out<<std::setw(1)<<"|";
		out<<"\n";
		out<<"-------------------------------------------------------------\n";
	}

	void printLitPoolElem(std::ostream& out = std::cout){
		out<<std::setw(10)<<std::left<<value;
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<addressOfInstruction;
		out<<std::setw(1)<<"|";
		out<<std::setw(10)<<std::left<<(resolved?"YES":"NOT");
		out<<std::setw(1)<<"|";
		if(resolved || reloc==NULL){
			out<<std::setw(15)<<std::left<<"";
		}else{
			out<<std::setw(15)<<std::left<<*(reloc->symbolName);
		}
		out<<std::setw(1)<<"|";
		out<<"\n";
	}
};

#endif // TABLE_ELEMENT_TYPES_HPP