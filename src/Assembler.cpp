#include "../inc/Assembler.hpp"
#include "../inc/parser.hpp"
#include "../inc/lexer.hpp"
#include <iostream>
#include <fstream>
#include "../inc/ObjFile.hpp"
#include <sstream>

uint32_t SymbolTableElem::idCounter=0;

// process
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

void Assembler::push32bitsToCodeBigEndian(uint32_t word){ //obrnuto jer sam zeznuo instr
	currentSection->code.push_back( (word>>24) & 0xFF);
	currentSection->code.push_back((word>>16) & 0xFF);
	currentSection->code.push_back((word>>8) & 0xFF);
	currentSection->code.push_back(word & 0xFF);
	LC+=4;
	currentSection->locationCounter+=4;
}

void Assembler::edit32bitsOfCode(Section& s, uint32_t address, uint32_t word){ //little endian
	s.code[address]= (word & 0xFF);
	s.code[address+1]= ((word>>8) & 0xFF);
	s.code[address+2]= ((word>>16) & 0xFF);
	s.code[address+3]= ((word>>24) & 0xFF);
	
}

void Assembler::edit32bitsOfCodeBigEndian(Section& s, uint32_t address, uint32_t word){ 
	s.code[address+3]= (word & 0xFF);
	s.code[address+2]= ((word>>8) & 0xFF);
	s.code[address+1]= ((word>>16) & 0xFF);
	s.code[address]= ((word>>24) & 0xFF);
	
}


void Assembler::addDispToInstruction(Section& s, uint32_t address, int32_t disp){
	s.code[address+3] = (disp & 0xFF);
	s.code[address+2] |= ((disp>>8) & 0xF);
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
	
	if(currentSection!=NULL && currentSection->locationCounter / 2000 > currentSection->litPoolThresholdsReached ){
		currentSection->litPoolThresholdsReached++;
		insertLitPool(); //TODO check if this works?
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
	std::cout<<"Declaring global symbol "<<*symbol<<"\n";
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
	if(currentSection!=NULL && currentSection->symbolTable.find(*symbol)!=currentSection->symbolTable.end()){
		// delete from local sym table, add to global after
		currentSection->symbolTable.erase(*symbol);
	}
	// add new global sym
	std::cout<<"Adding global symbol "<<*symbol<<"\n";
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
	}else if(currentSection!=NULL && currentSection->symbolTable.find(*symbol)!=currentSection->symbolTable.end()){
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
					sect.litPoolThresholdsReached=0;
					sections[ *(directive->symbol) ]=sect;
					
					currentSection= &( sections[ *(directive->symbol) ]) ;
					std::cout<<"Section "<<*(directive->symbol)<<" started\n";
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
						int32_t sym_val=0;
						if(symbolTable.find(*(sol.symbol))!=symbolTable.end()){
							SymbolTableElem* el = & (symbolTable[*(sol.symbol)]);
							if(el->type==SymbolType::SECTION ){
								std::cout<<"Error on line"<<this->inputFileLineNum<<": .word symbol previously defined as section\n";
								continue;
							}
								
							if(el->value!=0 || el->sectionName!=NULL){ // defined symbol
								sym_val=el->value; //global
							}else{
								currentSection->relocationTable.push_back(RelocTableElem(currentSection->locationCounter,
									currentSection->sectionName, sol.symbol ));
							}
							
							
						}else if(currentSection->symbolTable.find(*(sol.symbol))!=currentSection->symbolTable.end()){
							SymbolTableElem* el = & (currentSection->symbolTable[*(sol.symbol)]);
							if(el->value!=0 || el->sectionName!=NULL){ // defined symbol
								sym_val=el->value; //local
							}else{
								currentSection->relocationTable.push_back(RelocTableElem(currentSection->locationCounter,
									currentSection->sectionName, sol.symbol ));
							}
						}else{// not defined symbol, add to relocation table
							currentSection->relocationTable.push_back(RelocTableElem(currentSection->locationCounter,
									currentSection->sectionName, sol.symbol ));
						} 
						
						push32bitsToCode(sym_val);
						
					}else{
						//literal
						push32bitsToCode(sol.literal); std::cout<<"Literal "<<sol.literal<<" added to code at adr:"<<LC<<"\n";
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

					if(currentSection!=NULL && currentSection->locationCounter / 2000 > currentSection->litPoolThresholdsReached ){
						currentSection->litPoolThresholdsReached++;
						std::cout<<"Inserting litpool inside skip\n";
						insertLitPool();
					}

					currentSection->code.push_back(0x00);
					LC++;
					currentSection->locationCounter++;
				}
				break;
			}
			case Directive::END:{
				//end section
				postProccessInstructions(true); // here, final postprocessing is done, and mistakes are shown
				endSection();
				endFile();
				std::cout<<"End!\n";
				break;
			}
			default:
				std::cout<<"Error in directive type\n";
		}
}

// TODO kick out this function
void Assembler::addToLitPool(SymOrLit sol, Section* section=NULL, uint32_t addressOfInstruction=-1){
	std::cout<<"Adding to litpool: SymOrLit-";
	sol.print();
	std::cout<<", for instruction at:"<<addressOfInstruction<<"\n";
	section = section==NULL ? currentSection : section; // CHECK?
	addressOfInstruction = addressOfInstruction==(uint32_t)-1 ? section->locationCounter : addressOfInstruction; // CHECK?
	if(sol.type==SymOrLit::SYMBOL){
		// unfinished reloc entry
		/*
		section->relocationTable.push_back(
		//	RelocTableElem(/*offset*///section->locationCounter,  // instead of this, it will be adress of litpool el
		//	/*sectionName*/section->sectionName,
		//	/*type*/ RelocTableElem::VALUE, // correct, want to just paste the value to lit pool
		//	/*symbolName*/ sol.symbol ));
		//*/

		// D add to litpool
		section->litPool.push_back(
			LitPoolElem{ sol.symbol,
			addressOfInstruction,
			/*false,
			&(section->relocationTable.back()),
			section->relocationTable.back().id
			*/} 
		);
		
	}else{
		section->litPool.push_back(
			LitPoolElem{(uint32_t)sol.literal,
			addressOfInstruction,
			}
		);
		
	}
}
	
	

