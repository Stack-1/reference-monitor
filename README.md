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



This is a project developed for the Advanced Operating Systems class in Tor Vergata University, In particular the goal of the project is to implement a Kernel module for Linux Kernel to setup a reference monitor that handles a file system.

## Introduction
....
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
...
## Conclusion
...
