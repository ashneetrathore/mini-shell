# :shell: MINI SHELL

## :open_book: OVERVIEW
Date: Feb 2024\
Developer(s): Ashneet Rathore\
Based on assignment instructions from Prof. Jennifer Wong-Ma

Mini Shell is a custom Unix-like shell program that accepts and processes commands, supporting both foreground and background execution. Users can view their command history and a list of active background processes, run built-in commands including `cd` and `estatus`, and use simple pipelines and I/O redirection.

## :film_strip: DEMO
![Demo](demo.gif)

## :gear: HOW IT WORKS
Written in the **C** language, Mini Shell processes user commands as jobs, executing them in either the foreground or background. Jobs executed in the **foreground** run synchronously and block the shell until completion, and those executed in the **background** run asynchronously, allowing for continued user input. 

Mini Shell represents each inputted command as a `job_info` struct, containing one more `proc_info` structs for the processes in that job. Active background jobs are tracked using a **linked list data structure**, containing `bgentry_t` structs. The command history, limited to the last five commands, is also tracked using a linked list data structure.

Mini Shell follows a **parent-child model**, with the shell as the parent process and external commands executed as child processes created via `fork()`. The parent manages its children, waiting on foreground jobs until they complete and reaping background jobs when they terminate. 

The program supports a number of [built-in commands](#running_man-running-commands), including `bglist` to view running background jobs and `!` to re-execute the most recent command from history. **Built-in commands** are handled directly by the shell, whereas all other commands, known as external commands, run in child processes.

The shell supports **pipelines**, allowing the standard output of one command to be used as the standard input of another. This enables multiple processes to be chained together in a singular job without the creation of intermediate files. **Input/output redirection** for files is also supported. Each `job_info` struct contains `in_file` and `out_file` fields, which specify the files for standard input and standard output respectively. When a command with a redirection operator is executed, the shell sets up these streams so the process reads from or writes to the specified files instead of the terminal.

**Signal handling** is used to track child process termination (SIGCHLD) and to display active background jobs (SIGUSR2), ensuring proper process management. The shell also manages its own termination, upon the `exit` command, by freeing memory and terminating any remaining background processes.

## :open_file_folder: PROJECT FILE STRUCTURE
```bash
mini-shell/
│── src/
│   │── icssh.c        # Defines the main shell program
│   │── helpers.c      # Provides helper functions for the shell
│   └── linkedlist.c   # Defines linked list (used to track cmd history and background processes)
│── rsrc/
│   └── icssh.supp     # Suppresses Valgrind warnings during memory checks (for testing purposes)
│── lib/
│   └── icsshlib.o     # Provides pre-compiled helper functions
│── include/           # Contains header files (*.h) for source files (*.c)
│── Makefile           # Builds and cleans shell executable
│── README.md          # Project documentation
│── .gitignore         # Excludes files and folders from version control
└── demo.gif           # GIF showing the shell demo
```

## :rocket: SET UP & EXECUTION
> [!IMPORTANT]
> Mini Shell must be run in a Linux environment, as it relies on POSIX system calls.

**1. Clone the repository**
```bash
git clone https://github.com/ashneetrathore/mini-shell.git
```

**2. Run the program**
```bash
make clean
make
./bin/minishell
```

## :wrench: TRY IT OUT
### :running_man: RUNNING COMMANDS
Run a command normally to execute it in the foreground. The example below runs a `sleep` process for 10 seconds in the foreground.
```bash
sleep 10
```
Run a command, with `&` appended to the end of it, to execute it in the background. The example below runs a `sleep` process for 10 seconds in the background.
```bash
sleep 10 &
```

Here are all the available built-in commands supported by Mini Shell:
| Command     | Description                                                             |
|-------------|-------------------------------------------------------------------------|
| `estatus`   | Displays the exit status of the last command executed in the foreground |
| `cd <path>` | Changes the current working directory of the shell                      |
| `bglist`    | Displays a list of active background processes                          |
| `history`   | Displays the last five executed commands                                |
| `!`         | Re-executes the most recent command from history                        |
| `!<n>`      | Re-executes command number *n* from the last five commands in history   |
| `exit`      | Exits the shell program                                                 |

> [!NOTE]
> Commands not listed above may also be entered. These are treated as external commands and are executed in child processes, not directly by the shell.

### :link: PIPELINES
To use pipelines, separate multiple commands with the `|` operator. Mini Shell supports up to 2 pipes, allowing 3 processes to be chained together. The example below lists the files in the `src` directory with the `ls` command, filters only those ending in `.c` using the `grep` command, and counts the number of matching files with the `wc -l` command.
```bash
ls src | grep '\.c$' | wc -l
```

### :keyboard: I/O REDIRECTION
To redirect input or output, use the `<` operator to read from a file and the `>` to write to a file. The example below reads from `README.md`, searches for the string `shell`, and writes the matching lines to `result.txt`.
```bash
grep "shell" < README.md > result.txt
```