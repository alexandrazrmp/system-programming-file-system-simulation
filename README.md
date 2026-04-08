# File System Synchronization System (C, Linux)

## Overview
A multi-process file synchronization system implemented in C for Linux environments. The system monitors directories and ensures continuous synchronization between source and target paths using event-driven updates.

## Architecture
The system consists of three main components:

- **fss_manager** – core coordinator that monitors file changes and schedules synchronization tasks  
- **fss_console** – command-line interface for interacting with the system  
- **worker** – executes file operations (copy, update, delete)  

## Key Features
- Real-time directory monitoring using `inotify`
- Multi-process architecture using `fork()`
- Inter-process communication via named pipes (FIFOs)
- Worker queue for handling concurrency limits
- Configurable synchronization via config files
- Command-driven interaction (add, sync, cancel, status, shutdown)

## How It Works
1. The manager reads a configuration file containing directory pairs
2. `inotify` watches source directories for changes
3. Events trigger synchronization tasks
4. Tasks are executed by worker processes
5. If worker limit is reached, tasks are queued
6. The console communicates with the manager through named pipes

## Tech Stack
- C
- Linux / POSIX
- inotify
- Named Pipes (FIFOs)
- Makefile
- Bash

## Build
```bash
make

## Run

### Start Manager
```bash
./fss_manager -l <manager_logfile> -c <config_file> -n <worker_limit>
```

### Start Console
```bash
./fss_console -l <console_logfile>
```

### Run Worker (for testing)
```bash
./worker <source_dir> <target_dir> ALL FULL
```

### Run Script (partial)
```bash
./fss_script.sh -p <path> -c <command>
```

---

## Skills Demonstrated

- Systems programming in C
- Process management and IPC
- Event-driven programming (inotify)
- Concurrency control with worker queues
- Debugging multi-process systems
- Designing modular low-level architectures

---

## Notes

- Worker limit is managed via a queue for pending tasks
- Logging system implemented but can be further refined
- Script functionality partially implemented
