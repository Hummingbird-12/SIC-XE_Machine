#include "20161577.h"
#include "linkedList.h"
#include "memory.h"
#include "linkLoader.h"

void pAddrCMD(INPUT_CMD ipcmd) {
    progAddr = strtol(ipcmd.arg[0], NULL, 16);
}

bool loaderCMD(INPUT_CMD ipcmd) {
    FILE* objFptr[3];
    int progLen;
    int i;

    objFptr[0] = objFptr[1] = objFptr[2] = NULL;
    extSymTableFree();

    for(i = 0; i < ipcmd.argCnt; i++) {
        if(strcmp(ipcmd.arg[i] + strlen(ipcmd.arg[i]) - 4, ".obj")) {
            puts("ERROR: Non-object file selected.");
            fcloseObj(objFptr);
            return false;
        }
        if(!(objFptr[i] = fopen(ipcmd.arg[i], "r"))) {
            printf("ERROR: %s not found.\n", ipcmd.arg[i]);
            fcloseObj(objFptr);
            return false;
        }
    }
    progLen = linkLoaderPass1(objFptr);
    if(!progLen)
        return false;
    printList(extSymTable, printCntSecTable);
    printf("---------------------------------------------------------\n");
    printf("\t\t\t\tTotal length\t%04X\n", progLen);

    for(i = 0; i < 3; i++)
        rewind(objFptr[i]);
    linkLoaderPass2(objFptr);
    fcloseObj(objFptr);
    return true;
}

int linkLoaderPass1(FILE** objFptr) {
    int i = 0, j;
    int CSADDR = 0;
    char record[101];
    char csName[CS_LEN], esName[CS_LEN], addr[CS_LEN];
    CNT_SEC* newCntSec = NULL;
    EXT_SYMBOL* newExtSym = NULL;

    newCntSec = (CNT_SEC*) malloc(sizeof(CNT_SEC));
    newCntSec->stAddress = CSADDR = progAddr; // start address of first control section

    while(objFptr[i]) {
        fgets(record, 101, objFptr[i]); // header record
        if(record[strlen(record) - 1] == '\n')
            record[strlen(record) - 1] = '\0';
        newCntSec->length = hexToDec(record + strlen(record) - 6); // control section length
        strncpy(csName, record + 1, CS_LEN - 1); // get control section name
        if(record[0] != 'H') {
            printf("ERROR: missing header record in .obj file number %d.\n", i + 1);
            extSymTableFree();
            free(newCntSec);
            return 0;
        }
        if(searchCS(csName)) { // same name for multiple control sections
            puts("ERROR: overlapping control section name.");
            extSymTableFree();
            free(newCntSec);
            return 0;
        }
        else { // add new control section to ESTAB
            strcpy(newCntSec->csName, csName);
            addToList(&extSymTable, (void*) newCntSec);
        }

        fgets(record, 101, objFptr[i]); // read next record
        if(record[strlen(record) - 1] == '\n')
            record[strlen(record) - 1] = '\0';
        while(record[0] != 'E') {
            if(record[0] == 'D') { // if a D record is found
                for(j = 1; record[j]; j += 12) {
                    strncpy(esName, record + j, CS_LEN - 1); // get symbol name
                    strncpy(addr, record + j + 6, CS_LEN - 1); // get symbol address
                    if(searchES(esName) != -1) { // same name for multiple symbols in single control section
                        puts("ERROR: overlapping symbol name.");
                        extSymTableFree();
                        return 0;
                    }
                    newExtSym = (EXT_SYMBOL*) malloc(sizeof(EXT_SYMBOL));
                    strncpy(newExtSym->symName, esName, CS_LEN - 1);
                    newExtSym->address = hexToDec(addr) + CSADDR;
                    addToList(&(newCntSec->extSym), (void*) newExtSym);
                }
            }
            fgets(record, 101, objFptr[i]); // read next record
            if(record[strlen(record) - 1] == '\n')
                record[strlen(record) - 1] = '\0';
        }

        CSADDR += newCntSec->length;
        i++;
        if(i == CS_MAX)
            break;
        if(objFptr[i]) { // next control section exists
            newCntSec = (CNT_SEC*) malloc(sizeof(CNT_SEC));
            newCntSec->stAddress = CSADDR;
        }
    }
    return CSADDR - progAddr;
}

