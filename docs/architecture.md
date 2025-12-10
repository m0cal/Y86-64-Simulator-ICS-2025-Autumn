# Simulator Architecture

The overall simulator is modularized and comprised of the following core components (classes):

- `CPU`
- `Bus`
- `RAM`
- `ProgramLoader`
- `Logger`

These components are instantiated and integrated within the top-level `Simulator` class, which manages their interconnections and overall execution flow.

All of the classes listed above are defined within the `Y86_64` namespace.

## Simulator

### `CPU`

The core module of the computer. It implements the SEQ (Sequential) architecture for the Y86-64 instruction set. This means that the execution of each instruction is broken down into six logical stages and completed sequentially within a single `run_cycle` call.

#### Core State

##### RF: Program Registers

Contains 15 general-purpose registers (`%rax`, `%rcx`, `%rdx`, `%rbx`, `%rsp`, `%rbp`, `%rsi`, `%rdi`, `%r8`-`%r14`) following the Y86-64 specification.

##### CC: Conditional Codes

Three condition code flags:
- `ZF` (Zero Flag)
- `SF` (Sign Flag)
- `OF` (Overflow Flag)

These flags are set by arithmetic and logical operations and used by conditional jump and move instructions.

##### Stat: Program Status

Represents the current execution status of the CPU. Uses the `Y86Stat` enumeration:

1: AOK
2: HLT
3: ADR
4: INS

##### PC: Program Counter

Holds the address of the next instruction to be executed. Updated after each instruction cycle.

#### Interface & Execution

The `CPU` class does not contain the main execution loop; it is only responsible for executing a single cycle. It relies on the `Bus` for all memory interactions.

```cpp
// Constructor: The CPU must be bound to a system Bus instance
explicit CPU(Bus& system_bus);

// Resets the CPU to its initial state (PC=0, Stat=AOK, CC={1,0,0})
void reset();

// Executes a single instruction cycle
// Completion status is observed via the CPU Stat register.
void run_cycle();
```

#### Internal Stages

Although execution is sequential, the internal logic of `run_cycle()` is strictly divided into the standard Y86 six stages. These private helper functions are called in sequence:

1.  **`fetch()`**: Reads the instruction bytes from the `Bus` at the `PC`.
      * Parses the `icode` (instruction code) and `ifun` (function code).
      * Any invalid address or instruction sets the CPU `Stat` to `ADR` or `INS` for the simulator to observe.
2.  **`decode()`**: Reads the operands (`valA`, `valB`) from the Register File (RF).
3.  **`execute()`**: The ALU performs calculation or memory address computation.
      * Updates the `CC` flags.
      * Marks `Stat = HLT` when a `halt` instruction is encountered.
4.  **`memory()`**: Reads or writes data via the `Bus`.
      * Relies on Bus status codes to detect address faults instead of raising exceptions.
5.  **`write_back()`**: Writes the result back to the Register File.
6.  **`update_pc()`**: Updates the PC to the newly calculated address (`valP` or jump target).


### `Bus`

When the CPU needs to access memory or other devices, it does not access the device directly; the instruction address is sent to the Bus, and the Bus acts as an intermediary to access the actual device.

Register devices (for example, RAM) by:

```cpp
void Bus::register_device(Device& device, uint64_t start_addr, uint64_t end_addr);
```
Define a struct `BusResult` in a shared .h file to include both result and fault type.
```cpp
struct BusResult{
    uint8_t data;
    Y86Stat status_code; // only could be AOK or ADR
};
```
CPU access absolute address by:

```cpp
BusResult Bus::read(uint64_t addr);
BusResult Bus::write(uint64_t addr, uint8_t data);
```

Then Bus transform the absolute address to relative address, using the relative one to access the actual device.

$$
Address_{relative} = Address_{absolute} - Address_{start}
$$

If the address is illegal, the function returns an ADR status. Please refer to [Exceptions](#exceptions) for more information.

### `RAM`

The size of RAM is 4 KB, or 4096 Bytes.

* Derived from base class `Device`.

```cpp
uint8_t RAM::read(uint64_t addr); // return the data in byte
void RAM::write(uint64_t addr, uint8_t data);
```

### `ProgramLoader`

Not a part of computer, but a part of simulator.

Read from input stream, parse program, and write the machine code into RAM.

```cpp
void ProgramLoader::load_program(RAM& ram);
```

### `Logger`

Not a part of computer, too.

Get a snapshot of CPU in the end of each cycle, print all of them out in JSON when the program ends.

Provide **two** methods:

```cpp
void Logger::trace(const CPU& cpu, const RAM& ram) // constantly called in each cycle
void Logger::report() // called once in the end
```

## Exceptions

Inside the `CPU` class has a enum called Stat.

If exceptions happen, CPU writes them into Stat in following priority order:

> HLT > ADR > INS > AOK

For example, in a single cycle, we first get an ADR exception at `fetch` phase. It replaces `AOK` in Stat, because ADR has higher priority than AOK.
Then another HLT condition is detected at `execute` phase. It replaces ADR in Stat due to same relative priority.

The cycle works like this:

```cpp
while(cpu.Stat == AOK){
  cpu.run_cycle(); // inside this function cpu sets Stat.
}
```

## Additional devices

The devices following is not the typical part of a Y86-64 Simulator, but make it more like a Y86-64 computer. With features such as display and IO, we hope to run the game, Pong on this computer.

### Joysticks

The joysticks of this machine may include more than 6, but no more than 8 keys so they could be mapped within 1 byte.

For convenience, the joysticks is mapped at 8192，or 0x2000.

For pong:

| Low end | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | High end |
| ------- | - | - | - | - | - | - | - | - | -------- |
|   null  | Player A UP | Player A DOWN | Player B UP | Player B DOWN | Start | Reset | Reserved | Reserved | null |

### Display

The game will be rendered on terminal, with a fixed height of 30, fixed width of 120, 2-color display. so the overall VRAM size is 30*120 / 8 = 450 bytes. To render things fluently, we take a simple double buffering approach. The frame will be drawed to termial at the end of cycle.

The VRAM is designed as a part of PPU, so it should NOT be attached on System Bus.

### Pixel Processing Unit (PPU)

Draw sprites on buffer.

For each sprite, following info is crucial to draw them on display:

* storage location, or start address. (8 bytes)
* height & width, in pixel. Also used to calculate the size to read. (2 bytes)
* location on display, in pixel. (2 bytes)

Support render 16 sprites at the same time, so the overall size of PPU is 16*(8+2+2) = 192 bytes, on System Bus.

The sprites are numbered from 0 to 15.

PPU is mapped from 0x3000 to 0x30C0.

### Timer

To run the game at a fixed speed, we set a 60Hz Timer with auto-increment.
Size: 1 byte.
Address: 0x4000
