#include "../inc/Assembler.hpp"
#include "../inc/parser.hpp"
#include "../inc/lexer.hpp"

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
void Assembler::edit32bitsOfCode(Section& s, uint32_t address, uint32_t word){ //little endian
	s.code[address]= (word & 0xFF);
	s.code[address+1]= ((word>>8) & 0xFF);
	s.code[address+2]= ((word>>16) & 0xFF);
	s.code[address+3]= ((word>>24) & 0xFF);
	
}


void Assembler::addDispToInstruction(Section& s, uint32_t address, int32_t disp){
	s.code[address]= (disp & 0xFF);
	s.code[address+1]= ((disp>>8) & 0xF);
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
							}
							//currentSection->relocationTable.push_back({currentSection->locationCounter,
							//		currentSection->sectionName, RelocTableElem::VALUE, sol.symbol });
							
						}else if(currentSection->symbolTable.find(*(sol.symbol))!=currentSection->symbolTable.end()){
							SymbolTableElem* el = & (currentSection->symbolTable[*(sol.symbol)]);
							if(el->value!=0 || el->sectionName!=NULL){ // defined symbol
								sym_val=el->value; //local
							}
						}else{} // not defined symbol, add to relocation table
						currentSection->relocationTable.push_back({currentSection->locationCounter,
									currentSection->sectionName, RelocTableElem::VALUE, sol.symbol });
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


void Assembler::addToLitPool(SymOrLit sol, Section* section=NULL, uint32_t addressOfInstruction=-1){
	section = section==NULL ? currentSection : section;
	addressOfInstruction = addressOfInstruction==(uint32_t)-1 ? section->locationCounter : addressOfInstruction;
	if(sol.type==SymOrLit::SYMBOL){
		// unfinished reloc entry
		section->relocationTable.push_back(
			RelocTableElem{.offset=section->locationCounter,  // instead of this, it will be adress of litpool el
			.sectionName=section->sectionName,
			.type = RelocTableElem::VALUE, // correct, want to just paste the value to lit pool
			.symbolName= sol.symbol }
		);

		// D add to litpool
		section->litPool.push_back(
			LitPoolElem{ 0,
			addressOfInstruction,
			false,
			&(section->relocationTable.back())} 
		);
		
	}else{
		section->litPool.push_back(
			LitPoolElem{(int32_t)sol.literal,
			addressOfInstruction,
			}
		);
		
	}
}
	
	

/**
 * OCM=0 when gpr<=gpr+D; 
 * OCM=1 when gpr<=mem32[gpr+D]; (litpool)
 * D:disp to lit pool adr of operand
 */
