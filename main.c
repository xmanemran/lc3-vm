#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* unix */
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void console(char* str) {
  printf("\%s\n", str);
}

void read_image_file(FILE* program);
void inst_add(uint16_t instr);
void inst_and(uint16_t instr);
void inst_not(uint16_t instr);
void inst_br(uint16_t instr);
void inst_jmp(uint16_t instr);
void inst_jsr(uint16_t instr);
void inst_ld(uint16_t instr);
void inst_ldi(uint16_t instr);
void inst_ldr(uint16_t instr);
void inst_lea(uint16_t instr);
void inst_st(uint16_t instr);
void inst_sti(uint16_t instr);
void inst_str(uint16_t instr);
void inst_trap(uint16_t instr);
void trap_getc();
void trap_out();
void trap_puts();
void trap_in();
void trap_putsp();
void trap_halt();
void update_flag(uint16_t data);
uint16_t sign_extend(uint16_t data, uint16_t bit_length);
uint16_t comp_dr(uint16_t instr);
uint16_t comp_sr(uint16_t instr);
uint16_t swap16(uint16_t x);
read_image(const char* image_path);
void mmr_write(uint16_t address, uint16_t data);
uint16_t mmr_read(uint16_t address);
uint16_t check_key();
void disable_input_buffering();
void restore_input_buffering();
void handle_interrupt(int signal);
void abort();

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

enum {
  TRAP_GETC = 0x20,
  TRAP_OUT = 0x21,
  TRAP_PUTS = 0x22,
  TRAP_IN = 0x23,
  TRAP_PUTSP = 0x24,
  TRAP_HALT = 0x25,
} TrapCode;

enum {
  MMR_KBSR = 0xFE00,
  MMR_KBDR = 0xFE02,
} MemoryMapedRegister;

uint16_t memory[UINT16_MAX];
uint16_t reg[R_COUNT];
int running = 1;
struct termios original_tio;

void printBits(uint16_t num) {
  int i;
  for (i = 8 * sizeof(num) - 1; i >= 0; i--) {
    (num & (1 << i)) ? putchar('1') : putchar('0');
  }
  printf("\n");
}

int main(int argc, const char* argv[]) {
  // printBits(sign_extend(data, 5));

  signal(SIGINT, handle_interrupt);
  disable_input_buffering();

  if (argc < 2) {
    printf("❌ lc3 [image-file] ...\n");
    exit(2);
  }

  for (int j = 1; j < argc; j++) {
    if (!read_image(argv[j])) {
      printf("❌ failed to load image: %s\n", argv[j]);
      exit(1);
    }
  }

  enum { PC_START = 0x3000 };
  reg[R_PC] = PC_START;

  while (running) {
    uint16_t inst = mmr_read(reg[R_PC]++);
    uint16_t op = inst >> 12;
    switch (op) {
      case OP_ADD:
        inst_add(inst);
        break;
      case OP_AND:
        inst_and(inst);
        break;
      case OP_NOT:
        inst_not(inst);
        break;
      case OP_BR:
        inst_br(inst);
        break;
      case OP_JMP:
        inst_jmp(inst);
        break;
      case OP_JSR:
        inst_jsr(inst);
        break;
      case OP_LD:
        inst_ld(inst);
        break;
      case OP_LDI:
        inst_ldi(inst);
        break;
      case OP_LDR:
        inst_ldr(inst);
        break;
      case OP_LEA:
        inst_lea(inst);
        break;
      case OP_ST:
        inst_st(inst);
        break;
      case OP_STI:
        inst_sti(inst);
        break;
      case OP_STR:
        inst_str(inst);
        break;
      case OP_TRAP:
        inst_trap(inst);
        break;
      case OP_RES:
      case OP_RTI:
      default:
        abort();
        break;
    }
  }
  restore_input_buffering();
}

void read_image_file(FILE* program) {
  uint16_t origin;
  fread(&origin, sizeof(origin), 1, program);
  origin = swap16(origin);

  uint16_t max_read = UINT16_MAX - origin;
  uint16_t* p = memory + origin;
  size_t read = fread(p, sizeof(uint16_t), max_read, program);
  while (read-- > 0) {
    *p = swap16(*p);
    ++p;
  }
}

void inst_add(uint16_t instr) {
  uint16_t r_addr = (instr >> 9) & 0x7;
  uint16_t sr1 = reg[(instr >> 6) & 0x7];
  uint16_t isImmediateMode = (instr >> 5) & 0x1;
  uint16_t source2 = instr & 0x7;

  if (isImmediateMode) {
    uint16_t immediateData = sign_extend(instr & 0x1F, 5);
    reg[r_addr] = sr1 + immediateData;
  } else {
    reg[r_addr] = sr1 + reg[instr & 0x7];
  }

  update_flag(reg[r_addr]);
}

void inst_and(uint16_t instr) {
  uint16_t r_addr = (instr >> 9) & 0x7;
  uint16_t dr = reg[r_addr];
  uint16_t sr1 = reg[(instr >> 6) & 0x7];
  if ((instr >> 5) & 0x1) {
    reg[r_addr] = sr1 & sign_extend(instr & 0x1F, 5);
  } else {
    reg[r_addr] = sr1 & reg[instr & 0x7];
  }
  update_flag(reg[r_addr]);
}

void inst_not(uint16_t instr) {
  uint16_t r_addr = (instr >> 9) & 0x7;
  uint16_t dr = reg[r_addr];

  uint16_t sr = reg[(instr >> 6) & 0x7];
  reg[r_addr] = ~sr;
  update_flag(reg[r_addr]);
}

