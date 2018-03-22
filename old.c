#include "main.h"

int main() {
	char inp[CMD_LEN];
	USR_CMD cmdExec;

	resetCMD();

	while(1) {
		printf("sicsim> ");
		fgets(inp, CMD_LEN, stdin);
		inp[strlen(inp) - 1] = '\0';
		cmdExec = findCMD(inp);
		if(cmdExec.cmd != inv)
			hist_add(inp);

		switch(cmdExec.cmd) {
			case help:
				helpCMD();
				break;
			case dir:
				dirCMD();
				break;
			case quit:
				quitCMD();
				break;
			case hist:
				histCMD();
				break;
			case dump:
				dumpCMD(cmdExec);
				break;
			case edit:
				editCMD(cmdExec);
				break;
			case fill:
				fillCMD(cmdExec);
				break;
			case reset:
				resetCMD();
				break;
			case op:
				break;
			case oplist:
				break;
			case inv:
				invCMD();
				break;
		}
	}
	return 0;
}

USR_CMD findCMD(char* str) {
	int i, j;
	char delim[] = " ,\t\n";
	char inp[CMD_LEN];
	char* tok;
	USR_CMD u_cmd;

	strcpy(inp, str);

	u_cmd.cmd = inv; // initialize as invalid
	u_cmd.param_cnt = 0;

	tok = strtok(inp, delim); // first word of input
	for(i = 0; i < CMD_CNT - 1; i++)
		if(!strcmp(tok, cmdList[i].str) || !strcmp(tok, cmdList[i].abb)) {
			u_cmd.cmd = cmdList[i].func;
			break;
		}

	if(u_cmd.cmd == inv) // invalid command
		return u_cmd;

	if(cmdList[i].param) { // get parameters
		j = 0;
		while((tok = strtok(NULL, delim)))
			strcpy(u_cmd.param[j++], tok);
		u_cmd.param_cnt = j;
		if(j > 3)
			u_cmd.cmd = inv;
	}
	else if((tok = strtok(NULL, delim))) // not expected parameter
		u_cmd.cmd = inv; // set as invalid

	return u_cmd;
}

void helpCMD() {
	printf("h[elp]\n"
			"d[ir]\n"
			"q[uit]\n"
			"hi[story]\n"
			"du[mp] [start, end]\n"
			"e[dit] address, value\n"
			"f[ill] start, end, value\n"
			"reset\n"
			"opcode mnemonic\n"
			"opcodelist\n");
}

void dirCMD() {
	DIR* dir = opendir("."); // current directory
	char* e_str;
	char path[258] = "./"; // entry path string
	ENTRY* ent; // entry
	STBUF buf; // stat

	if(!dir) {
		puts("ERROR opening directory...");
		return;
	}
	ent = readdir(dir); // read entry
	while(ent) {
		path[2] = '\0'; // clear path string
		e_str = ent->d_name; // entry name
		stat(strcat(path, e_str), &buf);
		printf("\t%-s", e_str); // print entry name

		if(S_ISDIR(buf.st_mode)) // check for directory
			printf("/");
		else if(buf.st_mode & S_IXUSR) // check for exec file
			printf("*");

		ent = readdir(dir); // read next entry
	}
	closedir(dir);
	puts("");
}

void quitCMD() {
	puts("Exiting SIC...");
	hist_free();
	exit(0);
}

void histCMD() {
	HIST_NODE* cur = hist_head;
	int cnt = 1;

	while(cur) {
		printf("\t%-3d  ", cnt++);
		puts(cur->str);
		cur = cur->next;
	}
}

void dumpCMD(USR_CMD uscmd) {
	static int st = 0;
	int i, j, ed;
	char* hex;
	char tmp[3] = {'\0'};
	ed = st + 160;
	if(uscmd.param_cnt > 2) {
		invCMD();
		return;
	}
	if(uscmd.param_cnt) {
		st = hexToDec(uscmd.param[0]);
		if(uscmd.param_cnt == 1) {
			ed = st + 160;
			strcpy(uscmd.param[1], "100001");
		}
		else
			ed = hexToDec(uscmd.param[1]) + 1;
		if(!testValidAdr(uscmd.param[0], uscmd.param[1])) {
			invCMD();
			return;
		}
	}
	if(ed >= MEM_SIZE)
		ed = MEM_SIZE;

	hex = decToHex(st / 16 * 16);
	printf("%s ", hex);
	free(hex);
	for(i = st / 16 * 16; i < st; i++)
		printf("   ");
	for(i = st; i < ed; i++) {
		printf("%c%c ", mem[2 * i], mem[2 * i + 1]);
		if(!((i + 1) % 16) && (i + 1) <= ed) {
			printf("; ");
			for(j = 0; j < 10; j++) {
				strncpy(tmp, mem + (i - 15 + j) * 2, 2);
				if(hexToDec(tmp) >= 32 && hexToDec(tmp) <= 126)
					printf("%c", hexToDec(tmp));
				else
					printf(".");
			}
			puts("");
			if(i + 1 != ed) {
				hex = decToHex(i + 1);
				printf("%s ", hex);
				free(hex);
			}
		}
	}
	if(i % 16) {
		while(i++ % 16)
			printf("   ");
		printf("; ");
		for(j = 0; j < 10; j++) {
			strncpy(tmp, mem + (i - 17 + j) * 2, 2);
			if(hexToDec(tmp) >= 32 && hexToDec(tmp) <= 126)
				printf("%c", hexToDec(tmp));
			else
				printf(".");
		}

		puts("");
	}
	st = i;
}