uint32_t Assembler::processJumpInstructions(Operand op){
// highest byte set to 0 when adressing is direct(pc rel), 1 when indirect(mem[pc rel])

	uint32_t instrWord=0;

	if(op.type==Operand::LIT){
		if((int32_t)op.literal>2047 || (int32_t)op.literal<-2048 ){ // lit bigger than 12 bits,has to go to litpool
			instrWord |= 1 <<24; // indirect adressing
			addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=op.literal}); //CORECT
			return instrWord;
		}else{
			// lit can be put in instruction
			instrWord |= 0 <<24; // direct adressing
			instrWord |= op.literal & 0xFFF; // D=disp
			return instrWord;
		}
	}else 
	if (op.type==Operand::SYM){
		if( symbolTable.find(*(op.symbol))!=symbolTable.end() ){
			//
			SymbolTableElem* el = & (symbolTable[*(op.symbol)]);
			if(el->type==SymbolType::EXTERN){
				instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
				addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol});
			}else 
			if(el->type==SymbolType::GLOBAL){
				if(el->value!=0 || el->sectionName!=NULL){ 
					//defined global sym
					// now acts as literal
					if(el->sectionName==currentSection->sectionName){
						int32_t disp= el->value - currentSection->locationCounter;
						if(disp>2047 || disp<-2048){
							//disp bigger than 12 bits, has to go to litpool
							instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
							addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=disp}); //CORECT
							return instrWord;
						}else{
							//disp can be put in instruction
							instrWord |= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24;
							instrWord |= disp & 0xFFF; // D=disp
							return instrWord;
						}
					}else{
						//another section global sym, litpool
						instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
						addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol});
					}
				}else{
					//undefined global sym litpool
					instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
					addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol});
				}
			}else 
			if(el->type==SymbolType::SECTION)
				std::cout<<"Error on line"<<this->inputFileLineNum<<": Call to section symbol\n";			
		}else if(currentSection->symbolTable.find(*(op.symbol))!=currentSection->symbolTable.end()){
			//defined local elem
			SymbolTableElem* el = & (currentSection->symbolTable[*(op.symbol)]);
			int32_t disp= el->value - currentSection->locationCounter;
			if(disp>2047 || disp<-2048){
				//disp bigger than 12 bits, has to go to litpool
				instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
				addToLitPool(SymOrLit{.type=SymOrLit::LITERAL, .literal=disp}); // correct, known value
			}else{
				//disp can be put in instruction
				instrWord |= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24;
				instrWord |= disp & 0xFFF; // D= disp
			}

		}else{
			//not found anywhere
			instrWord |= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24;
			addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol});
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

				instrWord = processJumpInstructions(instr->operands[0]); // sets D and adressing

				if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
					instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::CALL_M<<24;
				}else{
					instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::CALL<<24;
				}

				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc

				push32bitsToCode(instrWord);

				//currentSection->unprocessedInstructions.push_back({instr, currentSection->locationCounter});
				//push32bitsToCode(0);

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
			}
			case Instruction::JMP:{
				// pc<=op;
				// OC=30: pc<=gpr[A]+D; // pc rel
				// OC=31: pc<=mem32[gpr[A]+D]; // pc rel from mem
				// A:pc, D:disp to lit pool adr of operand
				instrWord = processJumpInstructions(instr->operands[0]); // sets D and adressing

				if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
					instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::JMP_M<<24;
				}else{
					instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::JMP<<24;
				}
				
				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc

				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::BEQ:{
				//  if (gpr1 == gpr2) pc <= operand; 
				// OCM=0x31 if (gpr[B] == gpr[C]) pc<=gpr[A]+D;
				// OCM=0x39 if (gpr[B] == gpr[C]) pc<=mem32[gpr[A]+D];
				// A:pc, B:gpr1, C:gpr2, D:disp to lit pool adr of operand

				instrWord = processJumpInstructions(instr->operands[2]); // sets D and adressing
				if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
					instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BEQ_M<<24;
				}else{
					instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BEQ<<24;
				}

				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
				instrWord |= (uint32_t)instr->operands[0].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 12; // C=gpr2
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::BNE:{
				//  if (gpr1 != gpr2) pc <= operand; 
				// OCM=0x32 if (gpr[B] != gpr[C]) pc<=gpr[A]+D;
				// OCM=0x3A if (gpr[B] != gpr[C]) pc<=mem32[gpr[A]+D];
				// A:pc, B:gpr1, C:gpr2, D:disp to lit pool adr of operand

				instrWord = processJumpInstructions(instr->operands[2]); // sets D and adressing
				if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
					instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BNE_M<<24;
				}else{
					instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BNE<<24;
				}

				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
				instrWord |= (uint32_t)instr->operands[0].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 12; // C=gpr2
				push32bitsToCode(instrWord);
				break;
			}
			case Instruction::BGT:{
				//   if (gpr1 signed> gpr2) pc <= operand; 
				// OCM=0x33  if (gpr[B] signed> gpr[C]) pc<=gpr[A]+D;
				// OCM=0x39  if (gpr[B] signed> gpr[C]) pc<=mem32[gpr[A]+D];
				// A:pc, B:gpr1, C:gpr2, D:disp to lit pool adr of operand

				instrWord = processJumpInstructions(instr->operands[2]); // sets D and adressing
				if( instrWord>>24 == (uint32_t)InstructionCode::INDIRECT_ADDRESSING){
					instrWord ^= (uint32_t)InstructionCode::INDIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BGT_M<<24;
				}else{
					instrWord ^= (uint32_t)InstructionCode::DIRECT_ADDRESSING<<24; // clear adressing
					instrWord |= (uint32_t)InstructionCode::BGT<<24;
				}

				instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
				instrWord |= (uint32_t)instr->operands[0].gpr << 16; // B=gpr1
				instrWord |= (uint32_t)instr->operands[1].gpr << 12; // C=gpr2
				push32bitsToCode(instrWord);
				break;
			}
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
			}
			case Instruction::LD:
			// ld op,gpr
				currentSection->unprocessedInstructions.push_back({instr, currentSection->locationCounter});
				push32bitsToCode(0);
				switch(instr->operands[0].dataOperand.type){
					case DataOperand::SYM:
					case DataOperand::LIT:
					case DataOperand::REL_GPR_SYM: // 2 instructions needed
						push32bitsToCode(0);
					default:
						break;
				}
				break;
			case Instruction::ST:
				currentSection->unprocessedInstructions.push_back({instr, currentSection->locationCounter});
				push32bitsToCode(instrWord);
				break;
			case Instruction::CSRRD:
				// csrrd csr, gpr -> gpr<=csr;
				// OC=0x90 gpr[A]<=csr[B];
				// A:gpr, B:csr
				instrWord |= (uint32_t)InstructionCode::CSRRD<<24;
				instrWord |= (uint32_t)instr->operands[0].gpr << 20; // A=gpr
				instrWord |= (uint32_t)instr->operands[1].csr << 16; // B=csr
				push32bitsToCode(instrWord);

				break;
			case Instruction::CSRWR:
				// csrwr gpr, csr -> csr<=gpr;
				// OC=0x94 csr[A]<=gpr[B];
				// A:csr, B:gpr
				instrWord |= (uint32_t)InstructionCode::CSRWR<<24;
				instrWord |= (uint32_t)instr->operands[1].csr << 20; // A=csr
				instrWord |= (uint32_t)instr->operands[0].gpr << 16; // B=gpr
				push32bitsToCode(instrWord);
				break;
			default:
				std::cout<<"Error in line "<<this->inputFileLineNum <<" in instruction type\n";
		}
}

