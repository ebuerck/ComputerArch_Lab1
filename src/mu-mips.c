#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available. */
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
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory. */
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
/* Write a 32-bit word to memory. */
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
/* Execute one cycle. */
/***************************************************************/
void cycle() {
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles. */
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
/* simulate to completion. */
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
/* Dump a word-aligned region of memory to the terminal. */
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
/* Dump current values of registers to the teminal. */
/***************************************************************/
void rdump() {
	int i;
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
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
/* Read a command from standard input. */
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
			runAll();
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
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program. */
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
/* Allocate and set memory to zero. */
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
/* load program into memory. */
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

void getSingleInstruct(MIPS* instrAddress){
	uint32_t instr = mem_read_32(CURRENT_STATE.PC);

	char string[9];
	sprintf(string,"%08x", instr);

	char fullbinary[33];
	fullbinary[0] = '\0';

	for(int i = 0; i < 9; i++)
	{
		strcat(fullbinary, hex_to_binary(string[i]));
	}

	switch(FindFormat(fullbinary)) {
		case 'R': {
			returnRFormat(fullbinary,instrAddress);
			break;
		}
		case 'I': {
			returnIFormat(fullbinary,instrAddress);
			break;
		}
		case 'J': {
			returnJFormat(fullbinary,instrAddress);
			break;
		}
		default: {
			printf("You messed up.\n");
			break;
		}
	}
}

/************************************************************/
/* decode and execute instruction. */
/************************************************************/
void handle_instruction()
{
	CURRENT_STATE = NEXT_STATE;
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	MIPS instruct;
	getSingleInstruct(&instruct);

   //****************************** ALU INSTRUCTIONS ******************************
	if(!strcmp(instruct.op, "ADD")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] + CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "ADDU")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] + CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "ADDI")){
		CURRENT_STATE.REGS[instruct.rt] = CURRENT_STATE.REGS[instruct.rs] + instruct.immediate;
	}
	else if(!strcmp(instruct.op, "ADDIU")){
		CURRENT_STATE.REGS[instruct.rt] = CURRENT_STATE.REGS[instruct.rs] + instruct.immediate;
	}
	else if(!strcmp(instruct.op, "SUB")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] - CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "SUBU")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] - CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "MULT")){
		// HOW SHOULD WE DO THIS????
		CURRENT_STATE.HI = CURRENT_STATE.REGS[instruct.rs] * CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "MULTU")){
		// HOW SHOULD WE DO THIS????
		CURRENT_STATE.HI = CURRENT_STATE.REGS[instruct.rs] * CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "DIV")){
		CURRENT_STATE.HI = CURRENT_STATE.REGS[instruct.rs] % CURRENT_STATE.REGS[instruct.rt];
		CURRENT_STATE.LO = CURRENT_STATE.REGS[instruct.rs] / CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "DIVU")){
		CURRENT_STATE.HI = CURRENT_STATE.REGS[instruct.rs] % CURRENT_STATE.REGS[instruct.rt];
		CURRENT_STATE.LO = CURRENT_STATE.REGS[instruct.rs] / CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "AND")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] & CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "ANDI")){
		// Check and see if this is what it actually should be
		CURRENT_STATE.REGS[instruct.rt] = CURRENT_STATE.REGS[instruct.rs] & instruct.immediate;
	}
	else if(!strcmp(instruct.op, "OR")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] | CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "ORI")){
		// Check and see if this is what it actually should be
		CURRENT_STATE.REGS[instruct.rt] = CURRENT_STATE.REGS[instruct.rs] | instruct.immediate;
	}
	else if(!strcmp(instruct.op, "XOR")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] ^ CURRENT_STATE.REGS[instruct.rt];
	}
	else if(!strcmp(instruct.op, "XORI")){
		// Check and see if this is what it actually should be
		CURRENT_STATE.REGS[instruct.rt] = CURRENT_STATE.REGS[instruct.rs] ^ instruct.immediate;
	}
	else if(!strcmp(instruct.op, "NOR")){
		CURRENT_STATE.REGS[instruct.rd] = !(CURRENT_STATE.REGS[instruct.rs] | CURRENT_STATE.REGS[instruct.rt]);
	}
	else if(!strcmp(instruct.op, "SLT")){
		if(instruct.rs < instruct.rt){
			CURRENT_STATE.REGS[instruct.rd] = 1;
		}
		else{
			CURRENT_STATE.REGS[instruct.rd] = 0;
		}
	}
	else if(!strcmp(instruct.op, "SLTI")){
		if(instruct.rs < instruct.immediate){
			CURRENT_STATE.REGS[instruct.rd] = 1;
		}
		else{
			CURRENT_STATE.REGS[instruct.rd] = 0;
		}
	}
	else if(!strcmp(instruct.op, "SLL")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] << instruct.shamt;
	}
	else if(!strcmp(instruct.op, "SRL")){
		// What is the difference between this and SRA
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] >> instruct.shamt;
	}
	else if(!strcmp(instruct.op, "SRA")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.REGS[instruct.rs] >> instruct.shamt;
	}
	//****************************** Load/Store INSTRUCTIONS ******************************
	else if(!strcmp(instruct.op, "LUI")){
		CURRENT_STATE.REGS[instruct.rt] = instruct.immediate;
		CURRENT_STATE.REGS[instruct.rt] = CURRENT_STATE.REGS[instruct.rt] << 16;
	}
	else if(!strcmp(instruct.op, "LW")){
		uint32_t memAddress = (uint32_t)instruct.rs + instruct.immediate + MEM_DATA_BEGIN;
		uint32_t mem = mem_read_32(memAddress);
		CURRENT_STATE.REGS[instruct.rt] = mem;
	}
	else if(!strcmp(instruct.op, "SW")){
		uint32_t memAddress = (uint32_t)instruct.rs + instruct.immediate + MEM_DATA_BEGIN;
		mem_write_32(memAddress,(uint32_t)instruct.rt);
	}
	else if(!strcmp(instruct.op, "LB")){
		uint32_t memAddress = (uint32_t)instruct.rs + instruct.immediate + MEM_DATA_BEGIN;
		uint32_t mem = mem_read_32(memAddress);
		CURRENT_STATE.REGS[instruct.rt] = mem;
	}
	else if(!strcmp(instruct.op, "LH")){
		uint32_t memAddress = (uint32_t)instruct.rs + instruct.immediate + MEM_DATA_BEGIN;
		uint32_t mem = mem_read_32(memAddress);
		CURRENT_STATE.REGS[instruct.rt] = mem;
	}
	else if(!strcmp(instruct.op, "SB")){
		uint32_t memAddress = (uint32_t)instruct.rs + instruct.immediate + MEM_DATA_BEGIN;
		mem_write_32(memAddress,(uint32_t)instruct.rt);
	}
	else if(!strcmp(instruct.op, "SH")){
		uint32_t memAddress = (uint32_t)instruct.rs + instruct.immediate + MEM_DATA_BEGIN;
		mem_write_32(memAddress,(uint32_t)instruct.rt);
	}
	else if(!strcmp(instruct.op, "MFHI")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.HI;
	}
	else if(!strcmp(instruct.op, "MFLO")){
		CURRENT_STATE.REGS[instruct.rd] = CURRENT_STATE.LO;
	}
	else if(!strcmp(instruct.op, "MTHI")){
		CURRENT_STATE.HI = CURRENT_STATE.REGS[instruct.rs];
	}
	else if(!strcmp(instruct.op, "MTLO")){
		CURRENT_STATE.LO = CURRENT_STATE.REGS[instruct.rs];
	}

	//******************************* Control Flow INSTRUCTIONS *************************** BEQ, BNE, BLEZ, BLTZ, BGEZ, BGTZ, J, JR, JAL,JALR
    else if(!strcmp(instruct.op, "BEQ")) {
		 if(CURRENT_STATE.REGS[instruct.rt] == CURRENT_STATE.REGS[instruct.rs]){
			 uint32_t memAddress = strtoul(instruct.address, NULL, 2);
			 CURRENT_STATE.PC += memAddress;
		 }
    }
    else if(!strcmp(instruct.op, "BNE")) {
		 if(CURRENT_STATE.REGS[instruct.rt] != CURRENT_STATE.REGS[instruct.rs]){
			 uint32_t memAddress = strtoul(instruct.address, NULL, 2);
			 CURRENT_STATE.PC += memAddress;
		 }
    }
    else if(!strcmp(instruct.op, "BLEZ")) {
		 if(CURRENT_STATE.REGS[instruct.rt] <= 0){
			 uint32_t memAddress = strtoul(instruct.address, NULL, 2);
			 CURRENT_STATE.PC += memAddress;
		 }
    }
    else if(!strcmp(instruct.op, "BGEZ")) {
		 if(CURRENT_STATE.REGS[instruct.rt] >= 0){
			 uint32_t memAddress = strtoul(instruct.address, NULL, 2);
			 CURRENT_STATE.PC += memAddress;
		 }
    }
    else if(!strcmp(instruct.op, "BGTZ")) {
		 if(CURRENT_STATE.REGS[instruct.rt] > 0){
			 uint32_t memAddress = strtoul(instruct.address, NULL, 2);
			 CURRENT_STATE.PC += memAddress;
		 }
    }
	else if(!strcmp(instruct.op, "J")) {
        uint32_t memAddress = strtoul(instruct.address, NULL, 2);
        CURRENT_STATE.PC += memAddress;
    }
    else if(!strcmp(instruct.op, "JR")) {
        CURRENT_STATE.PC = CURRENT_STATE.REGS[instruct.rs];
    }
    else if(!strcmp(instruct.op, "JAL")) {
        uint32_t memAddress = strtoul(instruct.address, NULL, 2);
        CURRENT_STATE.PC += memAddress;
        CURRENT_STATE.PC += 4;
    }
    else if(!strcmp(instruct.op, "JALR")) {
        CURRENT_STATE.REGS[31] = CURRENT_STATE.PC;
        CURRENT_STATE.PC = CURRENT_STATE.REGS[instruct.rs];
    }

	NEXT_STATE = CURRENT_STATE;
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

}

