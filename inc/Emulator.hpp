#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include <cstdint>
#include <vector>
#include <map>

#define START_ADDRESS 0x40000000
#define STACK_START_ADDRESS 0xFFFFFF00
// 0xFFFFFF00- 0xFFFFFFFF reserved
#define PC 15
#define SP 14
#define STATUS 0
#define HANDLER 1
#define CAUSE 2

class Emulator {
public:
	Emulator(char* filename);
	~Emulator();

	void loadProgram(const std::vector<uint8_t>& program);
	void run();

private:
	/**Returns false in case of error in instruction, or in case of HALT instruction, ending the execution */
	bool executeInstruction(uint32_t address);

	std::map<uint32_t, uint8_t> memory;
	uint32_t registers[16] = {0};
	// r15 - PC (address of next instruction)
	// r14 - SP (stack pointer; pointing to last occupied address; stack grows downwards)
	uint32_t programCounter; // adress of current instruction
	uint32_t csr[3] = {0}; // status, handler, cause


	void printRegisters();
	void pushToStack32(uint32_t value);
	uint32_t popFromStack32();
	void memorySet32(uint32_t address, uint32_t value);
	uint32_t memoryGet32(uint32_t address);
};

namespace InstructionCode{
	enum InstructionCode : uint8_t {
		HALT=0x00,
		INT=0x10,
		IRET=0x97,
		CALL=0x20, // pc+D
		CALL_M=0x21, // lit pool m[pc+D]
		RET, //* nema kod
		JMP=0x30,
		JMP_M=0x38,
		BEQ=0x31,
		BEQ_M=0x39,
		BNE=0x32,
		BNE_M=0x3A,
		BGT=0x33,
		BGT_M=0x3B,
		PUSH=0x81,
		POP=0x93,
		XCHG=0x40,
		ADD=0x50,
		SUB=0x51,
		MUL=0x52,
		DIV=0x53,
		NOT=0x60,
		AND=0x61,
		OR=0x62,
		XOR=0x63,
		SHL=0x70,
		SHR=0x71,
		LD=0x91,
		LD_M=0x92,
		ST=0x80,
		ST_M=0x82,
		CSRRD=0x90,
		CSRWR=0x94, INVALID
	};
};


#endif // EMULATOR_HPP