#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXIMUM_COMMANDS_NR 15
#define MAXIMUM_ARGS_NR 10
#define MAXIMUM_COMMAND_LEN 512

struct command {
  // Array of strings to contain all arguments of a command
  char *arguments[MAXIMUM_ARGS_NR];
  int argno;
};

void parse_command(char* input, int start, int finish) {

    char symbols[MAXIMUM_COMMANDS_NR] = {"0"};
    int nr_symbols = 0;

    // Array for commands
    struct command commands[MAXIMUM_COMMANDS_NR];
    int nr_commands = 0;

    for(int i=start; i<finish; i++) {

        if(input[i] == ' ') { continue; }
        if(input[i] == '>' || input[i] == '<' || input[i] == '|') {
            symbols[nr_symbols] = input[i];
            nr_symbols++;
            continue;
        }

        int nr_arguments = 0;

        while(input[i] != '>' && input[i] != '<' && input[i] != '|' && input[i] != ' ' && i<finish) {

            int arg_start = i;

            while(input[i] != '>' && input[i] != '<' && input[i] != '|' && input[i] != ' ' && i<finish) {
                i++;
            }

            // Might be next argument
            int arg_len = i-arg_start;
            commands[nr_commands].arguments[nr_arguments] = malloc((sizeof(char)*arg_len)+sizeof(char));

            for(int j=0; j<arg_len; j++) {
                commands[nr_commands].arguments[nr_arguments][j] = input[arg_start+j];
            }

            commands[nr_commands].arguments[nr_arguments][arg_len] = 0;
            nr_arguments++;

            if(input[i] != ' ') {
                i--;
                break;
            }

            i++; // might cause trouble
        }

        if(i<=finish && (input[i] == '>' || input[i] == '<' || input[i] == '|')) { i--; }

        commands[nr_commands].arguments[nr_arguments] = 0;

        nr_commands++;
        commands[nr_commands].argno = nr_arguments;
    }

    if(nr_symbols==0 && nr_commands ==1) {
      if(fork() == 0) {
        exec(commands[0].arguments[0], commands[0].arguments);
      }
      wait(0);
      return;
    }

    int is_redir_input = 0;
    int redir_input_command_nr = 0;

    int is_redir_output = 0;
    int redir_output_command_nr = 0;

    int no_pipe = 1;

    for(int i=0; i<nr_symbols; i++) {
      if(symbols[i] == '|') {
        no_pipe = 0;
      }
      if(symbols[i] == '<') {
        is_redir_input = 1;
        redir_input_command_nr = i+1;
      }
      if(symbols[i] == '>') {
        is_redir_output = 1;
        redir_output_command_nr = i+1;
      }
    }

    // potential pipes
    int p[MAXIMUM_ARGS_NR-1][2];

    int symbol_counter = 0;
    int pipe_counter = 0;

    for(int i=0; i<nr_commands; i++) {

      if(i==0 && no_pipe) {
        if(fork() == 0) {
          if(is_redir_input) {
            close(0);
            open(commands[redir_input_command_nr].arguments[0], O_RDONLY);
          }
          if(is_redir_output) {
            close(1);
            open(commands[redir_output_command_nr].arguments[0], O_WRONLY | O_CREATE | O_TRUNC);
          }
          exec(commands[0].arguments[0], commands[0].arguments);
        }
      }

        // pipe on the left - connect read end
        // left only
        else if(symbol_counter > 0 && symbols[symbol_counter-1] == '|' && symbols[symbol_counter] != '|') {



            if(fork()==0) {
                close(0);
                dup(p[pipe_counter][0]);
                close(p[pipe_counter][0]);
                close(p[pipe_counter][1]);

                // Redir on the right
                if(symbols[symbol_counter] == '>') {
                  close(1);
                  open(commands[i+1].arguments[0], O_WRONLY | O_CREATE | O_TRUNC);
                }

                exec(commands[i].arguments[0], commands[i].arguments);
            }
            else {
              close(p[pipe_counter][0]);
              close(p[pipe_counter][1]);
            }

            pipe_counter++;
        }

        // pipe on left and right
        else if(symbol_counter > 0 && symbols[symbol_counter-1] == '|' && symbols[symbol_counter] == '|') {

          pipe(p[pipe_counter+1]);

            if(fork()==0) {

                close(0);
                close(1);

                dup(p[pipe_counter][0]);
                dup(p[pipe_counter+1][1]);
                close(p[pipe_counter][0]);
                close(p[pipe_counter][1]);
                close(p[pipe_counter+1][0]);
                close(p[pipe_counter+1][1]);
                exec(commands[i].arguments[0], commands[i].arguments);
            }

            else {
              close(p[pipe_counter][0]);
              close(p[pipe_counter][1]);
            }

            pipe_counter++;
        }

        // pipe on the right only
        // pipe on the right - create the pipe + connect write end
        else if(symbols[symbol_counter] == '|') {

            pipe(p[pipe_counter]);

            if(fork()==0) {
                close(1);
                dup(p[pipe_counter][1]);
                close(p[pipe_counter][0]);
                close(p[pipe_counter][1]);

                if(is_redir_input && i==0) {
                  close(0);
                  open(commands[redir_input_command_nr].arguments[0], O_RDONLY);
                }
                exec(commands[i].arguments[0], commands[i].arguments);
            }

        }

        symbol_counter++;

    }

    for(int i=0; i<nr_commands; i++) {
      if(wait(0) == -1) { break; }
    }
}

int main(int argc, char *argv[]) {

	while (1) {

    char *command = malloc(sizeof(char)*MAXIMUM_COMMAND_LEN);

		printf(">>> ");
    gets(command, 1024);

    int command_len = strlen(command)-1;

    if(command[0]=='c' && command[1]=='d' && command[2]==' ') {

      command[command_len] = 0;
      chdir(command+3);
      continue;
    }

    if(strcmp(command, "exit\n") == 0) { break; }

    // Split if ; used
    int last_split = 0;
    for(int i=0; i < command_len; i++) {

        if(command[i] == ';') {
            parse_command(command, last_split, i-1);
            last_split = i+1;
        }
    }

    parse_command(command, last_split, command_len);
	}

        exit(0);
}
