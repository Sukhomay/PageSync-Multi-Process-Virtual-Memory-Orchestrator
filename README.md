# PageSync: Multi-Process Virtual Memory Orchestrator

This project simulates demand-paged virtual memory in a multiprogramming environment. It mimics the behavior of a system where processes generate virtual addresses, which are then translated to physical addresses by the Memory Management Unit (MMU). Page faults are handled by bringing pages from disk to main memory. The simulation includes four modules:

1. **Master**: Initializes data structures, creates scheduler and MMU, and generates processes.
2. **Scheduler**: Schedules processes using the First-Come, First-Served (FCFS) algorithm.
3. **MMU**: Translates page numbers to frame numbers, handles page faults, and manages frame allocation.
4. **Processes**: Generate page numbers from reference strings and communicate with the MMU.

## Input
- Total number of processes (k)
- Maximum number of pages required per process (m)
- Total number of frames (f)

## Master Process
- Creates and initializes data structures:
  - Page Table
  - Free Frame List
  - Ready Queue
- Creates scheduler and MMU
- Generates processes with reference strings
- Coordinates termination of modules

### Implementation Details
- Implemented in `Master.c`
- Reads inputs (k, m, f) and initializes data structures
- Generates reference strings for processes
- Handles process creation and termination

## Scheduler
- Schedules processes using FCFS
- Receives notifications from MMU
- Handles process termination

### Implementation
- Implemented in `sched.c`
- Informed by Master to terminate all modules after process execution

## Processes
- Generate page numbers from reference strings
- Communicate with MMU via message queue
- Handle page faults and termination

### Implementation
- Implemented in `process.c`
- Pauses until scheduled by the scheduler
- Sends page numbers to MMU and receives frame numbers or error codes

## Memory Management Unit (MMU)
- Translates page numbers to frame numbers
- Handles page faults and invalid references
- Maintains a global timestamp

### Implementation
- Implemented in `MMU.c`
- Receives page numbers from processes and updates page table
- Handles page faults and updates free frame list
- Notifies scheduler of process completion or termination

## Data Structures
- **Page Table**: Shared memory for each process
- **Free Frame List**: Shared memory managed by MMU
- **Process to Page Number Mapping**: Can be implemented using shared memory

## Output
- MMU.c prints page fault information in an xterm:
  - Page fault sequence (pi,xi)
  - Invalid page reference (pi,xi)
  - Global ordering (ti,pi,xi)
- All outputs are also written to `result.txt`.

### Running the Code
Run in terminal:
  make all       [to run the code]
  make clean     [to clean old executable and object files]

### Dependencies
- POSIX message queues for inter-process communication
- Shared memory for data structures management

## Acknowledgments
This project is part of an assignment in Operating System course of IIT KGP.

### Contributors
- [Sukhomay Patra]
- [Raj Parikh]
