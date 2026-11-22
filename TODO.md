# TODO
This file will serve as a simple todo listfor what we still need implemented. Tick things off as you go when you commit your changes, and put your name beside what you did.

*NOTE: This TODO's initial state was generated with the assistance of AI. If there are any inconsistencies or info that needs to be changed as we figure out what to do, please do so*

- [ ] Configuration & Initialization (`config.txt` / `initialize`)
  - [x] Parse and validate new memory parameters in power-of-2 format within $[2^6, 2^{16}]$ (64 to 65,536 bytes): `max-overall-mem`, `mem-per-frame`, `min-mem-per-proc`, and `max-mem-per-proc` [Gian]
  - [ ] Compute total frames as `max-overall-mem / mem-per-frame` (and page size equal to `mem-per-frame`)
  - [ ] Support process memory allocation using rolled value $M$ in `[min-mem-per-proc, max-mem-per-proc]` and required pages $P = M / \text{mem-per-frame}$ for processes spawned via `scheduler-start`

- [ ] Memory Manager & Demand Paging
  - [ ] Pre-allocate emulated physical memory space and manage free/occupied frames on demand
  - [ ] Implement demand paging allocator: handle page faults when a referenced page is not in a frame, load pages from backing store into a free frame, and select/evict a victim frame when no free frames are available
  - [ ] Ensure memory allocation and page fault handling occur when a process is assigned to a CPU worker, retrying the instruction until a valid page/frame is available
  - [ ] Implement backing store file (`csopesy-backing-store.txt`) read/write operations for swapping/evicting pages or context-switching processes when memory is low
  - [ ] Track memory & CPU metrics: total/used/free memory, active/inactive processes or memory, idle/active/total CPU ticks, and cumulative pages paged in / paged out

- [ ] Process Memory Representation & Instructions
  - [ ] Enforce per-process memory allocation rules (`[2^6, 2^{16}]` and power of 2; at least 64 bytes) and print `"invalid memory allocation"` if invalid
  - [ ] Implement fixed 64-byte symbol table segment per process storing up to 32 `uint16` variables (2 bytes each, clamped to `[0, max(uint16)]`; ignore new variable declarations once the 32-variable limit is reached)
  - [ ] Implement `READ (var, memory_address)` (hex address; uninitialized block reads as `0`) and `WRITE (memory_address, value)` (`uint16` value written to hex address)
  - [ ] Include `READ` and `WRITE` instructions in randomized instruction generation for `scheduler-start`
  - [ ] Handle out-of-bounds/invalid memory access violations by shutting down the offending process and recording the timestamp (`<HH:MM:SS>`) and invalid hex address

- [ ] Main Menu & Screen Commands
  - [ ] Update `screen -s <process_name> <process_memory_size>` to require and validate process memory size
  - [ ] Add `screen -c <process_name> <process_memory_size> "<instructions>"` to parse 1–50 semicolon-separated instructions (print `"invalid command"` if instruction count is outside 1–50)
  - [ ] Update `screen -r <process_name>` to print `"Process <process name> shut down due to memory access violation error that occurred at <HH:MM:SS>. <Hex memory address> invalid."` for processes terminated by a memory access violation
  - [ ] Add `process-smi` command in main menu (CPU utilization, memory usage/utilization, and running processes with memory usage)
  - [ ] Add `vmstat` command in main menu (detailed view of memory, pages paged in/out, and CPU ticks)

- [ ] Deliverables & Pre-Quiz Prep
  - [ ] Update `README.txt` (group info, run instructions, entry file with `main`) and verify source code / repo
  - [ ] Prepare technical report PPT covering grouping, command recognition, process & memory representation/addressing, scheduler implementation, and demand paging / backing store operations
  - [ ] Prepare `.MP4` demo recordings and parameter-only test configurations (no recompiling during quiz)

### M01 TODO

I'm just saving this here for documentation purposes, but you can basically ignore this for MO2.

- [x] FOR instruction [Gian]
- [x] The other instructions [Sky]
- [x] RR Scheduler function [Sky]
- [x] Main menu console commands
  - [x] scheduler-start 
    - [x] update generateInstructions function to properly use all the other instructions.
  - [x] scheduler-stop
  - [x] report-util [MJ]
  - [x] screen (update to match specs** -- a bit of conflicting info about what -s should do, so idk)[^1] [MJ] (went with -s creates, -r attaches)
- [x] process screen commands [Gian]
  - [x] process-smi + changing the behavior of the print command**



[^1]: The `screen -s` command is mentioned multiple times in the specs to do these two functions: to "open" the process screen up or basically attach the process screen (see specs page 3) AND to create a new process (see specs page 2). BOTH follow the exact same format, however another command `screen -r` is mentioned that does the screen attaching. Just in my opinion (Gian's opinion), `screen -s` should probably be used for creating the processes, and `screen -r` for attaching to a process screen.