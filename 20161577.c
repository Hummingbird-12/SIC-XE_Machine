/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *                    Sogang University                          *
 *         Department of Computer Science and Engineering        *
 *                                                               *
 * Subject name: System Programming                              *
 * Project title: [1] SIC/XE Machine - The Basics                *
 *                                                               *
 * Author: Inho Kim                                              *
 * Student ID: 20161577                                          *
 *                                                               *
 * File name: 20161577.c                                         *
 * File description: Main file for the project.                  *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "20161577.h"
#include "cmdProc.h"
#include "shell.h"
#include "memory.h"
#include "hash.h"

SYMBOL_ENTRY* symTable;
char directives[7][6] = {
    "START",
    "END",
    "BASE",
    "BYTE",
    "WORD",
    "RESB",
    "RESW"
};

void assembleCMD(INPUT_CMD);
ASM_SRC* parseASM(char*, int);
void symbolCMD();
void symTableAdd(char*, int);
bool symTableSearch(char*);
void symTableFree();

int main() {
    char inp[CMD_LEN];  // input string
    char tmp[CMD_LEN];  // temporary string to copy input
    int i, j;
    INPUT_CMD input;    // storage for parsed input

    resetCMD(); // initialize memory
    hashCreate(); // create hash table of opcodes

    while(1) {
        printf("sicsim> ");
        fgets(inp, CMD_LEN, stdin); // get input string
        inp[strlen(inp) - 1] = '\0'; // replace \n with null character

        // copy input string to tmp but place one space before and after comma ','
        j = 0;
        for(i = 0; inp[i]; i++) {
            if(inp[i] == ',') {
                strcpy(tmp + j, " , "); // place space around commma ','
                j += 3;
            }
            else
                tmp[j++] = inp[i];
        }
        tmp[j] = '\0';

        input = findCMD(tmp); // find the command format from input string
        if(input.cmd < invFormat)
            histAdd(inp); // if command is not invalid add to history

        // call function for each command
        switch(input.cmd) {
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
                dumpCMD(input);
                break;
            case edit:
                editCMD(input);
                break;
            case fill:
                fillCMD(input);
                break;
            case reset:
                resetCMD();
                break;
            case op:
                opCMD(input);
                break;
            case oplist:
                oplistCMD();
                break;
            case type:
                typeCMD(input);
                break;
            case assemble:
                assembleCMD(input);
                break;
            case symbol:
                symbolCMD();
                break;
            case invFormat:
                invFormatCMD();
                break;
            case invHex:
                invHexCMD();
                break;
            case invVal:
                invValCMD();
                break;
            case invFile:
                invFileCMD();
                break;
        }
    }
    return 0;
}

void assembleCMD(INPUT_CMD ipcmd) {
    FILE* fp = fopen(ipcmd.arg[0], "r");
    char source[ASM_LEN];
    ASM_SRC* parsed;
    int location = 0;

    if(symTable) {
        symTableFree();
        symTable = NULL;
    }

    if(!fp) {
        puts("ERROR: File not found.");
        return;
    }
    while(fgets(source, ASM_LEN, fp) != NULL) {
        parsed = parseASM(source, location);
        char tmp[30];
        switch(parsed->type) {
            case ERROR:
                strcpy(tmp, "ERROR");
                switch(parsed->errorCode) {
                    case SYMBOL:
                        strcat(tmp, "(SYM)");
                        break;
                    case INSTRUCTION:
                        strcat(tmp, "(INST)");
                        break;
                    case OPERAND:
                        strcat(tmp, "(OPR)");
                        break;
                    default:
                        break;
              }
                break;
            case INST:
                strcpy(tmp, "INST");
                break;
            case PSEUDO:
                strcpy(tmp, "PSEUDO");
                break;
            case COMMENT:
                strcpy(tmp, "COMMENT");
                break;
        }
        printf("%s : %s", tmp, source);
        if(!strcmp(parsed->inst, "END"))
            break;
    }
    if(fclose(fp))
        puts("WARNING: Error closing file.");
}

void symbolCMD() {
}