/************************************************************/
/* Initialize Memory. */
/************************************************************/
void initialize() {
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format). */
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

char* hex_to_binary(char Hexdigit)
{
	if(isalpha(Hexdigit))
		Hexdigit = toupper(Hexdigit);

	switch (Hexdigit)
	{
	case ' ':
		return "0000";
	case '0':
		return "0000";
	case '1':
		return "0001";
	case '2':
		return "0010";
	case '3':
		return "0011";
	case '4':
		return "0100";
	case '5':
		return "0101";
	case '6':
		return "0110";
	case '7':
		return "0111";
	case '8':
		return "1000";
	case '9':
		return "1001";
	case 'A':
		return "1010";
	case 'B':
		return "1011";
	case 'C':
		return "1100";
	case 'D':
		return "1101";
	case 'E':
		return "1110";
	case 'F':
		return "1111";
	default:
		return "\0";
		break;
	}
}

char FindFormat(char* instruction)
{
	char opcode[7];
	strncpy(opcode,instruction, 6);
	opcode[6] = '\0';

	if(!strcmp(opcode, "000000"))
	{
		return 'R';
	}
	else if (!strcmp(opcode, "000010") || !strcmp(opcode, "000011"))
	{
		return 'J';
	}
	else
	{
		return 'I';
	}
}

