#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "mu-mips.h"
#include "mu-cache.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
				(MEM_REGIONS[i].mem[offset+2] << 16) |
				(MEM_REGIONS[i].mem[offset+1] <<  8) |
				(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      

	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		case 'f':
			if (scanf("%d", &ENABLE_FORWARDING) != 1) {
				break;
			}
			ENABLE_FORWARDING == 0 ? printf("Forwarding OFF\n") : printf("Forwarding ON\n"); break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;

	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}

	/*load program*/
	load_program();

	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */

	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/
	uint32_t opcode, function, rd, rt;

	opcode = (MEM_WB.IR & 0xFC000000) >> 26;
	function = MEM_WB.IR & 0x0000003F;
	rt = (MEM_WB.IR & 0x001F0000) >> 16;
	rd = (MEM_WB.IR & 0x0000F800) >> 11;

	//printf("%u\n", MEM_WB.IR);

	if(opcode == 0x00 && MEM_WB.IR != 0){
		switch(function){
			case 0x00: //SLL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x01:
				IF_EX.FLAG = TRUE;
				INSTRUCTION_COUNT--;
				//printf("Set IF_EX.FLAG = TRUE.\n");
                print_instruction(CURRENT_STATE.PC - 12);
				break;
			case 0x02: //SRL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x03: //SRA 
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
            case 0x08: //JR
                branch_taken = FALSE; // Reset flag
				print_instruction(CURRENT_STATE.PC - 16);
				break;
            case 0x09: //JALR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput + 4;
                branch_taken = FALSE; // Reset flag
			case 0x0C: //SYSCALL
				if(MEM_WB.ALUOutput == 0xa){
					RUN_FLAG = FALSE;
				}
                MEM_WB.RegisterRd = rd;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x10: //MFHI
				CURRENT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
                //printf("MEM_WB.RegisterRd: %u", MEM_WB.RegisterRd);
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x12: //MFLO
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = 33;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x18: //MULT
				NEXT_STATE.LO = (MEM_WB.AA & 0x00000000FFFFFFFF);
				NEXT_STATE.HI = (MEM_WB.AA & 0XFFFFFFFF00000000) >> 32;
				MEM_WB.RegisterRd = rd;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x19: //MULTU
				NEXT_STATE.LO = (MEM_WB.AA & 0x00000000FFFFFFFF);
				NEXT_STATE.HI = (MEM_WB.AA & 0XFFFFFFFF00000000) >> 32;
				MEM_WB.RegisterRd = rd;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x1A: //DIV 
				NEXT_STATE.LO = MEM_WB.ALUOutput;
				NEXT_STATE.HI = MEM_WB.A;
				MEM_WB.RegisterRd = rd;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x1B: //DIVU
				NEXT_STATE.LO = MEM_WB.ALUOutput;
				NEXT_STATE.HI = MEM_WB.A;
				MEM_WB.RegisterRd = rd;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x20: //ADD
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x21: //ADDU 
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x22: //SUB
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x23: //SUBU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x24: //AND
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x25: //OR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x26: //XOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x27: //NOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x2A: //SLT
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rd;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			default:
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				INSTRUCTION_COUNT--;
				break;
		}
	}
	else{
		switch(opcode){
            case 0x01:
                branch_taken = FALSE; // Reset flag
                print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x02: // J 
				branch_taken = FALSE; // Reset flag
                print_instruction(CURRENT_STATE.PC - 16);
                break;
            case 0x03: //JAL
                branch_taken = FALSE; // Reset flag
				NEXT_STATE.REGS[31] = MEM_WB.ALUOutput + 4;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
            case 0x04: //BEQ
				branch_taken = FALSE; // Reset flag
                //printf("Set branch_taken = false.\n");
				print_instruction(CURRENT_STATE.PC - 16);
				break;
            case 0x05: //BNE
				branch_taken = FALSE; // Reset flag
				print_instruction(CURRENT_STATE.PC - 16);
				break;
            case 0x06: //BLEZ
				branch_taken = FALSE; // Reset flag
				print_instruction(CURRENT_STATE.PC - 16);
				break;
            case 0x07: //BGTZ
				branch_taken = FALSE; // Reset flag
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x08: //ADDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x09: //ADDIU
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x0A: //SLTI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x0C: //ANDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x0D: //ORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x0E: //XORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x0F: //LUI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x20: //LB
				NEXT_STATE.REGS[rt] = ((MEM_WB.LMD & 0x000000FF) & 0x80) > 0 ? (MEM_WB.LMD | 0xFFFFFF00) : (MEM_WB.LMD & 0x000000FF);
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x21: //LH
				NEXT_STATE.REGS[rt] = ((MEM_WB.LMD & 0x0000FFFF) & 0x8000) > 0 ? (MEM_WB.LMD | 0xFFFF0000) : (MEM_WB.LMD & 0x0000FFFF);
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x23: //LW
				CURRENT_STATE.REGS[rt] = MEM_WB.LMD;
				MEM_WB.RegisterRd = rt;
				//IF_EX.FLAG = TRUE;
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x28: //SB
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				MEM_WB.RegisterRd = rd;
				//for count the instruction
				print_instruction(CURRENT_STATE.PC - 16);				
				break;
			case 0x29: //SH
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				MEM_WB.RegisterRd = rd;
				//for count the instruction
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			case 0x2B: //SW
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				MEM_WB.RegisterRd = rd;
				//for count the instruction
				print_instruction(CURRENT_STATE.PC - 16);
				break;
			default:
				// put more things here
				printf("WB at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				INSTRUCTION_COUNT--;
				break;
		}
	}
    if(MEM_WB.ff == FALSE){
        IF_EX.FLAG = TRUE;
        MEM_WB.ff = TRUE;
    }
	INSTRUCTION_COUNT++;
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	/*IMPLEMENT THIS*/
	if (MEM_WB.FLAG == FALSE){
		if (MEM_WB.RegWrite == 1 && (MEM_WB.RegisterRd != 0) && (MEM_WB.RegisterRd == ID_IF.RegisterRs)){
			if(ENABLE_FORWARDING == 0){
				//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRs:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRs);
				IF_EX.FLAG = FALSE;
				//MEM_WB.RegisterRd = 0;
				//printf("flag3\n");
				MEM_WB.IR = 0x00000001;
                MEM_WB.ff = FALSE;
			}
			else{
				if ((EX_MEM.RegWrite == 1 && (EX_MEM.RegisterRd == ID_IF.RegisterRs)) != 1){
					//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRs:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRs);
					//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRs:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRs);
					ForwardA = 01;
					//MEM_WB.RegisterRd = 0;
					//printf("flag7\n");
				}
			}
		}
		if (MEM_WB.RegWrite == 1 && (MEM_WB.RegisterRd != 0) && (MEM_WB.RegisterRd == ID_IF.RegisterRt)){
			if(ENABLE_FORWARDING == 0){
				//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRt:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRt);
				IF_EX.FLAG = FALSE;
				//MEM_WB.RegisterRd = 0;
				//printf("flag4\n");
				MEM_WB.IR = 0x00000001;
                MEM_WB.ff = FALSE;
			}
			else{
				if ((EX_MEM.RegWrite == 1 && (EX_MEM.RegisterRd == ID_IF.RegisterRt)) != 1){
					//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRt:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRt);
					//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRt:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRt);
					ForwardB = 01;
					//MEM_WB.RegisterRd = 0;
					//printf("flag8\n");
				}
			}
		}
		MEM_WB.FLAG = TRUE;
		MEM_WB.RegisterRd = 0;
	}
	else {
		if (MEM_WB.RegWrite == 1 && (MEM_WB.RegisterRd != 0) && (MEM_WB.RegisterRd == IF_EX.RegisterRs)){
			if(ENABLE_FORWARDING == 0){
				//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRs:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRs);
				IF_EX.FLAG = FALSE;
				//MEM_WB.RegisterRd = 0;
				//printf("flag3\n");
				MEM_WB.IR = 0x00000001;
                MEM_WB.ff = FALSE; 
			}
			else{
				if ((EX_MEM.RegWrite == 1 && (EX_MEM.RegisterRd == IF_EX.RegisterRs)) != 1){
					//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRs:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRs);
					//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRs:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRs);
					ForwardA = 01;
					//MEM_WB.RegisterRd = 0;
					//printf("flag7\n");
				}
			}
		}
		if (MEM_WB.RegWrite == 1 && (MEM_WB.RegisterRd != 0) && (MEM_WB.RegisterRd == IF_EX.RegisterRt)){
			if(ENABLE_FORWARDING == 0){
				//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRt:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRt);
				IF_EX.FLAG = FALSE;
				//MEM_WB.RegisterRd = 0;
				//printf("flag4\n");
				MEM_WB.IR = 0x00000001;
                MEM_WB.ff = FALSE; 
			}
			else{
				if ((EX_MEM.RegWrite == 1 && (EX_MEM.RegisterRd == IF_EX.RegisterRt)) != 1){
					//printf("MEM_WB.RegWrite: %d MEM_WB.RegisterRd: %d IF_EX.RegisterRt:%d\n", MEM_WB.RegWrite, MEM_WB.RegisterRd, IF_EX.RegisterRt);
					//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRt:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRt);
					ForwardB = 01;
					//MEM_WB.RegisterRd = 0;
					//printf("flag8\n");
				}
			}
		}
		MEM_WB.RegisterRd = 0;
	}

        MEM_WB.IR = EX_MEM.IR;

	uint32_t opcode, function, data, rt, rd;

	opcode = (MEM_WB.IR & 0xFC000000) >> 26;
	function = MEM_WB.IR & 0x0000003F;
	rt = (IF_EX.IR & 0x001F0000) >> 16;
	rd = (IF_EX.IR & 0x0000F800) >> 11;

	if(opcode == 0x00 && MEM_WB.IR != 0){
		switch(function){
			case 0x00: //SLL
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				// print_instruction(CURRENT_STATE.PC);
				break;
			case 0x02: //SRL
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //SRA 
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
            case 0x08: //JR
                EX_MEM.FLAG = FALSE;
                branch_taken = TRUE; 
				is_branch_jump = FALSE;
				//print_instruction(CURRENT_STATE.PC);
				break;
            case 0x09: //JALR
                EX_MEM.FLAG = FALSE;
                MEM_WB.ALUOutput = EX_MEM.ALUOutput;
                branch_taken = TRUE; 
				is_branch_jump = FALSE;
			case 0x0C: //SYSCALL
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x10: //MFHI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x11: //MTHI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
                MEM_WB.HI = 1;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x12: //MFLO
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x13: //MTLO
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
                MEM_WB.LO = 1;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x18: //MULT
				MEM_WB.AA = EX_MEM.AA;
                MEM_WB.HI = 2;
                MEM_WB.LO = 2;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x19: //MULTU
				MEM_WB.AA = EX_MEM.AA;
                MEM_WB.HI = 2;
                MEM_WB.LO = 2;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1A: //DIV 
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				MEM_WB.A = EX_MEM.A;
                MEM_WB.HI = 3;
                MEM_WB.LO = 3;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1B: //DIVU
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				MEM_WB.A = EX_MEM.A;
                MEM_WB.HI = 3;
                MEM_WB.LO = 3;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //ADD
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //ADDU 
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x22: //SUB
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //SUBU
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x24: //AND
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x25: //OR
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
                //printf("or: %u\n", MEM_WB.ALUOutput);
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x26: //XOR
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x27: //NOR
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2A: //SLT
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rd;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				printf("MEM at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{
		switch(opcode){
            case 0x01:
				if(rt == 0x00000){ //BLTZ
				    if((IF_EX.A & 0x80000000) > 0){
                        EX_MEM.FLAG = FALSE;
                        branch_taken = TRUE; 
				        is_branch_jump = FALSE;
				    }
                    else{
                        branch_not_taken = TRUE; 
                        is_branch_jump = FALSE;
                    }
				//print_instruction(CURRENT_STATE.PC);
				}
				else if(rt == 0x00001){ //BGEZ
				    if((IF_EX.A & 0x80000000) == 0x0){
                        EX_MEM.FLAG = FALSE;
                        branch_taken = TRUE; 
				        is_branch_jump = FALSE;
				    }
                    else{
                        branch_not_taken = TRUE; 
                        is_branch_jump = FALSE;
                    }
				//print_instruction(CURRENT_STATE.PC);
				}
				break;
			case 0x02: // J
                EX_MEM.FLAG = FALSE;
				branch_taken = TRUE; 
				is_branch_jump = FALSE;
                break;
            case 0x03: //JAL
                EX_MEM.FLAG = FALSE;
                branch_taken = TRUE; 
				is_branch_jump = FALSE;
                MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//print_instruction(CURRENT_STATE.PC);
				break;
            case 0x04: //BEQ
				if(IF_EX.A == IF_EX.B){
                    EX_MEM.FLAG = FALSE;
				    branch_taken = TRUE; 
				    is_branch_jump = FALSE;
                    //printf("BEQ TAKEN\n");
                    //printf("is_branch_jump: %d", is_branch_jump);
				}
                else{
                    //printf("BEQ NOT TAKEN\n");
                    branch_not_taken = TRUE; 
                    //EX_MEM.FLAG = FALSE;
                    is_branch_jump = FALSE;
                    //printf("is_branch_jump: %d", is_branch_jump);
                }
				//print_instruction(CURRENT_STATE.PC);
				break;
            case 0x05: //BNE
				if(IF_EX.A != IF_EX.B){
                    EX_MEM.FLAG = FALSE;
                    branch_taken = TRUE; 
				    is_branch_jump = FALSE;
				}
                else{
                    branch_not_taken = TRUE; 
                    is_branch_jump = FALSE;
                }
				//print_instruction(CURRENT_STATE.PC);
				break;
            case 0x06: //BLEZ
				if((IF_EX.A & 0x80000000) > 0 || IF_EX.A == 0){
                    EX_MEM.FLAG = FALSE;
                    branch_taken = TRUE; 
				    is_branch_jump = FALSE;
				}
                else{
                    branch_not_taken = TRUE; 
                    is_branch_jump = FALSE;
                }
				//print_instruction(CURRENT_STATE.PC);
				break;
            case 0x07: //BGTZ
				if((IF_EX.A & 0x80000000) == 0x0 || IF_EX.A != 0){
                    EX_MEM.FLAG = FALSE;
                    branch_taken = TRUE; 
				    is_branch_jump = FALSE;
				}
                else{
                    branch_not_taken = TRUE; 
                    is_branch_jump = FALSE;
                }
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x08: //ADDI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rt;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //ADDIU
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rt;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0A: //SLTI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rt;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //ANDI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rt;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0D: //ORI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rt;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0E: //XORI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rt;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0F: //LUI
				MEM_WB.ALUOutput = EX_MEM.ALUOutput;
				//MEM_WB.RegisterRd = rt;
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //LB
				 if(1 == cache_isHit(EX_MEM.ALUOutput)) {
					MEM_WB.LMD = cache_read_32(EX_MEM.ALUOutput);
				 }
				 else { // Miss -- cache load loads into cache and returns correct word
					MEM_WB.LMD = cache_load_32(EX_MEM.ALUOutput);
				 }
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				break;
			case 0x21: //LH
				 if(1 == cache_isHit(EX_MEM.ALUOutput)) {
					MEM_WB.LMD = cache_read_32(EX_MEM.ALUOutput);
				 }
				 else { // Miss -- cache load loads into cache and returns correct word
					MEM_WB.LMD = cache_load_32(EX_MEM.ALUOutput);
				 }
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				break;
			case 0x23: //LW
				 if(1 == cache_isHit(EX_MEM.ALUOutput)) {
					MEM_WB.LMD = cache_read_32(EX_MEM.ALUOutput);
				 }
				 else { // Miss -- cache load loads into cache and returns correct word
					MEM_WB.LMD = cache_load_32(EX_MEM.ALUOutput);
				 }
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x28: //SB
				data = mem_read_32( EX_MEM.ALUOutput);
				data = (data & 0xFFFFFF00) | (EX_MEM.B & 0x000000FF);
				mem_write_32(EX_MEM.ALUOutput, data);
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//MEM_WB.RegisterRd = 0;
				//print_instruction(CURRENT_STATE.PC);				
				break;
			case 0x29: //SH
				data = mem_read_32( EX_MEM.ALUOutput);
				data = (data & 0xFFFF0000) | (EX_MEM.B & 0x0000FFFF);
				mem_write_32(EX_MEM.ALUOutput, data);
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//MEM_WB.RegisterRd = 0;
				//print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2B: //SW
				mem_write_32(EX_MEM.ALUOutput, EX_MEM.B);
				MEM_WB.RegWrite = EX_MEM.RegWrite;
				//MEM_WB.RegisterRd = 0;
				//print_instruction(CURRENT_STATE.PC);
				break;
			default:
				// put more things here
				printf("MEM at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	/*IMPLEMENT THIS*/
	while(1){
		if (EX_MEM.RegWrite == 1 && (EX_MEM.RegisterRd != 0) && (EX_MEM.RegisterRd == IF_EX.RegisterRs)){
			if(ENABLE_FORWARDING == 0){
				//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRs:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRs);
				IF_EX.FLAG = FALSE;
				EX_MEM.FLAG = FALSE;
				//printf("flag\n");
				EX_MEM.IR = 0x00000001;
				//EX_MEM.RegisterRd = 0;
				//printf("%u\n", EX_MEM.IR);
				//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRs:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRs);
			}
			else{
				//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRs:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRs);
				ForwardA = 10;
				EX_MEM.FLAG = FALSE;
				MEM_WB.FLAG = FALSE;
				//EX_MEM.RegisterRd = 0;
				//printf("flag5\n");
				INSTRUCTION_COUNT--;
				EX_MEM.forward = 1;
			}
		}
		//else{
		//printf("nothing\n");
		//}
		if (EX_MEM.RegWrite == 1 && (EX_MEM.RegisterRd != 0) && (EX_MEM.RegisterRd == IF_EX.RegisterRt)){
			if(ENABLE_FORWARDING == 0){
				//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRt:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRt);
				IF_EX.FLAG = FALSE;
				EX_MEM.FLAG = FALSE;
				//printf("flag2\n");
				EX_MEM.IR = 0x00000001;
				//EX_MEM.RegisterRd = 0;
				//printf("%u\n", EX_MEM.IR);
			}
			else{
				//printf("EX_MEM.RegWrite: %d EX_MEM.RegisterRd: %d IF_EX.RegisterRt:%d\n", EX_MEM.RegWrite, EX_MEM.RegisterRd, IF_EX.RegisterRt);
				ForwardB = 10;
				EX_MEM.FLAG = FALSE;
				MEM_WB.FLAG = FALSE;
				//EX_MEM.RegisterRd = 0;
				//printf("flag6\n");
				if (EX_MEM.forward == 0){
					INSTRUCTION_COUNT--;
				}
			}
		}
		//else{
		//printf("nothing2\n");
		//}
		EX_MEM.forward = 0;
		EX_MEM.RegisterRd = 0;
		break;
	}

	if (EX_MEM.FLAG == TRUE && (FALSE == is_branch_jump) && branch_not_taken == FALSE){
		EX_MEM.IR = IF_EX.IR;
		//printf("EX_MEM.IR: %u\n", EX_MEM.IR);
	}
    if (branch_not_taken == TRUE){
        EX_MEM.IR = 0x00000000;
    }

	uint32_t opcode, function, rt, rd;
	uint64_t p1, p2;

	opcode = (EX_MEM.IR & 0xFC000000) >> 26;
	function = EX_MEM.IR & 0x0000003F;
	rt = (IF_EX.IR & 0x001F0000) >> 16;
	rd = (IF_EX.IR & 0x0000F800) >> 11;

	if(EX_MEM.FLAG == TRUE && (FALSE == is_branch_jump) && branch_not_taken == FALSE){
		if(opcode == 0x00 && EX_MEM.IR != 0){
			switch(function){
				case 0x00: //SLL
					EX_MEM.ALUOutput = IF_EX.A << IF_EX.imm;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x02: //SRL
					EX_MEM.ALUOutput = IF_EX.A >> IF_EX.imm;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x03: //SRA 
					if ((IF_EX.A & 0x80000000) == 1){
						EX_MEM.ALUOutput = ~(~IF_EX.A >> IF_EX.imm);
					}
					else{
						EX_MEM.ALUOutput = IF_EX.A >> IF_EX.imm;
					}
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
                case 0x08: //JR
				    NEXT_STATE.PC = IF_EX.A;
				    is_branch_jump = TRUE;
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x09: //JALR
                    EX_MEM.ALUOutput = CURRENT_STATE.PC - 8;
				    NEXT_STATE.PC = IF_EX.A;
				    is_branch_jump = TRUE;
				case 0x0C: //SYSCALL
					EX_MEM.ALUOutput = IF_EX.A;
                    EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x10: //MFHI
					EX_MEM.ALUOutput = IF_EX.A;
                    NEXT_STATE.REGS[rd] = EX_MEM.ALUOutput;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x11: //MTHI
					EX_MEM.ALUOutput = IF_EX.A;
                    EX_MEM.HI = 1;
                    EX_MEM.RegisterRd = 33;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x12: //MFLO
					EX_MEM.ALUOutput = IF_EX.A;
                    NEXT_STATE.REGS[rd] = EX_MEM.ALUOutput;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x13: //MTLO
					EX_MEM.ALUOutput = IF_EX.A;
                    NEXT_STATE.LO = EX_MEM.ALUOutput;
                    EX_MEM.LO = 1;
                    EX_MEM.RegisterRd = 33;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x18: //MULT
					if ((IF_EX.A & 0x80000000) == 0x80000000){
						p1 = 0xFFFFFFFF00000000 | IF_EX.A;
					}else{
						p1 = 0x00000000FFFFFFFF & IF_EX.A;
					}
					if ((IF_EX.B & 0x80000000) == 0x80000000){
						p2 = 0xFFFFFFFF00000000 | IF_EX.B;
					}else{
						p2 = 0x00000000FFFFFFFF & IF_EX.B;
					}
					EX_MEM.AA = p1 * p2;
                    EX_MEM.HI = 2;
                    EX_MEM.LO = 2;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x19: //MULTU
					EX_MEM.AA = IF_EX.A * IF_EX.B;
                    EX_MEM.HI = 2;
                    EX_MEM.LO = 2;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x1A: //DIV 
					if (IF_EX.B != 0){
						EX_MEM.ALUOutput = IF_EX.A / IF_EX.B;
						EX_MEM.A = IF_EX.A % IF_EX.B;
					}
                    EX_MEM.HI = 3;
                    EX_MEM.LO = 3;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x1B: //DIVU
					if (IF_EX.B != 0){
						EX_MEM.ALUOutput = IF_EX.A / IF_EX.B;
						EX_MEM.A = IF_EX.A % IF_EX.B;
					}
                    EX_MEM.HI = 3;
                    EX_MEM.LO = 3;
                    EX_MEM.RegisterRd = 33;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x20: //ADD
					EX_MEM.ALUOutput = IF_EX.A + IF_EX.B;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x21: //ADDU 
					EX_MEM.ALUOutput = IF_EX.A + IF_EX.B;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x22: //SUB
					EX_MEM.ALUOutput = IF_EX.A - IF_EX.B;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x23: //SUBU
					EX_MEM.ALUOutput = IF_EX.A - IF_EX.B;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x24: //AND
					EX_MEM.ALUOutput = IF_EX.A & IF_EX.B;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x25: //OR
					EX_MEM.ALUOutput = IF_EX.A | IF_EX.B;
                    //printf("or: %u\n", EX_MEM.ALUOutput);
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x26: //XOR
					EX_MEM.ALUOutput = IF_EX.A ^ IF_EX.B;
                    NEXT_STATE.REGS[rd] = EX_MEM.ALUOutput;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x27: //NOR
					EX_MEM.ALUOutput = ~(IF_EX.A | IF_EX.B);
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x2A: //SLT
					if(IF_EX.A < IF_EX.B){
						EX_MEM.ALUOutput = 0x1;
					}
					else{
						EX_MEM.ALUOutput = 0x0;
					}
                    NEXT_STATE.REGS[rd] = EX_MEM.ALUOutput;
					EX_MEM.RegisterRd = rd;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				default:
					printf("EX at 0x%x is not implemented!\n", CURRENT_STATE.PC);
					break;
			}
		}
		else{
			switch(opcode){
                case 0x01:
				    if(rt == 0x00000){ //BLTZ
					   if((IF_EX.A & 0x80000000) > 0){
						  NEXT_STATE.PC = (CURRENT_STATE.PC - 8) + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000)<<2 : (IF_EX.imm & 0x0000FFFF)<<2);
						  is_branch_jump = TRUE;
					   }
                        is_branch_jump = TRUE;
					   //print_instruction(CURRENT_STATE.PC);
				    }
				    else if(rt == 0x00001){ //BGEZ
					   if((IF_EX.A & 0x80000000) == 0x0){
						  NEXT_STATE.PC = (CURRENT_STATE.PC - 8) + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000)<<2 : (IF_EX.imm & 0x0000FFFF)<<2);
						  is_branch_jump = TRUE;
					   }
                        is_branch_jump = TRUE;
					   //print_instruction(CURRENT_STATE.PC);
				    }
				    break;
				case 0x02: // J
					NEXT_STATE.PC = ((CURRENT_STATE.PC ) & 0xF0000000) | (IF_EX.imm << 2);
					is_branch_jump = TRUE; // Unset so IF fetches correct instr
					break;
                case 0x03: //JAL
				    NEXT_STATE.PC = ((CURRENT_STATE.PC ) & 0xF0000000) | (IF_EX.imm << 2);
				    EX_MEM.ALUOutput = CURRENT_STATE.PC - 8;
				    is_branch_jump = TRUE;
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x04: //BEQ
				    if(IF_EX.A == IF_EX.B){
					   NEXT_STATE.PC = (CURRENT_STATE.PC - 8) + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000)<<2 : (IF_EX.imm & 0x0000FFFF)<<2);
					   is_branch_jump = TRUE;
				    }
                        is_branch_jump = TRUE;
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x05: //BNE
				    if(IF_EX.A != IF_EX.B){
                        NEXT_STATE.PC = (CURRENT_STATE.PC - 8) + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000)<<2 : (IF_EX.imm & 0x0000FFFF)<<2);
                        is_branch_jump = TRUE;
				    }
                        is_branch_jump = TRUE;
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x06: //BLEZ
				    if((IF_EX.A & 0x80000000) > 0 || IF_EX.A == 0){
                        NEXT_STATE.PC = (CURRENT_STATE.PC - 8) +  ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000)<<2 : (IF_EX.imm & 0x0000FFFF)<<2);
                        is_branch_jump = TRUE;
				    }
                        is_branch_jump = TRUE;
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x07: //BGTZ
				    if((IF_EX.A & 0x80000000) == 0x0 || IF_EX.A != 0){
                        NEXT_STATE.PC = (CURRENT_STATE.PC - 8) +  ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000)<<2 : (IF_EX.imm & 0x0000FFFF)<<2);
                        is_branch_jump = TRUE;
				    }
                        is_branch_jump = TRUE;
				    //print_instruction(CURRENT_STATE.PC);
				    break;
				case 0x08: //ADDI
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x09: //ADDIU
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0A: //SLTI
					if ( (  IF_EX.A - (int32_t)( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF))) < 0){
						EX_MEM.ALUOutput = 0x1;
					}else{
						EX_MEM.ALUOutput = 0x0;
					}
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0C: //ANDI
					EX_MEM.ALUOutput = IF_EX.A & (IF_EX.imm & 0x0000FFFF);
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0D: //ORI
					EX_MEM.ALUOutput = IF_EX.A | (IF_EX.imm & 0x0000FFFF);
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0E: //XORI
					EX_MEM.ALUOutput = IF_EX.A ^ (IF_EX.imm & 0x0000FFFF);
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0F: //LUI
					EX_MEM.ALUOutput = IF_EX.imm << 16;
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x20: //LB
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x21: //LH
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x23: //LW
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.RegisterRd = rt;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x28: //SB
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.B = IF_EX.B;
					EX_MEM.RegisterRd = 0;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);				
					break;
				case 0x29: //SH
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.B = IF_EX.B;
					EX_MEM.RegisterRd = 0;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x2B: //SW
					EX_MEM.ALUOutput = IF_EX.A + ( (IF_EX.imm & 0x8000) > 0 ? (IF_EX.imm | 0xFFFF0000) : (IF_EX.imm & 0x0000FFFF));
					EX_MEM.B = IF_EX.B;
					EX_MEM.RegisterRd = 0;
					EX_MEM.RegWrite = IF_EX.RegWrite;
					//print_instruction(CURRENT_STATE.PC);
					break;
				default:
					// put more things here
					printf("EX at 0x%x is not implemented!\n", CURRENT_STATE.PC);
					break;
			}
		}
	}
	if(TRUE == branch_taken) { // Flush
		EX_MEM.IR = 0;
		EX_MEM.A = 0;
		EX_MEM.B = 0;
		EX_MEM.AA = 0;
		EX_MEM.BB = 0;
		EX_MEM.RegisterRs = 0;
		EX_MEM.RegisterRt = 0;
		EX_MEM.RegisterRd = 0;
		EX_MEM.imm = 0;
        //branch_not_taken = FALSE; 
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	/*IMPLEMENT THIS*/
	if (ENABLE_FORWARDING == 1 && IF_EX.MemRead == 1 && ((IF_EX.RegisterRt == ID_IF.RegisterRs) || (IF_EX.RegisterRt == ID_IF.RegisterRt))){
		IF_EX.IR = 0x00000001;
		IF_EX.FLAG = FALSE;
	}

        //printf("IF_EX.FLAG = %d\n", IF_EX.FLAG);
    //printf("EX_MEM.FLAG = %d\n", EX_MEM.FLAG);
	if (IF_EX.FLAG == TRUE && EX_MEM.FLAG == TRUE && (FALSE == is_branch_jump)) {
		IF_EX.IR = ID_IF.IR;
		//printf("IF_EX.IR: %u\n", IF_EX.IR);
        branch_not_taken = FALSE;
	}

	uint32_t opcode, function, rs, rt, sa, immediate, target;

	//printf("[0x%x]\t", CURRENT_STATE.PC);

	opcode = (IF_EX.IR & 0xFC000000) >> 26;
	function = IF_EX.IR & 0x0000003F;
	rs = (IF_EX.IR & 0x03E00000) >> 21;
	rt = (IF_EX.IR & 0x001F0000) >> 16;
	//rd = (IF_EX.IR & 0x0000F800) >> 11;
	sa = (IF_EX.IR & 0x000007C0) >> 6;
	immediate = IF_EX.IR & 0x0000FFFF;
	target = IF_EX.IR & 0x03FFFFFF;

	if(IF_EX.FLAG == TRUE){
		if(opcode == 0x00 && IF_EX.IR != 0){
			switch(function){
				case 0x00: //SLL
					if (ForwardB == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rt];
					}
					IF_EX.imm = sa;
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x02: //SRL
					if (ForwardB == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rt];
					}
					IF_EX.imm = sa;
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = 0;
					IF_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x03: //SRA 
					if (ForwardB == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rt];
					}
					IF_EX.imm = sa;
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = 0;
					IF_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
                case 0x08: //JR
                    IF_EX.A = CURRENT_STATE.REGS[rs];
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x09: //JALR
                    IF_EX.A = CURRENT_STATE.REGS[rs];
				    //print_instruction(CURRENT_STATE.PC);
				    break;
				case 0x0C: //SYSCALL
					IF_EX.A = CURRENT_STATE.REGS[2];
                    IF_EX.RegisterRs = 0x2;
                    IF_EX.RegWrite = 1;
                    //printf("syscall: %u", IF_EX.A);
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x10: //MFHI
                    if (ForwardB == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.HI;
					}
					//IF_EX.A = CURRENT_STATE.HI;
                    //printf("HI: %u", CURRENT_STATE.HI);
                    IF_EX.RegisterRs = 33;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x11: //MTHI
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x12: //MFLO
					IF_EX.A = CURRENT_STATE.LO;
                    //printf("LO: %u", CURRENT_STATE.LO);
                    IF_EX.RegisterRs = 33;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x13: //MTLO
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
                        //printf("flag1: %u", IF_EX.A);
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
                        //printf("flag2: %u", IF_EX.A);
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
                        //printf("flag3: %u", IF_EX.A);
					}
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x18: //MULT
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					// print_instruction(CURRENT_STATE.PC);
					break;
				case 0x19: //MULTU
					if (ForwardA == 10){
						IF_EX.AA = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.AA = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.AA = (int64_t)CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.BB = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.BB = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.BB = (int64_t)CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x1A: //DIV 
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = (int32_t)CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = (int32_t)CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x1B: //DIVU
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x20: //ADD
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x21: //ADDU 
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x22: //SUB
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x23: //SUBU
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x24: //AND
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x25: //OR
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
                        //printf("F1\n");
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
                        //printf("F2\n");
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
                        //printf("RS: %u\n", CURRENT_STATE.REGS[rs]);
                        //printf("F3");
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
                    //printf("RS: %u\n", IF_EX.A);
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
                    //printf("$RS: %u\n", IF_EX.RegisterRs);
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x26: //XOR
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x27: //NOR
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x2A: //SLT
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.RegisterRt = rt;
					IF_EX.RegisterRs = rs;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				default:
					printf("ID at 0x%x is not implemented!\n", CURRENT_STATE.PC);
					break;
			}
		}
		else{
			switch(opcode){
				case 0x01:
                    IF_EX.A = CURRENT_STATE.REGS[rs];
                    IF_EX.imm = immediate; 
				    break;
				case 0x02: // J
					IF_EX.imm = target; 
					break;
                case 0x03: //JAL
                    IF_EX.imm = target; 
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x04: //BEQ
				    IF_EX.A = CURRENT_STATE.REGS[rs];
                    IF_EX.B = CURRENT_STATE.REGS[rt];
                    IF_EX.imm = immediate; 
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x05: //BNE
				    IF_EX.A = CURRENT_STATE.REGS[rs];
                    IF_EX.B = CURRENT_STATE.REGS[rt];
                    IF_EX.imm = immediate; 
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x06: //BLEZ
				    IF_EX.A = CURRENT_STATE.REGS[rs];
                    IF_EX.imm = immediate; 
				    //print_instruction(CURRENT_STATE.PC);
				    break;
                case 0x07: //BGTZ
                    IF_EX.A = CURRENT_STATE.REGS[rs];
                    IF_EX.imm = immediate; 
				    //print_instruction(CURRENT_STATE.PC);
				    break;
				case 0x08: //ADDI
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x09: //ADDIU
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0A: //SLTI
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = (int32_t)CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0C: //ANDI
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0D: //ORI
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0E: //XORI
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = 0;
					IF_EX.RegWrite = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x0F: //LUI
					IF_EX.imm = immediate;
					IF_EX.RegWrite = 1;
					IF_EX.RegisterRs = 0;
					IF_EX.RegisterRt = 0;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x20: //LB
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = rt;
					IF_EX.RegWrite = 1;
					IF_EX.MemRead = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x21: //LH
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = rt;
					IF_EX.RegWrite = 1;
					IF_EX.MemRead = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x23: //LW
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = rt;
					IF_EX.RegWrite = 1;
					IF_EX.MemRead = 1;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x28: //SB
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.imm = immediate;	
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = rt;
					IF_EX.RegWrite = 0;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x29: //SH
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = rt;
					IF_EX.RegWrite = 0;
					//print_instruction(CURRENT_STATE.PC);
					break;
				case 0x2B: //SW
					if (ForwardA == 10){
						IF_EX.A = EX_MEM.ALUOutput;
					}
					else if (ForwardA == 01){
						IF_EX.A = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.A = CURRENT_STATE.REGS[rs];
					}
					if (ForwardB == 10){
						IF_EX.B = EX_MEM.ALUOutput;
					}
					else if (ForwardB == 01){
						IF_EX.B = MEM_WB.ALUOutput;
					}
					else{
						IF_EX.B = CURRENT_STATE.REGS[rt];
					}
					IF_EX.imm = immediate;
					IF_EX.RegisterRs = rs;
					IF_EX.RegisterRt = rt;
					IF_EX.RegWrite = 0;
					//print_instruction(CURRENT_STATE.PC);
					break;
				default:
					// put more things here
					printf("ID at 0x%x is not implemented!\n", CURRENT_STATE.PC);
					break;
			}
		}
	}
    //printf("branch_taken: %d\n", branch_taken);
    //printf("is_branch_jump: %d\n", is_branch_jump);
    
	if(TRUE == branch_taken) { // Flush
		IF_EX.IR = 0;
		IF_EX.A = 0;
		IF_EX.B = 0;
		IF_EX.AA = 0;
		IF_EX.BB = 0;
		IF_EX.RegisterRs = 0;
		IF_EX.RegisterRt = 0;
		IF_EX.imm = 0;
	}
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	if (IF_EX.FLAG == TRUE && EX_MEM.FLAG == TRUE && (FALSE == is_branch_jump) ){ // Execute as normal
		ID_IF.IR = mem_read_32(CURRENT_STATE.PC);
		ID_IF.PC = CURRENT_STATE.PC + 4;
		NEXT_STATE.PC = ID_IF.PC;
	}
	//   else{
	//  print_instruction(CURRENT_STATE.PC);
	//}
	if (ID_IF.IR == 0){
		printf("NO INSTRUCTIONS FOR IF.\n");
	}
	//else{
	//print_instruction(CURRENT_STATE.PC);
	//}
	if (IF_EX.FLAG == TRUE && EX_MEM.FLAG == FALSE){
		EX_MEM.FLAG = TRUE;
		ForwardA = 00;
		ForwardB = 00;
	}
	if (IF_EX.FLAG == FALSE && EX_MEM.FLAG == TRUE){
		IF_EX.MemRead = 0;
	}
    if (branch_taken == TRUE){
        ID_IF.IR = 0;
    }
	//show_pipeline();
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
	EX_MEM.RegWrite = 1;
	MEM_WB.RegWrite = 1;
	IF_EX.MemRead = 0;
	EX_MEM.FLAG = TRUE;
	MEM_WB.FLAG = TRUE;
	IF_EX.FLAG = TRUE;
	ENABLE_FORWARDING = 0;
	ForwardA = 0;
	ForwardB = 0;
	EX_MEM.forward = 0;
	branch_taken = FALSE;
	is_branch_jump = FALSE;
	branch_not_taken = FALSE;
	// Init cache to 0
	cache_misses = 0;
	cache_hits = 0;
	int i=0, j=0;
	for(i=0; i<NUM_CACHE_BLOCKS; i++) {
		L1Cache.blocks[i].valid = 0;
		L1Cache.blocks[i].tag = 0;
		for(j=0; j<WORD_PER_BLOCK; j++) {
			L1Cache.blocks[i].words[j] = 0;
		}
	}
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;

	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;

	instruction = mem_read_32(addr);

	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;

	if(opcode == 0x00){
		/*R format instructions here*/

		switch(function){
			case 0x00:
				printf("SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				printf("SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				printf("SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				printf("JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					printf("JALR $r%u\n", rs);
				}
				else{
					printf("JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				printf("SYSCALL\n");
				break;
			case 0x10:
				printf("MFHI $r%u\n", rd);
				break;
			case 0x11:
				printf("MTHI $r%u\n", rs);
				break;
			case 0x12:
				printf("MFLO $r%u\n", rd);
				break;
			case 0x13:
				printf("MTLO $r%u\n", rs);
				break;
			case 0x18:
				printf("MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				printf("MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				printf("DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				printf("DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				printf("ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				printf("ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				printf("SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x23:
				printf("SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				printf("AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				printf("OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				printf("XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				printf("NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				printf("SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					printf("BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					printf("BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				printf("J 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				printf("JAL 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				printf("BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				printf("BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				printf("BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				printf("BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				printf("ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				printf("ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				printf("SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				printf("ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				printf("ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				printf("XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				printf("LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				printf("LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				printf("LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				printf("LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				printf("SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				printf("SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				printf("SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
	printf("\nCurrent PC:[0x%x]\n", CURRENT_STATE.PC);
	printf("ID_IF.IR:%u\n", ID_IF.IR);
	print_instruction(ID_IF.PC - 4);
	printf("ID_IF.PC:%u\n\n", ID_IF.PC);
	printf("IF_EX.IR:%u\n", IF_EX.IR);
	print_instruction(ID_IF.PC - 8);
	printf("IF_EX.A:%u\n", IF_EX.A);
	printf("IF_EX.B:%u\n", IF_EX.B);
	printf("IF_EX.RegisterRs:%d\n\n", IF_EX.RegisterRs);
	printf("IF_EX.RegisterRt:%d\n\n", IF_EX.RegisterRt);
	printf("IF_EX.imm:%u\n\n", IF_EX.imm);
	printf("EX_MEM.IR:%u\n", EX_MEM.IR);
	print_instruction(ID_IF.PC - 12);
	printf("EX_MEM.A:%u\n", EX_MEM.A);
	printf("EX_MEM.B:%u\n", EX_MEM.B);
	printf("EX_MEM.ALUOutput:%u\n\n", EX_MEM.ALUOutput);
	printf("EX_MEM.RegisterRd:%d\n\n", EX_MEM.RegisterRd);
	printf("MEM_WB.IR:%u\n", MEM_WB.IR);
	print_instruction(ID_IF.PC - 16);
	printf("MEM_WB.ALUOutput:%u\n", MEM_WB.ALUOutput);
	printf("MEM_WB.LMD:%u\n", MEM_WB.LMD);
	printf("MEM_WB.RegisterRd:%d\n\n", MEM_WB.RegisterRd);
	printf("CYCLE %u\n", CYCLE_COUNT);
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");

	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
