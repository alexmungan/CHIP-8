#include "chip8.h"

int key_pressed = -1;

uint8_t chip8_fontset[80] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void cleanup(std::ofstream& state_file) {
  if (state_file) state_file.close();
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

    //Stack
    for(int i = 0; i < 16; ++i) {
        file << "S[" << i << "]: " << static_cast<int>(chip8_state.stack[i]) << ", ";
    }

    file << "\n";

    //keypad state
    for(int i = 0; i < 16; ++i) {
      file << "keypad[" << i << "]: " << static_cast<int>(chip8_state.keypad[i]) << ", ";
    }

    /*file << "Memory:\n";
    for (int i = 0; i < 16; ++i) {
      file << std::hex << static_cast<int>(chip8.mem[i]) << " ";
    }*/

    file << "\n\n\n";  // Add a newline for better readability

}

bool emulateCycle(Chip8& chip8_state, uint16_t& instruction, std::ofstream& state_file) {
    bool running = true;

    if ((chip8_state.PC >= (loadAddress + chip8_state.romSize)))
        return (running = false);

    //Fetch instruction from virtual memory
    instruction = (chip8_state.mem[chip8_state.PC] << 8)  | chip8_state.mem[chip8_state.PC + 1];

    //Dump chip8_state contents
    writeStateToFile(chip8_state, instruction, state_file);

    //Determine operation from extracted opcode
    switch (NIBBLE3) {
      case 0: {
        if(instruction == 0x00E0) { //clear the screen
          for (int i = 0; i < 64 * 32; ++i) {
            chip8_state.gfx[i] = 0; // Set each pixel to 0 (black)
          }
        }
        else if(instruction == 0x00EE) //return from subroutine
        {
          if(chip8_state.SP > 0) {
            chip8_state.PC = chip8_state.stack[--chip8_state.SP];
            chip8_state.stack[chip8_state.SP] = 0; //clear old return value, not technically necessary, just for clarity in statedump file
            return true;
          } else {
            std::cerr << "Stack underflow\n";
            cleanup(state_file);
            exit(EXIT_FAILURE );
          }
        }
        break;
      }
      case 1:
        //(1NNN) Jump to address NNN instruction
        chip8_state.PC = (instruction & 0x0FFF);
        return true;
        break;
      case 2:
        //(2NNN) Execute subroutine at NNN
        if(chip8_state.SP < 16) {
          chip8_state.stack[chip8_state.SP++] = chip8_state.PC + 2;
          chip8_state.PC = (instruction & 0x0FFF);
          return true;
        } else {
          std::cerr << "Stack overflow\n";
          cleanup(state_file);
          exit(EXIT_FAILURE);
        }
        break;
      case 3:
        //(3XNN) Skip the following instruction if VX equals NN
        if (chip8_state.V[NIBBLE2] == (instruction & 0x00FF)) {
          chip8_state.PC += 4; //skip
          return true;
        }
        break;
      case 4:
        //(4XNN) Skip the following instruction if VX does not equal NN
          if (chip8_state.V[NIBBLE2] != (instruction & 0x00FF)) {
            chip8_state.PC += 4; //skip
            return true;
          }
        break;
      case 5:
        //(5XY0) Skip the following instruction if VX equals VY
          if (chip8_state.V[NIBBLE2] == chip8_state.V[NIBBLE1]) {
            chip8_state.PC += 4; //skip
            return true;
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
            uint8_t flag;
            if (result > UINT8_MAX)
              flag= 1;
            else
              flag = 0;
            chip8_state.V[NIBBLE2] = static_cast<uint8_t>(result);
            chip8_state.V[0xF] = flag;
            break;
          }
          case 5: {
            //(8XY5) Subtract VX = VX - VY, set VF (underflow flag)
            uint16_t result = static_cast<uint16_t>(chip8_state.V[NIBBLE2]) - static_cast<uint16_t>(chip8_state.V[NIBBLE1]);
            uint8_t flag;
            if (chip8_state.V[NIBBLE2] < chip8_state.V[NIBBLE1])
              flag = 0;
            else
              flag = 1;
            chip8_state.V[NIBBLE2] = static_cast<uint8_t>(result);
            chip8_state.V[0xF] = flag;
            break;
          }
          case 6: {
            //(8XY6) VX = VY >> 1, VF is VY's lsb (before shift)
            uint8_t result = chip8_state.V[NIBBLE1] >> 1;
            uint8_t lsb = chip8_state.V[NIBBLE1] & static_cast<uint8_t>(0x01);
            chip8_state.V[NIBBLE2] = result;
            chip8_state.V[0xF] = lsb;
            break;
          }
          case 7: {
            //(8XY7) Subtract VX = VY - VX, set VF (underflow flag)
            uint16_t result = static_cast<uint16_t>(chip8_state.V[NIBBLE1]) - static_cast<uint16_t>(chip8_state.V[NIBBLE2]);
            uint8_t flag;
            if (chip8_state.V[NIBBLE1] < chip8_state.V[NIBBLE2])
              flag = 0;
            else
              flag = 1;
            chip8_state.V[NIBBLE2] = static_cast<uint8_t>(result);
            chip8_state.V[0xF] = flag;
            break;
          }
          case 14: {
            //(8XYE) VX = VY << 1, VF is VY's msb (before shift)
            uint8_t result = chip8_state.V[NIBBLE1] << 1;
            uint8_t msb = (chip8_state.V[NIBBLE1] & static_cast<uint8_t>(0x80)) >> 7;
            chip8_state.V[NIBBLE2] = result;
            chip8_state.V[0xF] = msb;
            break;
          }
          default:
            std::cout << "Instruction not implemented or ROM error!" << std::endl;
            cleanup(state_file);
            exit(EXIT_FAILURE);
        }
        //End of nested switch
        break;
      case 9:
        //(9XY0) Skip next instr if VX != VY
        if(chip8_state.V[NIBBLE2] != chip8_state.V[NIBBLE1]) {
          chip8_state.PC += 4;
          return true;
        }
        break;
      case 0xA:
        //(ANNN): Store address NNN in register I
        chip8_state.I = instruction & 0x0FFF;
        break;
      case 0xB:
        //(BNNN): Jump to address NNN + V0
        chip8_state.PC = (instruction & 0x0FFF) + static_cast<uint16_t>(chip8_state.V[0]);
        return true;
        break;
      case 0xC: {
        //(CXNN): Set VX to random number w/ mask of NN
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        chip8_state.V[NIBBLE2] = dist(gen) & static_cast<uint8_t>(instruction & 0x00FF);
        break;
      }
      case 0xD: {
        //(DXYN) Draw a sprite using XOR
        int x = chip8_state.V[NIBBLE2];
        int y = chip8_state.V[NIBBLE1];
        chip8_state.V[0xF] = 0; // VF is used for collision detection

        for (int row = 0; row < NIBBLE0; row++) {
          uint8_t sprite_byte = chip8_state.mem[chip8_state.I + row]; // Read one row of the sprite

          for (int col = 0; col < 8; col++) { // Iterate over 8 bits in the byte
            int pixel_x = (x + col) % 64;
            int pixel_y = (y + row) % 32;
            int pixel_index = pixel_y * 64 + pixel_x;

            uint8_t sprite_pixel = (sprite_byte >> (7 - col)) & 1; // Extract one pixel (bit)

            if (sprite_pixel) {
              if (chip8_state.gfx[pixel_index]) {
                chip8_state.V[0xF] = 1; // Collision detected
              }
              chip8_state.gfx[pixel_index] ^= 1; // XOR pixel value
            }
          }
        }
        break;
      }
      case 0xE: {
        if ((instruction & 0x00FF) == 0x9E) { //If key is pressed, skip next instr
          int key = chip8_state.V[NIBBLE2];
          if (chip8_state.keypad[key]) {
            chip8_state.PC += 4;
            return true;
          }
        }
        else if ((instruction & 0x00FF) == 0xA1) { //If key is NOT pressed, skip next instr
          int key = chip8_state.V[NIBBLE2];
          if (!chip8_state.keypad[key]) {
            chip8_state.PC += 4;
            return true;
          }
        }
        break;
      }
      case 0xF: {
        if(instruction == 0xFFFF) { //CUSTOM HALT INSTRUCTION
          return (running = false);
        }
        switch (instruction & 0x00FF) {
          case 0x07:
            //Set VX to the current value of delay timer
            chip8_state.V[NIBBLE2] = chip8_state.delay_timer;
            break;
          case 0x0A: {
            //Wait for a keypress, store the result in VX
            if ((key_pressed != -1) && chip8_state.keypad[key_pressed] == 0) {
              key_pressed = -1;
              break;
            }

            //bool keyPressed = false;
            for (uint8_t key = 0; key < 16; key++) {
              if (chip8_state.keypad[key]) {
                key_pressed = key;
                chip8_state.V[NIBBLE2] = key;
                //keyPressed = true;
                break;
              }
            }
            //if (!keyPressed) {
              chip8_state.PC -= 2; // Re-execute the instruction until a key is pressed and released
            //}
            break;
          }
          case 0x15:
            //Set the delay timer to VX
            chip8_state.delay_timer = chip8_state.V[NIBBLE2];
            break;
          case 0x18:
            //Set the sound timer to VX
            chip8_state.sound_timer = chip8_state.V[NIBBLE2];
            break;
          case 0x1E:
            //Add VX to register I
            chip8_state.I += chip8_state.V[NIBBLE2];
            break;
          case 0x29:
            //Set I to the address of the sprite data for the hex digit stored in VX
            chip8_state.I = (FONT_START) + (chip8_state.V[NIBBLE2] * 5);
            break;
          case 0x33: {
            //Store BCD encoded version of VX into I, I+1, I+2
            uint8_t val = chip8_state.V[NIBBLE2];
            chip8_state.mem[chip8_state.I] = val / 100;
            chip8_state.mem[chip8_state.I + 1] = (val / 10) % 10;
            chip8_state.mem[chip8_state.I + 2] = val % 10;
            break;
          }
          case 0x55:
            //Fill mem locations I,...,I+X with V0,...,VX, set I to I + X + 1
            for (int i = 0; i <= NIBBLE2; i++) {
              chip8_state.mem[chip8_state.I+i] = chip8_state.V[i];
            }
            chip8_state.I = chip8_state.I + static_cast<uint16_t>(NIBBLE2) + 1;
            break;
          case 0x65:
            //Fill V0 to VX with values stored at I, I+1, ..., set I to I + X + 1
            for (int i = 0; i <= NIBBLE2; i++) {
              chip8_state.V[i] = chip8_state.mem[chip8_state.I+i];
            }
            chip8_state.I = chip8_state.I + static_cast<uint16_t>(NIBBLE2) + 1;
            break;
          default:
            std::cout << "Instruction not implemented or ROM error!" << std::endl;
            cleanup(state_file);
            exit(EXIT_FAILURE);
        }
        break;
      }
      default:
        std::cout << "Instruction not implemented or ROM error!" << std::endl;
        cleanup(state_file);
        exit(EXIT_FAILURE);
    }

    //Increment instruction counter
    chip8_state.PC += 2;


    return running;
}