int linkLoaderPass2(FILE** objFptr) {
    int i = 0, j, k;
    int offset, tLen, hByteCnt, modAddress;
    int CSADDR = 0, EXECADDR = 0, CSLTH = 0;
    int* refNum = NULL;
    char record[101];
    char esName[CS_LEN], temp[CS_LEN];
    NODE* curCntSec = extSymTable;

    CSADDR = EXECADDR = progAddr; // start address of first control section

    while(objFptr[i]) {
        fgets(record, 101, objFptr[i]); // header record
        if(record[strlen(record) - 1] == '\n')
            record[strlen(record) - 1] = '\0';
        CSLTH = ((CNT_SEC*)(curCntSec->data))->length;

        fgets(record, 101, objFptr[i]); // read next record
        if(record[strlen(record) - 1] == '\n')
            record[strlen(record) - 1] = '\0';

        while(record[0] != 'E') {
            if(record[0] == 'T') { // if a T record is found
                memset(temp, '\0', CS_LEN);
                strncpy(temp, record + 1, CS_LEN - 1);
                offset = hexToDec(temp); // text record start address offset
                
                memset(temp, '\0', CS_LEN);
                strncpy(temp, record + 7, 2);
                tLen = hexToDec(temp); // text record length

                for(j = 0; j < tLen; j++) {
                    memset(temp, '\0', CS_LEN);
                    strncpy(temp, record + 9 + j * 2, 2);
                    mem[CSADDR + offset + j] = hexToDec(temp); // load to memory
                }
            }
            else if(record[0] == 'M') { // if M record is found
                memset(temp, '\0', CS_LEN);
                strncpy(temp, record + 1, CS_LEN - 1);
                offset = hexToDec(temp); // byte location to modify

                memset(temp, '\0', CS_LEN);
                strncpy(temp, record + 7, 2);
                hByteCnt = hexToDec(temp); // half byte count to modify

                memset(temp, '\0', CS_LEN);
                strcpy(temp, record + 10);

                modAddress = mem[CSADDR + offset] % (hByteCnt % 2 ? 0x10 : 0x100);
                for(j = 1; j <= (hByteCnt - 1) / 2; j++) {
                    modAddress *= 0x100;
                    modAddress += mem[CSADDR + offset + j];
                }
                modAddress += (record[9] == '+' ? 1 : -1) * refNum[hexToDec(temp)]; // modification address

                for(j = (hByteCnt - 1) / 2; j; j--) {
                    mem[CSADDR + offset + j] = modAddress % 0x100;
                    modAddress /= 0x100;
                }
                mem[CSADDR + offset] = (hByteCnt % 2 ? (mem[CSADDR + offset] / 0x10) * 0x10 + modAddress : modAddress);
            }
            else if(record[0] == 'R') { // if R record is found
                if(refNum)
                    free(refNum);
                refNum = (int*) malloc(sizeof(int) * ((strlen(record) - 1) / 8 + 3));
                for(j = 2; j < (strlen(record) - 1) / 8 + 3; j++) {
                    strncpy(esName, record + (j - 2) * 8 + 3, CS_LEN - 1);
                    for(k = strlen(esName); k < CS_LEN - 1; k++)
                        esName[k] = ' ';
                    refNum[j] = searchES(esName);
                }
                refNum[1] = CSADDR;
            }

            fgets(record, 101, objFptr[i]); // read next record
            if(record[strlen(record) - 1] == '\n')
                record[strlen(record) - 1] = '\0';
        }

        if(strlen(record) > 1) // if address is specified in End record
            EXECADDR = CSADDR + hexToDec(record + 1);
        CSADDR += CSLTH;

        i++;
        if(i == CS_MAX)
            break;
        curCntSec = curCntSec->next;
    }
    return EXECADDR;
}

void fcloseObj(FILE** objFptr) {
    int i;
    for(i = 0; i < 3; i++) {
        if(objFptr[i])
            fclose(objFptr[i]);
        objFptr[i] = NULL;
    }
}

bool searchCS(char* csName) {
    NODE* cur = extSymTable;
    CNT_SEC* data;
    while(cur) {
        data = (CNT_SEC*) cur->data;
        if(!strcmp(data->csName, csName))
            return true;
        cur = cur->next;
    }
    return false;
}

int searchES(char* symName) {
    NODE* curCS = extSymTable;
    NODE* curES;
    while(curCS) {
        curES = ((CNT_SEC*)(curCS->data))->extSym;
        while(curES) {
            if(!strcmp(symName, ((EXT_SYMBOL*)(curES->data))->symName))
                return ((EXT_SYMBOL*)(curES->data))->address;
            curES = curES->next;
        }
        curCS = curCS->next;
    }
    return -1;
}