/**
 * OCM=0 when gpr<=gpr+D;  => pc=pc+D
 * OCM=1 when gpr<=mem32[gpr+D]; (litpool)  => pc=mem32[pc+D]
 * D:disp to lit pool adr of operand, or disp to jump adr
 */
uint32_t Assembler::processJumpInstructions(Operand op,Section* section=NULL, uint32_t addressOfInstruction=-1, bool isFinalProcessing=false){
// highest byte set to 0 when adressing is direct(pc rel), 1 when indirect(mem[pc rel])
	section = section==NULL ? currentSection : section;
	addressOfInstruction = addressOfInstruction==(uint32_t)-1 ? section->locationCounter : addressOfInstruction;
	uint32_t instrWord=0;

	if(op.type==Operand::LIT){
		//if((int32_t)op.literal>2047 || (int32_t)op.literal<-2048 ){ // lit bigger than 12 bits,has to go to litpool
			instrWord |= 1 <<24; // indirect adressing
			addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=op.literal},section,addressOfInstruction); //CORECT
			return instrWord;
		/*}else{
			// lit can be put in instruction
			instrWord |= 0 <<24; // direct adressing
			instrWord |= op.literal & 0xFFF; // D=disp
			return instrWord;
		}*/
	}else 
	if (op.type==Operand::SYM){
		if( symbolTable.find(*(op.symbol))!=symbolTable.end() ){
			
			SymbolTableElem* el = & (symbolTable[*(op.symbol)]);
			if(el->type==SymbolType::EXTERN){
				instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
				addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol},section,addressOfInstruction);
			}else 
			if(el->type==SymbolType::GLOBAL){
				if(el->value!=0 || el->sectionName!=NULL){ 
					//defined global sym
					
					if(el->sectionName==section->sectionName){
						int32_t disp= el->value - (addressOfInstruction + 4); // +4 bcs during execution pc is already incremented
						if(disp>2047 || disp<-2048){
							//disp bigger than 12 bits, jump addr has to go to litpool
							instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
							addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol},section,addressOfInstruction);
							return instrWord;
						}else{
							//disp can be put in instruction
							instrWord |= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24;
							instrWord |= disp & 0xFFF; // D=disp
							return instrWord;
						}
					}else{
						//  global sym from another section, must go to litpool
						instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
						addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol},section,addressOfInstruction);
					}
				}else{
					//undefined global sym , to litpool
					instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
					addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol},section,addressOfInstruction);
				}
			}else 
			if(el->type==SymbolType::SECTION)
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Jump to section symbol\n";			
		}else if(section->symbolTable.find(*(op.symbol))!=section->symbolTable.end()){
			//defined local elem
			SymbolTableElem* el = & (section->symbolTable[*(op.symbol)]);
			int32_t disp= el->value - (addressOfInstruction+4); // +4 bcs during execution pc is already incremented
			if(disp>2047 || disp<-2048){
				//disp bigger than 12 bits, has to go to litpool
				instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
				addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol},section,addressOfInstruction);
			}else{
				//disp can be put in instruction
				instrWord |= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24;
				instrWord |= disp & 0xFFF; // D= disp
			}

		}else{
			//not found anywhere
			//instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
			//addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol},section,addressOfInstruction);
			if( isFinalProcessing) {
				std::cout<<"Error on address "<<addressOfInstruction<<": Jump to undefined symbol\n"; 
				return instrWord |= (uint32_t)InstructionCode::ERROR<<24; // in case of error
			}else {
				instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
				addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol},section,addressOfInstruction);
			}
		}
	}
	return instrWord;
}

