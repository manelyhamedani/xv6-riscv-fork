# xv6 - A re-implementation of Unix Version 6

xv6 is a re-implementation of Dennis Ritchie's and Ken Thompson's Unix
Version 6 (v6). xv6 loosely follows the structure and style of v6,
but is implemented for a modern RISC-V multiprocessor using ANSI C.

## ACKNOWLEDGMENTS

xv6 is inspired by John Lions's Commentary on UNIX 6th Edition (Peer
to Peer Communications; ISBN: 1-57398-013-7; 1st edition (June 14,
2000)). See also https://pdos.csail.mit.edu/6.1810/, which provides
pointers to online resources for v6.

The following people have made contributions: Russ Cox (context switching,
locking), Cliff Frey (MP), Xiao Yu (MP), Nickolai Zeldovich, and Austin
Clements.

We are also grateful for the bug reports and patches contributed by
Takahiro Aoyagi, Marcelo Arroyo, Silas Boyd-Wickizer, Anton Burtsev,
carlclone, Ian Chen, Dan Cross, Cody Cutler, Mike CAT, Tej Chajed,
Asami Doi, Wenyang Duan, eyalz800, Nelson Elhage, Saar Ettinger, Alice
Ferrazzi, Nathaniel Filardo, flespark, Peter Froehlich, Yakir Goaron,
Shivam Handa, Matt Harvey, Bryan Henry, jaichenhengjie, Jim Huang,
Matúš Jókay, John Jolly, Alexander Kapshuk, Anders Kaseorg, kehao95,
Wolfgang Keller, Jungwoo Kim, Jonathan Kimmitt, Eddie Kohler, Vadim
Kolontsov, Austin Liew, l0stman, Pavan Maddamsetti, Imbar Marinescu,
Yandong Mao, Matan Shabtay, Hitoshi Mitake, Carmi Merimovich, Mark
Morrissey, mtasm, Joel Nider, Hayato Ohhashi, OptimisticSide,
phosphagos, Harry Porter, Greg Price, RayAndrew, Jude Rich, segfault,
Ayan Shafqat, Eldar Sehayek, Yongming Shen, Fumiya Shigemitsu, snoire,
Taojie, Cam Tenny, tyfkda, Warren Toomey, Stephen Tu, Alissa Tung,
Rafael Ubal, Amane Uehara, Pablo Ventura, Xi Wang, WaheedHafez,
Keiichi Watanabe, Lucas Wolf, Nicolas Wolovick, wxdao, Grant Wu, x653,
Jindong Zhang, Icenowy Zheng, ZhUyU1997, and Zou Chang Wei.

## ERROR REPORTS

Please send errors and suggestions to Frans Kaashoek and Robert Morris
(kaashoek,rtm@mit.edu). The main purpose of xv6 is as a teaching
operating system for MIT's 6.1810, so we are more interested in
simplifications and clarifications than new features.

## BUILDING AND RUNNING XV6

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu. Once they are installed, and in your shell
search path, you can run "make qemu".

## Project Modifications by Manely Ghasemnia Hamedani

In this project, the following system calls and features have been added:

### Part 1: Adding `child_processes` System Call

A new system call, `child_processes`, has been implemented to retrieve and display the list of all descendants for a given process. The system call takes the current process and outputs the list of its descendants. This feature improves process management by allowing easier tracking of child processes in the system.

Key Features:
- Retrieves and displays descendants for the current process.
- Outputs the name, pid, parent pid, and status of each all descendant.

### Part 2: Adding `report_traps` System Call

The `myrep` system call was introduced to report crash events of all descendants. The system call allows the user to view the last 10 crash reports of descendants. Additionally, `sysrep`, an advanced version of the system call, provides system-wide crash reports.

Key Features:
- Displays the last 10 crash reports of all descendants.
- Stores the reports in a circular buffer.

### Part 3: Storing Crash Reports in a File

The new feature stores crash reports in a file (`/reports.bin`) for persistence. The crash reports are written to the file in binary format, with the total number of reports stored in the first 4 bytes of the file. This allows for the tracking and retrieval of historical crash data.

Key Features:
- Stores crash reports to a file (`/reports.bin`).
- Supports reading the reports from the file with the `sysrep` system call.
- Handles file reading and writing from kernel space without causing deadlocks.

### 4. **Additional Enhancements**

- **Process Cleanup (`killall` System Call)**: A new system call, `killall`, was added to terminate all processes except for essential ones such as `init` and `shell`. This is useful for cleaning up the system during testing.
- **Testing Programs**: A suite of user-space programs were developed to demonstrate and validate the new system calls:
  - **`childrentest.c`**: A test program that forks multiple child processes and displays the list of child processes.
  - **`reptest.c`**: A test program that simulates process crashes and displays crash reports via the `myrep` system call.
  - **`sysrep.c`**: A program that reads and displays the crash reports stored in `/reports.bin`.

## LICENSE

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
