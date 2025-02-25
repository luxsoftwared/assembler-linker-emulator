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
		std::cerr<<"ERROR: symbol table construction failed\n";
		return;
	}

	// place sections
	
	if(!placeSections()){
		std::cerr<<"ERROR: section placement failed\n";
		return;
	}

	// resolve relocations
	if(!resolveRelocations()){
		std::cerr<<"ERROR: resolving relocations failed\n";
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
	std::ofstream outTxt(("./outputs/"+outputFilename+".hex").c_str(), std::ios::out);
	uint32_t displayedAddress = 0;
	uint32_t i = 0;
	for(auto& code: initCode){
		// print byte by byte
		//outTxt<<std::hex<<std::setfill('0')<<std::setw(8)<<code.first<<":"<<std::setw(2)<<(uint32_t)code.second<<"\n";
		// print 8 bytes per line
		if(code.second == 0 && i==0) continue;
		// print address, fill missing spaces w zeros
		if(i==0){
			// set adr an levo, deljivo sa 8
			displayedAddress = code.first - code.first%8;
			outTxt<<std::hex<<std::setfill('0')<<std::setw(8)<<displayedAddress<<": ";
			// fill with leading zeros, if needed
			for(; i<code.first%8; i++){
				outTxt<<std::setw(2)<<"00 ";
			}			
		}else if(code.first != displayedAddress+i){
			for(;i<8 && code.first != displayedAddress+i;i++){
				outTxt<<std::setw(2)<<"00 ";
			}
			if(code.first == displayedAddress+i){
				break;
			}
			outTxt<<"\n";
			i=0;
			if(code.first%8 != 0){
				displayedAddress = code.first - code.first%8;
				// fill with leading zeros
				for(; i<code.first%8; i++){
					outTxt<<std::setw(2)<<"00 ";
				}
			}else{
				displayedAddress = code.first;
			}
			
			outTxt<<std::hex<<std::setfill('0')<<std::setw(8)<<displayedAddress<<": ";
			displayedAddress = code.first;
		}

		outTxt<<std::setw(2)<<(uint32_t)code.second<<" ";
		i++;
		if(i==8){
			outTxt<<"\n";
			i=0;
		}

		
	}

	for(;i>0 && i<8;i++){
		outTxt<<std::setw(2)<<"00 ";
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
							relocEntry.address += globalSymbolTable[sym.first].value; 
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
			if(reloc.type != RelocTableElem::RelocType::VALUE){
				std::cerr<<"ERROR: relocation type different from VALUE (absolute), not implemented\n";
				return false;
			}
			
			uint32_t relocAddress;
			uint32_t relocValue ;
			SymbolTableElem& sectionOfSymbolUsage = globalSymbolTable[*reloc.sectionName];
			relocAddress = sectionOfSymbolUsage.value + reloc.address;
			SymbolTableElem sectionOfSymbolDefinition;
			
			if(*reloc.symbolName == *reloc.sectionName){
				//symbol is a local symbol; defined in the same section where it is used 
				sectionOfSymbolDefinition = sectionOfSymbolUsage;
				relocValue = initCode[relocAddress]; // value of the symbol(relative to its section), written in code durring assembly
				relocValue |= (initCode[relocAddress+1] << 8);
				relocValue |= (initCode[relocAddress+2] << 16);
				relocValue |= (initCode[relocAddress+3] << 24);
				relocValue += sectionOfSymbolDefinition.value;
			}else{
				//global symbol; 
				if(globalSymbolTable.find(*reloc.symbolName) == globalSymbolTable.end() ){
					std::cerr<<"ERROR: symbol not found in global symbol table\n";
					return false;
				}else if(globalSymbolTable[*reloc.symbolName].type == SymbolType::EXTERN){
					std::cerr<<"ERROR: extern symbol " +*reloc.symbolName +" is not defined in any file\n";
					return false;
				}

				sectionOfSymbolDefinition = globalSymbolTable[*(globalSymbolTable[*reloc.symbolName].sectionName)];
				relocValue = sectionOfSymbolDefinition.value + globalSymbolTable[*reloc.symbolName].value;
			}

			
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
				uint32_t address = std::stoul(argument.substr(pos+1),NULL,16);
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