void Assembler::processInstruction(Instruction* instr){
	if(currentSection==NULL){
		std::cout<<"Error: Instruction outside of section\n";
		exit(1);
	}

	uint32_t instrWord=0;
	//---------------------------------------------------------------
	// 1	  1			2		2   		3    	3    	4    4
	// OC	  MOD		RegA	RegB		RegC 	Disp	Disp Disp
	// 31-28  27-24		23-20	19-16		15-12	11-8	7-4	 3-0
	//---------------------------------------------------------------
	switch(instr->type){
			case Instruction::HALT:{
				instrWord = 0;
				push32bitsToCodeBigEndian(instrWord);
				break;
			}
			case Instruction::INT:{
				instrWord |= (uint32_t)InstructionCode::INT<<24;
				push32bitsToCodeBigEndian(instrWord);
				break;
			}
			case Instruction::IRET:{
				// pop status; pop pc; (mora prvo status, iako tekst kaze drugacije)

				// pop status --> csr[STATUS]<=mem32[sp]; sp<=sp+4;
				// csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
				// A:status, B:sp D:4
				instrWord |= (uint32_t)InstructionCode::IRET<<24;
				instrWord |= (uint32_t)CSRType::STATUS << 20; // A=status
				instrWord |= (uint32_t)GPRType::SP << 16; // B=sp
				instrWord |= 4; // D=4
				push32bitsToCodeBigEndian(instrWord);
				
				instrWord=0;
				// pop pc --> pc<=mem32[sp]; sp<=sp+4;
				//OC=1001 M=0011: gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;
				// A:pc, B:sp, D:4
				instrWord |= (uint32_t)InstructionCode::POP<<24;
				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
				instrWord |= (uint32_t)GPRType::SP << 16; // B=sp
				instrWord |= 4; // D=4
				push32bitsToCodeBigEndian(instrWord);
				break;
			}
			case Instruction::CALL:{

				currentSection->unprocessedInstructions.push_back({instr, currentSection->locationCounter});
				push32bitsToCodeBigEndian(0);

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
				push32bitsToCodeBigEndian(instrWord);
				break;
			}
			case Instruction::JMP:
			case Instruction::BEQ:
			case Instruction::BNE:
			case Instruction::BGT:{
				
				currentSection->unprocessedInstructions.push_back({instr, currentSection->locationCounter});
				push32bitsToCodeBigEndian(0);
				break;
			}
			case Instruction::PUSH:{
				// push gpr	-->	sp<=sp-4;	mem32[sp]<=gpr; sp<=sp-4;
				// OCM=0x81 gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];
				// A:sp, B:0, C:gpr, D:-4
				instrWord |= (uint32_t)InstructionCode::PUSH<<24;
				instrWord |= (uint32_t)GPRType::SP << 20; // A=sp
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr
				instrWord |= -4&0xFFF; // D=-4
				push32bitsToCodeBigEndian(instrWord);
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
				push32bitsToCodeBigEndian(instrWord);


				break;
			}
			case Instruction::XCHG:{
				// temp <= gpr1;	gpr1 <= gpr0;	gpr0 <= temp;
				// OC=4 M=0: temp<=gpr[B]; gpr[B]<=gpr[C]; gpr[C]<=temp;
				// A:0, B:gpr1, C:gpr0
				instrWord |= (uint32_t)InstructionCode::XCHG<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[0].gpr << 12; // C=gpr0
				push32bitsToCodeBigEndian(instrWord);
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
				push32bitsToCodeBigEndian(instrWord);
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
				push32bitsToCodeBigEndian(instrWord);
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
				push32bitsToCodeBigEndian(instrWord);
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
				push32bitsToCodeBigEndian(instrWord);
				break;
			}
			case Instruction::NOT:{
				// gpr <= ~gpr;
				// OCM=60 gpr[A]<=~gpr[B];
				// A:gpr, B:gpr
				instrWord |= (uint32_t)InstructionCode::NOT<<24;
				instrWord |= (uint32_t)instr->operands[0].gpr << 20; // A=gpr
				instrWord |= (uint32_t)instr->operands[0].gpr << 16; // B=gpr
				push32bitsToCodeBigEndian(instrWord);

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

				push32bitsToCodeBigEndian(instrWord);
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
				push32bitsToCodeBigEndian(instrWord);
				
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
				push32bitsToCodeBigEndian(instrWord);
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
				
				push32bitsToCodeBigEndian(instrWord);
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
				
				push32bitsToCodeBigEndian(instrWord);
				break;
			}
			case Instruction::LD:
			// ld op,gpr
				currentSection->unprocessedInstructions.push_back({instr, currentSection->locationCounter});
				push32bitsToCode(0);
				switch(instr->operands[0].dataOperand.type){
					case DataOperand::SYM:
					case DataOperand::LIT:
					case DataOperand::REL_GPR_SYM: // 2 instructions needed
						push32bitsToCodeBigEndian(0);
					default:
						break;
				}
				break;
			case Instruction::ST: // TODO trebaju li dve instr?
				currentSection->unprocessedInstructions.push_back({instr, currentSection->locationCounter});
				push32bitsToCodeBigEndian(instrWord);
				break;
			case Instruction::CSRRD:
				// csrrd csr, gpr -> gpr<=csr;
				// OC=0x90 gpr[A]<=csr[B];
				// A:gpr(op1), B:csr(op0)
				instrWord |= (uint32_t)InstructionCode::CSRRD<<24;
				instrWord |= (uint32_t)instr->operands[1].gpr << 20; // A=gpr
				instrWord |= (uint32_t)instr->operands[0].csr << 16; // B=csr
				push32bitsToCodeBigEndian(instrWord);

				break;
			case Instruction::CSRWR:
				// csrwr gpr, csr -> csr<=gpr;
				// OC=0x94 csr[A]<=gpr[B];
				// A:csr(op0), B:gpr(op1)
				instrWord |= (uint32_t)InstructionCode::CSRWR<<24;
				instrWord |= (uint32_t)instr->operands[1].csr << 20; // A=csr
				instrWord |= (uint32_t)instr->operands[0].gpr << 16; // B=gpr
				push32bitsToCodeBigEndian(instrWord);
				break;
			default:
				std::cout<<"Error in line "<<this->inputFileLineNum <<" in instruction type\n";
		}
}

void Assembler::insertLitPool(){
	postProccessInstructions(); // everything that can be resolved by relative adressing will be available, the rest have to use litpool
	// insert jump over litpool
	if(currentSection->litPool.size()==0){
		return; // no litpool
	}else{
		//print litpool
		for(LitPoolElem& el : currentSection->litPool){
			std::cout<<"Litpool elem: "<<el.value<<" at address "<<el.addressOfInstruction<<"\n";
		}
	}
	outTxt<<"Relloc table when inserting litpoool:\n";
	printRelocTable();

	uint32_t dispAferLitPool = currentSection->litPool.size() * 4; // jmp is 4B long, but pc will already be incremented by 4
	if((int32_t)dispAferLitPool>2047){
		std::cout<<"Error: Litpool too big\n";
		exit(1);
		// TODO make sure pol never gets too big, or too far from begginging of section
	}


	uint32_t instrWord=0;
	instrWord |= (uint32_t)InstructionCode::JMP<<24;
	instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
	instrWord |= dispAferLitPool & 0xFFF; // D=disp
	push32bitsToCodeBigEndian(instrWord);

	std::cout<<"Litpool starting from addr:"<<currentSection->locationCounter<<"\n";
	
	//lc<2047
	// insert litpool
	for(LitPoolElem& el : currentSection->litPool){
		int32_t disp = currentSection->locationCounter - (el.addressOfInstruction+4);
		if(disp>2047){
			std::cout<<"Error: Litpool too far from instruction; Instr addr:"<<el.addressOfInstruction<<" LC:"<<currentSection->locationCounter<<"\n";
			exit(1);
		}
		//edit D in instruction
		currentSection->code[el.addressOfInstruction+3] = disp & 0xFF;
		currentSection->code[el.addressOfInstruction+2] |= (disp>>8) & 0xF;

		if(el.type == LitPoolElem::VALUE){
			push32bitsToCode(el.value);
		}
		else { //symbol
			// just add to reloc table, resolve at the end
			currentSection->relocationTable.push_back(RelocTableElem(currentSection->locationCounter, currentSection->sectionName, el.symbolName));
			push32bitsToCode(0);
		}
			
		
	}

	outTxt<<"Reloc tb after insering lipool:\n";
	printRelocTable();

	currentSection->oldLitPools.push_back(currentSection->litPool);
	currentSection->litPool.clear();
}

