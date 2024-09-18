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
	currentSection->code.push_back(word & 0xFF);
	currentSection->code.push_back((word>>8) & 0xFF);
	currentSection->code.push_back((word>>16) & 0xFF);
	currentSection->code.push_back((word>>24) & 0xFF);
	LC+=4;
	currentSection->locationCounter+=4;
}

void Assembler::processLine(Line* line){
	this->inputFileLineNum=line->lineNumber;
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
		processDirective(line->directive);
	// process instruction
	}else if (line->type==Line::INSTRUCTION){ 
		if(currentSection==NULL){
			std::cout<<"Error: Instruction outside of section\n";
			exit(1);
		}	
		processInstruction(line->instruction);
	}
}

void Assembler::addLabel(uint32_t address, std::string* label){

	if(symbolTable.find(*label)!=symbolTable.end()){
		SymbolTableElem* el = & (symbolTable[*label]);
		el->printSymbolTableElem();
		switch(el->type){
			case SymbolType::EXTERN:
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Label "<<*label<<" is already defined as external symbol\n";
				break;
			case SymbolType::GLOBAL:
				if(el->value==0 && el->sectionName==NULL){ //nije definisan ranije
					el->value=address;
					el->sectionName=currentSection->sectionName;
					
				}else{
					std::cout<<"Error on line"<<this->inputFileLineNum<<":Global Label "<<*label<<" is already defined";
				}
				break;
			default:
				break;
		}
	}else{ //not in  global symbol table, local
		if(currentSection->symbolTable.find(*label)!=currentSection->symbolTable.end()){
			std::cout<<"Error on line"<<this->inputFileLineNum<<": Label "<<*label<<" is already defined in this section\n";
		}else{ // add new local sym
			SymbolTableElem sym;
			sym.id=SymbolTableElem::idCounter++;
			sym.symbolName=label;
			sym.sectionName=currentSection->sectionName;
			sym.value=address;
			sym.type=SymbolType::LOCAL;
			currentSection->symbolTable[*label]=sym;
		}
	}
	
}

void Assembler::declareGlobalSymbol(std::string* symbol){
	if(symbolTable.find(*symbol)!=symbolTable.end()){
		SymbolTableElem* el = &(symbolTable[*symbol]);
		switch(el->type){
			case SymbolType::EXTERN:
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Symbol "<<*symbol<<" is already defined as external symbol\n";
				break;
			case SymbolType::GLOBAL:
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Symbol "<<*symbol<<" is already defined as global symbol\n";
				break;
			case SymbolType::SECTION:
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Symbol "<<*symbol<<" is already defined as section\n";
				break;
			default:
				break;
		}
	}else//not in global symbol table, maybe local
	if(currentSection->symbolTable.find(*symbol)!=currentSection->symbolTable.end()){
		// delete from local sym table, add to global after
		currentSection->symbolTable.erase(*symbol);
	}
	// add new global sym

	SymbolTableElem sym;
	sym.id=SymbolTableElem::idCounter++;
	sym.symbolName=symbol;
	sym.value=0;
	sym.type=SymbolType::GLOBAL;
	sym.sectionName=NULL;
	symbolTable[*symbol]=sym;
	
}

