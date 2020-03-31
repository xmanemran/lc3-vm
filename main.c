#include <stdio.h>

enum {
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC,
  R_COND,
  R_COUNT
} Registers;

enum {
  OP_BR = 0,  // branch
  OP_ADD,     // add
  OP_LD,      // load
  OP_ST,      // store
  OP_JSR,     // jump register
  OP_AND,     // bitwise add
  OP_LDR,     // load register
  OP_STR,     // store register
  OP_RTI,     // unused
  OP_NOT,     // bitwise not
  OP_LDI,     // load indirect
  OP_STI,     // store indirect
  OP_JMP,     // jump
  OP_RES,     // reversed (unused)
  OP_LEA,     // load effective address
  OP_TRAP     // execute trap
} InstructionSet;

enum {
  FL_POS = 1 << 0,  // P
  FL_ZRO = 1 << 1,  // Z
  FL_NEG = 1 << 2   // N
} ConditionFlag;

u_int16_t memory[__UINT16_MAX__];
u_int16_t reg[R_COUNT];

int main(int argc, const char* argv[]) {
  if (argc < 2) {
    printf("❌ lc3 [image-file] ...\n");
    exit(2);
  }

  for (int j = 1; j < argc; j++) {
    if (!read_image(argv[j])) {
      printf("❌ failed to load image: %s\n", argv[j]);
      exit(2);
    }
  }

  enum { PC_START = 0x3000 };
  reg[R_PC] = PC_START;

  int running = 0;
  /*
    while (running) {
      u_int16_t inst = mem_read(reg[R_PC]++);
      u_int16_t op = inst >> 12;
      switch (op) {
        case OP_ADD:
          // ({A)DD, 6}
          break;
        case OP_AND:
          // ({A)ND, 7}
          break;
        case OP_NOT:
          // ({N)OT, 7}
          break;
        case OP_BR:
          // ({B)R, 7}
          break;
        case OP_JMP:
          // ({J)MP, 7}
          break;
        case OP_JSR:
          // ({J)SR, 7}
          break;
        case OP_LD:
          // ({L)D, 7}
          break;
        case OP_LDI:
          // ({L)DI, 6}
          break;
        case OP_LDR:
          // ({L)DR, 7}
          break;
        case OP_LEA:
          // ({L)EA, 7}
          break;
        case OP_ST:
          // ({S)T, 7}
          break;
        case OP_STI:
          // ({S)TI, 7}
          break;
        case OP_STR:
          // ({S)TR, 7}
          break;
        case OP_TRAP:
          // ({T)RAP, 8}
          break;
        case OP_RES:
        case OP_RTI:
        default:
          // ({B)AD OPCODE, 7}
          break;
      }
    }
    printf("Hello world\n");
    */
  return 0;
}