void Assembler::postProccessInstructions(bool isFinalProcessing){
	for(UnprocessedInstruction& instr : currentSection->unprocessedInstructions){
		Section* section = currentSection;
		uint32_t instrWord=0;
		switch (instr.instruction->type){
		case Instruction::LD:{
			// ld op, gpr -> gpr<=op;
			// OC=0x91 	gpr[A]<=gpr[B]+D; // reg, ?imm lit
			// OC=0x92  gpr[A]<=mem32[gpr[B]+gpr[C]+D]; // [lit] [sym] immLit immSym [reg] [reg+reg] [reg+lit] [reg+sym]
			// A:gpr

			DataOperand& op = instr.instruction->operands[0].dataOperand;
			instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr


			switch(op.type){
				case DataOperand::IMM_LIT:
					std::cout<< "ld sa imm lit: "<<op.literal<<"\n";
					if(op.literal>2047 || op.literal<-2048){
						//litpool
						addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=op.literal}, section, instr.address);
						// LD 0x92 B/C:pc D:later
						instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
						instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
					}else{
						//put in instruction
						// LD 0x91  D:lit
						instrWord |= (uint32_t) InstructionCode::LD << 24;
						instrWord |= op.literal & 0xFFF; // direct adressing; ST OCM=0x91  // LD error
					}
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					break;
				case DataOperand::IMM_SYM:{
					// LD 0x92 B/C:pc D:later

					addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol}, section, instr.address);
					// LD 0x92 B:pc D:later
					instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
					instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);

					break;
				}
				case DataOperand::LIT: {
					// gpr<=mem32[ mem32[pc+Dlit]]

					// gpr<=mem32[ pc+Dlit]
					// LD 0x92 B:pc D:later 
					addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=op.literal }, section, instr.address);
					// LD 0x92 B/C:pc D:later 
					instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
					instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
					//D will be edited
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);

		
					//gpr<=mem32[gpr]
					// LD 0x92  A :gpr B:gpr
					instrWord = 0;
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
					edit32bitsOfCodeBigEndian(*section,instr.address+4, instrWord);
					break;
				}
				case DataOperand::SYM:{

					// gpr<=mem32[ mem32[pc+Dlit]]

					// gpr<=mem32[ pc+Dlit]
					// LD 0x92 B:pc D:later 
					addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
					// LD 0x92 B/C:pc D:later 
					instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
					instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
					//D will be edited
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);

					//gpr<=mem32[gpr]
					// LD 0x92  A :gpr B:gpr
					instrWord = 0;
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
					edit32bitsOfCodeBigEndian(*section,instr.address+4, instrWord);
					break;
				}
				case DataOperand::GPR:{
					// gpr<=op.gpr
					// LD 0x91  B:op
					instrWord |= (uint32_t) InstructionCode::LD << 24;
					instrWord |= (uint32_t)op.gpr <<16; // B=op.gpr
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					break;
				}
				case DataOperand::REL_GPR:{
					// gpr<=mem[op.gpr]
					// LD 0x92  B:op.gpr
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)op.gpr <<16; // B=op.gpr
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					break;
				}
				case DataOperand::REL_GPR_LIT:{
					// gpr<=mem[op.gpr+op.lit]
					// LD 0x92  B:op.gpr
					
					if(op.literal>2047 || op.literal<-2048){
						std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+lit] -> literal out of bounds \n";
						break;
					}

					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)op.gpr <<16; // B=op.gpr
					instrWord |= op.literal & 0xFFF; // D=lit
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);

					/*addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=op.literal }, section, instr.address);
					// LD 0x92 B/C:pc D:later 
					//gpr<=lit
					instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
					instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
					//D will be edited
					edit32bitsOfCode(*section,instr.address, instrWord);

					//gpr<=mem32[gpr(lit)+op.reg]
					// LD 0x92  A :gpr B:gpr
					instrWord = 0;
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
					instrWord |= (uint32_t)op.gpr << 12; // C=op.gpr
					edit32bitsOfCode(*section,instr.address+4, instrWord);*/
					break;
				}
				case DataOperand::REL_GPR_SYM:{

					addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
					// LD 0x92 B/C:pc D:later  => gpr<=mem32[pc + d]
					//gpr=sym
					instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
					instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
					//D will be edited
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);

					//A = mem[gprB(sym)+C]  => gpr<=mem32[gpr(sym) + op.gpr]
					// LD 0x92  A :gpr B:gpr C:
					instrWord = 0;
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
					instrWord |= (uint32_t)op.gpr << 12; // C=op.gpr

					edit32bitsOfCodeBigEndian(*section,instr.address+4, instrWord);

					// check symbol
					/*
					if(symbolTable.find(*(op.symbol))!=symbolTable.end()){
						SymbolTableElem* sym = & (symbolTable[*(op.symbol)]);
						if(sym->type==SymbolType::GLOBAL){

							addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
							// LD 0x92 B/C:pc D:later 
							//gpr=sym
							instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
							instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
							//D will be edited
							edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);

							//gpr<=mem32[gpr(sym) + op.gpr]
							// LD 0x92  A :gpr B:gpr C:
							instrWord = 0;
							instrWord |= (uint32_t) InstructionCode::LD_M << 24;
							instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
							instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
							instrWord |= (uint32_t)op.gpr << 12; // C=op.gpr

							edit32bitsOfCodeBigEndian(*section,instr.address+4, instrWord);

							if(sym->value!=0 || sym->sectionName!=NULL){
								//defined global sym
								// debug upis ovo ostaje u relocu
								;
							}else{
								if(isFinalProcessing) {
									std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> symbol still undefined at end of file\n";
								}/*else{
									leftUnprocessedInstructions.push_back(instr);
								}*//*
							
							}
						}else if(sym->type==SymbolType::EXTERN){
							if(sym->value!=0 || sym->sectionName!=NULL){
								std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> extern symbol\n";
								// TODO da li je ovo greska? Ili treba da se pokrije

							}
						}else if(sym->type==SymbolType::SECTION){
							std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> reloc section symbol\n";
							exit(1);
						}
					}else
					if(section->symbolTable.find(*(op.symbol))!=section->symbolTable.end()){
						SymbolTableElem* sym = & (section->symbolTable[*(op.symbol)]);
						if(sym->value!=0 || sym->sectionName!=NULL){
							//defined local sym
							addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
							// LD 0x92 B/C:pc D:later 
							//gpr=sym
							instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
							instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
							//D will be edited
							edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);

							//gpr<=mem32[gpr(sym) + op.gpr]
							// LD 0x92  A :gpr B:gpr C:
							instrWord = 0;
							instrWord |= (uint32_t) InstructionCode::LD_M << 24;
							instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
							instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
							instrWord |= (uint32_t)op.gpr << 12; // C=op.gpr

							edit32bitsOfCodeBigEndian(*section,instr.address+4, instrWord);
						}else{
							if(isFinalProcessing) std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> local symbol still undefined at the end of file\n";
							else leftUnprocessedInstructions.push_back(instr);
						}
					}else{
						if(isFinalProcessing) std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> undefined symbol\n";
						else leftUnprocessedInstructions.push_back(instr);
					}
					*/
				
					break;
				}
				default:
					break;
			}	

			break;
		}
		case Instruction::ST:{
			// st gpr, op -> op<=gpr;
			// OC=0x80 mem32[gpr[A]+gpr[B]+D]<=gpr[C];
			// OC=0x82  mem32[mem32[gpr[A]+gpr[B]+D]]<=gpr[C];
			DataOperand op = instr.instruction->operands[1].dataOperand;
			instrWord |= (uint32_t)instr.instruction->operands[0].gpr << 12; // C=gpr

			switch(op.type){
				case DataOperand::IMM_LIT:
					std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST with $literal val\n";
					break;
				case DataOperand::IMM_SYM:
					std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST with $symbol val\n";
					break;
				case DataOperand::LIT:{
					// mem32[op.lit]<=gpr

					if(op.literal>2047 || op.literal<-2048){
						// litpool
						addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=op.literal }, section, instr.address);
						// mem[mem[pc+Dlit]]<=gpr
						// ST 0x81  A:pc B:0 C:gpr D:lit 
						instrWord |= (uint32_t) InstructionCode::ST_M << 24;
						instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
						// D will be edited
						edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
						
					}else{
						// mem[op.lit]<=gpr
						// ST 0x80  A:0 B:0 C:gpr D:lit
						instrWord |= (uint32_t) InstructionCode::ST << 24;
						instrWord |= op.literal & 0xFFF; 
						edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					}
					break;
				}
				case DataOperand::SYM:{
					// mem32[op.sym]<=gpr
					// ST 0x81  A:pc B:0 C:gpr D:lit 
					// mem[mem[pc+Dlit]]<=gpr
					addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
					instrWord |= (uint32_t) InstructionCode::ST_M << 24;
					instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
					// D will be edited
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					break;
				}
				case DataOperand::GPR:{
					// op.gpr<=gpr
					// same as LD 0x91 gpr[A]<=gpr[B]+D;
					// A=op.gpr, B=gpr
					instrWord |= 0;
					instrWord |= (uint32_t) InstructionCode::LD << 24;
					instrWord |= (uint32_t)op.gpr <<20; // A=op.gpr
					instrWord |= (uint32_t)instr.instruction->operands[0].gpr <<16; // B=gpr
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					break;
				}
				case DataOperand::REL_GPR:{
					// mem32[op.gpr]<=gpr
					// ST 0x80  A:op.gpr B:0  
					instrWord |= (uint32_t) InstructionCode::ST << 24;
					instrWord |= (uint32_t)op.gpr << 20; // A=op.gpr
					edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					break;
				}
				case DataOperand::REL_GPR_LIT:{
					// mem32[op.gpr+op.lit]<=gpr

					if(op.literal>2047 || op.literal<-2048){
						std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST [reg+lit] -> literal out of bounds \n";
						break;
					}else{
						// mem32[op.gpr+op.lit]<=gpr
						// ST 0x80  A:op.gpr B:0 C:gpr D:lit
						instrWord |= (uint32_t) InstructionCode::ST << 24;
						instrWord |= (uint32_t)op.gpr << 20; // A=op.gpr
						instrWord |= op.literal & 0xFFF; // D=lit
						edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
					}
					break;
				}
				case DataOperand::REL_GPR_SYM:{ // getting sym val as pc + disp
					// mem32[op.reg +pc+Dsym ]<=gpr   // sym=pc+Dsym????
					if(symbolTable.find(*(op.symbol))!=symbolTable.end()){
						SymbolTableElem* sym = & (symbolTable[*(op.symbol)]);
						if(sym->type==SymbolType::GLOBAL){
							if(sym->value!=0 || sym->sectionName!=NULL){
								//defined global sym
								if(sym->sectionName!=section->sectionName){
									std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST [reg+sym] -> global symbol in other section\n";
									break;
								}
								// symbol defined in this section
								int32_t disp = sym->value - (instr.address+4);
								if(disp>2047 || disp<-2048){
									std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST [reg+sym] -> symbol too far from instruction\n";
									break;
								}
								// 0x80  A:op.gpr B:pc C:gpr D:disp
								instrWord |= (uint32_t) InstructionCode::ST << 24;
								instrWord |= (uint32_t)op.gpr << 20; // A=op.gpr
								instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
								instrWord |= disp & 0xFFF; // D=disp
								edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);


								/*addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
								// LD 0x92  gpr[A]<=mem32[gpr[B]+gpr[C]+D];
								// 
								//gpr=sym
								instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
								instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
								//D will be edited
								edit32bitsOfCode(*section,instr.address, instrWord);

								//gpr<=mem32[gpr(sym) + op.gpr]
								// LD 0x92  A :gpr B:gpr C:
								instrWord = 0;
								instrWord |= (uint32_t) InstructionCode::LD_M << 24;
								instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
								instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
								instrWord |= (uint32_t)op.gpr << 12; // C=op.gpr

								edit32bitsOfCode(*section,instr.address+4, instrWord);*/
							}else{
								if(isFinalProcessing)	std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> symbol still undefined at end of file\n";
								else currentSection->relocationTable.push_back(RelocTableElem(RelocTableElem::RELATIVE, instr.address, section->sectionName, op.symbol));
							}
						}else if(sym->type==SymbolType::EXTERN){
							if(sym->value!=0 || sym->sectionName!=NULL)
								std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> extern symbol\n";
						}else if(sym->type==SymbolType::SECTION){
							std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> reloc section symbol\n";
							exit(1);
						}
					}else
					if(section->symbolTable.find(*(op.symbol))!=section->symbolTable.end()){
						SymbolTableElem* sym = & (section->symbolTable[*(op.symbol)]);
						if(sym->value!=0 || sym->sectionName!=NULL){
							//defined local sym
							int32_t disp = sym->value - (instr.address+4);
							if(disp>2047 || disp<-2048){
								std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST [reg+sym] -> symbol too far from instruction\n";
								break;
							}
							// 0x80  A:op.gpr B:pc C:gpr D:disp
							instrWord |= (uint32_t) InstructionCode::ST << 24;
							instrWord |= (uint32_t)op.gpr << 20; // A=op.gpr
							instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
							instrWord |= disp & 0xFFF; // D=disp
							edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);


							/*addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
							// LD 0x92 B/C:pc D:later 
							//gpr=sym
							instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
							instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
							//D will be edited
							edit32bitsOfCode(*section,instr.address, instrWord);

							//gpr<=mem32[gpr(sym) + op.gpr]
							// LD 0x92  A :gpr B:gpr C:
							instrWord = 0;
							instrWord |= (uint32_t) InstructionCode::LD_M << 24;
							instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
							instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
							instrWord |= (uint32_t)op.gpr << 12; // C=op.gpr

							edit32bitsOfCode(*section,instr.address+4, instrWord);*/
						}else{
							if(isFinalProcessing)	std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> local symbol still undefined at end of file\n";
							currentSection->relocationTable.push_back(RelocTableElem(RelocTableElem::RELATIVE, instr.address, section->sectionName, op.symbol));
						}
					}else{
						if(isFinalProcessing)	std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> undefined symbol\n";
						currentSection->relocationTable.push_back(RelocTableElem(RelocTableElem::RELATIVE, instr.address, section->sectionName, op.symbol));
					}
				}
				default:
					break;
			
			}

			break;
		}
		case Instruction::CALL:{
			// push pc; pc<=op;
			// OC=21: push pc; pc<=mem32[gpr[A]+gpr[B]+D];
			// A:pc, B:0, D:disp to lit pool adr of operand

			instrWord = processJumpInstructions(instr.instruction->operands[0],section,instr.address, isFinalProcessing); // sets D and adressing


			if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
				instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::CALL_M<<24;
			}else{
				instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::CALL<<24;
			}

			instrWord |= (uint32_t)GPRType::PC << 20; // A=pc

			edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
			break;
		}
		case Instruction::JMP:{
			// pc<=op;
			// OC=30: pc<=gpr[A]+D; // pc rel
			// OC=31: pc<=mem32[gpr[A]+D]; // pc rel from mem
			// A:pc, D:disp to lit pool adr of operand
			instrWord = processJumpInstructions(instr.instruction->operands[0], section, instr.address, isFinalProcessing); // sets D and adressing

			if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
				instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::JMP_M<<24;
			}else{
				instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::JMP<<24;
			}
			
			instrWord |= (uint32_t)GPRType::PC << 20; // A=pc

			edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
			break;
		}
		case Instruction::BEQ:{
			//  if (gpr1 == gpr2) pc <= operand; 
			// OCM=0x31 if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
			// OCM=0x39 if (gpr[B] == gpr[C]) pc<=mem32[gpr[A]+D];
			// A:pc, B:gpr1, C:gpr2, D:disp to lit pool adr of operand

			instrWord = processJumpInstructions(instr.instruction->operands[2], section, instr.address, isFinalProcessing); // sets D and adressing

			
			if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
				instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::BEQ_M<<24;
			}else{
				instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::BEQ<<24;
			}

			instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
			instrWord |= (uint32_t)instr.instruction->operands[0].gpr << 16; // B=gpr1
			instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 12; // C=gpr2
			edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
			break;
		}
		case Instruction::BNE:{
			//  if (gpr1 != gpr2) pc <= operand; 
			// OCM=0x32 if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
			// OCM=0x3A if (gpr[B] != gpr[C]) pc<=mem32[gpr[A]+D];
			// A:pc, B:gpr1, C:gpr2, D:disp to lit pool adr of operand

			instrWord = processJumpInstructions(instr.instruction->operands[2], section, instr.address, isFinalProcessing); // sets D and adressing

			
			if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
				instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::BNE_M<<24;
			}else{
				instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
				instrWord |= (uint32_t)InstructionCode::BNE<<24;
			}

			instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
			instrWord |= (uint32_t)instr.instruction->operands[0].gpr << 16; // B=gpr1
			instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 12; // C=gpr2
			edit32bitsOfCodeBigEndian(*section,instr.address, instrWord);
			break;
		}
		case Instruction::BGT:{
			//   if (gpr1 signed> gpr2) pc <= operand; 
				// OCM=0x33  if (gpr[B] signed> gpr[C]) pc<=gpr[A]+D;
				// OCM=0x39  if (gpr[B] signed> gpr[C]) pc<=mem32[gpr[A]+D];
				// A:pc, B:gpr1, C:gpr2, D:disp to lit pool adr of operand

				instrWord = processJumpInstructions(instr.instruction->operands[2], section, instr.address, isFinalProcessing); // sets D and adressing

				
				if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
					instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BGT_M<<24;
				}else{
					instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BGT<<24;
				}

				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
				instrWord |= (uint32_t)instr.instruction->operands[0].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 12; // C=gpr2

				edit32bitsOfCodeBigEndian(*section, instr.address, instrWord);
				break;
		}
	
		default:
			break;
		}
	}

	currentSection->unprocessedInstructions.clear();
	
}


