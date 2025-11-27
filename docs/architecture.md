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

CPU access absolute address by:

```cpp
uint8_t Bus::read(uint64_t addr);
void Bus::write(uint64_t addr, uint8_t data);
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

The simulator uses C++ exception handling to manage runtime errors and status changes. All Y86-specific exceptions derive from the `Y86Exception` base class.

### Exception Hierarchy

```
std::runtime_error
    └── Y86Exception (base class for all Y86 exceptions)
        ├── InvalidInstructionException (INS)
        ├── AddressErrorException (ADR)
        └── HaltInstruction (HLT)
```

### `Y86Exception`

Base class for all Y86-related exceptions. Stores the corresponding `Y86Stat` code.

**Key Methods:**
```cpp
Y86Stat get_y86_stat_code() const; // Returns the associated Y86 status code
```

### `InvalidInstructionException`

Thrown when an illegal instruction is encountered (sets status to `INS`).

**Constructor:**
```cpp
InvalidInstructionException(long long pc, int icode);
```
- `pc`: Program counter where the invalid instruction was encountered
- `icode`: The illegal instruction code

### `AddressErrorException`

Thrown when an illegal memory address is accessed (sets status to `ADR`).

**Constructor:**
```cpp
AddressErrorException(long long pc, long long addr, const std::string& type);
```
- `pc`: Program counter at the time of the error
- `addr`: The illegal address being accessed
- `type`: Type of access (`"read"` or `"write"`)

### `HaltInstruction`

Thrown when a halt instruction is executed (sets status to `HLT`). While not an error, using an exception allows clean control flow interruption.

**Constructor:**
```cpp
HaltInstruction(long long pc);
```
- `pc`: Program counter where the halt instruction was encountered

### Exception Handling Pattern

The simulator's main execution loop catches these exceptions to update the CPU status:

```cpp
while (cpu.Stat == AOK) {
    try {
        // Execute instruction cycle
    } 
    catch (const Y86Exception& e) {
        cpu.Stat = e.get_y86_stat_code();
        // Log error information
    } 
    catch (const std::exception& e) {
        cpu.Stat = Y86Stat::ERR;
        // Handle system-level exceptions
    }
}
```