#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <random>
#include <cstdlib>
#include <cstring>

#define MEM_SIZE 4096

#define NIBBLE3 (instruction & 0xF000) >> 12
#define NIBBLE2 (instruction & 0x0F00) >> 8
#define NIBBLE1 (instruction & 0x00F0) >> 4
#define NIBBLE0 (instruction & 0x000F)

typedef struct Chip8 {
  // Memory
  uint8_t mem[MEM_SIZE];

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

    // Initialize special registers
    PC = 0x200;          // Set program counter to start of the program area
    I = 0;              // Clear index register
    SP = 0;             // Clear stack pointer
    delay_timer = 0;    // Initialize delay timer to 0
    sound_timer = 0;     // Initialize sound timer to 0
  }
} Chip8;

//Function to write the current state of the interpreter to a file dump
void writeStateToFile(const Chip8& chip8_state, uint16_t instruction, std::ofstream& file);

int main(int argc, char* argv[]) {
  //Validate command line arguments
  if (argc != 2) {
    std::cerr << "Usage: chip8 <path_to_rom>\n";
    exit(EXIT_FAILURE);
  }

  std::string rom_path(argv[1]);

  if (rom_path.size() < 4 || rom_path.substr(rom_path.size() - 4) != ".ch8") {
    std::cerr << "Error: ROM file must have a .ch8 extension." << std::endl;
    exit(EXIT_FAILURE);
  }

  /**** Load ROM into virtual memory and initialize the interpreter's state ****/
  std::cout << "Loading ROM: " << rom_path << std::endl;
  Chip8 chip8_state;
  //Set up chip8_state dump file
  std::filesystem::path path(rom_path);
  std::filesystem::path chip8_state_dump_path = path.parent_path().parent_path() / "chip8_state_dump" / (path.stem().string() + "_statedump.txt");
  std::ofstream state_file(chip8_state_dump_path.string(), std::ios::trunc);
  state_file << std::hex;
  if (!state_file.is_open()) {
    std::cerr << "Unable to open debugger (dump) file for writing\n";
  }

  //Load in ROM binary into file handle
  std::ifstream rom(rom_path, std::ios::binary | std::ios::ate);
  if (!rom) {
    std::cerr << "Error: Could not open ROM file: " << rom_path << std::endl;
    exit(EXIT_FAILURE);
  }

  //Get file size
  std::streamsize romSize = rom.tellg();
  rom.seekg(0, std::ios::beg);

  //Check ROM size
  const size_t loadAddress = 0x200; //CHIP-8 programs are usually loaded at address 0x200 (512)
  if (romSize > static_cast<std::streamsize>(MEM_SIZE - loadAddress)) {
    std::cerr << "Error: ROM file is too large to fit in memory." << std::endl;
    exit(EXIT_FAILURE);
  }

  //Read the ROM into the virtual memory array starting at address 0x200
  if (!rom.read(reinterpret_cast<char*>(&chip8_state.mem[loadAddress]), romSize)) {
    std::cerr << "Error reading ROM file." << std::endl;
    exit(EXIT_FAILURE);
  }
  /******************************************************************************/

  /**** main execution loop ****/
  bool done = false;
  uint16_t instruction;
  while((chip8_state.PC < (loadAddress + romSize)) && !done) {

    //Fetch instruction from virtual memory
    instruction = (chip8_state.mem[chip8_state.PC] << 8)  | chip8_state.mem[chip8_state.PC + 1];

    //Dump chip8_state contents
    writeStateToFile(chip8_state, instruction, state_file);

    //Determine operation from extracted opcode
    switch (NIBBLE3) {
      case 0: {
        if(instruction == 0x00E0) { //clear the screen
          ; //Todo
        }
        else if(instruction == 0x00EE) //return from subroutine
        {
          if(chip8_state.SP > 0) {
            chip8_state.PC = chip8_state.stack[--chip8_state.SP];
            continue;
          } else {
            std::cerr << "Stack underflow\n";
            exit(EXIT_FAILURE );
          }
        }
        break;
      }
      case 1:
        //(1NNN) Jump to address NNN instruction
        chip8_state.PC = (instruction & 0x0FFF);
        continue;
        break;
      case 2:
        //(2NNN) Execute subroutine at NNN
        if(chip8_state.SP < 16) {
          chip8_state.stack[chip8_state.SP++] = chip8_state.PC + 2;
          chip8_state.PC = (instruction & 0x0FFF);
          continue;
        } else {
          std::cerr << "Stack overflow\n";
          exit(EXIT_FAILURE);
        }
        break;
      case 3:
        //(3XNN) Skip the following instruction if VX equals NN
        if (chip8_state.V[NIBBLE2] == (instruction & 0x00FF)) {
          chip8_state.PC += 4; //skip
          continue;
        }
        break;
      case 4:
        //(4XNN) Skip the following instruction if VX does not equal NN
          if (chip8_state.V[NIBBLE2] != (instruction & 0x00FF)) {
            chip8_state.PC += 4; //skip
            continue;
          }
        break;
      case 5:
        //(5XY0) Skip the following instruction if VX equals VY
          if (chip8_state.V[NIBBLE2] == chip8_state.V[NIBBLE1]) {
            chip8_state.PC += 4; //skip
            continue;
          }
        break;
      case 6:
        //(6XNN) LD immediate instruction
        chip8_state.V[NIBBLE2] = (instruction & 0x00FF);
        break;
      case 7:
        //(7XNN) Add NN to VX
        chip8_state.V[NIBBLE2] += static_cast<uint8_t>(instruction & 0x00FF);
        break;
      case 8:
        //nested switch
        switch (NIBBLE0) {
          case 0:
            //(8XY0) copy VY into VX
              chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE1];
            break;
          case 1:
            //(8XY1) Set VX to VX OR VY
            chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE2] | chip8_state.V[NIBBLE1];
            break;
          case 2:
            //(8XY2) Set VX to VX AND VY
            chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE2] & chip8_state.V[NIBBLE1];
            break;
          case 3:
            //(8XY3) Set VX to VX XOR VY
            chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE2] ^ chip8_state.V[NIBBLE1];
            break;
          case 4: {
            //(8XY4) Add VY to VX, set VF (overflow flag)
            uint16_t result = static_cast<uint16_t>( chip8_state.V[NIBBLE2] ) + static_cast<uint16_t>( chip8_state.V[NIBBLE1] );
            if (result > UINT8_MAX)
              chip8_state.V[0xF] = 1;
            else
              chip8_state.V[0xF] = 0;
            chip8_state.V[NIBBLE2] = static_cast<uint8_t>(result);
            break;
          }
          case 5: {
            //(8XY5) Subtract VX = VX - VY, set VF (underflow flag)
            if (chip8_state.V[NIBBLE2] < chip8_state.V[NIBBLE1])
              chip8_state.V[0xF] = 0;
            else
              chip8_state.V[0xF] = 1;
            chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE2] - chip8_state.V[NIBBLE1];
            break;
          }
          case 6: {
            //(8XY6) VX = VY >> 1, VF is VY's lsb (before shift)
            chip8_state.V[0xF] = chip8_state.V[NIBBLE1] & static_cast<uint8_t>(0x01);
            chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE1] >> 1;
            break;
          }
          case 7: {
            //(8XY7) Subtract VX = VY - VX, set VF (underflow flag)
            if (chip8_state.V[NIBBLE1] < chip8_state.V[NIBBLE2])
              chip8_state.V[0xF] = 0;
            else
              chip8_state.V[0xF] = 1;
            chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE1] - chip8_state.V[NIBBLE2];
            break;
          }
          case 14: {
            //(8XYE) VX = VY << 1, VF is VY's msb (before shift)
            chip8_state.V[0xF] = (chip8_state.V[NIBBLE1] & static_cast<uint8_t>(0x80)) >> 7;
            chip8_state.V[NIBBLE2] = chip8_state.V[NIBBLE1] << 1;
            break;
          }
          default:
            std::cout << "Instruction not implemented or ROM error!" << std::endl;
            exit(EXIT_FAILURE);
        }
        //End of nested switch
        break;
      case 9:
        //(9XY0) Skip next instr if VX != VY
        if(chip8_state.V[NIBBLE2] != chip8_state.V[NIBBLE1]) {
          chip8_state.PC += 4;
          continue;
        }
        break;
      case 0xA:
        //(ANNN): Store address NNN in register I
        chip8_state.I = instruction & 0x0FFF;
        break;
      case 0xB:
        //(BNNN): Jump to address NNN + V0
        chip8_state.PC = (instruction & 0x0FFF) + static_cast<uint16_t>(chip8_state.V[0]);
        continue;
        break;
      case 0xC: {
        //(CXNN): Set VX to random number w/ mask of NN
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        chip8_state.V[NIBBLE2] = dist(gen) & static_cast<uint8_t>(instruction & 0x00FF);
        break;
      }
      case 0xF: {
        if(instruction == 0xFFF) { //CUSTOM HALT INSTRUCTION
          done = true;
        }
        break;
      }
      default:
        std::cout << "Instruction not implemented or ROM error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    //Increment instruction counter
    chip8_state.PC += 2;
  }
  /**** End of main execution loop ****/

  //FINAL dump chip8_state contents
  state_file << "STATE AFTER FINAL INSTRUCTION:\n";
  state_file << std::hex;
  writeStateToFile(chip8_state, instruction, state_file);

  //cleanup
  state_file.close();
  return EXIT_SUCCESS;
}

void writeStateToFile(const Chip8& chip8_state, uint16_t instruction, std::ofstream& file) {
  file << "PC: " << chip8_state.PC << "\n";
  file << "Instruction: 0x" << std::uppercase << instruction << "\n";
  file << "I: " << chip8_state.I << "\n";
  file << "SP: " << static_cast<int>(chip8_state.SP) << "\n";
  file << "Delay Timer: " << static_cast<int>(chip8_state.delay_timer) << "\n";
  file << "Sound Timer: " << static_cast<int>(chip8_state.sound_timer) << "\n";

  file << "Registers:\n";
  for (int i = 0; i < 16; ++i) {
    file << "V[" << i << "]: " << static_cast<int>(chip8_state.V[i]) << "\n";
  }

  for(int i = 0; i < 16; ++i) {
    file << "S[" << i << "]: " << static_cast<int>(chip8_state.stack[i]) << ", ";
  }

  /*file << "Memory:\n";
  for (int i = 0; i < 16; ++i) {
    file << std::hex << static_cast<int>(chip8.mem[i]) << " ";
  }*/

  file << "\n\n\n";  // Add a newline for better readability

}