void Assembler::endSection(){
	if(currentSection!=NULL){
		
		insertLitPool();	

		currentSection->endAddress=LC;
		currentSection->size=LC-currentSection->startAddress;

		symbolTable[ *(currentSection->sectionName) ].size=currentSection->size;
	}

	
}

void Assembler::endFile(){
	outTxt<<"Reloc table at the end";
	printRelocTable();
	// go through reloc table and ressolve
	for( auto& section: sections){
		for(RelocTableElem& el : section.second.relocationTable){
			//if(el.type==RelocTableElem::VALUE)
				resolveSymbol(el);
		}
		std::vector<int> indexesToErase;

		for(size_t i=0; i< section.second.relocationTable.size();i++){
			if(section.second.relocationTable[i].type==RelocTableElem::INVALID) // only relative will be deleted, bcs they are comepletely resolved here (for st instr)
				indexesToErase.push_back(i);
		}

		for(int i=indexesToErase.size()-1; i>=0; i--){
			section.second.relocationTable.erase(section.second.relocationTable.begin()+indexesToErase[i]);
		}


	}

	// check if all global symbols are defined

}



void Assembler::resolveSymbol(RelocTableElem& el){
	if(symbolTable.find(*(el.symbolName))!=symbolTable.end()){
		SymbolTableElem* sym = & (symbolTable[*(el.symbolName)]);
		if(sym->type==SymbolType::GLOBAL){
			if(sym->value!=0 || sym->sectionName!=NULL){
				//defined global sym
				// debug upis, ovo ostaje u relocu
				if(el.type==RelocTableElem::RELATIVE && *(sym->sectionName)==*(el.sectionName)){ // symbol defined in section of relocation, pc rel is possible at this stage
					addDispToInstruction( (sections[*(el.sectionName)]), el.address, sym->value - (el.address+4));
				}else
				if(el.type==RelocTableElem::VALUE){
					edit32bitsOfCode( (sections[*(el.sectionName)]), el.address, sym->value);
				}
				
			}else{
				std::cout<<"Error: global symbol "<< *(sym->symbolName)<<"still undefined at the end of the file\n";
				
			}
		}else if(sym->type==SymbolType::EXTERN){
			if(sym->value!=0 || sym->sectionName!=NULL)
				std::cout<<"Error: extern symbol "<< *(sym->symbolName)<<"defined inside the file\n";
				
		}else if(sym->type==SymbolType::SECTION){
			std::cout<<"Error: reloc section symbol\n";
			exit(1);
		}
	}else if(sections[*(el.sectionName)].symbolTable.find(*(el.symbolName))!=sections[*(el.sectionName)].symbolTable.end()){
		SymbolTableElem* sym = & (sections[*(el.sectionName)].symbolTable[*(el.symbolName)]);
		if(sym->value!=0 || sym->sectionName!=NULL){
			//defined local sym
			switch(el.type){
				case RelocTableElem::VALUE:
					edit32bitsOfCode( (sections[*(el.sectionName)]), el.address, sym->value);
					std::cout <<"Resolving symbol:"<<*(el.symbolName)<<" with value "<<sym->value<<" at addr:"<<el.address<<"\n";
					el.symbolName = el.sectionName;
					// value of symbol will change in linker (added w section start value)
					break;
				case RelocTableElem::RELATIVE:
					addDispToInstruction( (sections[*(el.sectionName)]), el.address, sym->value - (el.address+4));
					el.symbolName = el.sectionName = NULL;
					el.type = RelocTableElem::INVALID;
					break;
				default:
					std::cout<<"Error: Unknown reloc type\n";
					exit(1);
			}
		}else{
			std::cout<<"Error: local symbol "<<*(sym->symbolName)<<" still undefined at end of file\n";
		}
	}else{
		std::cout<<"Error: Undefined symbol "<<*(el.symbolName)<<"\n";

	}
}

