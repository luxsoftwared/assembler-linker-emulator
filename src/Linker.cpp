#include "../inc/Linker.hpp"
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
	std::cout<<"Linking...\n";
	
	for(const auto &filename : objectFilenames)
	{
		std::ifstream inBin(filename, std::ios::in | std::ios::binary);
		ObjectFile obj(inBin);
		objectFiles.push_back(obj);
	}
	print();

	// merge files
	if(!mergeSymbolTablesAndSections()){
		std::cerr<<"ERROR: linking failed\n";
		return;
	}

	// place sections
	
	if(!placeSections()){
		std::cerr<<"ERROR: linking failed\n";
		return;
	}

	// resolve relocations
	if(!resolveRelocations()){
		std::cerr<<"ERROR: linking failed\n";
		return;
	}

	// write to file?
	generateOutputFiles();

}

void Linker::setOutputFilename(const std::string &filename)
{
	outputFilename = filename;
}

void Linker::addSectionPlacementReq(const std::string &sectionName, uint32_t address)
{
	sectionPlacementRequests[sectionName] = address;
}

void Linker::generateOutputFiles()
{
	std::cout<<"Generating output files...\n";
	std::ofstream outTxt((outputFilename+".txt").c_str(), std::ios::out);
	for(auto& code: initCode){
		// TODO set format to print 8 bytes?
		outTxt<<std::hex<<std::setw(8)<<code.first<<":"<<std::setw(2)<<code.second;
	}
	outTxt.close();

}
void Linker::print()
{
	std::cout<<"Printing linker state...\n";
	std::cout<<"Object files:\n";
	for(auto &obj : objectFiles)
	{
		obj.print();
	}
	std::cout<<"Global symbol table:\n";
	for(auto &sym : globalSymbolTable)
	{
		sym.second.printSymbolTableElem();
	}
	std::cout<<"Sections:\n";
	for(auto &sect : sections)
	{
		std::cout<<sect.first<<" : "<<sect.second.placed<<"\n";
	}
	std::cout<<"Init code:\n";
	for(auto &code : initCode)
	{
		std::cout<<code.first<<" : "<<code.second<<"\n";
	}
	std::cout<<"Next available address: "<<nextAvailableAddress<<"\n";
	std::cout<<"Section placement requests:\n";
	for(auto &req : sectionPlacementRequests)
	{
		std::cout<<req.first<<" : "<<req.second<<"\n";
	}
	std::cout<<"Output filename: "<<outputFilename<<"\n";
	std::cout<<"Hex execution: "<<hexExecution<<"\n";
	std::cout<<"End of linker state\n";
}

bool Linker::mergeSymbolTablesAndSections() 
{
	std::cout<<"Merging symbol tables and sections...\n";
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
							std::cerr<<"ERROR: multiple symbols with same name(section name overlaps with var name): "+ sym.first+"\n";
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
							std::cerr<<"ERROR: multiple global symbols with same name: "+ sym.first+"\n";
							return false;
						}
						// 
						break;
					}
					case SymbolType::EXTERN:{
						if(sym.second.type == SymbolType::SECTION){
							std::cerr<<"ERROR: multiple symbols with same name(section name overlaps with var name): "+ sym.first+"\n";
							return false;
						}else if(sym.second.type == SymbolType::GLOBAL ){
							// turn him to GLOBAL and set his value and section
							globalSymbolTable[sym.first].type = SymbolType::GLOBAL;
							globalSymbolTable[sym.first].value = sym.second.value;
							globalSymbolTable[sym.first].sectionName = sym.second.sectionName;
						}
						// if both are extern nothing happends

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
	std::cout<<"Placing sections...\n";
	// check requests
	for(auto &sectPlaceReq : sectionPlacementRequests)
	{
		if(sections.find(sectPlaceReq.first) == sections.end())
		{
			std::cerr<<"ERROR: placement failed - section "<<sectPlaceReq.first<<" not found\n";
			return false;
		}else{
			uint32_t sectionSize = globalSymbolTable[sectPlaceReq.first].size;
			if(initCode.find(sectPlaceReq.second) == initCode.end())
			{

				initCode[sectPlaceReq.second] = 0;
				auto iter = ++(initCode.find(sectPlaceReq.second));
				if(iter==initCode.end() || iter->first >= sectPlaceReq.second+sectionSize )
				{
					// there is no overlap, section can be placed
					if(!placeSectionAtAddr(sectPlaceReq.first, sectPlaceReq.second) ) return false;

				}else{
					// overlap, regular placement
					if(!placeSectionAtAddr(sectPlaceReq.first, nextAvailableAddress) ) return false;
				}
			}
		}
	}
	// place remaining sections
	for(auto &section : sections)
	{
		if(!section.second.placed)
		{
			if(!placeSectionAtAddr(section.first, nextAvailableAddress) ) return false;
		}
	}

	return true;

}

