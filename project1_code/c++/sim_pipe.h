#ifndef SIM_PIPE_H_
#define SIM_PIPE_H_

#define NUM_REGS 32 // 32 general-purpose registers

#include <stdio.h>
#include <string>

using namespace std;

#define PROGRAM_SIZE 50

#define UNDEFINED 0xFFFFFFFF //used to initialize the registers
#define NUM_SP_REGISTERS 9
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 16 
#define NUM_STAGES 5

typedef enum {PC, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

typedef enum {LW, SW, ADD, ADDI, SUB, SUBI, XOR, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP} opcode_t;

typedef enum {IF, ID, EXE, MEM, WB} stage_t;

typedef enum {IF_ID, ID_EXE, EXE_MEM, MEM_WB} pipelinestage_t;

/*
Instruction encoding:
ADD <dest> <src1> <src2>
ADDI <dest> <src1> <immediate>
LW <dest> <immediate>(<src1>)
SW <src2> <immediate>(<src1>)
BRANCH <src1> <immediate>
*/

typedef struct{
        opcode_t opcode; //opcode
        unsigned src1; //source register #1 - see instruction encoding above 
        unsigned src2; //source register #2 - see instruction encoding above
        unsigned dest; //destination register
        unsigned immediate; //immediate field
        string label; //for conditional branches, label of the target instruction - used only for parsing/debugging purposes
} instruction_t; //data structure that defines the format of the instruction - when the parser passes the file, it passes it into another array of instructions

class sim_pipe{

        //instruction memory - models the part of the memory that contains the instruction
		//an array of intruction_t data type
        instruction_t instr_memory[PROGRAM_SIZE];

        //base address in the instruction memory where the program is loaded
        unsigned instr_base_address;

	//data memory - should be initialize to all 0xFF
	unsigned char *data_memory;

	//memory size in bytes
	unsigned data_memory_size;
	
	//memory latency in clock cycles
	unsigned data_memory_latency; // there will be some data structure modeling data memory latency

	//statistics
	unsigned clock_cycles;
	unsigned stalls;
	unsigned instructions_executed;

	/* registers */
    
	unsigned regs[NUM_REGS];

	unsigned ProgramCount;

	bool is_stall;

	struct PipelineStage {
		
		unsigned pc;
		unsigned npc;
		unsigned a;
		unsigned b;
		unsigned imm;
		unsigned lmd;
		unsigned alu_out;
		unsigned cond;

	};

	PipelineStage pipelineRegisters[NUM_STAGES-1];

	// IR is stored using the instruction_t data type
	instruction_t ir[NUM_STAGES-1];

public:

	//instantiates the simulator with a data memory of given size (in bytes) and latency (in clock cycles)
	sim_pipe(unsigned data_mem_size, unsigned data_mem_latency);
	
	//de-allocates the simulator
	~sim_pipe();

	//loads the assembly program in file "filename" in instruction memory at the specified address
	void load_program(const char *filename, unsigned base_address=0x0);

	//runs the simulator for "cycles" clock cycles (run the program to completion if cycles=0) 
	void run(unsigned cycles=0);
	
	//resets the state of the simulator
        /* Note: 
	   - registers should be reset to UNDEFINED value 
	   - data memory should be reset to all 0xFF values
	*/
	void reset();

	// returns value of the specified special purpose register for a given stage (at the "entrance" of that stage)
        // if that special purpose register is not used in that stage, returns UNDEFINED
        // this function does *not* apply to IR (since IR is encoded as instruction_t)
        //
        // Examples:
        // - get_sp_register(PC, IF) returns the value of PC
        // - get_sp_register(NPC, ID) returns the value of IF/ID.NPC
        // - get_sp_register(NPC, EX) returns the value of ID/EX.NPC
        // - get_sp_register(ALU_OUTPUT, MEM) returns the value of EX/MEM.ALU_OUTPUT
        // - get_sp_register(ALU_OUTPUT, WB) returns the value of MEM/WB.ALU_OUTPUT
	// - get_sp_register(LMD, ID) returns UNDEFINED
	unsigned get_sp_register(sp_register_t reg, stage_t stage);

	//returns value of the specified general purpose register
	int get_gp_register(unsigned reg);

	// set the value of the given general purpose register to "value"
	void set_gp_register(unsigned reg, int value);

	//returns the IPC
	float get_IPC();

	//returns the number of instructions fully executed
	unsigned get_instructions_executed();

	//returns the number of clock cycles 
	unsigned get_clock_cycles();

	//returns the number of stalls added by processor
	unsigned get_stalls();

	//prints the content of the data memory within the specified address range
	void print_memory(unsigned start_address, unsigned end_address);

	// writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness)
	void write_memory(unsigned address, unsigned value);

	//prints the values of the registers 
	void print_registers();

	void instruction_fetch();

	void instruction_decode();

	void execute_stage();

	void memory_stage();
	
	void write_back();

	void pipe_flush();

};

#endif /*SIM_PIPE_H_*/