void Assembler::printSymbolTable(){
	outTxt<<"Symbol table:\n";
	SymbolTableElem::printSymbolTableHeader(outTxt);
	for(auto el : symbolTable){
		el.second.printSymbolTableElem(outTxt);
	}
}

void Assembler::printSymbolTable(std::ostream &out){
	out<<"Symbol table:\n";
	SymbolTableElem::printSymbolTableHeader(out);
	for(auto el : symbolTable){
		el.second.printSymbolTableElem(out);
	}
}

void Assembler::printRelocTable(){
	outTxt<<"Relocation table:\n";
	RelocTableElem::printRelocTableHeader(outTxt);
	for(auto section : sections){
		for(RelocTableElem el : section.second.relocationTable){
			el.printRelocTableElem(outTxt);
		}
	}
}

void Assembler::printCode(){
	outTxt<<"Code:\n";
	uint32_t i=0;
	std::stringstream ss;
	while (i<LC){
		for(auto section : sections){
			if(section.second.startAddress==i){
				int j=0;
				int zeroesCount = 0;
				for(uint8_t byte : section.second.code){
					if(byte==0) {
						zeroesCount++;
						if(zeroesCount >20) { ss.clear(); ss.str(""); zeroesCount=0;}
						ss<<i++<<":\t"<<j++<<":\t";
						ss<<std::hex<< std::setw(2) <<std::setfill('0')<<std::setiosflags(std::ios::right)<<(int)byte<<std::setfill(' ')<<"\n"<<std::dec;
					}else{
						if(zeroesCount>0 && zeroesCount<20){
							outTxt<<ss.rdbuf();
							ss.clear();
							ss.str("");
							zeroesCount=0;
						}
						outTxt<<i++<<":\t"<<j++<<":\t";
						outTxt<< std::hex<< std::setw(2)<< std::setfill('0')<<std::setiosflags(std::ios::right)<<(int)byte<<std::setfill(' ')<<"\n"<<std::dec;
					}
					
				}
			}
		}
	}
	outTxt<<"\n";
}

