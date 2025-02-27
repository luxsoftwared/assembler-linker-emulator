#ifndef OBJFILE_HPP
#define OBJFILE_HPP

#include <string>
#include <vector>
#include "Assembler.hpp"


class LinkerSection : Serializable{
public:
	LinkerSection(Section s){
		sectionName = new std::string(*s.sectionName);
		objFileNum=-1;
		code = s.code;
		relocationTable = s.relocationTable;
	}
	LinkerSection(){};
	
	~LinkerSection(){
		//delete sectionName;
	}
	int objFileNum ; 
	std::string* sectionName;
	std::vector<uint8_t> code;
	std::vector<RelocTableElem> relocationTable;
	bool placed; // if section is placed into output code

	virtual void serialize(std::ofstream& out);
	virtual void deserialize(std::ifstream& in);

};

class ObjectFile : public Serializable{
	//symbol table
	//sections(wth code and relocation tables)
public:
	std::map<std::string, SymbolTableElem> symbolTable;
	std::map<std::string, LinkerSection> sections;
public:
	ObjectFile(Assembler& as){
		symbolTable = as.getSymbolTable();
		for(auto& s : as.getSections()) sections[s.first]=LinkerSection(s.second); 
	}
	ObjectFile(){}
	ObjectFile(std::ifstream& in){
		deserialize(in);
	}
	//~ObjectFile();
	virtual void serialize(std::ofstream& out);
	virtual void deserialize(std::ifstream& in);
	std::map<std::string, SymbolTableElem> getSymbolTable() const { return symbolTable; }
	void print(){
		std::cout<<"Symbol table:\n";
		SymbolTableElem::printSymbolTableHeader();
		for(auto& el : symbolTable){
			el.second.printSymbolTableElem();
		}
		std::cout<<"Sections:\n";
		for(auto& el : sections){
			std::cout<<"Section: "<<*(el.second.sectionName)<<"\n";
			std::cout<<"Code:\n";
			for(auto& byte : el.second.code){
				std::cout<<std::hex<<(int)byte<<" ";
			}
			std::cout<<"\n";
			std::cout<<"Relocation table:\n";
			for(auto& rel : el.second.relocationTable){
				rel.printRelocTableElem();
			}
		}
	}

};


void ObjectFile::serialize(std::ofstream &out ){
	// serialize symbol table
	uint32_t size = symbolTable.size();
	out.write((char*)&size, sizeof(uint32_t));
	for(auto el : symbolTable){
		el.second.serialize(out);
	}

	// serialize sections & their relocation tables
	size = sections.size();
	out.write((char*)&size, sizeof(uint32_t));
	for(auto section : sections){
		section.second.serialize(out);
	}



}

void ObjectFile::deserialize(std::ifstream &in){
	// deserialize symbol table
	uint32_t size;
	in.read((char*)&size, sizeof(uint32_t));
	for(uint32_t i=0; i<size; i++){
		SymbolTableElem el;
		el.deserialize(in);
		symbolTable[*(el.symbolName)]=el;
	}

	// deserialize sections & their relocation tables
	in.read((char*)&size, sizeof(uint32_t));
	for(uint32_t i=0; i<size; i++){
		LinkerSection sect;
		sect.deserialize(in);
		sections[*(sect.sectionName)]=sect;
	}

	// thats the whole file; close it
	in.close();
	
}


void LinkerSection::serialize(std::ofstream &out)
{
	std::cout<<"LinkerSection serialziation";
	// section name
	uint32_t len = sectionName->length();
	out.write((char*)&len,sizeof(len));
	out.write(sectionName->c_str(),len);
	//size
	//out.write((char*)&size,sizeof(size));
	// reloc table
	len = relocationTable.size();
	out.write((char*)&len,sizeof(len));
	for(auto& el : relocationTable){
		el.serialize(out);
	}
	// code
	len = code.size();
	out.write((char*)&len,sizeof(len));
	out.write((char*)&code[0],len);
	
}

void LinkerSection::deserialize(std::ifstream &in)
{
	// section name
	uint32_t len;
	in.read((char*)&len,sizeof(len));
	char* name = new char[len];
	in.read(name,len);
	sectionName = new std::string(name);
	delete[] name;
	//size
	//in.read((char*)&size,sizeof(size));
	// reloc table
	in.read((char*)&len,sizeof(len));
	for(uint32_t i=0; i<len; i++){
		RelocTableElem el;
		el.deserialize(in);
		relocationTable.push_back(el);
	}
	// code
	in.read((char*)&len,sizeof(len));
	code.resize(len);
	in.read((char*)&code[0],len);
}





#endif // OBJFILE_HPP