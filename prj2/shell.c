// Shell starter file
// You may make any changes to any part of this file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>


#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10
char history[HISTORY_DEPTH][COMMAND_LENGTH];
int arrCounter = 0;
int currentIndex = 0;
int ctrlc = 0;


/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if ( (length < 0) && (errno !=EINTR) ){
    	perror("Unable to read command. Terminating.\n");
    	exit(-1);  /* terminate with error */
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
}

void addHistory(char *tokens[], _Bool in_background) {
	int tokenNum = 0;
	int i;
	if(currentIndex == 10) {
		currentIndex = 0;
	}
	//this is to fix that weird issue where it showed:
	//'pwd tory'
	memset(history[currentIndex], 0, strlen(history[currentIndex]));

	for(i = 0; i < COMMAND_LENGTH; i++) {
		//if the token is empty, stop.
		if(tokens[tokenNum] == NULL) {
			break;
		}
		//change null terminator to a space
		//so that when printing, it doesn't end early
		if(tokens[0][i] == '\0') {
			history[currentIndex][i] = ' ';
			tokenNum++; //on to the next token
		}else{
			history[currentIndex][i] = tokens[0][i];
		}
	}

	if(in_background) {
		if(history[currentIndex][i] == '\0') {
			history[currentIndex][i-1] = ' ';
		}
		history[currentIndex][i] = '&';
	}
	currentIndex++;
	arrCounter++;
}



void printHistory() {
	if(arrCounter > 10) {
		//elements in history >9, must change numbering
		int n = arrCounter - 9; //starting num
		int index = currentIndex;

		for(int i = 0; i < 10; i++) {
			if(index == 10) {
				index = 0;
			}
			printf("%d\t", n);
			printf("%s\n", history[index]);
			n++;
			index++;
		}
	}else{
		//elements in history <= 9
		for(int i = 0; i < arrCounter; i++) {
			printf("%d\t", (i + 1));
			printf("%s\n", history[i]);
		}
	}

}


void handle_SIGINT()
{
	write(STDOUT_FILENO, "\n", strlen("\n"));
	ctrlc = 1;
	printHistory();
	//display history
}


/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];
	int status;
	char *cd;
	struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	sigaction(SIGINT, &handler, NULL);

	while (true) {
		ctrlc = 0;
		
		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		cd = getcwd(NULL, 0);
		write(STDOUT_FILENO, cd, strlen(cd));
		write(STDOUT_FILENO, "> ", strlen("> "));
		_Bool in_background = false;
		read_command(input_buffer, tokens, &in_background);

		// DEBUG: Dump out arguments:
		/*
		for (int i = 0; tokens[i] != NULL; i++) {
			write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
			write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}
		if (in_background) {
			write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
		}
			//write(STDOUT_FILENO, input_buffer, strlen(input_buffer));
		*/
		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */


		if(tokens[0] != NULL) {

			//internal commands

			if(ctrlc == 1) { // Print History
				//printHistory();
				//tokens[0] = NULL;
				continue;
			}

			// !! has to be at the top so the tokenized commands can run if they are internal commands
			if(strcmp(tokens[0], "!!") == 0) {
				//run last command
				if(arrCounter == 0) {
					write(STDOUT_FILENO, "SHELL: Unknown history command.\n", strlen("SHELL: Unknown history command.\n"));
					continue;
				}else{
					//can't directly do history[currentIndex] for some reason
					//Note: arrCounter starts at 0
					//		history holds 10 commands
					//temp = history[(arrCounter-1)%10];
					memcpy(input_buffer, history[(arrCounter-1)%10], strlen(history[(arrCounter-1)%10] + 1));
					
					write(STDOUT_FILENO, history[(arrCounter-1)%10], strlen(history[(arrCounter-1)%10]));
					write(STDOUT_FILENO, "\n", strlen("\n"));

					tokenize_command(input_buffer, tokens);
					for(int i = 0; i < COMMAND_LENGTH; i++) {
						if(history[(arrCounter-1)%10] == NULL) {
							break;
						}
						if(strchr(history[(arrCounter-1)%10], '&')) {
							in_background = true;
						}else{
							in_background = false;
						}
					}
				}
				
			}else

			if(strchr(tokens[0], '!') /*&& strcmp(tokens[0], "!!") != 0*/) {
				int n = atoi(&tokens[0][1]);
				if(n <= 0 || n > 10 || n > arrCounter) {
					write(STDOUT_FILENO, "SHELL: Unknown history command.\n", strlen("SHELL: Unknown history command.\n"));
					continue;
				}else{
					memcpy(input_buffer, history[n-1], strlen(history[n-1] + 1));
					write(STDOUT_FILENO, history[n-1], strlen(history[n-1]));
					write(STDOUT_FILENO, "\n", strlen("\n"));
					
					for(int i = 0; i < COMMAND_LENGTH; i++) {
						if(history[n-1] == NULL) {
							break;
						}
						if(strchr(history[n-1], '&')) {
							in_background = true;
						}else{
							in_background = false;
						}
					}

				}

				tokenize_command(input_buffer, tokens);

			}

			
			if(strcmp(tokens[0], "exit") == 0) { // EXIT
				return 0;
			}

			if(strcmp(tokens[0], "cd") == 0) { // CD - change directory
				if(tokens[1] == NULL) {
					//don't need to account for the command cd without an arguement
					//no need to go to home directory
				}else {
					//there is a directory as an argument
					if(chdir(tokens[1]) < 0) {
						write(STDOUT_FILENO, "Invalid directory.\n", strlen("Invalid directory.\n"));
						//write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
						//write(STDOUT_FILENO, "\n", strlen("\n"));
						
					}
					//chdir(tokens[1]);
					
				}
				addHistory(tokens, in_background);
				continue;

			}

			if(strcmp(tokens[0], "pwd") == 0) { // PWD - current working directory
				//write(STDOUT_FILENO, cd, strlen(cd));
				//won't work with write()...
				//addHistory(tokens);
				//printf("%s\n", cd);
				write(STDOUT_FILENO, cd, strlen(cd));
				write(STDOUT_FILENO, "\n", strlen("\n"));
				addHistory(tokens, in_background);
				continue;
			}

			if(strcmp(tokens[0], "history") == 0) { // History - Last 10 commands
				addHistory(tokens, in_background);
				printHistory();
				continue;
			}



			

			pid_t pid;
			pid = fork();
			if(pid == 0) {
				//code executed by child process

				if(execvp(tokens[0], tokens) < 0) {
					//write(STDOUT_FILENO, "Invalid Command.", strlen("Invalid Command."));
					write(STDOUT_FILENO, strcat(tokens[0], ": Unknown command.") , strlen(strcat(tokens[0], ": Unknown command.")));
					write(STDOUT_FILENO, "\n", strlen("\n"));
				}

				exit(-1);

			}else if(pid < 0) {
			//fork failed
			}else {
				//if we're not running n the background then wait for proccess
				//to finish
				if(in_background == false) {
					waitpid(pid, &status, 0);
				}
				
				
			}
			addHistory(tokens, in_background);
			while (waitpid(-1, NULL, WNOHANG) > 0)
			; // do nothing.
			
		}


	}
	return 0;
}
