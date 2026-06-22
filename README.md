# :shell: MINI SHELL

## :open_book: OVERVIEW
Date: Feb 2024\
Developer(s): Ashneet Rathore

Mini Shell is a custom Unix-like shell program that accepts and processes commands, supporting both foreground and background execution. Users can view their command history and a list of active background processes, run built-in commands, and use simple pipelines and I/O redirection.

## :film_strip: DEMO
![Demo](demo.gif)

## :gear: HOW IT WORKS
Written in **C**, Mini Shell processes user commands as jobs executed in either the foreground or background. **Foreground jobs** run synchronously and block the shell until completion, while **background jobs** run asynchronously, allowing for continued user input. Active background jobs and command history, limited to the last five commands, are tracked using **linked list data structures**.

Mini Shell follows a **parent-child model**, with the shell as the parent process and external commands executed as child processes. The parent manages its children, waiting on foreground jobs until they complete and reaping background jobs when they terminate. 

**Built-in commands** are handled directly by the shell, while external commands run in child processes. **Pipelines** allow the output of one process to be used as the input of another, chaining multiple processes together without intermediate files. **I/O redirection** allows processes to read from or write to files instead of the terminal.

**Signal handling** tracks child process termination and triggers the display of active background jobs, ensuring proper process management. On exit, the shell frees memory and terminates any remaining background processes.

## :open_file_folder: PROJECT FILE STRUCTURE
```bash
mini-shell/
│── src/
│   │── icssh.c        # Main shell program
│   │── helpers.c      # Shell helper functions
│   └── linkedlist.c   # Linked list for cmd history and background processes
│── rsrc/
│   └── icssh.supp     # Valgrind warning suppressions for memory checks
│── lib/
│   └── icsshlib.o     # Pre-compiled helper functions
│── include/           # Header files for source files
│── Makefile           # Builds and clean config
│── README.md          # Project documentation
│── .gitignore         # Ignored files
└── demo.gif           # Demo GIF
```

## :rocket: EXECUTION
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
Run a command normally to execute it in the foreground
```bash
sleep 10
```
Append `&` to run a command in the background
```bash
sleep 10 &
```

Built-in commands supported by Mini Shell:
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
Separate commands with the `|` operator to chain them - up to 2 pipes (3 processes) are supported. The example below lists files in `src/`, filters those ending in `.c`, and counts the matches:
```bash
ls src | grep '\.c$' | wc -l
```

### :keyboard: I/O REDIRECTION
Use `<` to read from a file and `>` to write to a file. The example below reads from `README.md`, searches for the string `shell`, and writes the matching lines to `result.txt`:
```bash
grep "shell" < README.md > result.txt
```