bool Linker::placeSectionAtAddr(const std::string &sectionName, uint32_t address)
{
	globalSymbolTable[sectionName].value = address;
	for(uint8_t byte: sections[sectionName].code){
		if(address==MAX_ADDRES) return false;
		initCode[address++] = byte;
	}
	if(address > nextAvailableAddress) nextAvailableAddress = address;
	sections[sectionName].placed = true;
	return true;
}

bool Linker::resolveRelocations()
{
	std::cout<<"Resolving relocations...\n";
	for(auto &section : sections )
	{
		for(auto &reloc : section.second.relocationTable)
		{
			if(*reloc.symbolName == "drugaSekcija"){
				continue;
			}
			//std::cout<<"Resolving relocation: "<<*reloc.symbolName;//<<" in section: "<<section.first<<"\n";
			uint32_t relocAddress;
			uint32_t relocValue;
			std::cout<<"err check -1";
			SymbolTableElem& symbol = globalSymbolTable[*reloc.symbolName];
			std::cout<<"err check 0";
			SymbolTableElem& sectionOfSymbolDefinition = globalSymbolTable[*(globalSymbolTable[*reloc.symbolName].sectionName)];
			std::cout<<"err check 1";
			SymbolTableElem& sectionOfSymbolUsage = globalSymbolTable[*reloc.sectionName];
			std::cout<<"err check 2";
			 relocAddress = sectionOfSymbolUsage.value + reloc.offset;
			 std::cout<<"err check 3";
			 relocValue = symbol.value + globalSymbolTable[*sectionOfSymbolDefinition.sectionName].value; // val of sym + address of section where its defined
			
			
			
			switch(reloc.type){
				case RelocTableElem::RelocType::VALUE:
					std::cout<<"Relocating VALUE\n";
					initCode[relocAddress] = (relocValue & 0xFF);
					initCode[relocAddress+1] = ((relocValue>>8) & 0xFF);
					initCode[relocAddress+2] = ((relocValue>>16) & 0xFF);
					initCode[relocAddress+3] = ((relocValue>>24) & 0xFF);
					break;
				default:
					std::cerr<<"ERROR: unknown relocation type\n";
					return false;
			}
		}
	}
	return true;
}

int main(int argc, char *argv[])
{
	Linker linker;
	linker.setOutputFilename("linkingOutput");
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

			}else if(std::string(argv[i]) == "-hex")
			{
				linker.setHexExecution(true);
			}else if((std::string(argv[i])).substr(1,5) == "place" ){
				// place section at address
				std::string argument = std::string(argv[i]).substr(7); // skip -place=
				int pos = argument.find("@");
				std::string sectionName = argument.substr(0, pos);
				uint32_t address = std::stoul(argument.substr(pos+1));
				linker.addSectionPlacementReq(sectionName, address);

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
	if(!linker.getHexExecution()) {std::cerr<<"Linker called without -hex option\n"; return 1; }

	linker.link();
	std::cout<<"Linking successful\n";
	return 0;
}