void Assembler::insertLitPool(){
	// insert jump over litpool
	uint32_t dispAferLitPool = 4/*jmp*/ + currentSection->litPool.size() * 4;
	if((int32_t)dispAferLitPool>2047){
		std::cout<<"Error: Litpool too big\n";
		exit(1);
		// TODO make sure pol never gets too big, or too far from begginging of section
	}


	uint32_t instrWord=0;
	instrWord |= (uint32_t)InstructionCode::JMP<<24;
	instrWord |= (uint32_t)GPRType::PC << 20; // A=pc
	instrWord |= dispAferLitPool & 0xFFF; // D=disp
	push32bitsToCode(instrWord);
	
	//lc<2047
	// insert litpool
	for(LitPoolElem& el : currentSection->litPool){
		int32_t disp = currentSection->locationCounter - el.addressOfInstruction;
		if(disp>2047){
			std::cout<<"Error: Litpool too far from instruction\n";
			exit(1);
		}
		//edit D in instruction
		currentSection->code[el.addressOfInstruction] = disp & 0xFF;
		currentSection->code[el.addressOfInstruction+1] = (disp>>8) & 0xF;

		if(el.resolved){
			push32bitsToCode(el.value);
		}
		else if(el.reloc!=NULL && el.reloc->type==RelocTableElem::VALUE){
			el.reloc->offset = currentSection->locationCounter;
			el.reloc->sectionName = currentSection->sectionName;
			push32bitsToCode(0);// process in reloc
		}
		
	}
}