void Assembler::declareExternSymbol(std::string* symbol){
	if(symbolTable.find(*symbol)!=symbolTable.end()){
		SymbolTableElem el = symbolTable[*symbol];
		switch(el.type){
			case SymbolType::EXTERN:
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Symbol "<<*symbol<<" is already defined as external symbol\n";
				break;
			case SymbolType::GLOBAL:
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Symbol "<<*symbol<<" is already defined as global symbol\n";
				break;
			case SymbolType::SECTION:
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Symbol "<<*symbol<<" is already defined as section symbol\n";
				break;
			default:
				break;
		}
	}else if(currentSection->symbolTable.find(*symbol)!=currentSection->symbolTable.end()){
		// delete from local sym table, add to global after
		currentSection->symbolTable.erase(*symbol);
	}

	SymbolTableElem sym;
	sym.id=SymbolTableElem::idCounter++;
	sym.symbolName=symbol;
	sym.value=0;
	sym.type=SymbolType::EXTERN;
	sym.sectionName=NULL;
	symbolTable[*symbol]=sym;
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
				if( sections.find( *(directive->symbol) )!=sections.end()){
					std::cout<<"Error on line"<<this->inputFileLineNum<<": Section "<<*(directive->symbol)<<" is already defined\n";
					
					// TODO drugo pojavljivanje sekcije
				}else{
					//end previous section
					endSection();
					//add new section
					SymbolTableElem sec;
					sec.type=SymbolType::SECTION;
					sec.symbolName= directive->symbol;
					sec.id=SymbolTableElem::idCounter++;
					sec.value=LC;
					sec.size=0;
					symbolTable[ *(sec.symbolName) ]=sec;

					Section sect;
					sect.id=sec.id;
					sect.startAddress=LC;
					sect.endAddress=0;
					sect.size=0;
					sect.sectionName=directive->symbol;
					sect.locationCounter=0;
					sections[ *(directive->symbol) ]=sect;
					
					currentSection= &( sections[ *(directive->symbol) ]) ;
				}
				break;
			}
			case Directive::WORD:{
				if(currentSection==NULL){
					std::cout<<"Error on line"<<this->inputFileLineNum<<": Word directive outside of section\n";
					break;
				}
				for(SymOrLit sol : *(directive->symOrLitList)){
					if(sol.type==SymOrLit::SYMBOL){
						// TODO! dodaj u relocation table 
						int32_t sym_val=0;
						if(symbolTable.find(*(sol.symbol))!=symbolTable.end()){
							SymbolTableElem* el = & (symbolTable[*(sol.symbol)]);
							if(el->value!=0 || el->sectionName!=NULL){ // defined symbol
								sym_val=el->value; //global
							}
						}else if(currentSection->symbolTable.find(*(sol.symbol))!=currentSection->symbolTable.end()){
							SymbolTableElem* el = & (currentSection->symbolTable[*(sol.symbol)]);
							if(el->value!=0 || el->sectionName!=NULL){ // defined symbol
								sym_val=el->value; //local
							}
						}else{ // not defined symbol, add to relocation table
							relocationTable.push_back({currentSection->locationCounter,
									currentSection->sectionName, RelocTableElem::VALUE, sol.symbol });
						}
						push32bitsToCode(sym_val);
						
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
					currentSection->code.push_back(0x00);
					LC++;
					currentSection->locationCounter++;
				}
				break;
			}
			case Directive::END:{
				//end section
				endSection();
				endFile();
				std::cout<<"End\n";
				break;
			}
			default:
				std::cout<<"Error in directive type\n";
		}
}
/*
void Assembler::processJMPoperand(Operand op){
	if(op.type==Operand::LIT){
		if(op.literal& 0x000 != 0 ){ // lit bigger than 12 bits,has to go to litpool
		
		}else{
			// lit can be but in instruction
		}
	}else if (op.type==Operand::SYM){
		if( symTabNameToId.find(*(op.symbol))!=symTabNameToId.end() ){
			SymbolTableElem* el = & (symbolTable[symTabNameToId[*(op.symbol)]]);
			if(el->type==SymbolType::EXTERN){
				// TODO! dodaj u relocation table 
				relocationTable.push_back({LC, currentSection->sectionName, RelocTableElem::PCRELATIVE, op.symbol });
				
			}else if(el->type==SymbolType::GLOBAL){
				// TODO! dodaj u relocation table 
				relocationTable.push_back({LC, currentSection->sectionName, RelocTableElem::ABSOLUTE, op.symbol });
				
			}else if(el->type==SymbolType::LOCAL){
				// TODO! dodaj u relocation table 
				relocationTable.push_back({LC, currentSection->sectionName, RelocTableElem::PCRELATIVE, op.symbol });
				
			}else{
				std::cout<<"Error: Symbol "<<*(op.symbol)<<" is not defined\n";
				exit(1);
			}
		}
	}
}
*/
void Assembler::processCallInstruction(Instruction* instr){

}

