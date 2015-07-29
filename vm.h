/* -*- mode: c++ -*- */
#include <array>
#include <functional>
#include <cstdint>

#define DECLARE_U_INS(name) void ins_##name(uint8_t, uint8_t, uint8_t)
#define DECLARE_B_INS(name) void ins_##name(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)

namespace vm {
  using namespace std;

  class VM {
  public:
    VM();
    void load(unsigned char*, size_t);

  private:
    const uint16_t CODE_START = 0x8000;
    const uint16_t STACK_START = 0x5FF;
    const uint16_t STACK_END = 0x200;
    typedef function<void(uint8_t, uint8_t, uint8_t)> UnaryInstruction;
    typedef function<void(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t)> BinaryInstruction;

    enum SpecialReg {
      PC = 0, SP, SR
    };

    enum StatusFlag {
      OVERFLOW = 8, NEGATIVE = 2, ZERO =  1 , CARRY = 0
    };


    // 64 KB
    array<uint8_t, 64 * 1024> memory;
    uint16_t r[16];

    array<UnaryInstruction, 8> unaryInstructions;
    array<BinaryInstruction, 12> binaryInstructions;

    uint16_t fetch();
    void execute_unary_ins(uint16_t);
    void execute_conditional_ins(uint16_t);
    void execute_binary_ins(uint16_t);

    uint16_t read_word(uint8_t, uint8_t, uint8_t);
    uint16_t read(uint8_t, uint8_t);
    void write_word(uint16_t, uint8_t, uint8_t);

    uint16_t make_word(uint8_t l, uint8_t h) {
      return (uint16_t)(h << 8) | l;
    }

    void stack_push(uint8_t byte) {
      r[SP] -= 2;
      // high byte is garbage
      memory[r[SP]] = byte;
    }

    void stack_push(uint16_t word) {
      r[SP] -= 2;
      memory[r[SP]] = word & 0xff;
      memory[r[SP] + 1] = (word & 0xff00) >> 8;
    }

    // Unary Instructions
    DECLARE_U_INS(rrc);
    DECLARE_U_INS(swpb);
    DECLARE_U_INS(rra);
    DECLARE_U_INS(sxt);
    DECLARE_U_INS(push);
    DECLARE_U_INS(call);
  };
} // namespace vm