char* GetRFunction(char* instruction)
{
	if(!strcmp(instruction, "100000"))
	{
		return "ADD";
	}
	if(!strcmp(instruction, "100001"))
	{
		return "ADDU";
	}
	if(!strcmp(instruction, "100010"))
	{
		return "SUB";
	}
	if(!strcmp(instruction, "100011"))
	{
		return "SUBU";
	}
	if(!strcmp(instruction, "011000"))
	{
		return "MULT";
	}
	if(!strcmp(instruction, "011001"))
	{
		return "MULTU";
	}
	if(!strcmp(instruction, "011010"))
	{
		return "DIV";
	}
	if(!strcmp(instruction, "011011"))
	{
		return "DIVU";
	}
	if(!strcmp(instruction, "100100"))
	{
		return "AND";
	}
	if(!strcmp(instruction, "100101"))
	{
		return "OR";
	}
	if(!strcmp(instruction, "100110"))
	{
		return "XOR";
	}
	if(!strcmp(instruction, "100111"))
	{
		return "NOR";
	}
	if(!strcmp(instruction, "101010"))
	{
		return "SLT";
	}
	if(!strcmp(instruction, "000000"))
	{
		return "SLL";
	}
	if(!strcmp(instruction, "000010"))
	{
		return "SRL";
	}
	if(!strcmp(instruction, "000011"))
	{
		return "SRA";
	}
	if(!strcmp(instruction, "010000"))
	{
		return "MFHI";
	}
	if(!strcmp(instruction, "010010"))
	{
		return "MFLO";
	}
	if(!strcmp(instruction, "010001"))
	{
		return "MTHI";
	}
	if(!strcmp(instruction, "010011"))
	{
		return "MTLO";
	}
	if(!strcmp(instruction, "001000"))
	{
		return "JR";
	}
	if(!strcmp(instruction, "001001"))
	{
		return "JALR";
	}
	return NULL;
}