void Assembler::postProccessInstructions(){
	for(UnprocessedInstruction& instr : currentSection->unprocessedInstructions){
		Section* section = currentSection;
		switch (instr.instruction->type){
		case Instruction::LD:{
			// ld op, gpr -> gpr<=op;
			// OC=0x91 	gpr[A]<=gpr[B]+D; // reg, ?imm lit
			// OC=0x92  gpr[A]<=mem32[gpr[B]+gpr[C]+D]; // [lit] [sym] immLit immSym [reg] [reg+reg] [reg+lit] [reg+sym]
			// A:gpr

			DataOperand& op = instr.instruction->operands[0].dataOperand;
			uint32_t instrWord=0;
			instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr


			switch(op.type){
				case DataOperand::IMM_LIT:
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
					edit32bitsOfCode(*section,instr.address, instrWord);
					break;
				case DataOperand::IMM_SYM:{
					// LD 0x92 B/C:pc D:later

					addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, .symbol=op.symbol}, section, instr.address);
					// LD 0x92 B:pc D:later
					instrWord |= (uint32_t) InstructionCode::LD_M << 24; 
					instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
					edit32bitsOfCode(*section,instr.address, instrWord);

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
					edit32bitsOfCode(*section,instr.address, instrWord);

					//gpr<=mem32[gpr]
					// LD 0x92  A :gpr B:gpr
					instrWord = 0;
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
					edit32bitsOfCode(*section,instr.address+4, instrWord);
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
					edit32bitsOfCode(*section,instr.address, instrWord);

					//gpr<=mem32[gpr]
					// LD 0x92  A :gpr B:gpr
					instrWord = 0;
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 20; // A=gpr
					instrWord |= (uint32_t)instr.instruction->operands[1].gpr << 16; // B=gpr
					edit32bitsOfCode(*section,instr.address+4, instrWord);
					break;
				}
				case DataOperand::GPR:{
					// gpr<=op.gpr
					// LD 0x91  B:op
					instrWord |= (uint32_t) InstructionCode::LD << 24;
					instrWord |= (uint32_t)op.gpr <<16; // B=op.gpr
					edit32bitsOfCode(*section,instr.address, instrWord);
					break;
				}
				case DataOperand::REL_GPR:{
					// gpr<=mem[op.gpr]
					// LD 0x92  B:op.gpr
					instrWord |= (uint32_t) InstructionCode::LD_M << 24;
					instrWord |= (uint32_t)op.gpr <<16; // B=op.gpr
					edit32bitsOfCode(*section,instr.address, instrWord);
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
					edit32bitsOfCode(*section,instr.address, instrWord);

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
					// check symbol
					if(symbolTable.find(*(op.symbol))!=symbolTable.end()){
						SymbolTableElem* sym = & (symbolTable[*(op.symbol)]);
						if(sym->type==SymbolType::GLOBAL){
							if(sym->value!=0 || sym->sectionName!=NULL){
								//defined global sym
								// debug upis ovo ostaje u relocu
								addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
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

								edit32bitsOfCode(*section,instr.address+4, instrWord);
							}else{
								std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> symbol still undefined at end of file\n";
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
							addToLitPool(SymOrLit{.type=SymOrLit::SYMBOL, op.symbol }, section, instr.address);
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

							edit32bitsOfCode(*section,instr.address+4, instrWord);
						}else{
							std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> local symbol still undefined at end of file\n";
						}
					}else{
						std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> undefined symbol\n";
					}

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
			uint32_t instrWord=0;
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
						edit32bitsOfCode(*section,instr.address, instrWord);
						
					}else{
						// mem[op.lit]<=gpr
						// ST 0x80  A:0 B:0 C:gpr D:lit
						instrWord |= (uint32_t) InstructionCode::ST << 24;
						instrWord |= op.literal & 0xFFF; 
						edit32bitsOfCode(*section,instr.address, instrWord);
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
					edit32bitsOfCode(*section,instr.address, instrWord);
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
					edit32bitsOfCode(*section,instr.address, instrWord);
					break;
				}
				case DataOperand::REL_GPR:{
					// mem32[op.gpr]<=gpr
					// ST 0x80  A:op.gpr B:0  
					instrWord |= (uint32_t) InstructionCode::ST << 24;
					instrWord |= (uint32_t)op.gpr << 20; // A=op.gpr
					edit32bitsOfCode(*section,instr.address, instrWord);
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
						edit32bitsOfCode(*section,instr.address, instrWord);
					}
					break;
				}
				case DataOperand::REL_GPR_SYM:{ // getting sym val as pc + disp
					// mem32[op.reg +pc+Dsym ]<=gpr   // sym=pc+Dsym
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
								int32_t disp = sym->value - instr.address;
								if(disp>2047 || disp<-2048){
									std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST [reg+sym] -> symbol too far from instruction\n";
									break;
								}
								// 0x80  A:op.gpr B:pc C:gpr D:disp
								instrWord |= (uint32_t) InstructionCode::ST << 24;
								instrWord |= (uint32_t)op.gpr << 20; // A=op.gpr
								instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
								instrWord |= disp & 0xFFF; // D=disp
								edit32bitsOfCode(*section,instr.address, instrWord);


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
								std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> symbol still undefined at end of file\n";
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
							int32_t disp = sym->value - instr.address;
							if(disp>2047 || disp<-2048){
								std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": ST [reg+sym] -> symbol too far from instruction\n";
								break;
							}
							// 0x80  A:op.gpr B:pc C:gpr D:disp
							instrWord |= (uint32_t) InstructionCode::ST << 24;
							instrWord |= (uint32_t)op.gpr << 20; // A=op.gpr
							instrWord |= (uint32_t)GPRType::PC << 16; // B=pc
							instrWord |= disp & 0xFFF; // D=disp
							edit32bitsOfCode(*section,instr.address, instrWord);


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
							std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> local symbol still undefined at end of file\n";
						}
					}else{
						std::cout<<"Error in section "<<*(section->sectionName)<<" at adress "<<instr.address<<": LD [reg+sym] -> undefined symbol\n";
					}
				}
			}

			break;
		}
		default:
			break;
		}
	}
}

