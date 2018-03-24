/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 																 *
 * 					  Sogang University							 *
 *        Department of Computer Science and Engineering		 *
 *																 *
 * Subject name: System Programming								 *
 * Project title: [1] SIC/XE Machine - The Basics				 *
 *																 *
 * Author: Inho Kim												 *
 * Student ID: 20161577											 *
 *																 *
 * File name: cmdProc.c											 *
 * File description: Tasks related to process input by user.	 *
 *																 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "20161577.h"
#include "cmdProc.h"

// store command format in structure array to compare
COMMAND cmdList[CMD_CNT] = {
	{ "help", "h", shell, help, false  }, { "dir", "d", shell, dir, false  },
	{ "quit", "q", shell, quit, false  }, { "history", "hi", shell, hist, false  },
	{ "dump", "du", memory, dump, true  }, { "edit", "e", memory, edit, true  },
	{ "fill", "f", memory, fill, true  }, { "reset", "reset", memory, reset, false  },
	{ "opcode", "opcode", opcode, op, true  }, { "opcodelist", "opcodelist", opcode, oplist, false },
	{ "invalid", "invalid", invalid, invFormat, true  }

};

INPUT_CMD findCMD(char* str) {
	int i, j;
	char delim[] = " \t\n"; // characters used to tokenize
	char inp[CMD_LEN];
	char* tok;
	INPUT_CMD ipcmd;

	strcpy(inp, str); // copy input string

	// initialize as invalid
	ipcmd.cmd = invFormat;
	ipcmd.argCnt = 0;
	
	if(!strlen(str))
		return ipcmd; // if empty string, return as invalid

	tok = strtok(inp, delim); // first word of input
	if(!tok)
		return ipcmd; // emtpy token
	for(i = 0; i < CMD_CNT - 1; i++)
		if(!strcmp(tok, cmdList[i].str) || !strcmp(tok, cmdList[i].abb)) {
			ipcmd.cmd = cmdList[i].func; // if input command matches one of hard coded commands
			break;
		}

	if(ipcmd.cmd == invFormat) // invalid command
		return ipcmd;

	// get arguments
	j = 0;
	while(tok) {
		tok = strtok(NULL, delim); // next token (NOT expected comma if valid command)
		if(!j && !tok) // no argument for command 
			break;
		if((j && !tok) || tok[0] == ',') { // there was a previous argument but empty token or comma found
			ipcmd.cmd = invFormat; //invalid command
			return ipcmd;
		}
		strcpy(ipcmd.arg[j++], tok); // copy argument to input command structure
		tok = strtok(NULL, delim); // next token (expected a comma if valid command)
		if(tok && tok[0] != ',') { // if token not empty, expected a comma
			ipcmd.cmd = invFormat;
			return ipcmd;
		}
	}
	ipcmd.argCnt = j; // save argument count

	// after input string parsed, do further check for validity
	switch(testValidInput(ipcmd, cmdList[i])) { // get error type, if any
		case FORMAT:
			ipcmd.cmd = invFormat;
			break;
		case HEX:
			ipcmd.cmd = invHex;
			break;
		case VALUE:
			ipcmd.cmd = invVal;
			break;
		default: // no error found
			break;
	}
	return ipcmd;
}

ERROR_CODE testValidInput(INPUT_CMD ipcmd, COMMAND format) {
	int i;
	int arg[3];
	ERROR_CODE code = SAFE; // initialize as correct command
	if(ipcmd.cmd == invFormat)
		return FORMAT;

	// check argument count
	switch(ipcmd.cmd) {
		// strictly 0 arguments
		case help:
		case dir:
		case quit:
		case hist:
		case reset:
		case oplist:
			if(ipcmd.argCnt)
				code = FORMAT;
			break;
		// strictly 1 argument
		case op:
			if(ipcmd.argCnt != 1)
				code = FORMAT;
			break;
		// strictly 2 arguments
		case edit:
			if(ipcmd.argCnt != 2)
				code = FORMAT;
			break;
		// strictly 3 arguments
		case fill:
			if(ipcmd.argCnt != 3)
				code = FORMAT;
			break;
		// need less than 3
		case dump:
			if(ipcmd.argCnt > 2)
				code = FORMAT;
			break;
		default:
			break;
	}
	if(code == FORMAT)
		return code;

	// check hexadecimal number if command is memory-related
	if(format.type == memory) {
		for(i = 0; i < ipcmd.argCnt; i++)
			if((arg[i] = hexToDec(ipcmd.arg[i])) == -1)
				code = HEX;
		// check with each command's criteria
		switch(ipcmd.cmd) {
			case edit:
				if(arg[0] >= MEM_SIZE || arg[1] > 255)
					code = VALUE;
				break;
			case fill:
				if(arg[0] >= MEM_SIZE || arg[1] >= MEM_SIZE || arg[0] > arg[1] || arg[2] > 255)
					code = VALUE;
				break;
			case dump:
				switch(ipcmd.argCnt) {
					case 2:
						if(arg[1] >= MEM_SIZE || arg[0] > arg[1])
							code = VALUE;
					case 1:
						if(arg[0] >= MEM_SIZE)
							code = VALUE;
						break;
				}
				break;
			default:
				break;
		}
	}
	return code;
}

void invFormatCMD() {
	puts("ERROR: Invalid command.");
	puts("Type \"help\" for list and formats of commands.");
}

void invHexCMD() {
	puts("ERROR: Incorrect hexadecimal.");
}

void invValCMD() {
	puts("ERROR: Invalid address.");
	puts("Memory size:\t\t1MB [0x00000 ~ 0xFFFFF]");
	puts("Edit/Fill value range:\t 1B [0x00 ~ 0xFF]");
}

int hexToDec(char* hex) {
	int i, dec = 0, multiplier = 1;
	for(i = strlen(hex) - 1; i >= 0; i--) {
		if(!isxdigit(hex[i])) // check if character if hexadecimal digit
			return -1;
		dec += multiplier * (isdigit(hex[i]) ? (hex[i] - '0') : (toupper(hex[i]) - 'A' + 10));
		multiplier *= 16;
	}
	return dec;
}
