
//#define DEBUG
#include "sim_pipe.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <map>

using namespace std;

//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};
static const char *instr_names[NUM_OPCODES] = {"LW", "SW", "ADD", "ADDI", "SUB", "SUBI", "XOR", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP"};

/* =============================================================

   HELPER FUNCTIONS

   ============================================================= */


/* converts integer into array of unsigned char - little indian */
inline void int2char(unsigned value, unsigned char *buffer){
	memcpy(buffer, &value, sizeof value);
}

/* converts array of char into integer - little indian */
inline unsigned char2int(unsigned char *buffer){
	unsigned d;
	memcpy(&d, buffer, sizeof d);
	return d;
}

/* implements the ALU operations */
unsigned alu(opcode_t opcode, unsigned a, unsigned b, unsigned imm, unsigned npc){
	switch(opcode){
			case ADD:
				return (a+b);
			case ADDI:
				return(a+imm);
			case SUB:
				return(a-b);
			case SUBI:
				return(a-imm);
			case XOR:
				return(a ^ b);
			case LW:
			case SW:
				return(a + imm);
			case BEQZ:
			case BNEZ:
			case BGTZ:
			case BGEZ:
			case BLTZ:
			case BLEZ:
			case JUMP:
				return(npc+imm);
			default:	
				return (-1);
	}
}

/* returns true if the instruction is a taken branch/jump */
bool taken_branch(opcode_t opcode, unsigned a){
        switch(opcode){
                case BEQZ:
                        if (a==0) return true;
                        break;
                case BNEZ:
                        if (a!=0) return true;
                        break;
                case BGTZ:
                        if ((int)a>0)  return true;
                        break;
                case BGEZ:
                        if ((int)a>=0) return true;
                        break;
                case BLTZ:
                        if ((int)a<0)  return true;
                        break;
                case BLEZ:
                        if ((int)a<=0) return true;
                        break;
                case JUMP:
                        return true;
                default:
                        return false;
        }
        return false;
}

/* return the kind of instruction encoded */ 

bool is_branch(opcode_t opcode){
        return (opcode == BEQZ || opcode == BNEZ || opcode == BLTZ || opcode == BLEZ || opcode == BGTZ || opcode == BGEZ || opcode == JUMP);
}

bool is_memory(opcode_t opcode){
        return (opcode == LW || opcode == SW);
}

bool is_int_r(opcode_t opcode){
        return (opcode == ADD || opcode == SUB || opcode == XOR);
}

bool is_int_imm(opcode_t opcode){
        return (opcode == ADDI || opcode == SUBI);
}

/* =============================================================

   CODE PROVIDED - NO NEED TO MODIFY FUNCTIONS BELOW

   ============================================================= */

/* loads the assembly program in file "filename" in instruction memory at the specified address */
void sim_pipe::load_program(const char *filename, unsigned base_address){

   /* initializing the base instruction address */
   instr_base_address = base_address;

   /* creating a map with the valid opcodes and with the valid labels */
   map<string, opcode_t> opcodes; //for opcodes
   map<string, unsigned> labels;  //for branches
   for (int i=0; i<NUM_OPCODES; i++)
	 opcodes[string(instr_names[i])]=(opcode_t)i;

   /* opening the assembly file */
   ifstream fin(filename, ios::in | ios::binary);
   if (!fin.is_open()) {
      cerr << "error: open file " << filename << " failed!" << endl;
      exit(-1);
   }

   /* parsing the assembly file line by line */
   string line;
   unsigned instruction_nr = 0;
   while (getline(fin,line)){
	// set the instruction field
	char *str = const_cast<char*>(line.c_str());

  	// tokenize the instruction
	char *token = strtok (str," \t");
	map<string, opcode_t>::iterator search = opcodes.find(token);
        if (search == opcodes.end()){
		// this is a label for a branch - extract it and save it in the labels map
		string label = string(token).substr(0, string(token).length() - 1);
		labels[label]=instruction_nr;
                // move to next token, which must be the instruction opcode
		token = strtok (NULL, " \t");
		search = opcodes.find(token);
		if (search == opcodes.end()) cout << "ERROR: invalid opcode: " << token << " !" << endl;
	}
	instr_memory[instruction_nr].opcode = search->second;

	//reading remaining parameters
	char *par1;
	char *par2;
	char *par3;
	switch(instr_memory[instruction_nr].opcode){
		case ADD:
		case SUB:
		case XOR:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
			instr_memory[instruction_nr].src2 = atoi(strtok(par3, "R"));
			break;
		case ADDI:
		case SUBI:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			par3 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].src1 = atoi(strtok(par2, "R"));
			instr_memory[instruction_nr].immediate = strtoul (par3, NULL, 0); 
			break;
		case LW:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].dest = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
			break;
		case SW:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src2 = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].immediate = strtoul(strtok(par2, "()"), NULL, 0);
			instr_memory[instruction_nr].src1 = atoi(strtok(NULL, "R"));
			break;
		case BEQZ:
		case BNEZ:
		case BLTZ:
		case BGTZ:
		case BLEZ:
		case BGEZ:
			par1 = strtok (NULL, " \t");
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].src1 = atoi(strtok(par1, "R"));
			instr_memory[instruction_nr].label = par2;
			break;
		case JUMP:
			par2 = strtok (NULL, " \t");
			instr_memory[instruction_nr].label = par2;
		default:
			break;

	} 

	/* increment instruction number before moving to next line */
	instruction_nr++;
   }
   //reconstructing the labels of the branch operations
   int i = 0;
   while(true){
   	instruction_t instr = instr_memory[i];
	if (instr.opcode == EOP) break;
	if (instr.opcode == BLTZ || instr.opcode == BNEZ ||
            instr.opcode == BGTZ || instr.opcode == BEQZ ||
            instr.opcode == BGEZ || instr.opcode == BLEZ ||
            instr.opcode == JUMP
	 ){
		instr_memory[i].immediate = (labels[instr.label] - i - 1) << 2;
	}
        i++;
   }

   ProgramCount = instr_base_address;

}

/* writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness) */
void sim_pipe::write_memory(unsigned address, unsigned value){
	int2char(value,data_memory+address);
}

/* prints the content of the data memory within the specified address range */
void sim_pipe::print_memory(unsigned start_address, unsigned end_address){
	cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	for (unsigned i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3) cout << endl;
	} 
}

/* prints the values of the registers */
void sim_pipe::print_registers(){
        cout << "Special purpose registers:" << endl;
        unsigned i, s;
        for (s=0; s<NUM_STAGES; s++){
                cout << "Stage: " << stage_names[s] << endl;
                for (i=0; i< NUM_SP_REGISTERS; i++)
                        if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
        }
        cout << "General purpose registers:" << endl;
        for (i=0; i< NUM_GP_REGISTERS; i++)
                if (get_gp_register(i)!=(int)UNDEFINED) cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}

/* initializes the pipeline simulator */
sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency){
	data_memory_size = mem_size;
	data_memory_latency = mem_latency;
	data_memory = new unsigned char[data_memory_size];
	reset();
}
	
/* deallocates the pipeline simulator */
sim_pipe::~sim_pipe(){
	delete [] data_memory;
}

/* execution statistics */
unsigned sim_pipe::get_clock_cycles(){return clock_cycles;}

unsigned sim_pipe::get_instructions_executed(){return instructions_executed;}

unsigned sim_pipe::get_stalls(){return stalls;}

float sim_pipe::get_IPC(){return (float)instructions_executed/clock_cycles;}
                                
/* =============================================================

   CODE TO BE COMPLETED

   ============================================================= */