uint32_t Assembler::processDataOperand(DataOperand op, Section* sec, uint32_t address){
	uint32_t instrWord=0;
	
}

void Assembler::endSection(){
	if(currentSection!=NULL){
		currentSection->endAddress=LC;
		currentSection->size=LC-currentSection->startAddress;

		symbolTable[ *(currentSection->sectionName) ].size=currentSection->size;

		postProccessInstructions();
		insertLitPool();	
	}

	
}

void Assembler::endFile(){

	// go through reloc table and ressolve
	for( auto& section: sections){
		for(RelocTableElem& el : section.second.relocationTable){
			//if(el.type==RelocTableElem::VALUE)
				resolveSymbol(el);

			
		}
		for(int i=0;i< section.second.relocationTable.size();i++){
			if(section.second.relocationTable[i].type==RelocTableElem::INVALID){
				section.second.relocationTable.erase(section.second.relocationTable.begin()+i);
				i--;
			}
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
				// debug upis ovo ostaje u relocu
				if(*(sym->sectionName)==*(el.sectionName)){ // symbol defined in section of relocation, pc rel is possible at this stage

				}
				addDispToInstruction( (sections[*(el.sectionName)]), el.offset, sym->value);
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
					edit32bitsOfCode( (sections[*(el.sectionName)]), el.offset, sym->value);
					el.type = RelocTableElem::INVALID;
					break;
				case RelocTableElem::PCREL://TODO false value, need to change operations too	
					addDispToInstruction( (sections[*(el.sectionName)]), el.offset, sym->value - el.offset);
					//el.type = RelocTableElem::INVALID;
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
	std::cout<<"Symbol table:\n";
	SymbolTableElem::printSymbolTableHeader();
	for(auto el : symbolTable){
		el.second.printSymbolTableElem();
	}
}

void Assembler::printRelocTable(){
	std::cout<<"Relocation table:\n";
	RelocTableElem::printRelocTableHeader();
	for(auto section : sections){
		for(RelocTableElem el : section.second.relocationTable){
			el.printRelocTableElem();
		}
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

void Assembler::printLitPools(){
	for(auto section : sections){
		std::cout<<"Lit pool for section "<<section.first<<":\n";
		LitPoolElem::printLitPoolHeader();
		for(LitPoolElem el : section.second.litPool){
			el.printLitPoolElem();
		}
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
  std::cout<<"\n";
  assembler.printRelocTable();
  std::cout<<"\n";
  assembler.printCode();
  std::cout<<"\n";
  assembler.printSections();
  std::cout<<"\n";
  assembler.printLitPools();
  
}

void yyerror(Assembler* assembler,const char *s) {
  std::cout << "parse error on line " << yylineno << "!  Message: " << s << std::endl;
  // might as well halt now:
  exit(-1);
}
