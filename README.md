# Operating-Systems
Writing program linkers, io &amp; process schedulers for Operating Systems
This repository contains the following C++ files:

ioshed.cpp

Description: This C++ file implements an I/O scheduler, responsible for optimizing the order of input/output requests to storage devices.
Purpose: The I/O scheduler aims to improve overall I/O performance by reducing seek times and minimizing I/O latency.
linker.cpp

Description: This C++ file implements a program linker, responsible for combining compiled object files into a single executable program.
Purpose: The linker resolves symbol references, performs memory address relocation, and handles library inclusion to create an executable binary.
sched.cpp

Description: This C++ file implements a process scheduler, responsible for managing the allocation of CPU time to multiple processes.
Purpose: The process scheduler determines the execution order of processes, handles time-sharing, and ensures fair access to the CPU.
mmu.cpp

Description: This C++ file implements a Memory Management Unit (MMU), responsible for managing memory resources and virtual memory mapping.
Purpose: The MMU translates virtual addresses to physical addresses, enforces memory protection, and allows processes to have independent address spaces.
Compilation and Execution
To compile each of the C++ files, use your preferred C++ compiler and link the corresponding object files together.

For example, using g++ on the command line:

scheduler: sched.cpp
	g++ -std=gnu++11 sched.cpp -o sched.exe 

clean:
	rm -f sched.exe *~