void Assembler::processInstruction(Instruction* instr){
	uint32_t instrWord=0;
	//---------------------------------------------------------------
	// 1	  1			2		2   		3    	3    	4    4
	// OC	  MOD		RegA	RegB		RegC 	Disp	Disp Disp
	// 31-28  27-24		23-20	19-16		15-12	11-8	7-4	 3-0
	//---------------------------------------------------------------
	switch(instr->type){
			case Instruction::HALT:{
				instrWord = 0;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::INT:{
				instrWord |= (uint32_t)InstructionCode::INT<<24;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::IRET:{
				// pop pc; pop status;

				// pop pc --> pc<=mem32[sp]; sp<=sp+4;
				//OC=1001 M=0011: gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
				// A:pc, B:sp, D:4
				instrWord |= (uint32_t)InstructionCode::POP<<24;
				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
				instrWord |= (uint32_t)GPRType::SP << 16; // B=sp
				instrWord |= 4; // D=4
				push32bitsToCode(instrWord);

				// pop status --> csr[STATUS]<=mem32[sp]; sp<=sp+4;
				// csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
				// A:status, B:sp D:4
				instrWord |= (uint32_t)InstructionCode::IRET<<24;
				instrWord |= (uint32_t)CSRType::STATUS << 20; // A=status
				instrWord |= (uint32_t)GPRType::SP << 16; // B=sp
				instrWord |= 4; // D=4
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::CALL:{
				// push pc; pc<=op;
				// OC=21: push pc; pc<=mem32[gpr[A]+gpr[B]+D];
				// A:pc, B:0, D:disp to lit pool adr of operand

				

				// TODO dodaj litpool i ovu instrukciju
				break;
			}
			case Instruction::RET:{
				// pop pc --> pc<=mem32[sp]; sp<=sp+4;
				//OC=1001 M=0011: gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
				// A:pc, B:sp, D:4
				instrWord |= (uint32_t)InstructionCode::POP<<24;
				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
				instrWord |= (uint32_t)GPRType::SP << 16; // B=sp
				instrWord |= 4; // D=4
				push32bitsToCode(instrWord);
				break;
			}/*
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
			}*/
			case Instruction::PUSH:{
				// push gpr	-->	sp<=sp-4;	mem32[sp]<=gpr; sp<=sp-4;
				// OCM=0x81 gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
				// A:sp, B:0, C:gpr, D:4
				instrWord |= (uint32_t)InstructionCode::PUSH<<24;
				instrWord |= (uint32_t)GPRType::SP << 20; // A=sp
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr
				instrWord |= 4; // D=4
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::POP:{
				//pop gpr	-->	gpr<=mem32[sp]; sp<=sp+4;
				//OC=1001 M=0011: gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
				// A:gpr, B:sp, D:4
				instrWord |= (uint32_t)InstructionCode::POP<<24;
				instrWord |= (uint32_t)instr->operands[0].gpr << 20; // A=gpr
				instrWord |= (uint32_t)GPRType::SP << 16; // B=sp
				instrWord |= 4; // D=4
				push32bitsToCode(instrWord);


				break;
			}
			case Instruction::XCHG:{
				// temp <= gpr1;	gpr1 <= gpr0;	gpr0 <= temp;
				// OC=4 M=0: temp<=gpr[B]; gpr[B]<=gpr[C]; gpr[C]<=temp;
				// A:0, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::XCHG<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::ADD:{
				// gpr1=gpr1+gpr0
				// OCM=0x50: gpr[A]<=gpr[B] + gpr[C];
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::ADD<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::SUB:{
				// gpr1=gpr1-gpr0	
				// OCM=0x51: gpr[A]<=gpr[B] - gpr[C];	
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::SUB<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::MUL:{
				// gpr1 = gpr1 * gpr0	
				// OCM=0x52: gpr[A]<=gpr[B] * gpr[C];	
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::MUL<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::DIV:{
				// gpr1 = gpr1 / gpr0	
				// OCM=0x53: gpr[A]<=gpr[B] / gpr[C];	
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::DIV<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::NOT:{
				// gpr <= ~gpr;
				// OCM=60 gpr[A]<=~gpr[B];
				// A:gpr, B:gpr
				instrWord |= (uint32_t)InstructionCode::NOT<<24;
				instrWord |= (uint32_t)instr->operands[0].gpr << 20; // A=gpr
				instrWord |= (uint32_t)instr->operands[0].gpr << 16; // B=gpr
				push32bitsToCode(instrWord);

				break;
			}
			case Instruction::AND:{
				//gpr1 <= gpr1 & gpr0;
				// OCM=0x61 gpr[A]<=gpr[B] & gpr[C];
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::AND<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0

				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::OR:{
				//gpr1 <= gpr1 | gpr0;
				// OCM=0x62 gpr[A]<=gpr[B] | gpr[C];
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::OR<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCode(instrWord);
				
				break;
			}
			case Instruction::XOR:{
				//gpr1 <= gpr1 ^ gpr0;
				// OCM=0x63 gpr[A]<=gpr[B] ^ gpr[C];
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::XOR<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::SHL:{
				// gpr1 <= gpr1 << gpr0; 
				// OCM=0x70 gpr[A]<=gpr[B] << gpr[C];
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::SHL<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::SHR:{
				// gpr1 <= gpr1 >> gpr0;
				// OCM=0x71 gpr[A]<=gpr[B] >> gpr[C];
				// A:gpr1, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::SHR<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				
				push32bitsToCode(instrWord);
				break;
			}/*
			case Instruction::LD:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				instrWord |= instr->rs1 << 12;
				instrWord |= instr->imm << 16;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::ST:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rs1 << 8;
				instrWord |= instr->rs2 << 12;
				instrWord |= instr->imm << 16;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::CSRRD:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rd << 8;
				instrWord |= instr->csr << 12;
				instrWord |= instr->imm << 16;
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::CSRWR:{
				instrWord |= instr->opcode;
				instrWord |= instr->cond << 4;
				instrWord |= instr->rs1 << 8;
				instrWord |= instr->csr << 12;
				instrWord |= instr->imm << 16;
				push32bitsToCode(instrWord);
				break;
			}
			
			*/
		}
}

void Assembler::endSection(){
	if(currentSection!=NULL){
		currentSection->endAddress=LC;
		currentSection->size=LC-currentSection->startAddress;

		symbolTable[ *(currentSection->sectionName) ].size=currentSection->size;	
	}

	// 
}

void Assembler::endFile(){
	// go through reloc table and ressolve
}


void Assembler::printSymbolTable(){
	SymbolTableElem::printSymbolTableHeader();
	for(auto el : symbolTable){
		el.second.printSymbolTableElem();
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
	while (i<LC){
		for(auto section : sections){
			if(section.second.startAddress==i){
				int j=0;
				for(uint8_t byte : section.second.code){
					std::cout<<i++<<":\t"<<j++<<":\t";
					std::cout<<std::hex<<(int)byte<<"\n"<<std::dec;
				}
			}
		}
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
