#ifndef LINKER_HPP
#define LINKER_HPP

#include <string>
#include <vector>
#include "ObjFile.hpp" // for ObjectFile, for Section? TODO write alt section class

#define MAX_ADDRES 0xFFFFFFFF



class Linker {

public:
	//Linker();
	//~Linker();

	void addObjectFile(const std::string& filename);
	void addObjectFiles(const std::vector<std::string>& filenames);
	void link();
	void setOutputFilename(const std::string& filename);
	void addSectionPlacementReq(const std::string& sectionName, uint32_t address);
	void setHexExecution(bool isHex){ hexExecution = isHex; }
	bool getHexExecution(){ return hexExecution; }
	void print();
private:
	bool mergeSymbolTablesAndSections();
	bool placeSections();
	bool placeSectionAtAddr(const std::string& sectionName, uint32_t address);
	bool resolveRelocations();
	void generateOutputFiles();

private:
	bool hexExecution;
	std::string outputFilename;
	std::vector<std::string> objectFilenames;
	std::vector<ObjectFile> objectFiles;
	std::map<std::string, uint32_t> sectionPlacementRequests;
	std::map<std::string, SymbolTableElem> globalSymbolTable;
	std::map<std::string, LinkerSection> sections;
	std::map<uint32_t, uint8_t> initCode;
	uint32_t nextAvailableAddress = 0;


	
};

#endif // LINKER_HPP