char* GetIFunction(char* instruction, char* rt)
{
	if(!strcmp(instruction, "001000"))
	{
		return "ADDI";
	}
	else if(!strcmp(instruction, "001001"))
	{
		return "ADDIU";
	}
	else if(!strcmp(instruction, "001100"))
	{
		return "ANDI";
	}
	else if(!strcmp(instruction, "001101"))
	{
		return "ORI";
	}
	else if(!strcmp(instruction, "001110"))
	{
		return "XORI";
	}
	else if(!strcmp(instruction, "001010"))
	{
		return "SLTI";
	}
	else if(!strcmp(instruction, "100011"))
	{
		return "LW";
	}
	else if(!strcmp(instruction, "100000"))
	{
		return "LB";
	}
	else if(!strcmp(instruction, "100001"))
	{
		return "LH";
	}
	else if(!strcmp(instruction, "001111"))
	{
		return "LUI";
	}
	else if(!strcmp(instruction, "101011"))
	{
		return "SW";
	}
	else if(!strcmp(instruction, "101000"))
	{
		return "SB";
	}
	else if(!strcmp(instruction, "101001"))
	{
		return "SH";
	}
	else if(!strcmp(instruction, "000100"))
	{
		return "BEQ";
	}
	else if(!strcmp(instruction, "000101"))
	{
		return "BNE";
	}
	else if(!strcmp(instruction, "000110"))
	{
		return "BLEZ";
	}
	else if(!strcmp(instruction, "000001") && !strcmp(rt, "00000"))
	{
		return "BLTZ";
	}
	else if(!strcmp(instruction, "000001") && !strcmp(rt, "00001"))
	{
		return "BGEZ";
	}
	else if(!strcmp(instruction, "000111"))
	{
		return "BGTZ";
	}
	else{
		return "Instruction not found";
	}

	return NULL;
}

char* GetJFunction(char* instruction)
{
	if(!strcmp(instruction, "000010"))
	{
		return "J";
	}
	if(!strcmp(instruction, "000011"))
	{
		return "JAL";
	}
	if(!strcmp(instruction, "000011"))
	{
		return "JAL";
	}
	return NULL;
}

int convertBinarytoDecimal(char * binary) {
	int num = atoi(binary);
    unsigned int dec_value = 0;

    // Initializing base value to 1, i.e 2^0
    int base = 1;

    unsigned int temp = num;
    while (temp) {
        int last_digit = temp % 10;
        temp = temp / 10;

        dec_value += last_digit * base;

        base = base * 2;
    }

    return dec_value;
}