/* reset the state of the pipeline simulator */
void sim_pipe::reset(){

	// initializing data memory to all 0xFF
	for (unsigned i=0; i<data_memory_size; i++) data_memory[i]=0xFF;

	// initializing instuction memory
        for (int i=0; i<PROGRAM_SIZE;i++){
                instr_memory[i].opcode=(opcode_t)NOP;
                instr_memory[i].src1=UNDEFINED;
                instr_memory[i].src2=UNDEFINED;
                instr_memory[i].dest=UNDEFINED;
                instr_memory[i].immediate=UNDEFINED;
        }
	instr_base_address = UNDEFINED;

	// general purpose registers initialization
	for (int i = 0; i<NUM_REGS; i++){
		regs[i] = UNDEFINED;
	}

	// pipeline registers initialization
	for (int i = 0; i<NUM_STAGES-1; i++) {
		
		pipelineRegisters[i].pc = UNDEFINED;
		pipelineRegisters[i].npc = UNDEFINED;
		pipelineRegisters[i].a = UNDEFINED;
		pipelineRegisters[i].b = UNDEFINED;
		pipelineRegisters[i].imm = UNDEFINED;
		pipelineRegisters[i].lmd = UNDEFINED;
		pipelineRegisters[i].alu_out = UNDEFINED;
		pipelineRegisters[i].cond = UNDEFINED;

	}

	// IR initialization
	for (int i=0; i<NUM_STAGES-1; i++){
		ir[i].opcode=(opcode_t)NOP;
		ir[i].src1=UNDEFINED;
		ir[i].src2=UNDEFINED;
		ir[i].dest=UNDEFINED;
		ir[i].immediate=UNDEFINED;
	}

	// other required initializations (statistics, etc.)
	clock_cycles = 0; //clock cycles
	stalls = 0; //stalls
	instructions_executed = 0; //instruction count
	is_stall = false; //stall flag
}

//returns value of special purpose register (see sim_pipe.h for more details)
unsigned sim_pipe::get_sp_register(sp_register_t reg, stage_t s){
	
	switch(s){
		case IF:
			if (reg == PC){
				return ProgramCount;
			}
			else {
				return UNDEFINED;
			}
			break;
		case ID:
			if (reg == NPC){
				return pipelineRegisters[IF_ID].npc;
			}
			else {
				return UNDEFINED;
			}
		case EXE:
			if (reg == NPC){
				return pipelineRegisters[ID_EXE].npc;
			}
			else if (reg == A){
				return pipelineRegisters[ID_EXE].a;
			}
			else if (reg == B){
				return pipelineRegisters[ID_EXE].b;
			}
			else if (reg == IMM){
				return pipelineRegisters[ID_EXE].imm;
			}
			else {
				return UNDEFINED;
			}
		case MEM:

			if (reg == ALU_OUTPUT){
				return pipelineRegisters[EXE_MEM].alu_out;
			}
			else if (reg == B){
				return pipelineRegisters[EXE_MEM].b;
			}
			
			else {
				return UNDEFINED;
			}
			
		case WB:
			if (reg == ALU_OUTPUT){
				return pipelineRegisters[MEM_WB].alu_out;
			}
			else if (reg == LMD){
				return pipelineRegisters[MEM_WB].lmd;
			}
			else {
				return UNDEFINED;
			}
		default: return UNDEFINED;
	}
}

//returns value of general purpose register
int sim_pipe::get_gp_register(unsigned reg){
	return regs[reg];
}

//sets the value of referenced general purpose register
void sim_pipe::set_gp_register(unsigned reg, int value){
	 regs[reg] = value;
}

/* <TODO: BODY OF THE SIMULATOR */
// Note: processing the stages in reverse order simplifies the data propagation through pipeline registers
void sim_pipe::run(unsigned cycles){

	unsigned start_cycles = clock_cycles;

	/* initialization at the beginning of simulation */
	if (clock_cycles == 0){
		ProgramCount = instr_base_address;
	}

	pipelineRegisters[IF_ID].pc = ProgramCount;

	/* ====== MAIN SIMULATION LOOP (one iteration per clock cycle)  ========= */
	while(cycles==0 || clock_cycles-start_cycles!=cycles){

                /* =============== */
                /* PIPELINE STAGES */
                /* =============== */
		/* ============   WB stage   ============  */
			// <hint: the simulation loop should be exited when the instruction processed is EOP>
		
		if (ir[MEM_WB].opcode == EOP){
			break;
		} 

		write_back();

		/* ============   MEM stage   ===========  */
		memory_stage();
		/* ============   EXE stage   ===========  */
			// <suggestion: use "alu" and "taken_branch" helper functions above to update ALU_OUTPUT and COND registers>

		execute_stage();

		/* ============   ID stage   ============  */
			// <suggestion: use the helper functions "is_branch", "is_int_r", ..., above to improve code readability/compactness>

		instruction_decode();

		/* ============   IF stage   ============  */

		instruction_fetch();

		


                /* =============== */
                /* END STAGES      */
                /* =============== */

		/* Other bookkeeping code */
                /* ====================== */

		clock_cycles++; // increase clock cycles count

	}
}

