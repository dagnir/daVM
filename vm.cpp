#include <cstring>
#include <functional>
#include <iostream>
#include "vm.h"

#define U_INS(name) bind(&VM::ins_##name, this, _1, _2, _3)
#define B_INS(name) bind(&VM::ins_##name, this, _1, _2, _3, _4, _5)

#define DEF_U_INS(name) void VM::ins_##name(uint8_t bw, uint8_t As, uint8_t s_reg)
#define DEF_B_INS(name) void VM::ins_##name(uint8_t bw, uint8_t As, uint8_t s_reg, uint8_t Ad, uint8_t d_reg)

#define UNUSED(x) (void)(x)

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

    binaryInstructions = { {
	B_INS(mov),
	B_INS(add)
      } };
  }

  uint16_t VM::fetch() {
    uint16_t ret = make_word(memory[r[PC]], memory[r[PC] + 1]);
    r[PC] += 2;
    return ret;
  }

  void VM::execute_binary_ins(uint16_t ins) {
    uint8_t dst_reg = ins & 0xf;
    ins >>= 4;
    uint8_t As = ins & 0x3;
    ins >>= 2;
    uint8_t bw = ins & 0x1;
    ins >>= 1;
    uint8_t Ad = ins & 0x1;
    ins >>= 1;
    uint8_t src_reg = ins & 0xf;
    ins >>= 4;
    uint8_t opcode = (uint8_t)ins;
    binaryInstructions[opcode - 4](bw, As, src_reg, Ad, dst_reg);
  }

  uint16_t VM::read_word(uint8_t bw, uint8_t addr_mode, uint8_t reg) {
    auto w = read(addr_mode, reg);
    if (bw) {
      w &= 0x00ff;
    }
    return w;
  }

  void VM::write_word(uint16_t w, uint8_t addr_mode, uint8_t reg) {
    switch (addr_mode) {
      // register direct
      // Rn
    case 0:
      r[reg] = w;
      break;
      // register indirect
      // offset(Rn)
    case 1: {
      uint16_t ext = fetch();
      memory[ext + r[reg]] = w & 0xff;
      memory[ext + r[reg] + 1] = (w & 0xff00) >> 8;
      break;
    }
    default:
      std::cerr << "Unknown addressing mode: " << (int)addr_mode << std::endl;
      exit(1);
    }
  }

  uint16_t VM::read(uint8_t addr_mode, uint8_t reg) {
    switch (addr_mode) {
      // register direct
      // Rn
    case 0:
      if (reg == 3) {
	return 0;
      }
      return r[reg];
      // register index
      // offset(Rn)
    case 1: {
      if (reg == 3) {
	return 1;
      }
      uint16_t ext = fetch();
      if (reg == 2) {
	return make_word(memory[ext], memory[ext + 1]);
      }
      return make_word(memory[ext + r[reg]], memory[ext + r[reg] + 1]);
    }
      // register indirect
      // @Rn
    case 2:
      if (reg == 3) {
	return 2;
      }
      if (reg == 2) {
	return 4;
      }
      return make_word(memory[r[reg]], memory[r[reg] + 1]);
      // register indirect with post increment
      // @Rn+
    case 3: {
      if (reg == 3) {
	return 0xffff;
      }
      if (reg == 2) {
	return 8;
      }
      uint16_t ret = make_word(memory[r[reg]], memory[r[reg] + 1]);
      if (reg == PC) {
	r[reg] += 2;
      } else {
	r[reg] += 1;
      }
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
    UNUSED(bw);

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
      r[SR] |= 1 << ZERO;
    } else {
      r[SR] |= 1 << CARRY;
    }

    write_word(w, As, s_reg);
  }

  // push a word on to the stack (even for push.b)
  DEF_U_INS(push) {
    auto w = read_word(bw, As, s_reg);
    if (bw) {
      stack_push((uint8_t)(0xff & w));
    } else {
      stack_push(w);
    }
  }

  // PUSH PC, then jump.
  // no .b variant
  DEF_U_INS(call) {
    UNUSED(bw);

    auto w = read_word(0, As, s_reg);
    stack_push(r[PC]);
    r[PC] = w;
  }

  DEF_B_INS(mov) {
    auto w = read_word(bw, As, s_reg);
    write_word(w, Ad, d_reg);
  }

  DEF_B_INS(add) {
    add_common(bw, As, s_reg, Ad, d_reg, 0);
  }

  inline void VM::add_common(uint8_t bw, uint8_t As, uint8_t s_reg, uint8_t Ad,
			     uint8_t d_reg, uint8_t carry) {
    auto src = read_word(bw, As, s_reg);
    auto dst = read_word(0, Ad, d_reg);
    const unsigned res = src + dst + carry;
    uint16_t sign_mask = bw ? 0x0080 : 0x8000;
    bool src_neg = src & sign_mask;
    bool dst_neg = dst & (1 << 15);
    bool same_sign = src_neg == dst_neg;
    bool sign_flipped = (res ^ dst) & (1 << 15);

    if (same_sign && sign_flipped) {
      r[SR] |= (1 << OVERFLOW);
    } else {
      r[SR] &= ~(1 << OVERFLOW);
    }

    if (res & (1 << 15)) {
      r[SR] |= (1 << NEGATIVE);
    } else {
      r[SR] &= ~(1 << NEGATIVE);
    }

    if (!res) {
      r[SR] |= (1 << ZERO);
    } else {
      r[SR] &= ~(1 << ZERO);
    }

    if (res & (1 << 16)) {
      r[SR] |= (1 << CARRY);
    } else {
      r[SR] &= ~(1 << CARRY);
    }

    write_word((uint16_t)res, Ad, d_reg);
  }

} // namespace vm