void inst_br(uint16_t instr) {
  uint16_t offset = instr & 0x1FF;
  uint16_t cond_flag = (instr >> 9) & 0x7;
  if (cond_flag & reg[R_COND]) {
    reg[R_PC] = reg[R_PC] + sign_extend(offset, 9);
  }
}

void inst_jmp(uint16_t instr) {
  uint16_t baseR = (instr >> 6) & 0x1F;
  reg[R_PC] = reg[baseR];
}

void inst_jsr(uint16_t instr) {
  reg[R_R7] = reg[R_PC];
  if ((instr >> 11) & 0x1) {
    reg[R_PC] = reg[R_PC] + sign_extend(instr & 0x7FF, 11);
  } else {
    reg[R_PC] = reg[(instr >> 6) & 0x7];
  }
}

void inst_ld(uint16_t instr) {
  uint16_t r_addr = (instr >> 9) & 0x7;
  uint16_t memory_addr = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
  reg[r_addr] = memory[memory_addr];
  update_flag(reg[r_addr]);
}

void inst_ldi(uint16_t instr) {
  uint16_t r_addr = (instr >> 9) & 0x7;
  uint16_t memory_addr = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
  reg[r_addr] = memory[memory[memory_addr]];
  update_flag(reg[r_addr]);
}

void inst_ldr(uint16_t instr) {
  uint16_t r_addr = (instr >> 9) & 0x7;
  uint16_t base_r = reg[(instr >> 6) & 0x7];
  uint16_t memory_addr = base_r + sign_extend(instr & 0x3F, 6);
  reg[r_addr] = memory[memory_addr];
  update_flag(reg[r_addr]);
}

void inst_lea(uint16_t instr) {
  uint16_t r_addr = (instr >> 9) & 0x7;
  uint16_t memory_addr = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
  reg[r_addr] = memory_addr;
  update_flag(reg[r_addr]);
}

void inst_st(uint16_t instr) {
  uint16_t sr = reg[(instr >> 9) & 0x7];
  uint16_t memory_addr = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
  memory[memory_addr] = sr;
}

void inst_sti(uint16_t instr) {
  uint16_t sr = reg[(instr >> 9) & 0x7];
  uint16_t memory_addr = reg[R_PC] + sign_extend(instr & 0x1FF, 9);
  memory[memory[memory_addr]] = sr;
}

void inst_str(uint16_t instr) {
  uint16_t sr = reg[(instr >> 9) & 0x7];
  uint16_t base_r = reg[(instr >> 6) & 0x7];
  uint16_t memory_addr = base_r + sign_extend(instr & 0x3F, 6);
  memory[memory_addr] = sr;
}

void inst_trap(uint16_t inst) {
  switch (inst & 0xFF) {
    case TRAP_GETC:
      trap_getc();
      break;
    case TRAP_OUT:
      trap_out();
      break;
    case TRAP_PUTS:
      trap_puts();
      break;
    case TRAP_IN:
      trap_in();
    case TRAP_PUTSP:
      trap_putsp();
      break;
    case TRAP_HALT:
      trap_halt();
      break;
    default:
      trap_halt();
      break;
  }
}

void trap_getc() {
  reg[R_R0] = (uint16_t)getchar();
}

void trap_out() {
  putc((char)reg[R_R0], stdout);
  fflush(stdout);
}

void trap_puts() {
  uint16_t* c = memory + reg[R_R0];
  while (*c) {
    putc((char)*c, stdout);
    ++c;
  }
  fflush(stdout);
}

void trap_in() {
  printf("Enter a character: ");
  char ch = getchar();
  putc(ch, stdout);
  reg[R_R0] = (uint16_t)ch;
  fflush(stdout);
}

void trap_putsp() {
  uint16_t* chs = &memory[reg[R_R0]];
  char ch1 = (char)*(chs)&0xf;
  putc(ch1, stdout);
  char ch2 = (char)*(chs) >> 8;
  putc(ch2, stdout);
  fflush(stdout);
}

void trap_halt() {
  puts("HALT");
  fflush(stdout);
  running = 0;
}

uint16_t sign_extend(uint16_t data, uint16_t bit_length) {
  if ((data >> (bit_length - 1) & 0x1)) {
    data = (0xFFFF << bit_length) | data;
  }
  return data;
}

void update_flag(uint16_t data) {
  if (data == 0) {
    reg[R_COND] = FL_ZRO;
  } else if (data >> 0x0F) {
    reg[R_COND] = FL_NEG;
  } else {
    reg[R_COND] = FL_POS;
  }
}

uint16_t comp_dr(uint16_t instr) {
  return reg[(instr >> 9) & 0x7];
}

uint16_t comp_sr(uint16_t instr) {
  return comp_dr(instr);
}

uint16_t swap16(uint16_t x) {
  return (x << 8) | (x >> 8);
}

int read_image(const char* image_path) {
  FILE* file = fopen(image_path, "rb");
  if (!file) {
    return 0;
  };
  read_image_file(file);
  fclose(file);
  return 1;
}

void mmr_write(uint16_t address, uint16_t data) {
  memory[address] = data;
}

uint16_t mmr_read(uint16_t address) {
  if (address == MMR_KBSR) {
    if (check_key()) {
      memory[MMR_KBSR] = (1 << 15);
      memory[MMR_KBDR] = getchar();
    } else {
      memory[MMR_KBSR] = 0;
    }
  }
  return memory[address];
}

uint16_t check_key() {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

void disable_input_buffering() {
  tcgetattr(STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering() {
  tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal) {
  restore_input_buffering();
  printf("\n");
  exit(-2);
}

void abort() {
  exit(5);
}