void editCMD(USR_CMD uscmd) {
	int i, add = hexToDec(uscmd.param[0]);
	if(uscmd.param_cnt != 2) {
		invCMD();
		return;
	}
	for(i = 0; i < 2; i++)
		if(isalpha(uscmd.param[1][i]))
			uscmd.param[1][i] = toupper(uscmd.param[1][i]);
	strncpy(mem + add * 2, uscmd.param[1], 2);
}

void fillCMD(USR_CMD uscmd) {
	int st, ed, i;
	char* hex;
	USR_CMD tmp;
	tmp.param_cnt = 2;

	if(uscmd.param_cnt != 3) {
		invCMD();
		return;
	}
	if(!testValidAdr(uscmd.param[0], uscmd.param[1])) {
		invCMD();
		return;
	}
	if(!isValidHex(uscmd.param[2]) || strlen(uscmd.param[2]) > 2) {
		invCMD();
		return;
	}
	st = hexToDec(uscmd.param[0]);
	ed = hexToDec(uscmd.param[1]) + 1;
	for(i = 0; i < ed - st; i++) {
		hex = decToHex(st + i);
		strcpy(tmp.param[0], hex);
		strcpy(tmp.param[1], uscmd.param[2]);
		editCMD(tmp);
		free(hex);
	}
}

void resetCMD() {
	int i;
	for(i = 0; i < MEM_VLEN * MEM_HLEN; i++)
		mem[i] = '0';
}

void opCMD(USR_CMD uscmd) {

}

void oplistCMD() {

}

void invCMD() {
	puts("ERROR: Invalid command.");
	puts("Type \"help\" for list and formats of commands.");
}

void hist_add(char* str) {
	HIST_NODE* cur = hist_head;
	HIST_NODE* new_hist = malloc(sizeof(HIST_NODE));
	strcpy(new_hist->str, str);
	new_hist->next = NULL;

	if(!hist_head) {
		hist_head = new_hist;
		return;
	}
	while(cur->next)
		cur = cur->next;
	cur->next = new_hist;
}

void hist_free() {
	HIST_NODE* cur = hist_head;
	HIST_NODE* nex;
	while(cur) {
		nex = cur->next;
		free(cur);
		cur = nex;
	}
	hist_head = NULL;
}

char* decToHex(int dec) {
	char* hex = malloc(sizeof(char) * 6);
	int i = 4;
	strcpy(hex, "00000\0");
	while(dec) {
		hex[i--] = (dec % 16 < 10) ? dec % 16 + '0' : dec % 16 - 10 + 'A';
		dec /= 16;
	}
	return hex;
}

/*
char* hexToBin(char* hex) {
	char* bin = malloc(sizeof(char) * (ceil(strlen(hex) / 4.0) + 1));
	int i, j;
	for(i = strlen(hex) - 1; i >= 0; i -= 4) {
		for(j = i; j < i - 4; j--)
	}
}
*/

int hexToDec(char* hex) {
	int i, dec = 0, multiplier = 1;
	for(i = strlen(hex) - 1; i >= 0; i--) {
		if(!isalnum(hex[i]))
			return -1;
		dec += multiplier * (isdigit(hex[i]) ? (hex[i] - '0') : (toupper(hex[i]) - 'A' + 10));
		multiplier *= 16;
	}
	return dec;
}

bool isValidHex(char* str) {
	int i;
	for(i = 0; str[i]; i++)
		if(!isxdigit(str[i]))
			return false;
	return true;
}

bool testValidAdr(char* start, char* end) {
	int st = hexToDec(start), ed = hexToDec(end);
	if(st + ed < 0 || st > ed || st > MEM_VLEN * MEM_HLEN)
		return false;
	return true;
}