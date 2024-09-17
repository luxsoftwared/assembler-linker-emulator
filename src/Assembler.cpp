#include "../inc/Assembler.hpp"
#include "../inc/parser.hpp"
#include "../inc/lexer.hpp"

uint32_t SymbolTableElem::idCounter=0;

Assembler::Assembler(){
	LC=0;
	currentSection=NULL;
}

void Assembler::assemble(){

}

void Assembler::push32bitsToCode(uint32_t word){ //little endian
	code.push_back(word & 0xFF);
	code.push_back((word>>8) & 0xFF);
	code.push_back((word>>16) & 0xFF);
	code.push_back((word>>24) & 0xFF);
	LC+=4;
	currentSection->locationCounter+=4;
}

void Assembler::processLine(Line* line){
	// process labels
	static SymList unprocessedLabels;
	
	if(currentSection==NULL){
		for(std::string* sym: line->labels){
			unprocessedLabels.push_back(sym);
		}
	}else{
		for(std::string* sym: unprocessedLabels){
			this->addLabel(currentSection->locationCounter, sym);
		}
		unprocessedLabels.clear();
		for(std::string* sym: line->labels){
			this->addLabel(currentSection->locationCounter, sym);
		}
	}
	
	
	// process directive
	if(line->type==Line::DIRECTIVE){
		
	}else if (line->type==Line::INSTRUCTION){ // process instruction
		if(currentSection==NULL){
			std::cout<<"Error: Instruction outside of section\n";
			exit(1);
		}
		
		processInstruction(line->instruction);

	}
}

void Assembler::addLabel(uint32_t address, std::string* label){

	if(symTabNameToId.find(*label)!=symTabNameToId.end()){
		SymbolTableElem* el = & (symbolTable[symTabNameToId[*label]]);
		el->printSymbolTableElem();
		switch(el->type){
			case SymbolType::EXTERN:
				std::cout<<"Error: Label "<<*label<<" is already defined as external symbol\n";
				break;
			case SymbolType::GLOBAL:
				if(el->value==0 && el->sectionName==NULL){ //nije definisan ranije
					el->value=address;
					el->sectionName=currentSection->sectionName;
					// TODO dodaj kao globalni simbol u currentSection
				}else{
					std::cout<<"Error:Global Label "<<*label<<" is already defined";
				}
				break;
			case SymbolType::LOCAL:
				if(currentSection==NULL || el->sectionName==currentSection->sectionName){
					std::cout<<"Error: Label "<<*label<<" is already defined in this section\n";
					exit(1);
				}else{
					// TODO dodaj kao lokalni simbol u currentSection
				}
				break;
			default:
				break;
		}
	}else{ //not in symbol table, local
		SymbolTableElem sym;
		sym.id=SymbolTableElem::idCounter++;
		sym.symbolName=label;
		sym.sectionName=currentSection->sectionName;
		sym.value=address;
		sym.type=SymbolType::LOCAL;
		symbolTable.push_back(sym);
		symTabNameToId[*label]=sym.id;
		// TODO dodaj kao lokalni simbol u currentSection
	}
	
}

void Assembler::declareGlobalSymbol(std::string* symbol){
	if(symTabNameToId.find(*symbol)!=symTabNameToId.end()){
		SymbolTableElem el = symbolTable[symTabNameToId[*symbol]];
		switch(el.type){
			case SymbolType::EXTERN:
				std::cout<<"Error: Label "<<*symbol<<" is already defined as external symbol\n";
				break;
			case SymbolType::GLOBAL:
				std::cout<<"Error: Label "<<*symbol<<" is already defined as global symbol\n";
				break;
			case SymbolType::LOCAL:
				el.type=SymbolType::GLOBAL;
				break;
			default:
				break;
		}
	}else{ //not in symbol table, global
		SymbolTableElem sym;
		sym.id=SymbolTableElem::idCounter++;
		sym.symbolName=symbol;
		sym.value=0;
		sym.type=SymbolType::GLOBAL;
		sym.sectionName=NULL;
		symbolTable.push_back(sym);
		symTabNameToId[*symbol]=sym.id;
		//std::cout<< (symTabNameToId.find(symbol)!=symTabNameToId.end()? ("Nasao:"+(symTabNameToId[symbol])):"Ne pronalazi iako je dodao");
	}
}

void Assembler::declareExternSymbol(std::string* symbol){
	if(symTabNameToId.find(*symbol)!=symTabNameToId.end()){
		SymbolTableElem el = symbolTable[symTabNameToId[*symbol]];
		switch(el.type){
			case SymbolType::EXTERN:
				std::cout<<"Error: Label "<<*symbol<<" is already defined as external symbol\n";
				break;
			case SymbolType::GLOBAL:
				std::cout<<"Error: Label "<<*symbol<<" is already defined as global symbol\n";
				break;
			case SymbolType::LOCAL:
				std::cout<<"Error: Label "<<*symbol<<" is already defined as global symbol\n";
				break;
			default:
				break;
		}
	}else{ //not in symbol table, extern
		SymbolTableElem sym;
		sym.id=SymbolTableElem::idCounter++;
		sym.symbolName=symbol;
		sym.value=0;
		sym.type=SymbolType::EXTERN;
		sym.sectionName=NULL;
		symbolTable.push_back(sym);
		symTabNameToId[*symbol]=sym.id;
	}
}

