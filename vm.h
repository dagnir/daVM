/* -*- mode: c++ -*- */
#ifndef __GUARD_VM_H__
#define __GUARD_VM_H__

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

    uint16_t query_register(unsigned);

    uint8_t mem_read(uint16_t);

    void mem_write(uint16_t, uint8_t);

    void mem_write(uint16_t, uint16_t);

    void stack_push(uint8_t);

    void stack_push(uint16_t);

    uint16_t stack_pop_word();

    uint8_t stack_pop_byte();

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

    uint16_t read_data(uint8_t, uint8_t, uint8_t);
    uint16_t read(uint8_t, uint8_t);
    void write_data(uint8_t, uint16_t, uint8_t, uint8_t);

    uint16_t make_word(uint8_t l, uint8_t h) {
      return (uint16_t)(h << 8) | l;
    }


    // Unary Instructions
    DECLARE_U_INS(rrc);
    DECLARE_U_INS(swpb);
    DECLARE_U_INS(rra);
    DECLARE_U_INS(sxt);
    DECLARE_U_INS(push);
    DECLARE_U_INS(call);

    // Dual operand instructions
    DECLARE_B_INS(mov);
    DECLARE_B_INS(add);
    DECLARE_B_INS(addc);
    DECLARE_B_INS(subc);
    DECLARE_B_INS(sub);
    DECLARE_B_INS(cmp);

    uint16_t add_common(uint8_t, uint16_t, uint16_t, uint8_t);
  };
} // namespace vm
#endif // __GUARD_VM_H__
