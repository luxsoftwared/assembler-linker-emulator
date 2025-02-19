#include "../inc/Emulator.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

Emulator::Emulator(char* filename) : programCounter(START_ADDRESS) {
	// try to open the file
	std::ifstream inTxt(filename, std::ios::in);
	if(!inTxt.is_open()){
		std::cerr<<"ERROR: could not open file:"<<filename<<"\n";
		return;
	}

	// load the program; read the file, initialize the memory
	std::string line;
	
	while(std::getline(inTxt, line)){
		uint32_t address;

		std::string addressStr = line.substr(0, 8);
		std::string valuesStr = line.substr(10);

		address = std::stoul(addressStr, nullptr, 16);
		for(int i=0; i<8; i++){
			uint8_t value = std::stoul(valuesStr.substr(i*3, 2), nullptr, 16);
			memory[address++] = value;
		}
	}
	inTxt.close();

	// init registers
	registers[PC] = START_ADDRESS;
	registers[SP] = STACK_START_ADDRESS;


}

Emulator::~Emulator() {
	
}



void Emulator::run() {
	bool running = true;
	while (running) {
		programCounter = registers[PC];
		registers[PC]+=4;
		running = executeInstruction(programCounter);
	}
}




bool Emulator::executeInstruction(uint32_t address) {
	//first byte, oc&mod
	uint8_t byte = memory[address++];
	uint8_t opcode = (byte & 0xF0) >> 4;
	uint8_t mod = byte & 0x0F;
	//second byte, regA&regB
	byte = memory[address++];
	uint8_t regA = (byte & 0xF0) >> 4;
	uint8_t regB = byte & 0x0F;
	//third byte, regC&disp
	byte = memory[address++];
	uint8_t regC = (byte & 0xF0) >> 4;
	uint8_t disp1 = byte & 0x0F;
	//fourth byte, disp
	byte = memory[address++];
	uint8_t disp2 = byte;
	int16_t disp = (disp1 << 8) | disp2;
	if(disp & 0x800){ // sign extend
		disp |= 0xF000;
	}


	switch (opcode<<4 | mod) {
		case InstructionCode::HALT: 
			std::cout<<"Emulated processor executed halt instruction\n";
			printRegisters();
			return false;
			break;
		case InstructionCode::INT: 
			// push pc; push status; cause=4; status=mask; pc=handler;
			// ide prvo push pc, iako tekst govori drugacije, jer mora pc na kraju da se restaurira 
			pushToStack32(registers[PC]);
			pushToStack32(csr[STATUS]);
			csr[CAUSE] = 4;
			csr[STATUS] &= ~0x1;
			registers[PC] = csr[HANDLER];
			break;
		case InstructionCode::IRET: 
			// inst code 0x97; just pop status;
			// pop pc is done with separate instruction;
			csr[STATUS] = popFromStack32();
			break;
		case InstructionCode::CALL: //relative jump
			// push pc; pc<=pc+D;
			pushToStack32(registers[PC]);
			registers[PC] = registers[regA] + registers[regB] + disp;
			break;
		case InstructionCode::CALL_M:
			// push pc; pc<=mem32[pc+D]; adr of jump is in lit pool
			registers[SP] -= 4;
			memorySet32(registers[SP], registers[PC]); //memory[registers[SP]] = registers[PC];
			//registers[PC] = memory[registers[regA] + registers[regB] + disp];
			registers[PC] = memoryGet32(registers[regA] + registers[regB] + disp);
			break;
		case InstructionCode::RET: // ovo ne postoji nigde, zamenio sam sa pop
			//registers[PC] = memory[registers[SP]];
			registers[PC] = popFromStack32();
			break;
		case InstructionCode::JMP:
			registers[PC] = registers[regA] + disp;
			break;
		case InstructionCode::JMP_M:
			registers[PC] = memoryGet32(registers[regA]+disp);//memory[registers[regA] + disp];
			break;
		case InstructionCode::BEQ:
			if (registers[regB] == registers[regC]) {
				registers[PC] = registers[regA] + disp;
			}
			break;
		case InstructionCode::BEQ_M:
			if (registers[regB] == registers[regC]) {
				registers[PC] = memoryGet32(registers[regA]+disp);//memory[registers[regA] + disp];
			}
			break;
		case InstructionCode::BNE:
			if (registers[regB] != registers[regC]) {
				registers[PC] = registers[regA] + disp;
			}
			break;
		case InstructionCode::BNE_M:
			if (registers[regB] != registers[regC]) {
				registers[PC] = memoryGet32(registers[regA]+disp);//memory[registers[regA] + disp];
			}
			break;
		case InstructionCode::BGT:
			if ((int32_t)registers[regB] > (int32_t)registers[regC]) {
				registers[PC] = registers[regA] + disp;
			}
			break;
		case InstructionCode::BGT_M:
			if ((int32_t)registers[regB] > (int32_t)registers[regC]) {
				registers[PC] = memoryGet32(registers[regA]+disp);//memory[registers[regA] + disp];
			}
			break;
		case InstructionCode::PUSH:
			registers[regA] = registers[regA] + disp;
			memorySet32(registers[regA], registers[regC]);

			break;
		case InstructionCode::POP:
			registers[regA] = memoryGet32(registers[regB]);
			registers[regB] += disp;
			break;
		case InstructionCode::XCHG:{
			uint32_t temp = registers[regB];
			registers[regB] = registers[regC];
			registers[regC] = temp;
			break;
		}
		case InstructionCode::ADD:
			registers[regA] = registers[regB] + registers[regC];
			break;
		case InstructionCode::SUB:
			registers[regA] = registers[regB] - registers[regC];
			break;
		case InstructionCode::MUL:
			registers[regA] = registers[regB] * registers[regC];
			break;
		case InstructionCode::DIV:	
			registers[regA] = registers[regB] / registers[regC];
			break;
		case InstructionCode::NOT:	
			registers[regA] = ~registers[regB];
			break;
		case InstructionCode::AND:
			registers[regA] = registers[regB] & registers[regC];
			break;
		case InstructionCode::OR:
			registers[regA] = registers[regB] | registers[regC];
			break;
		case InstructionCode::XOR:
			registers[regA] = registers[regB] ^ registers[regC];
			break;
		case InstructionCode::SHL:	
			registers[regA] = registers[regB] << registers[regC];
			break;	
		case InstructionCode::SHR:
			registers[regA] = registers[regB] >> registers[regC];
			break;	
		case InstructionCode::LD:
			registers[regA] = registers[regB] + disp;
			break;
		case InstructionCode::LD_M:
			registers[regA] = memoryGet32(registers[regB]+registers[regC]+disp); //memory[memory[registers[regB] + disp]];
			break;
		case InstructionCode::ST:	
			//memory[registers[regB] + disp] = registers[regA];
			memorySet32(registers[regA]+registers[regB]+disp, registers[regC]);
			break;
		case InstructionCode::ST_M:{
			uint32_t addr = memoryGet32(registers[regA]+registers[regB]+disp);
			memorySet32(addr, registers[regC]);
			break;
		}
		case InstructionCode::CSRRD:	
			registers[regA] = csr[regB];
			break;
		case InstructionCode::CSRWR:	
			csr[regA] = registers[regB];
			break;
		default:
			std::cerr << "Unknown opcode: " << std::hex << static_cast<int>(opcode) << std::endl;
			return false;
			break;
	}
	return true;
}

