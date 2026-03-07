#include "icssh.h"
#include "helpers.h"
#include "linkedlist.h"
#include <readline/readline.h>

volatile int num_bg = 0;
volatile sig_atomic_t flag = 0;

void handler1(int sig) {
	flag = 1;
}

void handler2(int sig) {
	write(STDERR_FILENO, "Num of Background processes: ", strlen("Num of Background processes: "));
	char str[11];
	snprintf(str, 11, "%d", num_bg);
	write(STDERR_FILENO, str, strlen(str));
	write(STDERR_FILENO, "\n", strlen("\n"));
}

void handle_exec_error(proc_info* proc, job_info* job, char* line) {
	printf(EXEC_ERR, proc->cmd);
	// Cleaning up to make Valgrind happy 
	free_job(job);
	free(line);
	// Calling validate_input with NULL will free the memory it has allocated
	validate_input(NULL);
	exit(EXIT_FAILURE);
}

void insert_bgentry(list_t* bglist, job_info* job, pid_t pid, volatile int* num_bg) {
	// Malloc and initialize the bgentry_t struct
	bgentry_t* bgentry = malloc(sizeof(bgentry_t));
	bgentry->job = job;
	bgentry->pid = pid;
	bgentry->seconds = time(NULL);
	InsertAtTail(bglist, (void*) bgentry);
	(*num_bg)++;
}

void safe_wait(pid_t pid, int* exit_status, list_t* bglist, list_t* history) {
	pid_t wait_result = waitpid(pid, exit_status, 0);
	if (wait_result < 0) {
		printf(WAIT_ERR);
		DestroyBG(&bglist);
		DestroyHistory(&history);
		exit(EXIT_FAILURE);
	}
}


