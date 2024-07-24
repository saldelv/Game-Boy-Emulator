#include "Chip8.h"

using namespace std;

Chip8::Chip8()
{
    ProgramCounter = 0x200;
    I = 0;
    StackPointer = 0;
    SoundTimer = 0;
    DelayTimer = 0;
    key = 0xff;

    for (int i = 0; i < 4096; i++) {
        Memory[i] = 0;
    }
    for (int i = 0; i < 16; i++) {
        V[i] = 0;
        Stack[i] = 0;
    }
    for (int i = 0; i < 64; i++) {
        for (int j = 0; j < 32; j++) {
            display[i][j] = 0;
        }
    }      

    for (int i = 50; i < 80 + 50; i++) {
        Memory[i] = font[i-50];
    }

    sound = new Sound();
}

void Chip8::load_rom(const char* rom_name)
{
    char* raw;

    ifstream rom_file(rom_name, ios::in | ios::binary | ios::ate);
    if (!rom_file) {
        cout << "Invalid ROM name" << endl;
        exit(0);
    }

    ifstream::pos_type rom_size = rom_file.tellg();

    raw = new char[rom_size];

    rom_file.seekg(0, ios::beg);
    rom_file.read(raw, rom_size);
    rom_file.close();

    for (int i = 0; i < rom_size; i++) {
        Memory[512 + i] = raw[i];
    }
}

