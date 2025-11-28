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
// NOTE: This function does not catch exceptions. If an error (ADR/INS) 
// or halt (HLT) occurs, the corresponding Y86Exception is thrown 
// and must be caught at the Simulator level.
void run_cycle();
```

#### Internal Stages

Although execution is sequential, the internal logic of `run_cycle()` is strictly divided into the standard Y86 six stages. These private helper functions are called in sequence:

1.  **`fetch()`**: Reads the instruction bytes from the `Bus` at the `PC`.
      * Parses the `icode` (instruction code) and `ifun` (function code).
      * If the `Bus` throws an `AddressErrorException` during fetch, or an unknown `icode` is parsed (throwing `InvalidInstructionException`), the exception is propagated upwards.
2.  **`decode()`**: Reads the operands (`valA`, `valB`) from the Register File (RF).
3.  **`execute()`**: The ALU performs calculation or memory address computation.
      * Updates the `CC` flags.
      * Throws a `HaltInstruction` exception if a `halt` instruction is encountered.
4.  **`memory()`**: Reads or writes data via the `Bus`.
      * Handles `AddressErrorException` checking (which is delegated to the `Bus`).
5.  **`write_back()`**: Writes the result back to the Register File.
6.  **`update_pc()`**: Updates the PC to the newly calculated address (`valP` or jump target).


### `Bus`

When CPU needs to access memory or other devices, it DO NOT access the device directly, the instruction address is sent to Bus, and the Bus acts as an intermediary to access the actual device.

Register devices (for example, RAM) by:

```cpp
void Bus::register_device(Device& device, uint64_t start_addr, uint64_t end_addr);
```
Definite a struct `BusResult` in a shared .h file to include both result and fault type.

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

If the address is illegal, the function throws an ADR exception. Please refer to [Exceptions](#exceptions) for more information.

### `RAM`

The size of RAM is 4 KB, or 4096 Bytes.

* Derived from base class `Device`.

```cpp
uint8_t RAM::read(uint64_t addr); // return the data in byte
void RAM::write(uint64_t addr, uint8_t data);
```

### `ProgramLoader`

Not a part of computer, but a part of simulator.

Read from input flow, parse program, and write the machine code into RAM.

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
Then another HLT exception is thrown at `execute` phase. It replaces ADR in Stat due to same relative priority.

The cycle works like this:

```cpp
while(cpu.Stat == AOK){
  cpu.run_cycle(); // inside this function cpu sets Stat.
}
```
