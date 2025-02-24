#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#define MEM_SIZE 4096

typedef struct Chip8 {
  // Memory
  uint8_t mem[MEM_SIZE];

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
void writeStateToFile(const Chip8& chip8_state, std::ofstream& file);

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
  while(chip8_state.PC < (loadAddress + romSize)) {
    //Dump chip8_state contents
    writeStateToFile(chip8_state, state_file);

    //Fetch instruction from virtual memory
    uint16_t instruction = (chip8_state.mem[chip8_state.PC] << 8)  | chip8_state.mem[chip8_state.PC + 1];

    //Determine operation from extracted opcode
    switch ((instruction & 0xF000) >> 12) {
      case 6:
        //(6XNN) LD immediate instruction
        chip8_state.V[(instruction & 0x0F00) >> 8] = (instruction & 0x00FF);
        break;
      default:
        std::cout << "Instruction not implemented or ROM error!" << std::endl;
        break;
    }

    //Increment instruction counter
    chip8_state.PC += 2;
  }
  /**** End of main execution loop ****/

  //FINAL dump chip8_state contents
  writeStateToFile(chip8_state, state_file);

  //cleanup
  state_file.close();
  return EXIT_SUCCESS;
}

void writeStateToFile(const Chip8& chip8_state, std::ofstream& file) {
  file << "PC: " << chip8_state.PC << "\n";
  file << "I: " << chip8_state.I << "\n";
  file << "SP: " << static_cast<int>(chip8_state.SP) << "\n";
  file << "Delay Timer: " << static_cast<int>(chip8_state.delay_timer) << "\n";
  file << "Sound Timer: " << static_cast<int>(chip8_state.sound_timer) << "\n";

  file << "Registers:\n";
  for (int i = 0; i < 16; ++i) {
    file << "V[" << i << "]: " << static_cast<int>(chip8_state.V[i]) << "\n";
  }

  /*file << "Memory:\n";
  for (int i = 0; i < 16; ++i) {
    file << std::hex << static_cast<int>(chip8.mem[i]) << " ";
  }*/

  file << "\n\n";  // Add a newline for better readability

}


