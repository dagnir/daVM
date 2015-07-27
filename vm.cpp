#include <cstring>
#include <functional>
#include <iostream>
#include "vm.h"

#define U_INS(name) bind(&VM::ins_##name, this, _1, _2, _3)
#define B_INS(name) ind(&VM::ins_##name, this, _1, _2, _3, _4, _5)

#define DEF_U_INS(name) void VM::ins_##name(uint8_t bw, uint8_t As, uint8_t s_reg)
#define DEF_B_INS(name) void VM::ins_##name(uint8_t bw, uint8_t As, uint8_t s_reg, uint8_t Ad, uint8_t d_reg)

namespace vm {
  using namespace std;
  using namespace std::placeholders;

  VM::VM() {
    r[PC] = CODE_START;
    r[SP] = STACK_START;

    unaryInstructions = { {
	U_INS(rrc),
	U_INS(swpb),
	U_INS(rra),
	U_INS(sxt),
	U_INS(push),
	U_INS(call)
      } };

    binaryInstructions = {
    };
  }

  uint16_t VM::fetch() {
    uint16_t ret = memory[r[PC]];
    r[PC]++;
    return ret;
  }

  uint16_t VM::read_word(uint8_t bw, uint8_t addr_mode, uint8_t reg) {
    auto w = *resolve_to_ptr(addr_mode, reg);
    if (bw) {
      return w & 0x00ff;
    }
    return w;
  }

  void VM::write_word(uint16_t w, uint8_t addr_mode, uint8_t reg) {
    auto pw = resolve_to_ptr(addr_mode, reg);
    *pw = w;
  }

  uint16_t *VM::resolve_to_ptr(uint8_t addr_mode, uint8_t reg) {
    switch (addr_mode) {
      // register direct
      // Rn
    case 0:
      return &r[reg];
      // register index
      // offset(Rn)
    case 1: {
      uint16_t ext = fetch();
      return &memory[r[reg] + ext];
    }
      // register indirect
      // @Rn
    case 2:
      return &memory[r[reg]];
      // register indirect with post increment
      // @Rn+
    case 3: {
      uint16_t *ret = &memory[r[reg]];
      r[reg]++;
      return ret;
    }
    default:
      cerr << "Unknown addressing mode: " << (int)addr_mode << endl;
      exit(1);
    }
  }

  DEF_U_INS(rrc) {
    r[SR] &= ~(1 << NEGATIVE |
	       1 << ZERO     |
	       1 << OVERFLOW);

    auto w = read_word(bw, As, s_reg);
    bool set_carry = w & 1;
    w >>= 1;

    if (r[SR] & (1 << CARRY)) {
      if (bw) {
	w |= 0x0080;
      } else {
	w |= 0x8000;
      }
      r[SR] |= 1 << NEGATIVE;
    }

    if (!w) {
      r[SR] |= 1 << ZERO;
    }

    if (set_carry) {
      r[SR] |= 1 << CARRY;
    }

    write_word(w, As, s_reg);
  }

  // Swap low and high byte.
  // No .b variant.
  DEF_U_INS(swpb) {
    uint16_t w = read_word(0, As, s_reg);
    uint16_t temp = w & 0xFF;
    w >>= 8;
    w |= temp << 8;
    write_word(w, As, s_reg);
  }

  // Rotate right arithmetic
  // if LSB is set, set carry, shift right
  // sign bit should be preserved through rotate
  // i.e. if it's 1 before rotation, it must be 1 after
  DEF_U_INS(rra) {
    r[SR] &= ~(1 << OVERFLOW |
	       1 << NEGATIVE |
	       1 << ZERO     |
	       1 << CARRY);

    uint16_t w = read_word(bw, As, s_reg);

    if (w & 0x1) {
      r[SR] |= 1 << CARRY;
    }

    uint16_t msb;
    if (bw) {
      msb = w & 0x0080;
    } else {
      msb = w & 0x8000;
    }

    // shift
    w >>= 1;
    // preserve msb
    w |= msb;

    if (!w) {
      r[SR] |= 1 << ZERO;
    }
    if (msb) {
      r[SR] |= 1 << NEGATIVE;
    }

    write_word(w, As, s_reg);
  }

  DEF_U_INS(sxt) {
    r[SR] &= ~(1 << OVERFLOW |
	       1 << NEGATIVE |
	       1 << ZERO     |
	       1 << CARRY);

    auto w = read_word(bw, As, s_reg);
    auto sign = w & 0x0080;

    if (sign) {
      w |= 0xff00;
    } else {
      w &= 0x00ff;
    }

    if (sign) {
      r[SR] |= 1 << NEGATIVE;
    }

    if (!w) {
      r[SR] = 1 << ZERO;
    } else {
      r[SR] = 1 << CARRY;
    }

    write_word(w, As, s_reg);
  }

  // push a word on to the stack (even for push.b)
  DEF_U_INS(push) {
    auto w = read_word(bw, As, s_reg);
    r[SP]--;
    memory[r[SP]] = w;
  }

  // PUSH PC, then jump.
  // no .b variant
  DEF_U_INS(call) {
    auto w = read_word(0, As, s_reg);
    r[SP]--;
    memory[r[SP]] = r[PC];
    r[PC] = w;
  }

} // namespace vm