void sim_pipe::instruction_fetch() {
    
	ir[IF_ID] = instr_memory[(ProgramCount-instr_base_address)>>2];
	// Fetch the instruction from memory at the current program counter (PC)

	pipelineRegisters[IF_ID].npc = ProgramCount;

	
	if (ir[IF_ID].opcode != EOP && ir[IF_ID].opcode != NOP && !is_stall){
		ProgramCount += 4;
		pipelineRegisters[IF_ID].npc = ProgramCount;

	}

}

void sim_pipe::instruction_decode() {
	
	//retrieve instruction from IR

	unsigned rs1 = ir[IF_ID].src1;
	unsigned rs2 = ir[IF_ID].src2;
	unsigned immediate = ir[IF_ID].immediate;
	opcode_t opcode = ir[IF_ID].opcode;
	unsigned dest = ir[IF_ID].dest;
	

	if(!is_stall){
		if(opcode == NOP){
			pipelineRegisters[ID_EXE].a = UNDEFINED;
			pipelineRegisters[ID_EXE].b = UNDEFINED;
			pipelineRegisters[ID_EXE].imm = UNDEFINED;
		}

		// read operand values & load next pipeline register
		else if (opcode == LW){
			pipelineRegisters[ID_EXE].a = get_gp_register(rs1);
			pipelineRegisters[ID_EXE].imm = immediate;
			pipelineRegisters[ID_EXE].b = UNDEFINED;
		}
		else if (opcode == SW){
			pipelineRegisters[ID_EXE].a = get_gp_register(rs1);
			pipelineRegisters[ID_EXE].b = get_gp_register(rs2);
			pipelineRegisters[ID_EXE].imm = immediate;
		}
		else if (is_branch(ir[IF_ID].opcode) == true) {
			pipelineRegisters[ID_EXE].a = get_gp_register(rs1);
			pipelineRegisters[ID_EXE].imm = immediate;
			pipelineRegisters[ID_EXE].b = UNDEFINED;
		} 
		else if (opcode == ADDI || opcode == SUBI) {
			pipelineRegisters[ID_EXE].a = get_gp_register(rs1);
			pipelineRegisters[ID_EXE].imm = immediate;
			pipelineRegisters[ID_EXE].b = UNDEFINED;
		}
		else if (opcode == EOP){
			pipelineRegisters[ID_EXE].a = UNDEFINED;
			pipelineRegisters[ID_EXE].b = UNDEFINED;
			pipelineRegisters[ID_EXE].imm = UNDEFINED;

		} else {
			pipelineRegisters[ID_EXE].a = get_gp_register(rs1); // pull value from register files
			pipelineRegisters[ID_EXE].b = get_gp_register(rs2);
			pipelineRegisters[ID_EXE].imm = UNDEFINED;

		}

		
		if (opcode != EOP && opcode != NOP && !is_stall){
			instructions_executed++;
		}
		
		//pass instruction to the ID/EX pipeline register
		pipelineRegisters[ID_EXE].imm = immediate;
		ir[ID_EXE] = ir[IF_ID];
	} else {
		
		//stall
		stalls++;
		
	}

	// TO DO look for RAW stall conditions
	if(is_int_imm(ir[ID_EXE].opcode) && (ir[ID_EXE].src1 == ir[EXE_MEM].dest || ir[ID_EXE].src1 == ir[MEM_WB].dest)){
		is_stall = true;
		pipelineRegisters[ID_EXE].npc = UNDEFINED;
		pipelineRegisters[ID_EXE].a = UNDEFINED;
		pipelineRegisters[ID_EXE].b = UNDEFINED;
		pipelineRegisters[ID_EXE].imm = UNDEFINED;
		// ir[ID_EXE].opcode = NOP;
		// ir[ID_EXE].src1 = UNDEFINED;
		// ir[ID_EXE].src2 = UNDEFINED;
		// ir[ID_EXE].dest = UNDEFINED;
		// ir[ID_EXE].immediate = UNDEFINED;
		return;
	}
	else if(is_int_r(ir[ID_EXE].opcode) && (ir[ID_EXE].src1 == ir[EXE_MEM].dest || ir[ID_EXE].src2 == ir[EXE_MEM].dest || ir[ID_EXE].src1 == ir[MEM_WB].dest || ir[ID_EXE].src2 == ir[MEM_WB].dest)){
		is_stall = true;
		pipelineRegisters[ID_EXE].npc = UNDEFINED;
		pipelineRegisters[ID_EXE].a = UNDEFINED;
		pipelineRegisters[ID_EXE].b = UNDEFINED;
		pipelineRegisters[ID_EXE].imm = UNDEFINED;
		// ir[ID_EXE].opcode = NOP;
		// ir[ID_EXE].src1 = UNDEFINED;
		// ir[ID_EXE].src2 = UNDEFINED;
		// ir[ID_EXE].dest = UNDEFINED;
		// ir[ID_EXE].immediate = UNDEFINED;
		return;

	} else {
		
		is_stall = false;

	}

	pipelineRegisters[ID_EXE].npc = pipelineRegisters[IF_ID].npc;

}

