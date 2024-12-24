#include "Linker.hpp"
#include <fstream>

void Linker::addObjectFile(const std::string &filename)
{
	objectFilenames.push_back(filename);
}

void Linker::addObjectFiles(const std::vector<std::string> &filenames)
{
	for (const auto &filename : filenames)
	{
		objectFilenames.push_back(filename);
	}
}

void Linker::link()
{
	for(const auto &filename : objectFilenames)
	{
		std::ifstream inBin("./build/snazzleOut.bin", std::ios::in | std::ios::binary);
		ObjectFile obj(inBin);
		objectFiles.push_back(obj);
	}

	// merge files
	if(!mergeSymbolTablesAndSections()){
		std::cerr<<"ERROR: linking failed\n";
		return;
	}

	// place sections





}

void Linker::setOutputFilename(const std::string &filename)
{
	outputFilename = filename;
}

void Linker::placeSectionAtAddress(const std::string &sectionName, uint32_t address)
{
	sectionPlacementRequests[sectionName] = address;
}

bool Linker::mergeSymbolTablesAndSections()
{
	for( auto &obj : objectFiles)
	{
		for( auto &sym : obj.getSymbolTable() )
		{
			if(globalSymbolTable.find(sym.first) != globalSymbolTable.end())
			{
				// symbol already exists
				switch(globalSymbolTable[sym.first].type) {
					case SymbolType::SECTION:{
						if(sym.second.type != SymbolType::SECTION){
							std::cerr<<"ERROR: multiple symbols with same name\n";
							return false;
						}

						// merge sections
						
						// increase offset for reloc position by current size of the section
						for(auto &relocEntry: obj.sections[sym.first].relocationTable){
							relocEntry.offset += globalSymbolTable[sym.first].value; 
						}
						// merge relocation table
						sections[sym.first].relocationTable.insert(sections[sym.first].relocationTable.end(), obj.sections[sym.first].relocationTable.begin(), obj.sections[sym.first].relocationTable.end());

						// merge code
						sections[sym.first].code.insert(sections[sym.first].code.end(), obj.sections[sym.first].code.begin(), obj.sections[sym.first].code.end());

						// update size
						globalSymbolTable[sym.first].size += sym.second.size;

						break;
					}
					case SymbolType::GLOBAL:{
						if(sym.second.type != SymbolType::EXTERN){
							std::cerr<<"ERROR: multiple symbols with same name\n";
							return false;
						}
						// 
						break;
					}
					case SymbolType::EXTERN:{
						if(sym.second.type != SymbolType::GLOBAL){
							std::cerr<<"ERROR: multiple symbols with same name\n";
							return false;
						}
						// turn him to GLOBAL and set his value and section
						globalSymbolTable[sym.first].type = SymbolType::GLOBAL;
						globalSymbolTable[sym.first].value = sym.second.value;
						globalSymbolTable[sym.first].sectionName = sym.second.sectionName;

						break;
					}
					default:
						std::cerr<<"ERROR switch(globalSymbolTable[sym.first].type) in mergeSymbolTables\n";
				}
				
				
			}else{
				// symbol does not exist
				globalSymbolTable[sym.first] = sym.second;
				if(sym.second.type == SymbolType::SECTION){
					// add section to sections
					sections[sym.first] = obj.sections[sym.first];
				}
			}
			
		}
	}
	// one symbol table, and all sections merged
	return true;
}

bool Linker::placeSections()
{
	// check requests
	

}

int main(int argc, char *argv[])
{
	Linker linker;
	linker.setOutputFilename("linkingOutput.bin");
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] == 'o')
			{
				if(i+1 >= argc)
				{
					std::cerr<<"Output filename not provided\n";
					return 1;
				}
				linker.setOutputFilename(argv[++i]);

			}else if(argv[i] == "-hex")
			{
				linker.setHexExecution(true);
			}else if((std::string(argv[i])).substr(1,5) == "place" ){
				// place section at address
				std::string argument = std::string(argv[i]).substr(7); // skip -place=
				int pos = argument.find("@");
				std::string sectionName = argument.substr(0, pos);
				uint32_t address = std::stoul(argument.substr(pos+1));
				linker.placeSectionAtAddress(sectionName, address);

			}else{
				std::cerr<<"Unknown option: "<<argv[i]<<"\n";
				return 1;
			}
		}
		else
		{
			linker.addObjectFile(argv[i]);
		}
	}

	linker.link();
	return 0;
}
