#ifndef LINKER_HPP
#define LINKER_HPP

#include <string>
#include <vector>
#include "Assembler.hpp" // for ObjectFile, for Section? TODO write alt section class




class Linker {
public:
	Linker();
	~Linker();

	void addObjectFile(const std::string& filename);
	void addObjectFiles(const std::vector<std::string>& filenames);
	void link();
	void setOutputFilename(const std::string& filename);
	void placeSectionAtAddress(const std::string& sectionName, uint32_t address);
	void setHexExecution(bool isHex){ hexExecution = isHex; }
private:
	bool mergeSymbolTablesAndSections();
	bool placeSections();

private:
	bool hexExecution;
	std::string outputFilename;
	std::vector<std::string> objectFilenames;
	std::vector<ObjectFile> objectFiles;
	std::map<std::string, uint32_t> sectionPlacementRequests;
	std::map<std::string, SymbolTableElem> globalSymbolTable;
	std::map<std::string, Section> sections;

	
};

#endif // LINKER_HPP