void returnRFormat(char* instruction, MIPS* hold) {
	// Read in the rs register
	char rs[6];
	strncpy(rs, &instruction[6], 5);
	rs[5] = '\0';

	// Read in the rt register
	char rt[6];
	strncpy(rt, &instruction[11], 5);
	rt[5] = '\0';

	// read in the rd register
	char rd[6];
	strncpy(rd, &instruction[16], 5);
	rd[5] = '\0';

	// read in the op code
	char op[7];
	strncpy(op, &instruction[0], 6);
	op[6] = '\0';

	char shamnt[6];
	strncpy(shamnt, &instruction[21],5);
	shamnt[5] = '\0';

	char func[7];
	strncpy(func, &instruction[26],6);
	func[6] = '\0';

	printf("%s %s, %s, %s\n",GetRFunction(func),returnRegister(rd), returnRegister(rs), returnRegister(rt));
	hold->op = GetRFunction(func);
	hold->rd = convertBinarytoDecimal(rd);
	hold->rs = convertBinarytoDecimal(rs);
	hold->rt = convertBinarytoDecimal(rt);
	hold->shamt = atoi(shamnt);
	hold->funct = func;
	hold->immediate = 0;
}

void returnIFormat(char* instruction, MIPS* hold) {
	// read in the rs register
	char rs[6];
	strncpy(rs, &instruction[6], 5);
	rs[5] = '\0';

	// read in the rt register
	char rt[6];
	strncpy(rt, &instruction[11], 5);
	rt[5] = '\0';

	// read in the imm value and convert to decimal
	char imm[17];
	strncpy(imm, &instruction[16], 16);
	imm[16] = '\0';
	long unsigned int imm_hex = strtol(imm, NULL, 2);

	// read in the op code
	char op[7];
	strncpy(op, &instruction[0], 6);
	op[6] = '\0';

	if(!strcmp(GetIFunction(op, rt), "LUI"))
	{
		printf("%s %s, x%lx\n",GetIFunction(op, rt),returnRegister(rt), imm_hex);
		hold->op = GetIFunction(op,rt);
		hold->rt = convertBinarytoDecimal(rt);
		hold->rs = -1;
		hold->immediate = imm_hex;
		hold->shamt = 0;
		hold->funct = "";
		hold->rd = -1;
		hold->address = "";
	}
	else if(!strcmp(GetIFunction(op, rt), "SW") || !strcmp(GetIFunction(op, rt), "SB") || !strcmp(GetIFunction(op, rt), "SH"))
	{
		printf("%s %s, %d(%s)\n",GetIFunction(op, rt),returnRegister(rs),(int)imm_hex,returnRegister(rt));
		hold->op = GetIFunction(op,rt);
		hold->rs = convertBinarytoDecimal(rs);
		hold->rt = convertBinarytoDecimal(rt);
		hold->immediate = imm_hex;
		hold->shamt = 0;
		hold->funct = "";
		hold->rd = -1;
		hold->address = "";
	}
	else if(!strcmp(GetIFunction(op, rt), "LW"))
	{
		printf("%s %s, %d(%s)\n",GetIFunction(op, rt),returnRegister(rt),(int)imm_hex,returnRegister(rs));
		hold->op = GetIFunction(op,rt);
		hold->rs = convertBinarytoDecimal(rs);
		hold->rt = convertBinarytoDecimal(rt);
		hold->immediate = imm_hex;
		hold->shamt = 0;
		hold->funct = "";
		hold->rd = -1;
		hold->address = "";
	}
	else if(!strcmp(GetIFunction(op, rt), "BEQ") || !strcmp(GetIFunction(op, rt), "BNE"))
	{
		printf("%s %s, %s, x%lx\n",GetIFunction(op, rt),returnRegister(rs),returnRegister(rt), imm_hex);
		hold->op = GetIFunction(op,rt);
		hold->rs = convertBinarytoDecimal(rs);
		hold->rt = convertBinarytoDecimal(rt);
		hold->immediate = imm_hex;
		hold->shamt = 0;
		hold->funct = "";
		hold->rd = -1;
		hold->address = "";
	}
	else
	{
		printf("%s %s, %s, x%lx\n",GetIFunction(op, rt),returnRegister(rt), returnRegister(rs), imm_hex);
		hold->op = GetIFunction(op,rt);
		hold->rs = convertBinarytoDecimal(rs);
		hold->rt = convertBinarytoDecimal(rt);
		hold->immediate = imm_hex;
		hold->shamt = 0;
		hold->funct = "";
		hold->rd = -1;
		hold->address = "";
	}
}

