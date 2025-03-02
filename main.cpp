#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <random>
#include <cstdlib>
#include <cstring>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>

#include "chip8.h"

constexpr int CHIP8_WIDTH = 64;
constexpr int CHIP8_HEIGHT = 32;
constexpr int SCALE = 10; //Makes each chip8 pixel equal a 10x10 "pixel" on target display

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
  chip8_state.romSize = rom.tellg();
  rom.seekg(0, std::ios::beg);

  //Check ROM size
  if (chip8_state.romSize > static_cast<std::streamsize>(MEM_SIZE - loadAddress)) {
    std::cerr << "Error: ROM file is too large to fit in memory." << std::endl;
    exit(EXIT_FAILURE);
  }

  //Read the ROM into the virtual memory array starting at address 0x200
  if (!rom.read(reinterpret_cast<char*>(&chip8_state.mem[loadAddress]), chip8_state.romSize)) {
    std::cerr << "Error reading ROM file." << std::endl;
    exit(EXIT_FAILURE);
  }
  /******************************************************************************/

  /**** main execution loop ****/
  bool done = false;
  uint16_t instruction;
  while(emulateCycle(chip8_state, instruction, state_file)) {


  }

  /**** End of main execution loop ****/

  //FINAL dump chip8_state contents
  state_file << "STATE AFTER FINAL INSTRUCTION:\n";
  state_file << std::hex;
  writeStateToFile(chip8_state, instruction, state_file);

  cleanup(state_file);
  return EXIT_SUCCESS;
}