void sim_pipe::execute_stage() {

	unsigned A = pipelineRegisters[ID_EXE].a;
	unsigned B = pipelineRegisters[ID_EXE].b;		
	unsigned immediate = pipelineRegisters[ID_EXE].imm;
	unsigned npc = pipelineRegisters[ID_EXE].npc;
	instruction_t instruction = ir[ID_EXE];

	unsigned alu_result = alu(instruction.opcode, A, B, immediate, npc);


	if(!is_stall){	
		//recieve instruction and operands from pipeline register
		
	
		//check if branch TO DO
		bool is_taken_branch = taken_branch(instruction.opcode, A);
		
		pipelineRegisters[EXE_MEM].alu_out = alu_result;
		pipelineRegisters[EXE_MEM].b = pipelineRegisters[ID_EXE].b;

		pipelineRegisters[EXE_MEM].cond = is_taken_branch;

		ir[EXE_MEM] = ir[ID_EXE];


	} else {
		ir[EXE_MEM].opcode = NOP;
		ir[EXE_MEM].src1 = UNDEFINED;
		ir[EXE_MEM].src2 = UNDEFINED;
		ir[EXE_MEM].dest = UNDEFINED;
		ir[EXE_MEM].immediate = UNDEFINED;
		pipelineRegisters[EXE_MEM].alu_out = UNDEFINED;
		pipelineRegisters[EXE_MEM].b = UNDEFINED;

	}
}	

void sim_pipe::memory_stage(){

	unsigned ALUOutput = pipelineRegisters[EXE_MEM].alu_out;
	instruction_t instruction = ir[EXE_MEM];

	if (instruction.opcode == LW) {
		unsigned LMD = char2int(&data_memory[ALUOutput]);
		pipelineRegisters[MEM_WB].lmd = LMD;
		pipelineRegisters[MEM_WB].alu_out = ALUOutput;
	}
	else if (instruction.opcode == SW){
		write_memory(ALUOutput, pipelineRegisters[EXE_MEM].b);
		pipelineRegisters[MEM_WB].lmd = UNDEFINED;
		pipelineRegisters[MEM_WB].alu_out = ALUOutput;
	} 
	else {
		pipelineRegisters[MEM_WB].alu_out = ALUOutput;
		pipelineRegisters[MEM_WB].lmd = UNDEFINED;
	}

	ir[MEM_WB] = ir[EXE_MEM];

}

void sim_pipe::write_back(){

	unsigned ALUOut = pipelineRegisters[MEM_WB].alu_out;
	instruction_t instruction = ir[MEM_WB];
	unsigned LMD = pipelineRegisters[MEM_WB].lmd;
	unsigned dest = instruction.dest;

	if (instruction.opcode == NOP){
		return;
	}
	else if (instruction.opcode == LW) {
		regs[dest] = LMD;
	}	
	else if (instruction.opcode != SW) {
		regs[dest] = ALUOut;
	}
}
