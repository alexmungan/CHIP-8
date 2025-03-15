#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <random>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System.hpp>

#include "chip8.h"

constexpr int SCALE = 10; //Makes each chip8 pixel equal a 10x10 "pixel" on target display

void drawGraphics(sf::RenderWindow& window, const uint8_t display[CHIP8_HEIGHT * CHIP8_WIDTH]);
//Map SFML keys to CHIP-8 keys (0-15)
int mapKeyToChip8(sf::Keyboard::Scancode code);

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
    cleanup(state_file);
    exit(EXIT_FAILURE);
  }

  //Read the ROM into the virtual memory array starting at address 0x200
  if (!rom.read(reinterpret_cast<char*>(&chip8_state.mem[loadAddress]), chip8_state.romSize)) {
    std::cerr << "Error reading ROM file." << std::endl;
    cleanup(state_file);
    exit(EXIT_FAILURE);
  }
  /******************************************************************************/


  /******** main execution loop ********/
  uint16_t instruction;
  sf::RenderWindow window(sf::VideoMode({CHIP8_WIDTH * SCALE, CHIP8_HEIGHT * SCALE}), "CHIP-8 Emulator");

  //Load Sound Buffer
  sf::SoundBuffer beepBuffer;
  if (!beepBuffer.loadFromFile("beep.wav")) {
    std::cerr << "Failed to load beep sound!\n";
    cleanup(state_file);
    exit(EXIT_FAILURE);
  }
  sf::Sound beepSound(beepBuffer);
  beepSound.setLooping(true);

  //Chip8 refresh rate
  const int FPS = 60;
  const std::chrono::milliseconds FRAME_DURATION(1000 / FPS);
  auto last_timer_update = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds time_accumulator(0);

  while (window.isOpen()) {
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_timer_update);
    //std::cout << "Current time = " << std::chrono::duration_cast<std::chrono::seconds>(current_time.time_since_epoch()).count() << " ms" << std::endl;
    //std::cout << "last_timer_update = " << std::chrono::duration_cast<std::chrono::seconds>(last_timer_update.time_since_epoch()).count() << " ms" << std::endl;
    time_accumulator += elapsed_time;
    last_timer_update = current_time;
    //std::cout << "Actual frame time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() << " ms" << std::endl;
    //std::cout << "time accumulator: " << std::chrono::duration_cast<std::chrono::milliseconds>(time_accumulator).count() << std::endl;

    //Process events (keyboard, mouse, etc.)
    while (const std::optional event = window.pollEvent()) {
      //std::cout << "Processing event...\n";
      if (event->is<sf::Event::Closed>()) {
        window.close();  // Close the window when the close button is clicked
      }
      else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        int keyIndex = mapKeyToChip8(keyPressed->scancode);
        if (keyIndex != -1) {
          chip8_state.keypad[keyIndex] = 1; // Mark the key as pressed
        }
      }
      else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
        int keyIndex = mapKeyToChip8(keyReleased->scancode);
        if (keyIndex != -1) {
          chip8_state.keypad[keyIndex] = 0; // Mark the key as released
        }
      }
    }

    //FRAME
    if (time_accumulator >= FRAME_DURATION) { //If at least 16.67ms has passed
      //std::cout << "New FRAME\n";

      //Run multiple CPU cycles per frame
      int cycles_per_frame = 12; //Chip8 CPU's run about 8-16 instructions per frame I think???
      for (int i = 0; i < cycles_per_frame; i++) {
        if (!emulateCycle(chip8_state, instruction, state_file)) {
          window.close();
          break;
        }
      }

      if (chip8_state.delay_timer > 0) {
        chip8_state.delay_timer--;
      }
      if (chip8_state.sound_timer > 0) {
        chip8_state.sound_timer--;
        //std::cout << "sound_timer = " << static_cast<int>(chip8_state.sound_timer) << std::endl;
        if (beepSound.getStatus() != sf::SoundSource::Status::Playing) {
          beepSound.play();
        }
      } else {
        if (beepSound.getStatus() == sf::SoundSource::Status::Playing)
          beepSound.stop();
      }

      drawGraphics(window, chip8_state.gfx);

      time_accumulator -= FRAME_DURATION;

    } //END OF FRAME

    if (time_accumulator < FRAME_DURATION) {
      std::this_thread::sleep_for(FRAME_DURATION - time_accumulator);
    }

  }

  /******** End of main execution loop ********/

  //FINAL dump chip8_state contents
  state_file << "STATE AFTER FINAL INSTRUCTION:\n";
  state_file << std::hex;
  writeStateToFile(chip8_state, instruction, state_file);

  cleanup(state_file);
  return EXIT_SUCCESS;
}

// Assume chip8.display is a 64x32 array of 0s and 1s
void drawGraphics(sf::RenderWindow& window, const uint8_t display[CHIP8_HEIGHT* CHIP8_WIDTH]) {
  window.clear(sf::Color::Black);

  sf::RectangleShape pixel(sf::Vector2f(SCALE, SCALE));
  pixel.setFillColor(sf::Color::White);

  for (int y = 0; y < CHIP8_HEIGHT; y++) {
    for (int x = 0; x < CHIP8_WIDTH; x++) {
      if (display[y * CHIP8_WIDTH + x]) {  // Flattened indexing
        pixel.setPosition(sf::Vector2f(x * SCALE, y * SCALE));
        window.draw(pixel);
      }
    }
  }

  window.display();
}

int mapKeyToChip8(sf::Keyboard::Scancode code) {
  switch (code) {
    case sf::Keyboard::Scancode::Num1: return 1;
    case sf::Keyboard::Scancode::Num2: return 2;
    case sf::Keyboard::Scancode::Num3: return 3;
    case sf::Keyboard::Scancode::Num4: return 0xC;
    case sf::Keyboard::Scancode::Q: return 4;
    case sf::Keyboard::Scancode::W: return 5;
    case sf::Keyboard::Scancode::E: return 6;
    case sf::Keyboard::Scancode::R: return 0xD;
    case sf::Keyboard::Scancode::A: return 7;
    case sf::Keyboard::Scancode::S: return 8;
    case sf::Keyboard::Scancode::D: return 9;
    case sf::Keyboard::Scancode::F: return 0xE;
    case sf::Keyboard::Scancode::Z: return 0xA;
    case sf::Keyboard::Scancode::X: return 0;
    case sf::Keyboard::Scancode::C: return 0xB;
    case sf::Keyboard::Scancode::V: return 0xF;
    default: return -1;
  }
}