int main(int argc, char* argv[]) {
    int max_bgprocs = -1;
	int exec_result;
	int exit_status = -100;
	pid_t pid;
	pid_t pid1;
	pid_t pid2;
	pid_t pid3;
	char* line;
	list_t* bglist = CreateList(NULL, NULL, &bgDeleter);
	list_t* history = CreateList(NULL, NULL, &historyDeleter);
	signal(SIGCHLD, handler1);
	signal(SIGUSR2, handler2);

#ifdef GS
    rl_outstream = fopen("/dev/null", "w");
#endif

    // Check command line arg
    if(argc > 1) {
        int check = atoi(argv[1]);
        if(check != 0)
            max_bgprocs = check;
        else {
            printf("Invalid command line argument value\n");
            exit(EXIT_FAILURE);
        }
    }

	// Setup segmentation fault handler
	if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
		perror("Failed to set signal handler");
		exit(EXIT_FAILURE);
	}

    // Print the prompt & wait for the user to enter commands string
	while ((line = readline(SHELL_PROMPT)) != NULL) {
        // MAGIC HAPPENS! Command string is parsed into a job struct
        // Will print out error message if command string is invalid
		job_info* job = validate_input(line);
        if (job == NULL) { // Command was empty string or invalid
			free(line);
			continue;
		}
		// Check flag here
		if (flag == 1) {
			pid_t wpid;
			while ((wpid = waitpid(-1, &exit_status, WNOHANG)) > 0) {
				num_bg--;
				bgRemoveTerminated(bglist, wpid);
			}
			flag = 0;
		}

        // Prints out the job linked list struture for debugging
        #ifdef DEBUG   // If DEBUG flag removed in makefile, this will not longer print
     		debug_print_job(job);
        #endif
		// Built-in !
		if (job->procs->cmd[0] == '!') { 
			if (strlen(job->procs->cmd) == 1) { // Only '!' case
				free_job(job);
				free(line);
				if (history->head != NULL) {
					line = malloc(strlen((char*) history->head->data) + 1);
					strcpy(line, (char*) history->head->data);
					job = validate_input(line);
					if (job == NULL) { // Command was empty string or invalid
						free(line);
						continue;
					}
					else { fprintf(stdout, "%s\n", job->line); }
				}
				else { continue; }
			}
	
			else if (strlen(job->procs->cmd) == 2) {
				free_job(job);
				free(line);
				int executed = 0;
				char i = job->procs->cmd[1];
				if (i >= 49 && i <= 53) { // Char in between '1' and '5'
					int x = i - '0'; // Convert char to digit
					node_t* current = history->head; // Start at beginning of list
					int i = 1;
					while (current != NULL) {
						if (i == x) {
							line = malloc(strlen((char*) current->data) + 1);
							strcpy(line, (char*) current->data);
							job = validate_input(line);
							if (job == NULL) { // Command was empty string or invalid
								free(line);
								break;
							}
							else {
								fprintf(stdout, "%s\n", job->line);
								executed = 1;
								break;
							}
						}
						i++;
						current = current->next;
					}
					
				}
				if (!executed) { // Invalid index (ex: !9), invalid job command, reached end of history
					continue;
				}
				
			}
			else {  // Invalid index (ex: !35)
				free_job(job);
				free(line);
				continue; 
			}
		}
		// Update history
		if (!(strcmp(job->procs->cmd, "history") == 0)) { // If command not history, add to history list
			if (history->length == 5) { // If history already has 5 commands, remove oldest
				removeTail(history); // Decrements length
			}
			char* line_cmd = malloc(strlen(job->line) + 1);
			strcpy(line_cmd, job->line);
			InsertAtHead(history, line_cmd); // Add line to history list & increments length
		}
		// Built-in: exit
		if (strcmp(job->procs->cmd, "exit") == 0) {
			node_t* current = bglist->head; // Start at beginning of list
			while (current != NULL) { // Iterate until end of list is reached
				bgentry_t* bgentry = current->data;
				kill(bgentry->pid, SIGKILL); // Kill infinite processes
				fprintf(stdout, BG_TERM, bgentry->pid, bgentry->job->line);
				current = current->next; // Continue iteration
			}

			// Terminating the shell
			free(line);
			free_job(job);
            validate_input(NULL);
			DestroyBG(&bglist);
			DestroyHistory(&history);
            return 0;
		}
		// Built-in: cd
		else if (strcmp(job->procs->cmd, "cd") == 0) {
			int pass;
			if (job->procs->argc == 1) {
				pass = chdir(getenv("HOME"));
			}
			else {
				pass = chdir(job->procs->argv[1]);
			}

			if (pass == 0) {
				char* path = NULL;
				path = getcwd(path, 0);
				fprintf(stdout, "%s\n", path);
				free(path);
			}
			else {
				fprintf(stderr, "%s", DIR_ERR);
			}
			free_job(job);
			free(line);
			continue;
		}
		// Built-in: estatus
		else if (strcmp(job->procs->cmd, "estatus") == 0) {
			if (exit_status == -100) {
				fprintf(stdout, "-100\n");
			}
			else {
				fprintf(stdout, "%d\n", WEXITSTATUS(exit_status));
			}
			free_job(job);
			free(line);
			continue;
		}
		// Built-in: bglist
		else if (strcmp(job->procs->cmd, "bglist") == 0) {
			node_t* current = bglist->head; // Start at beginning of list
			while (current != NULL) { // Iterate until end of list is reached
				bgentry_t* bgentry = current->data;
				print_bgentry(bgentry);
				current = current->next; // Continue iteration
			}
			free_job(job);
			free(line);
			continue;
		}
		// Built-in: history
		else if (strcmp(job->procs->cmd, "history") == 0) {
			node_t* current = history->head; // Start at beginning of list
			int counter = 1;
			while (current != NULL) { // Iterate until end of list is reached
				fprintf(stdout, "%d: %s\n", counter, (char*) current->data);
				current = current->next; // Continue iteration
				counter++;
			}
			free_job(job);
			free(line);
			continue;
		}
		if (job->nproc == 1) { // One process
        // Create the child proccess
			if ((pid = fork()) < 0) {
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			if (pid == 0) {  // If zero, then it's the child process
				// Get the first command in the job list to execute
				proc_info* proc = job->procs;
				if (job->in_file != NULL) {
					if (redirection_invalid_error(job->in_file, job->out_file, proc->err_file)) { // Invalid combination
						exitOnRedirectionError(job, line);
					}
					int redirect_from = open(job->in_file, O_RDONLY, S_IRUSR);
					if (redirect_from < 0) {
						exitOnRedirectionError(job, line);
					}
					dup2(redirect_from, STDIN_FILENO);
					close(redirect_from);
				}
				if (job->out_file != NULL) {
					if (redirection_invalid_error(job->out_file, job->in_file, proc->err_file)) { // Invalid combination
						exitOnRedirectionError(job, line);
					}
					int redirect_to = open(job->out_file, O_WRONLY | O_CREAT, S_IWUSR);
					if (redirect_to < 0) {
						exitOnRedirectionError(job, line);
					}
					dup2(redirect_to, STDOUT_FILENO);
					close(redirect_to);
				}
				if (proc->err_file != NULL) {
					if (redirection_invalid_error(proc->err_file, job->in_file, job->out_file)) { // Invalid combination
						exitOnRedirectionError(job, line);
					}
					int redirect_to2 = open(proc->err_file, O_WRONLY | O_CREAT, S_IWUSR);
					if (redirect_to2 < 0) {
						exitOnRedirectionError(job, line);
					}
					dup2(redirect_to2, STDERR_FILENO);
					close(redirect_to2);
				}
				exec_result = execvp(proc->cmd, proc->argv);
				if (exec_result < 0) {  // Error checking
					handle_exec_error(proc, job, line);
				}
			}
			else {
				if (job->bg == 1) {
					insert_bgentry(bglist, job, pid, &num_bg);
				}
				else {
					// As the parent, wait for the foreground job to finish
					safe_wait(pid, &exit_status, bglist, history);
					// If a foreground job, we no longer need the data
					free_job(job);
				}
				free(line);
			}
		}
		else if (job->nproc == 2) { // 2 processes
			int p[2] = {0, 0};
			pipe(p); // Error check pipe before submission
			proc_info* proc1 = job->procs;
			proc_info* proc2 = proc1->next_proc;
			
			if ((pid1 = fork()) < 0) { // Fork once
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			if (pid1 == 0) { // First child
				close(p[0]); // Close read
				dup2(p[1], STDOUT_FILENO);
				close(p[1]);
				exec_result = execvp(proc1->cmd, proc1->argv);
				if (exec_result < 0) {  // Error checking
					handle_exec_error(proc1, job, line);
				}
				
			}
			if ((pid2 = fork()) < 0) { // Fork second time
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			if (pid2 == 0) { // Second child
				close(p[1]); // Close write
				dup2(p[0], STDIN_FILENO);
				close(p[0]);
				exec_result = execvp(proc2->cmd, proc2->argv);
				if (exec_result < 0) {  // Error checking
					handle_exec_error(proc2, job, line);
				}
			}
			if (job->bg == 1) {
				insert_bgentry(bglist, job, pid2, &num_bg);
			}
			else {
				// As the parent, wait for the foreground job to finish
				close(p[0]);
				close(p[1]);
				safe_wait(pid1, &exit_status, bglist, history);
				safe_wait(pid2, &exit_status, bglist, history);
				// If a foreground job, we no longer need the data
				free_job(job);
			}
			free(line);
		}
		else if (job->nproc == 3) { // 3 processes
			int p1[2] = {0, 0};
			pipe(p1); // Error check pipe before submission
			
			int p2[2] = {0, 0};
			pipe(p2); // Error check pipe before submission
			proc_info* proc1 = job->procs;
			proc_info* proc2 = proc1->next_proc;
			proc_info* proc3 = proc2->next_proc;

			if ((pid1 = fork()) < 0) { // Fork once
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			if (pid1 == 0) { // First child
				close(p1[0]); // Close read
				dup2(p1[1], STDOUT_FILENO);
				close(p1[1]);

				close(p2[0]);
				close(p2[1]);
				exec_result = execvp(proc1->cmd, proc1->argv);
				if (exec_result < 0) {  // Error checking
					handle_exec_error(proc1, job, line);
				}
			}
			if ((pid2 = fork()) < 0) { // Fork second time
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			if (pid2 == 0) { // Second child
				close(p1[1]); // Close write
				dup2(p1[0], STDIN_FILENO);
				close(p1[0]);

				close(p2[0]); // Close read
				dup2(p2[1], STDOUT_FILENO); 
				close(p2[1]);

				exec_result = execvp(proc2->cmd, proc2->argv);
				if (exec_result < 0) {  // Error checking
					handle_exec_error(proc2, job, line);
				}
			}
			if ((pid3 = fork()) < 0) { // Fork once
				perror("fork error");
				exit(EXIT_FAILURE);
			}
			if (pid3 == 0) { // Third child
				close(p2[1]); // Close write
				dup2(p2[0], STDIN_FILENO);
				close(p2[0]);

				close(p1[0]);
				close(p1[1]);

				exec_result = execvp(proc3->cmd, proc3->argv);
				if (exec_result < 0) {  // Error checking
					handle_exec_error(proc3, job, line);
				}
			}
			if (job->bg == 1) {
				insert_bgentry(bglist, job, pid3, &num_bg);
			}
			else {
				// As the parent, wait for the foreground job to finish
				close(p1[0]);
				close(p1[1]);
				close(p2[0]);
				close(p2[1]);
				safe_wait(pid1, &exit_status, bglist, history);
				safe_wait(pid2, &exit_status, bglist, history);
				safe_wait(pid3, &exit_status, bglist, history);
				// If a foreground job, we no longer need the data
				free_job(job);
			}
			free(line);
		}

	}
    	// Calling validate_input with NULL will free the memory it has allocated
    	validate_input(NULL);
		DestroyBG(&bglist);
		DestroyHistory(&history);

#ifndef GS
	fclose(rl_outstream);
#endif
	return 0;
}