void returnJFormat(char* instruction, MIPS* hold) {
	// read in the op code
	char op[7];
	strncpy(op, &instruction[0], 6);
	op[6] = '\0';

	// read in the Jump Addresss
	char address[27];
	strncpy(address, &instruction[6], 26);
	printf("%s %s\n", GetJFunction(op), address);
	hold->op = GetJFunction(op);
	hold->rs = -1;
	hold->rt = -1;
	hold->immediate = 0;
	hold->shamt = 0;
	hold->funct = "";
	hold->rd = -1;
	hold->address = address;
}

char* returnRegister(char* reg){
	if (!strcmp(reg, "00000")) return "$zero";
	if (!strcmp(reg, "00001")) return "$at";
	if (!strcmp(reg, "00010")) return "$v0";
	if (!strcmp(reg, "00011")) return "$v1";
	if (!strcmp(reg, "00100")) return "$a0";
	if (!strcmp(reg, "00101")) return "$a1";
	if (!strcmp(reg, "00110")) return "$a2";
	if (!strcmp(reg, "00111")) return "$a3";
	if (!strcmp(reg, "01000")) return "$t0";
	if (!strcmp(reg, "01001")) return "$t1";
	if (!strcmp(reg, "01010")) return "$t2";
	if (!strcmp(reg, "01011")) return "$t3";
	if (!strcmp(reg, "01100")) return "$t4";
	if (!strcmp(reg, "01101")) return "$t5";
	if (!strcmp(reg, "01110")) return "$t6";
	if (!strcmp(reg, "01111")) return "$t7";
	if (!strcmp(reg, "10000")) return "$s0";
	if (!strcmp(reg, "10001")) return "$s1";
	if (!strcmp(reg, "10010")) return "$s2";
	if (!strcmp(reg, "10011")) return "$s3";
	if (!strcmp(reg, "10100")) return "$s4";
	if (!strcmp(reg, "10101")) return "$s5";
	if (!strcmp(reg, "10110")) return "$s6";
	if (!strcmp(reg, "10111")) return "$s7";
	if (!strcmp(reg, "11000")) return "$t8";
	if (!strcmp(reg, "11001")) return "$t9";
	if (!strcmp(reg, "11010")) return "$k0";
	if (!strcmp(reg, "11011")) return "$k1";
	if (!strcmp(reg, "11100")) return "$gp";
	if (!strcmp(reg, "11101")) return "$sp";
	if (!strcmp(reg, "11110")) return "$fp";
	if (!strcmp(reg, "11111")) return "$ra";
	return NULL;
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format). */
/************************************************************/
void print_instruction(uint32_t addr){
	//Read in the instructions
	uint32_t instr = mem_read_32(addr);
	char string[9];

	sprintf(string,"%08x", instr);

	// Check for syscall
	if(instr == 0xC)
		return;

	char fullbinay[33];
	fullbinay[0] = '\0';

	MIPS junk;
	for(int i = 0; i < 8; i++)
	{
		strcat(fullbinay, hex_to_binary(string[i]));
	}

	switch(FindFormat(fullbinay)) {
		case 'R': {
			returnRFormat(fullbinay,&junk);
			break;
		}
		case 'I': {
			returnIFormat(fullbinay,&junk);
			break;
		}
		case 'J': {
			returnJFormat(fullbinay,&junk);
			break;
		}
		default: {
			printf("You messed up.\n");
			break;
		}
	}
}

/***************************************************************/
/* Main function. */
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