ASM_SRC* parseASM(char* source, int location) {
    char delim[] = " \t\n";
    //char tokens[5][ASM_LEN] = {'\0'};
    char tmp[ASM_LEN] = {'\0'};
    char *tok;
    int i;//, j, tokCnt = 0;
    ASM_SRC* parseResult;
    HASH_ENTRY* bucket;

    parseResult = (ASM_SRC*) malloc(sizeof(ASM_SRC));
    parseResult->hasLabel = false;
    parseResult->type = INST;
    parseResult->errorCode = OK;
    parseResult->indexing = false;
    parseResult->location = location;

    strcpy(tmp, source);
    tok = strtok(tmp, delim);

    if(tok[0] == '.') { // comment found
        parseResult->type = COMMENT;
        return parseResult;
    }

    // look for directives
    for(i = 0; i < 7; i++)
        if(!strcmp(tok, directives[i])) { // pseudo instruction found
            parseResult->type = PSEUDO;
            strcpy(parseResult->inst, tok);
            break;
        }

    if(parseResult->type != PSEUDO) { // if not a pseudo instruction
        bucket = bucketSearch(tok + (tok[0] == '+' ? 1 : 0) );
        
        // if first field is a label
        if(!bucket) {
            if(symTableSearch(tok)) { // symbol already exists
                parseResult->type = ERROR;
                parseResult->errorCode = SYMBOL;
                return parseResult;
            }
            else { // new symbol found, add to SYMTAB and move on
                parseResult->hasLabel = true;
                strcpy(parseResult->label, tok);
                symTableAdd(tok, location);
                tok = strtok(NULL, delim);
                bucket = bucketSearch(tok + (tok[0] == '+' ? 1 : 0) );
            }
        }

        // look for directives
        for(i = 0; i < 7; i++)
            if(!strcmp(tok, directives[i])) { // pseudo instruction found
                parseResult->type = PSEUDO;
                strcpy(parseResult->inst, tok);
                break;
            }
       
        // if field is an instruction
        if(bucket) {
            parseResult->type = INST;
            parseResult->format = bucket->format; // get format from hash table
            if(tok[0] == '+') {
                // not a format 3/4 instruction
                if(bucket->format != f34) {
                    printf("[A]");
                    parseResult->type = ERROR;
                    parseResult->errorCode = INSTRUCTION;
                    return parseResult;
                }
                // found a format 4 instruction
                strcpy(parseResult->inst, tok);
                parseResult->format = format4;
            }
        }
        // invalid instruction
        else if(parseResult->type != PSEUDO){
            printf("[B]");
            parseResult->type = ERROR;
            parseResult->errorCode = INSTRUCTION;
            return parseResult;
        }
    }

    return parseResult;

    /*
    i = 0;
    while(tok && i < 5) {
        strcpy(tokens[i++], tok);
        tok = strtok(NULL, delim);
    }

    
       j = 0;
       for(i = 0; source[i]; i++) {
       if(source[i] == ',') {
       strcpy(str + j, " , ");
       j += 3;
       }
       else
       str[j++] = source[i];
       }

       tok = strtok(str, delim);
       if(!tok)
       return parseResult;
       if(!strcmp(tok, ".")) { // check for comment
       parseResult = (ASM_SRC*) malloc(sizeof(ASM_SRC));
       parseResult->label[0] = '.';
       return parseResult;
       }
       while(tok) {
       strcpy(tokens[tokCnt++], tok);
       tok = strtok(NULL, delim);
       if(tokCnt == 5)
       break;
       }
       if(tok) // invalid field exists
       return parseResult; // returns NULL pointer

       parseResult = (ASM_SRC*) malloc(sizeof(ASM_SRC));
       if((bucket = bucketFound(tokens[0]))) { // if first token is the instruction
       strcpy(parseResult->inst, tokens[0]);
       i = 1;
       }
       else if ((bucket = bucketFound(tokens[1]))) { // if second token is the instruction
       strcpy(parseResult->label, tokens[0]);
       strcpy(parseResult->inst, tokens[1]);
       i = 2;
       }
       else if(!bucket) {
       free(parseResult);
       parseResult = NULL;
       return parseResult;
       }

       for(j = 0; i + j < tokCnt; j++) {
       strcpy(parseResult->operand[j], tokens[i + j]);
       if(i + j + 1 < tokCnt && strcmp(tokens[i + j + 1], ",")) { // no comma in between operands
       free(parseResult);
       parseResult = NULL;
       return parseResult;
       }
       j++;
       }

       return parseResult;
       */
}

void symTableAdd(char* symbol, int address) {
    SYMBOL_ENTRY* cur = symTable;
    SYMBOL_ENTRY* newEntry = (SYMBOL_ENTRY*) malloc(sizeof(SYMBOL_ENTRY));
    strcpy(newEntry->symbol, symbol);
    newEntry->address = address;
    newEntry->next = NULL;

    if(!symTable) {
        symTable = newEntry;
        return;
    }
    while(cur->next)
        cur = cur->next;
    cur->next = newEntry;
}

bool symTableSearch(char* symbol) {
    SYMBOL_ENTRY* cur = symTable;
    while(cur) {
        if(!strcmp(symbol, cur->symbol))
            return true;
        cur = cur->next;
    }
    return false;
}

void symTableFree() {
    SYMBOL_ENTRY *cur, *next;
    cur = symTable;
    while(cur) {
        next = cur->next;
        free(cur);
        cur = next;
    }
    symTable = NULL;
}
