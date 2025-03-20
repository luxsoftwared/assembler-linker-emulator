# Abstract System Toolchain

## Project Overview
This project implements a toolchain for an abstract computing system, including an assembler, linker, and emulator. The tools allow for assembling and linking programs written in a custom assembly
language and then executing them on an emulator simulating the target system. 

## Features
- **Assembler**:
  - Translates assembly source code into an object file.
  - Supports symbolic labels, data definitions, and basic directives.
  - Implements single-pass assembly for efficient translation.
  - Accepts command-line options for output file specification.
  
- **Linker**:
  - Merges multiple object files into a final executable format.
  - Resolves external symbols and relocates addresses as needed.
  - Supports section placement via command-line arguments.
  - Generates output in a simplified ELF-like format for ease of parsing.
  
- **Emulator**:
  - Simulates the execution of the assembled program on the abstract computing system.
  - Implements instruction decoding, memory management, and register emulation.
  - Supports terminal I/O and timer-based interrupts.
  - Outputs processor state after execution for debugging and analysis.

## System Requirements
- **Operating System**: Linux (execution and defense on Windows is not supported)
- **Programming Languages**: C/C++
- **Development Tools**:
  - GNU Compiler Collection (GCC)
  - Flex and Bison (for lexical and syntactic analysis)
  - Standard Template Library (STL) for data structures

# Usage
## Assembler
Compile the assembler:
```
make assembler
```
Compile an assembly file into an object file:
```
./assembler -o output.o input.s
```



## Linker
Compile the linker:
```
make linker
```


### Command-Line Options

 `./linker [options] <input_file_1> <input_file_2> ...`

#### Available options:

- `-o <output_file>`
  
  Specifies the name of the output file.
  
  **Example:**
  ```
  ./linker -o program.hex input1.o input2.o
  ```

- `-hex`
  
  Instructs the linker to generate a memory initialization file in a human-readable format.
  
  **Example:**
  ```
  ./linker -hex -o program.hex input1.o input2.o
  ```

- `-place=<section_name>@<address>`
  
  Explicitly places a specific section at a given memory address.  
  Can be used multiple times for different sections.
  
  **Example:**
  ```
  ./linker -hex -place=text@0x40000000 -place=data@0x4000F000 -o program.hex input1.o input2.o
  ```

- `-relocatable` (not implemented yet)
  
  Generates an object file that can later be linked again instead of a final executable.
  
  **Example:**
  ```
  ./linker -relocatable -o linked.o input1.o input2.o
  ```

### Example Usage

```
./linker -hex -place=text@0x40000000 -place=data@0x4000F000 -o program.hex input1.o input2.o
```

This command:
- Generates a memory initialization file (`program.hex`).
- Places the `.text` section at `0x40000000`.
- Places the `.data` section at `0x4000F000`.
- Links `input1.o` and `input2.o` into the final executable.




## Emulator
Compile the emulator:
```
make emulator
```
Execute the assembled program:
```
./emulator program.hex
```



## Example
Here is a simple example of assembling, linking, and running a program:
```
./assembler -o handler.o handler.s
./assembler -o main.o main.s
./linker -hex -place=my_code_main@0x40000000 -place=my_code_handler@0xC0000000 -o program.hex handler.o main.o
./emulator program.hex
```