void Emulator::printRegisters()
{
	std::cout<<"Emulated processor state: \n";
	for(int i=0; i<16; i++){
		std::cout<<"r"<<i<<"=0x"<< std::setw(8) << std::hex <<registers[i];
		if(i%4 == 3)
			std::cout<<"\n";
		else
			std::cout<<"\t";
	}
}

void Emulator::pushToStack32(uint32_t value)
{
	registers[SP] -= 4;
	memory[registers[SP]] = value & 0xFF;
	memory[registers[SP]+1] = (value>>8) & 0xFF;
	memory[registers[SP]+2] = (value>>16) & 0xFF;
	memory[registers[SP]+3] = (value>>24) & 0xFF;
}

uint32_t Emulator::popFromStack32(){
	uint32_t value = memory[registers[SP]] | (memory[registers[SP]+1]<<8) | (memory[registers[SP]+2]<<16) | (memory[registers[SP]+3]<<24);
	registers[SP] += 4;
	return value;
}

void Emulator::memorySet32(uint32_t address, uint32_t value)
{
	memory[address] = value & 0xFF;
	memory[address+1] = (value>>8) & 0xFF;
	memory[address+2] = (value>>16) & 0xFF;
	memory[address+3] = (value>>24) & 0xFF;
}

uint32_t Emulator::memoryGet32(uint32_t address)
{
	uint32_t value = memory[address] | (memory[address+1]<<8) | (memory[address+2]<<16) | (memory[address+3]<<24);
	return value;
}

int main(int argc, char *argv[])
{
	
	if(argc != 2 ){
		std::cerr<<"ERROR: invalid number of arguments\n";
		std::cerr<<"Usage: "<<argv[0]<<" <filename>\n";
		std::cerr<<"<filename> - path to the file with the program in hex format\n";
		return -1;
	}
	Emulator emulator(argv[1]);
	emulator.run();
	return 0;
}