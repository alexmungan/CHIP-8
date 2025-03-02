#ifndef CHIP8_H
#define CHIP8_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <random>
#include <cstdlib>
#include <cstring>

constexpr size_t MEM_SIZE = 4096;
constexpr size_t loadAddress = 0x200; //CHIP-8 programs are usually loaded at address 0x200 (512)
#define NIBBLE3 (instruction & 0xF000) >> 12
#define NIBBLE2 (instruction & 0x0F00) >> 8
#define NIBBLE1 (instruction & 0x00F0) >> 4
#define NIBBLE0 (instruction & 0x000F)

typedef struct Chip8 {
    // Memory
    uint8_t mem[MEM_SIZE];

    //Size of ROM currently loaded
    std::streamsize romSize;

    //Stack
    uint16_t stack[16];

    // Graphics memory
    uint8_t gfx[64 * 32];

    // Data Registers
    uint8_t V[16];

    // Special Registers
    uint16_t PC;
    uint16_t I;
    uint8_t SP;
    uint8_t delay_timer;
    uint8_t sound_timer;

    Chip8() {
        // Clear all memory and registers
        std::memset(stack, 0, sizeof(stack));
        std::memset(mem, 0, MEM_SIZE);          // Initialize memory to 0
        std::memset(gfx, 0, sizeof(gfx));       // Initialize graphics memory to 0
        std::memset(V, 0, sizeof(V));            // Initialize data registers to 0

        romSize = 0;

        // Initialize special registers
        PC = 0x200;          // Set program counter to start of the program area
        I = 0;              // Clear index register
        SP = 0;             // Clear stack pointer
        delay_timer = 0;    // Initialize delay timer to 0
        sound_timer = 0;     // Initialize sound timer to 0
    }
} Chip8;

//Function to free resources upon early program termination
void cleanup(std::ofstream& state_file);

//Function to write the current state of the interpreter to a file dump
void writeStateToFile(const Chip8& chip8_state, uint16_t instruction, std::ofstream& file);

//Emulates a single cycle. Returns true the chip8 still had an instruction to execute this cycle; otherwise, false signals program end
bool emulateCycle(Chip8& chip8_state, uint16_t& instruction, std::ofstream& state_file);

#endif //CHIP8_H