void Chip8::cycle(bool debug)
{
    int16_t opcode = (Memory[ProgramCounter] << 8) | Memory[ProgramCounter + 1]; // fetch
    ProgramCounter += 2;

    int16_t nnn = m_nnn(opcode);
    uint8_t xh = m_xh(opcode);
    uint8_t x = m_xl(opcode);
    uint8_t y = m_yh(opcode);
    uint8_t kk = m_low(opcode);
    uint8_t n = m_n(opcode);

    //decode and execute
    switch (xh) // checks hex of first 4 bits 
    {
        case 0x0:
        {
            switch (opcode) // checks opcode 
            {
                case 0x00E0: // 00E0 - CLS
                {
                    for (int i = 0; i < 64; i++) {
                        for (int j = 0; j < 32; j++) {
                            display[i][j] = 0;
                        }
                    }    

                    if (debug == true) cout << "00E0" << endl;                
                    break;
                }
                case 0x00EE: // RET
                {
                    ProgramCounter = Stack[StackPointer];
                    StackPointer--;

                    if (debug == true) cout << "00EE" << endl; 
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case 0x1: // 1nnn - JP addr
        {
            ProgramCounter = nnn;

            if (debug == true) cout << "1nnn" << endl;
            break;
        }
        case 0x2: // 2nnn - CALL addr
        {
            StackPointer++;
            Stack[StackPointer] = ProgramCounter;
            ProgramCounter = nnn;

            if (debug == true) cout << "2nnn" << endl;
            break;
        }
        case 0x3: // 3xkk - SE Vx, byte
        {
            if (V[x] == kk) {
                ProgramCounter += 2;
            }

            if (debug == true)cout << "3xkk" << endl;
            break;
        }
        case 0x4: // 4xkk - SNE Vx, byte
        {
            if (V[x] != kk) {
                ProgramCounter += 2;
            }

            if (debug == true) cout << "4xkk" << endl;
            break;
        }
        case 0x5: // 5xy0 - SE Vx, Vy
        {
            if (V[x] == V[y]) {
                ProgramCounter += 2;
            }

            if (debug == true) cout << "5xy0" << endl;
            break;
        }
        case 0x6: // 6xkk - LD Vx, byte
        {
            V[x] = kk;

            if (debug == true) cout << "6xkk" << endl;
            break;
        }
        case 0x7: // 7xkk - ADD Vx, byte
        {
            V[x] += kk;

            if (debug == true) cout << "7xkk" << endl;
            break;
        }
        case 0x8:
        {
            switch (n) // checks last 4 bits
            {
                case 0x0: // 8xy0 - LD Vx, Vy
                {
                    V[x] = V[y];

                    if (debug == true) cout << "8xy0" << endl;
                    break;
                }
                case 0x1: // 8xy1 - OR Vx, Vy
                {
                    V[x] = V[x] | V[y];

                    if (debug == true) cout << "8xy1" << endl;
                    break;
                }
                case 0x2: // 8xy2 - AND Vx, Vy
                {
                    V[x] = V[x] & V[y];

                    if (debug == true) cout << "8xy2" << endl;
                    break;
                }
                case 0x3: // 8xy3 - XOR Vx, Vy
                {
                    V[x] = V[x] ^ V[y];

                    if (debug == true) cout << "8xy3" << endl;
                    break;
                }
                case 0x4: // 8xy4 - ADD Vx, Vy
                {
                    if (V[x] + V[y] > 255) {
                        V[F] = 1;
                    }
                    else {
                        V[F] = 0;
                    }
                    V[x] = (V[x] + V[y]) & 0xff;

                    if (debug == true) cout << "8xy4" << endl;
                    break;
                }
                case 0x5: // 8xy5 - SUB Vx, Vy
                {
                    if (V[x] >= V[y]) {
                        V[F] = 1;
                    }
                    else {
                        V[F] = 0;
                    }
                    V[x] = V[x] - V[y];

                    if (debug == true) cout << "8xy5" << endl;
                    break;
                }
                case 0x6: // 8xy6 - SHR Vx {, Vy}
                {
                    if (V[x] & 0x1 == 1) {
                        V[F] = 1;
                    }
                    else {
                        V[F] = 0;
                    }
                    V[x] = V[x] >> 1;

                    if (debug == true) cout << "8xy6" << endl;
                    break;
                }
                case 0x7: // 8xy7 - SUBN Vx, Vy
                {
                    if (V[y] >= V[x]) {
                        V[F] = 1;
                    }
                    else {
                        V[F] = 0;
                    }
                    V[x] = V[y] - V[x];

                    if (debug == true) cout << "8xy7" << endl;
                    break;
                }
                case 0xE: // 8xyE - SHL Vx {, Vy}
                {
                    if (floor(log2(V[x])) == 1) {
                        V[F] = 1;
                    }
                    else {
                        V[F] = 0;
                    }
                    V[x] = V[x] << 1;

                    if (debug == true) cout << "8xyE" << endl;
                    break;
                }
            }
            break;
        }
        case 0x9: // 9xy0 - SNE Vx, Vy
        {
            if (V[x] != V[y]) {
                ProgramCounter += 2;
            }

            if (debug == true) cout << "9xy0" << endl;
            break;
        }
        case 0xA: // Annn - LD I, addr
        {
            I = nnn;

            if (debug == true) cout << "Annn" << endl;
            break;
        }
        case 0xB: // Bnnn - JP V0, addr
        {
            ProgramCounter = nnn + V[0];

            if (debug == true) cout << "Bnnn" << endl;
            break;
        }
        case 0xC: // Cxkk - RND Vx, byte
        {
            V[x] = (rand() % 255) & kk;

            if (debug == true) cout << "Cxkk" << endl;
            break;
        }
        case 0xD: // Dxyn - DRW Vx, Vy, nibble
        {
            V[F] = 0;
            for (int i = 0; i < n; i++) {
                uint8_t s = Memory[I + i]; // sprite from memory
                int32_t row = (V[y] + i) % 32; // row value that wraps arround
                for (int j = 0; j < 8; j++) {
                    int32_t col = (V[x] + j) % 64; // col value that wraps around
                    int32_t bit = (s & 0x80) >> 7; // most significant bit from the sprite to decide if the pixel should appear or not
                    if (bit == 1) {
                        if (display[col][row] == 1) {
                            display[col][row] = 0;
                            V[F] = 1;
                        }
                        else {
                            display[col][row] = 1;
                        }
                    }
                    s = s << 1; // shift to next bit of sprite
                }
            }

            if (debug == true) cout << "Dxyn" << endl;
            break;
        }
        case 0xE:
        {
            switch(kk)
            {
                case 0x9E: // Ex9E - SKP Vx
                {
                    if (key == V[x]) {
                        ProgramCounter += 2;
                    }

                    if (debug == true) cout << "Ex9E" << endl;
                    break;
                }
                case 0xA1: // ExA1 - SKNP Vx
                {
                    if (key != V[x]) {
                        ProgramCounter += 2;
                    }

                    if (debug == true) cout << "ExA1" << endl;
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case 0xF:
        {
            switch(kk) // checks last 8 bits
            {
                case 0x07: // Fx07 - LD Vx, DT
                {
                    V[x] = DelayTimer;

                    if (debug == true) cout << "Fx07" << endl;
                    break;
                }
                case 0x0A: // Fx0A - LD Vx, K
                {
                    // wait for key
                    if (key == 0xff) {
                        ProgramCounter -= 2;
                    }
                    else {
                        V[x] = key;
                    }

                    if (debug == true) cout << "Fx0A" << endl;
                    break;
                }
                case 0x15: // Fx15 - LD DT, Vx
                {
                    DelayTimer = V[x];

                    if (debug == true) cout << "Fx15" << endl;
                    break;
                }
                case 0x18: // Fx18 - LD ST, Vx
                {
                    SoundTimer = V[x];

                    if (debug == true) cout << "Fx18" << endl;
                    break;
                }
                case 0x1E: // Fx1E - ADD I, Vx
                {
                    I = I + V[x];

                    if (debug == true) cout << "Fx1E" << endl;
                    break;
                }
                case 0x29: // Fx29 - LD F, Vx
                {
                    I = (V[x] *5) & 0xfff;  

                    if (debug == true) cout << "Fx29" << endl;
                    break;
                }
                case 0x33: // Fx33 - LD B, Vx
                {
                    Memory[I] = (V[x] / 100) % 10;
                    Memory[I + 1] = (V[x] / 10) % 10;
                    Memory[I + 2] = V[x] % 10;

                    if (debug == true) cout << "Fx33" << endl;
                    break;
                }
                case 0x55: // Fx55 - LD [I], Vx
                {
                    int16_t I_Index = I;
                    for (uint8_t i = 0; i <= x; i++) {
                        Memory[I_Index] = V[i];
                        I_Index++; 
                    }

                    if (debug == true) cout << "Fx55" << endl;
                    break;
                }
                case 0x65: // Fx65 - LD Vx, [I]
                {
                    int16_t I_Index = I;
                    for (uint8_t i = 0; i <= x; i++) {
                        V[i] = Memory[I_Index];
                        I_Index++; 
                    }

                    if (debug == true) cout << "Fx65" << endl;
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
    }
}

void Chip8::decrement_timers()
{
    if (DelayTimer > 0) {
        DelayTimer--;
    }
    if (SoundTimer > 0) {
        sound->playSound();
        SoundTimer--;
    }
}
