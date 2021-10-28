#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

const int MAX_MEM_SIZE  = (1 << 13);

void fetchStage(int *icode, int *ifun, int *rA, int *rB, wordType *valC, wordType *valP) {
	wordType pc = getPC();
	byteType iByte = getByteFromMemory(pc);
	*ifun = iByte&0xf;
	*icode = iByte>>4;
	if(*icode == RRMOVQ || *icode == OPQ || *icode == PUSHQ || *icode == POPQ){
		byteType registerByte = getByteFromMemory(pc+1);
		*rB = registerByte&0xf;
		*rA = registerByte>>4;
		*valP = pc+2;
	}
	else if(*icode == IRMOVQ || *icode == RMMOVQ || *icode == MRMOVQ){
		 byteType registerByte = getByteFromMemory(pc+1);
                *rB = registerByte&0xf;
                *rA = registerByte>>4;
                *valC = getWordFromMemory(pc+2);
		*valP = pc+10;
	}
	else if(*icode == JXX || *icode == CALL){
		*valC = getByteFromMemory(pc+1);
		*valP = pc+9;
	}
	else if(*icode == NOP || *icode == RET){
		*valP = pc+1;
	}
	else if(*icode == HALT){
		*valP = pc+1;
		setStatus(STAT_HLT);
	}


}

void decodeStage(int icode, int rA, int rB, wordType *valA, wordType *valB) {
 	if(icode == RRMOVQ){
		*valA = getRegister(rA);
	}
	else if(icode == RMMOVQ || icode == OPQ){
		*valA = getRegister(rA);
		*valB = getRegister(rB);
	}
	else if(icode == MRMOVQ){
		*valB = getRegister(rB);
	}
	else if(icode == CALL){
		*valB = getRegister(RSP);
	}
	else if(icode == RET || icode == POPQ){
		*valA = getRegister(RSP);
		*valB = getRegister(RSP);
	}
	else if(icode == PUSHQ){
		*valA = getRegister(rA);
		*valB = getRegister(RSP);
	}
}

void executeStage(int icode, int ifun, wordType valA, wordType valB, wordType valC, wordType *valE, bool *Cnd) {
  	if(icode == RRMOVQ){
		*valE = 0 + valA;
	}
	else if(icode == IRMOVQ){
		*valE = 0 + valC;
	}
	else if(icode == RMMOVQ || icode == MRMOVQ){
		*valE = valB + valC;
	}
	else if(icode == OPQ){
		bool sf;
		bool zf;
		bool of;
		if(ifun == ADD){
			*valE = valB + valA;
			if(valA > 0 && valB > 0 && *valE < 0){
				of = 1;
			}
			else{
				if(valA < 0 && valB < 0 && *valE > 0){
					of = 1;
				}
				else{
					of = 0;
				}
			}
		}
		else if(ifun == SUB){
			*valE = valB - valA;
			if(valA > 0 && valB < 0 && *valE > 0){
                                of = 1;
                        }
                        else{
                                if(valA < 0 && valB > 0 && *valE < 0){
                                        of = 1;
                                }
                                else{
                                        of = 0;
                                }
                        }
		}
		else if(ifun == XOR){
			*valE = valB ^ valA;
		}
		else if(ifun == AND){
			*valE = valB & valA;
		}
		if(*valE < 0){
			sf = 1;
		}
		else{
			sf = 0;
		}
		if(*valE == 0){
			zf = 1;
		}
		else{
			zf = 0;
		}
		if(of == 1){
		}
		else{
			of = 0;
		}
		setFlags(sf, zf, of);	
	}
	else if(icode == JXX){
		*Cnd = Cond(ifun);
	}
	else if(icode == CALL || icode == PUSHQ){
		*valE = valB + (-8);
	}
	else if(icode == RET || icode == POPQ){
		*valE = valB + 8;
	}
}

void memoryStage(int icode, wordType valA, wordType valP, wordType valE, wordType *valM) {
 	if(icode == RMMOVQ || icode == PUSHQ){
		setWordInMemory(valE, valA);
	}
	else if(icode == MRMOVQ){
		*valM = getWordFromMemory(valE);
	}
	else if(icode == CALL){
		setWordInMemory(valE, valP);
	}
	else if(icode == RET || icode == POPQ){
		*valM = getWordFromMemory(valA);
	}
}

void writebackStage(int icode, int rA, int rB, wordType valE, wordType valM) {
 	if(icode == RRMOVQ || icode == IRMOVQ || icode == OPQ){
		setRegister(rB, valE);
	}
	else if(icode == MRMOVQ){
		setRegister(rA, valM);
	}
	else if(icode == CALL || icode == RET || icode == PUSHQ){
		setRegister(RSP, valE);
	}
	else if(icode == POPQ){
		setRegister(RSP, valE);
		setRegister(rA, valM);
	}
}

void pcUpdateStage(int icode, wordType valC, wordType valP, bool Cnd, wordType valM) {
	if(icode == JXX){
		if(Cnd == 1){
			setPC(valC);
		}
		else{
			setPC(valP);
		}
	}
	else if(icode == CALL){
		setPC(valC);
	}
	else if(icode == RET){
		setPC(valM);
	}
	else{
		setPC(valP);
	}
}

void stepMachine(int stepMode) {
  /* FETCH STAGE */
  int icode = 0, ifun = 0;
  int rA = 0, rB = 0;
  wordType valC = 0;
  wordType valP = 0;
 
  /* DECODE STAGE */
  wordType valA = 0;
  wordType valB = 0;

  /* EXECUTE STAGE */
  wordType valE = 0;
  bool Cnd = 0;

  /* MEMORY STAGE */
  wordType valM = 0;

  fetchStage(&icode, &ifun, &rA, &rB, &valC, &valP);
  applyStageStepMode(stepMode, "Fetch", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  decodeStage(icode, rA, rB, &valA, &valB);
  applyStageStepMode(stepMode, "Decode", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  executeStage(icode, ifun, valA, valB, valC, &valE, &Cnd);
  applyStageStepMode(stepMode, "Execute", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  memoryStage(icode, valA, valP, valE, &valM);
  applyStageStepMode(stepMode, "Memory", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  writebackStage(icode, rA, rB, valE, valM);
  applyStageStepMode(stepMode, "Writeback", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);
  
  pcUpdateStage(icode, valC, valP, Cnd, valM);
  applyStageStepMode(stepMode, "PC update", icode, ifun, rA, rB, valC, valP, valA, valB, valE, Cnd, valM);

  incrementCycleCounter();
}

/** 
 * main
 * */
int main(int argc, char **argv) {
  int stepMode = 0;
  FILE *input = parseCommandLine(argc, argv, &stepMode);

  initializeMemory(MAX_MEM_SIZE);
  initializeRegisters();
  loadMemory(input);

  applyStepMode(stepMode);
  while (getStatus() != STAT_HLT) {
    stepMachine(stepMode);
    applyStepMode(stepMode);
#ifdef DEBUG
    printMachineState();
    printf("\n");
#endif
  }
  printMachineState();
  return 0;
}
