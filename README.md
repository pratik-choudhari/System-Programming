# System-Programming
A collection of various C programs utilising system calls, signals, pipes, threads and BASH

## move_copy

- Moves or copies a source directory into destination directory
- Maintains the original files structure in destination folder
- Can work on a subset of file extensions like copy only pdf txt mp4 files
- Usage: ./cpmvdir source_dir destination_dir option [extension_list]
    - source_dir: directory to copy/move
    - destination_dir: directory where files will be copied/moved
    - option: intent(-cp/-mv)
    - extension_list: list of file extensions to copy/move

## process_search

- Searches for processes in a root process tree structure
- Based on the option provied, crawls through the process tree and prints required information
- Usage: ./prcsearch root_process process_id1 [process_id(n)] [option]
  - root_process: process who's process tree has to be searched
  - process_id1, process_id(n): processes to process based on option 
  - option:
    - dn: lists the PIDs of all the non-direct descendants of process_id1 (only)
    - id: lists the PIDs of all the immediate descendants of process_id1
    - lp: additionally lists the PIDs of all the sibling processes of process_id1
    - sz: Lists the PIDs of all sibling processes of process_id1 that are defunct
    - gp: lists the PIDs of all the grandchildren of process_id1
    - zz: additionally prints the status of process_id1 (Defunct/ Not Defunct)
    - zc: lists the PIDs of all the direct descendants of process_id1 that are currently in the defunct state
    - zx: lists the PIDs of the direct descendants of process_id1..process_id[n] that are currently in the defunct state

## my_bash

- Simulate BASH operations like piping, conditional execution, sequential execution and background processing
- Usage: ./my_bash

## automated_backups

- BASH script to generate backups of pdf and txt files, runs infinitely
- The backup process has 2 phases: 
    - Complete: Create tar of all pdf and text files
    - Incremental: Create tar of only those pdf and text files modified since previous backup
- TAR files are stored in $HOME/backup/cb (for complete backup) or $HOME/backup/ib (for incremental backup)
- Usage: ./createbackups.sh

## pipe_cmds_c

- Simulate `cat sample.txt | grep word | wc -w` BASH command in C
- Makes use of execlp(), pipe(), fork() and dup2()
- Usage: ./pipecmd