void Assembler::processDirective(Directive* directive){
	switch(directive->type){
			case Directive::GLOBAL:{
				
				for(std::string* sym : *(directive->symList) ){
					this->declareGlobalSymbol(sym);
				}
				break;
			}
			case Directive::EXTERN:{
				for(std::string* sym : *(directive->symList) ){
					this->declareExternSymbol(sym);
				}
				break;
			}
			case Directive::SECTION:{
				if( /*pretraga u tabeli ne ovde*/sections.find( *(directive->symbol) )!=sections.end()){
					std::cout<<"Error: Section "<<*(directive->symbol)<<" is already defined\n";
					
					// TODO drugo pojavljivanje sekcije
				}else{
					//end previous section
					if(currentSection!=NULL){
						currentSection->endAddress=LC;
						currentSection->size=LC-currentSection->startAddress;

						symbolTable[symTabNameToId[ *(currentSection->sectionName) ]].size=currentSection->size;	
					}
					//add new section
					SymbolTableElem sec;
					sec.type=SymbolType::SECTION;
					sec.symbolName= directive->symbol;
					sec.id=SymbolTableElem::idCounter++;
					sec.value=LC;
					sec.size=0;
					symbolTable.push_back(sec);
					symTabNameToId[ *(sec.symbolName) ]=sec.id;
					Section sect;
					sect.id=sec.id;
					sect.startAddress=LC;
					sect.endAddress=0;
					sect.size=0;
					sect.sectionName=directive->symbol;
					sect.locationCounter=0;
					sections[ *(directive->symbol) ]=sect;
					
					currentSection=&sections[ *(directive->symbol) ];
				}
				break;
			}
			case Directive::WORD:{
				if(currentSection==NULL){
					std::cout<<"Error: Word directive outside of section\n";
					exit(1);
				}
				for(SymOrLit sol : *(directive->symOrLitList)){
					if(sol.type==SymOrLit::SYMBOL){
						// TODO! dodaj u relocation table 
						relocationTable.push_back({LC, currentSection->sectionName, RelocTableElem::ABSOLUTE, sol.symbol });
						push32bitsToCode(0);
						
					}else{
						push32bitsToCode(sol.literal);
					}
				}
				break;
			}
			case Directive::SKIP:{
				if(currentSection==NULL){
					std::cout<<"Error: Skip directive outside of section\n";
					exit(1);
				}

				for(int i=0; i<directive->literal; i++){
					code.push_back(0x00);
					LC++;
					currentSection->locationCounter++;
				}
				break;
			}
			case Directive::END:{
				//end section
				if(currentSection!=NULL){
						currentSection->endAddress=LC;
						currentSection->size=LC-currentSection->startAddress;

						symbolTable[symTabNameToId[ *(currentSection->sectionName) ]].size=currentSection->size;	
					}
				std::cout<<"End\n";
				break;
			}
			default:
				std::cout<<"Error in directive type\n";
		}
}

void Assembler::processInstruction(Instruction* instr){
	uint32_t instrWord=0;
	switch(instr->type){
			case Instruction::HALT:{
				instrWord = 0;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::INT:{
				instrWord |= (uint32_t)InstructionCode::INT;
				push32bitsToCode(instrWord);
				break;
			}/*
			case Instruction::IRET:{
				instrWord |= (uint32_t)InstructionCode::INT;
				instrWord |= instr->cond << 4;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::CALL:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->imm << 8;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::RET:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::JMP:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->imm << 8;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::BEQ:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->imm << 8;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::BNE:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->imm << 8;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::BGT:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->imm << 8;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::PUSH:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::POP:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::XCHG:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				instrWord |= instr->rs1 << 12;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::ADD:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				instrWord |= instr->rs1 << 12;
				instrWord |= instr->rs2 << 16;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::SUB:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				instrWord |= instr->rs1 << 12;
				instrWord |= instr->rs2 << 16;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::MUL:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				instrWord |= instr->rs1 << 12;
				instrWord |= instr->rs2 << 16;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::DIV:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				instrWord |= instr->rs1 << 12;
				instrWord |= instr->rs2 << 16;
				push32bitsToCode(instrWord);
				break;
			}
			
			*/
		}
}


void Assembler::printSymbolTable(){
	SymbolTableElem::printSymbolTableHeader();
	for(SymbolTableElem el : symbolTable){
		el.printSymbolTableElem();
	}
}

void Assembler::printRelocTable(){
	RelocTableElem::printRelocTableHeader();
	for(RelocTableElem el : relocationTable){
		el.printRelocTableElem();
	}
}

void Assembler::printCode(){
	std::cout<<"Code:\n";
	int i=0;
	for(uint8_t byte : code){
		std::cout<<std::dec <<i++<<":\t";
		std::cout<<std::hex<<(int)byte<<"\n"<<std::dec;
	}
	std::cout<<"\n";
}

void Assembler::printSections(){
	Section::printSectionHeader();
	for(auto sect : sections){
		sect.second.printSection();
	}
}

void Assembler::printDebug(){
	
}

int main(int, char**) {
  // open a file handle to a particular file:
  FILE *myfile = fopen("./tests/in.snazzle", "r");
  // make sure it's valid:
  if (!myfile) {
    std::cout << "I can't open file!" << std::endl;
    return -1;
  }
  // set lex to read from it instead of defaulting to STDIN:
  yyin = myfile;

  // parse through the input until there is no more:
  Assembler assembler;
  do {
    yyparse(&assembler);
  } while (!feof(yyin));

  
  assembler.printSymbolTable();
  assembler.printRelocTable();
  assembler.printCode();
  assembler.printSections();
  
}

void yyerror(Assembler* assembler,const char *s) {
  std::cout << "parse error on line " << yylineno << "!  Message: " << s << std::endl;
  // might as well halt now:
  exit(-1);
}
