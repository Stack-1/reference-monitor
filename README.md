# Reference Monitor

<!-- OUTLINE -->
<summary>Outline</summary>
<ol>
  <li>
      <a href="#Introduction">Introduction</a>
  </li>
  <li>
      <a href="#project-specification">Project Specification</a>
  </li>
  <li>
    <a href="#project-implementation">Project Implementation</a>
  </li>
  <li>
    <a href="#getting-started">Getting Started</a>
  </li>
</ol>


This is a project developed for the Advanced Operating Systems class in Tor Vergata University, In particular the goal of the project is to implement a Kernel module for Linux OS using x86 processors to setup a reference monitor that handles a file system.

## Introduction
The main goal of this project is to implement a reference monitor to prevent any type of modification on files or directories specified in a blackilist. This goal has been achieved hacking the systemcall table and reusing its free entries to implement syscalls usefull to handle the reference monitor state, kernel side. Next, it was necessary to probe all the functions that where invoked during a write/move/copy/delete/rename of a file. This has been done by looking at linux documentation using https://elixir.bootlin.com/linux/latest/source checking different versions of the kernel to see the different possible implementation of the functions we wanted to probe and by using strace -c \<command\> to have an overview of the used functions. (E.g. strace -c vim temp.txt, where temp.txt is the blacklisted file).
Moreover, there were used kretprobes as core technology to intercept the different functions and check if the dentry passed as a parameter of these function is releted to a path that is blacklisted. If so, the entry kretprobe handler stops the invocation of the function and excecute the return handler.
In this portion of code it is printed on dmesg what has been done and is it called to write on a log different information about the offending thread (this is done in deferred work). 
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

## Project Implementation
The project is composed of three main modules:
- **Systemcall table discoverer** (syscall-table-discoverer/)
This module implements the search for the systemcall table using informations about the entries that points to ni_sy_syscall (address to identify a free entry). Nowing where those entries are it is possible to identify the address of the systemcall table, starting from the lower half of the canonical adressing scheme for 64-bit processors (0xfffffffffffff000) and iterate untill we find the addresses of the dummy syscall in the correct offsets.

NOTE: This module works only for some kernel versions, in particular it doesn't work for kernel 5.15.0-103-generico or higher (>= 103) or for kernel 6.5. It works fine with kernel 4.15, 5.15.0-102-generic and lower (<= 102) and 6.2. Further testing should be done to have more general results.
- **Filesystem for loggging** (log-filesystem/)
This is a filesystem mounted on /opt/mount/ , used to keep a log file reguarding the informations of the threads excecuting operations intercepted by the reference monitor. 
- **Reference Monitor** (reference-monitor/)
The first thing done in this module is the syscall table hacking adding four different systemcalls:
  - sys_switch_rf_state --> Set the RF as ON,OFF,REC_ON,REC_OFF (0,1,2,3)
  - sys_add_to_blacklist --> Add the full path of the file or directory to the blacklist
  - sys_remove_from_blacklist --> Remove the full path of the file or directory to the blacklist
  - sys_print_blacklist --> Print on dmesg the whole blacklist (used for debug purpose, it could be turned in a syscall that brings the blacklists entries in user space as a string)
  - sys_get_blacklist_size --> Returns the size of the blacklist (used for debug purpose)

  Each syscall is defined both using asmlinkage function and __SYSCALL_DEFINEx() macro, depending on the kenrel version (version 4.17.0 is the turning point).
  The other core aspect of the RF implementation is the use of kretprobes. In particular the approach is to have two handlers:
  - Entry handler: Used to check if a path is blacklisted (starting from the dentry)
  - Ret handler: Invoked only when the entry handler find a match i n the blacklist, prints infos of the offending operation blocked and save them in the log using a deferred work scheme.

  In this project different functions are probed, escpecially the ones reguirding inodes (link/unlink/mkdir ect...). In order to understand what functions to probe it was searched on the documentation the implementation of some of the ones used by the most common software approach to modify a file; testing has been done over the use of the most common shell commands (mv/cp/echo/rm/rm -r/rmdir/mkdir) and on the most common text editors (gedit/vim/nano/kate/emacs)

## Getting Started
To compile and run this project firstly disable the secure boot or sign the modules used with a valid CA sign. Secondly you can follow the automatic installation or the manual one:
- Automatic install
- Manual install