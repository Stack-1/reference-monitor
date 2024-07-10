# Reference Monitor

<!-- OUTLINE -->
<details>
  <summary>Outline</summary>
  <ol>
    <li>
        <a href="#Introduction">Introduction</a>
    </li>
    <li>
        <a href="#project-specification">Project Specification</a>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
    </li>
    <li>
      <a href="#conclusions">Conclusions</a>
    </li>
  </ol>
</details>



This is a project developed for the Advanced Operating Systems class in Tor Vergata University, In particular the goal of the project is to implement a Kernel module for Linux OS using x86 processors to setup a reference monitor that handles a file system.

## Introduction
The main goal of this project is to implement a reference monitor to prevent any type of modification on files or directories specified in a blackilist.
## Project Specification
This specification is related to a Linux Kernel Module (LKM) implementing a reference monitor for file protection. The reference monitor can be in one of the following four states:

* OFF, meaning that its operations are currently disabled;
* ON, meaning that its operations are currently enabled;
* REC-ON/REC-OFF, meaning that it can be currently reconfigured (in either ON or OFF mode). 

The configuration of the reference monitor is based on a set of file system paths. Each path corresponds to a file/dir that cannot be currently opened in write mode. Hence, any attempt to write-open the path needs to return an error, independently of the user-id that attempts the open operation.

Reconfiguring the reference monitor means that some path to be protected can be added/removed. In any case, changing the current state of the reference monitor requires that the thread that is running this operation needs to be marked with effective-user-id set to root, and additionally the reconfiguration requires in input a password that is reference-monitor specific. This means that the encrypted version of the password is maintained at the level of the reference monitor architecture for performing the required checks.

It is up to the software designer to determine if the above states ON/OFF/REC-ON/REC-OFF can be changed via VFS API or via specific system-calls. The same is true for the services that implement each reconfiguration step (addition/deletion of paths to be checked). Together with kernel level stuff, the project should also deliver user space code/commands for invoking the system level API with correct parameters.

In addition to the above specifics, the project should also include the realization of a file system where a single append-only file should record the following tuple of data (per line of the file) each time an attempt to write-open a protected file system path is attempted:

* the process TGID
* the thread ID
* the user-id
* the effective user-id
* the program path-name that is currently attempting the open
* a cryptographic hash of the program file content 

The the computation of the cryptographic hash and the writing of the above tuple should be carried in deferred work. 

## Getting Started
The project is composed of three main modules:
- **Systemcall table discoverer** (syscall-table-discoverer/)
This module implements the search for the systemcall table using informations about the entries that points to ni_sy_syscall (address to identify a free entry). Nowing where those entries are it is possible to identify the address of the systemcall table, starting from the lower half of the canonical adressing scheme for 64-bit processors (0xfffffffffffff000) and iterate untill we find the addresses of the dummy syscall in the correct offsets.

NOTE: This module works only for some kernel versions, in particular it doesn't work for kernel 5.15.0-103-generico or higher (>= 103) or for kernel 6.5. It works fine with kernel 4.15, 5.15.0-102-generic and lower (<= 102) and 6.2. Further testing should be done to have more general results.
- **Filesystem for loggging** (log-filesystem/)
- **Reference Monitor** (reference-monitor/)
## Conclusion
...