void Assembler::printSections(){
	Section::printSectionHeader(outTxt);
	for(auto sect : sections){
		sect.second.printSection(outTxt);
	}
}

void Assembler::printLitPools(){
	for(auto section : sections){
		outTxt<<"Lit pool for section "<<section.first<<":\n";
		LitPoolElem::printLitPoolHeader(outTxt);
		for(LitPoolElem el : section.second.litPool){
			el.printLitPoolElem(outTxt);
		}
		//outTxt<<"Old lit pools for section "<<section.first<<":\n";
		/*for(auto pool : section.second.oldLitPools){
			LitPoolElem::printLitPoolHeader(outTxt);
			for(LitPoolElem el : pool){
				el.printLitPoolElem(outTxt);
			}
		}*/
	}
	
	
}


void Assembler::printDebug(){
	
}

void Assembler::setOutputFiles(std::string outputFilename){
	outTxt.open((outputFilename+".txt").c_str(), std::ios::out);
	outBin.open((outputFilename+"").c_str(), std::ios::out | std::ios::binary);
	std::cout<<"napravljeni output fajlovi";
}



int main(int argc, char *argv[])
{
	Assembler assembler;
	std::string inputFilename="";
	std::string outputFilename="./outputs/";
	if(argc == 2){
		inputFilename+=argv[1];
		outputFilename+= std::string(argv[1]) + "_assemblyOut";
	}else
	if (argc == 4 || strcmp(argv[1], "-o") == 0)
	{
		inputFilename+=argv[3];
		outputFilename+=argv[2];
	}
	else{
		std::cout << "Invalid number or format of arguments!" << std::endl;
		return 1;
	}
	FILE *inputFile = fopen( inputFilename.c_str() , "r");
	if (!inputFile) {
		std::cout << "can't open file!" << std::endl;
		return -1;
	}
	//std::ofstream outputFile(outputFilename.c_str(), std::ios::out | std::ios::binary);
	yyin = inputFile;

	assembler.setOutputFiles(outputFilename);

	do {
		yyparse(&assembler);
	} while (!feof(yyin));
	//fclose(inputFile);
	std::cout<<"end of file\n";
	
	std::cout<<"output files set, start printing\n";
	assembler.printSymbolTable();
	std::cout<<"\n";
	assembler.printRelocTable();
	std::cout<<"\n";
	assembler.printCode();
	std::cout<<"\n";
	assembler.printSections();
	std::cout<<"\n";
	assembler.printLitPools();

	assembler.closeOutputFile(assembler.getOutTxt());

	

	ObjectFile(assembler).serialize(assembler.getOutBin());
	//assembler.serialize(assembler.getOutBin());
	
	std::cout<<"Serialization completed\n";
	assembler.closeOutputFile(assembler.getOutBin());

/* Testing Deserialization
	std::ifstream inBin("./build/snazzleOut.bin", std::ios::in | std::ios::binary);
	ObjectFile obj(inBin);
	std::cout<<"DeSerialization completed\n";
	//obj.printSymbolTable(std::cout);
	inBin.close();
*/
	
	return 0;
}



Assembler::~Assembler(){
	if(outTxt.is_open())
		outTxt.close();
	if(outBin.is_open())
		outBin.close();
	
}

/*
int main(int, char**) {
  
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
  std::cout<<"\n";
  assembler.printRelocTable();
  std::cout<<"\n";
  assembler.printCode();
  std::cout<<"\n";
  assembler.printSections();
  std::cout<<"\n";
  assembler.printLitPools();
  
}*/

void yyerror(Assembler* assembler,const char *s) {
  std::cout << "parse error on line " << yylineno << "!  Message: " << s << std::endl;
  // might as well halt now:
  exit(-1);
}

void Section::serialize(std::ofstream &out)
{
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

void Section::deserialize(std::ifstream &in)
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

