#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

//Source on handling jobs: https://www.gnu.org/software/libc/manual/html_node/Stopped-and-Terminated-Jobs.html
typedef struct job {
    pid_t pid;
    char **argv;  
    int status;
    struct job *next;
} job;

job *job_list = NULL; 

void add_job(pid_t pid, int status, char **input) {
    job *newjob = malloc(sizeof(job));
    newjob->pid = pid;
    newjob->status = status; //1 for active and 0 for suspended
    newjob->argv= input;
    //resetting head of linked list
    if(job_list == NULL){
        newjob->next = job_list;
        job_list = newjob;
    } else{
        newjob->next = NULL;
        job *j = job_list;
        while(j->next != NULL){
            j = j->next;
        }
        j->next = newjob;
    }
}

void print_jobs() {
    job *j = job_list;
    if(job_list == NULL){
        return;
    }
    int index = 1;
    while (j != NULL) {
        printf("[%d] ", index);
        int i = 0;
        int first = 1; 
        while (j->argv[i] != NULL) {
            if (first) {
                printf("%s", j->argv[i]);
                first = 0;
            } else {
                printf(" %s", j->argv[i]);
            }
            i++;
        }
        printf("\n");
        index++;
        j = j->next;
    } 
}

job * find_job (int index){
  job *j = job_list;
  int i = 1;
  while(j != NULL){
       if(i == index){
         return j;
       } else{
        j = j->next;
        i++;
       }
  }
    return NULL;
}


void remove_job(pid_t pid){
    job *j = job_list;
    job *last_job = NULL;
    while(j != NULL){
        if(j->pid == pid){
            if(last_job != NULL){
                last_job->next = j -> next;
            } else{
                job_list = j->next;
            }
        }
        last_job = j;
        j = j->next;
    }
}


void resume_job(int index){
    job *j = find_job(index);
    if (j == NULL) {
        fprintf(stderr, "Error: invalid job\n");
        return;
    }
    if (kill(j->pid, SIGCONT) == -1) {
        return;
    }
    j->status = 1; 
    if (waitpid(j->pid, &j->status, WUNTRACED) == -1) {
        return;
    }
    if (!WIFEXITED(j->status)) { 
        //If the process has not terminated, only stopped, we need to rearrange it in the linked list
          if (j == job_list) {
            job_list = job_list->next;
          } else{
            job *current = job_list;
            while (current->next != j) {
                current = current->next;
            }
            current->next = j->next;
          }
           j->next = NULL;
           if (job_list == NULL) {
            job_list = j;
            } else {
                job *last = job_list;
                while (last->next != NULL) {
                    last = last->next;
                }
                last->next = j;
            }
    } else {
        remove_job(j->pid);
    }
}

// Source on how to handle the getcwd() function: https://www.qnx.com/developers/docs/7.1/#com.qnx.doc.neutrino.lib_ref/topic/g/getcwd.html
void printPrompt(){
    char* currentpwd;
    char buffer[1001]; 
    currentpwd = getcwd(buffer,1001);
    if(currentpwd == NULL){
        perror("error at getcwd()");
        exit(0);
    }
    char* base = basename(currentpwd);
    if(base != NULL) {
        if(strcmp(base, "home")== 0 ||strcmp(base, "root")== 0){
            base = "/";
        }
        printf( "[nyush %s]$ ", base);
        fflush(stdout);
    }
}
//Source on how to use getline() https://opensource.com/article/22/5/safely-read-user-input-getline
char* readLine(){
    char *buffer = NULL;
    size_t buffer_length = 0;
    getline(&buffer, &buffer_length, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    return buffer;
}

//Source on how to extract tokens: https://www.educative.io/answers/splitting-a-string-using-strtok-in-c 
char** splitLine(char* input){
    char** split_words = malloc(1001 * sizeof(char*));
    char * word = strtok(input, " ");
    split_words[0] = word;
    int count = 1;
   while(word != NULL) {
      word = strtok(NULL, " ");
      split_words[count] = word;
      count++;
   }
   split_words[count] = NULL;
   return split_words;
}

   
//Source on how to use execvp: https://www.digitalocean.com/community/tutorials/execvp-function-c-plus-plus 
int exec(char* first, char ** input){
    pid_t pid;
    int run;
    //if no command, terminate
    if (first == NULL) {
        return 0;
    } 
    //Parent and Child processes 
    pid = fork();
    if(pid < 0){
        perror(0);
        exit(0);
    } else if(pid == 0){
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        if(execvp(first, input) < 0){
            fprintf(stderr, "Error: invalid command\n");
            exit(0);
        }
    } else{
        if(waitpid(pid, &run, WUNTRACED) > 0 && WIFSTOPPED(run)){
            add_job(pid, run, input);
        }
    }
    return 0;
}


void locate_program(char **input) {
    char full_path[1001];
    if (input[0][0] == '/') {
        strcpy(full_path, input[0]);
    } else if (strchr(input[0], '/') != NULL) {
        char *currentpwd;
        char buffer[1001]; 
        currentpwd = getcwd(buffer, 1001);
        if (currentpwd != NULL) {
            strcpy(full_path, currentpwd);
            strcat(full_path, "/");
            strcat(full_path, input[0]);
        } else {
            fprintf(stderr, "Error: unable to get current working directory\n");
            return;
        }
    } else {
        char *bin = "/usr/bin/";
        strcpy(full_path, bin);
        strcat(full_path, input[0]);
    }
    if (access(full_path, X_OK) == 0) {
        exec(full_path, input);
    } else {
        fprintf(stderr, "Error: invalid program\n");
        return;
    }
}

void output_redirection(char** input, char *output, int append){
    int open_out;
    int stdout_fd = dup(STDOUT_FILENO);
    if(append == 1){
        open_out = open(output, O_RDWR| O_CREAT| O_APPEND, 0644);
        if(open_out < 0){
            fprintf(stderr, "Error: open_out\n");
            return;
        }
    } else{
        open_out = open(output, O_RDWR| O_CREAT| O_TRUNC, 0644);
         if(open_out < 0){
            fprintf(stderr, "Error: open_out\n");
            return;
        }
    }
    dup2(open_out, STDOUT_FILENO); 
    pid_t pid;
    int run;

    if (input[0] == NULL) {
        return;
    } 
    pid = fork();
    if(pid < 0){
        perror(0);
        exit(0);
    } else if(pid == 0){
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        if(execvp(input[0], input) < 0){
            fprintf(stderr, "Error: invalid command\n");
            exit(0);
        }
    } else{
        waitpid(pid, &run, 0);
        close(open_out);
        dup2(stdout_fd, STDOUT_FILENO); 
        close(stdout_fd); 
    }
    return;
}

void input_redirection(char** input, char *input_file){
    int stdin = dup(STDIN_FILENO); 
    int open_in = open(input_file, O_RDONLY, 0644); 
    if(open_in < 0){
        fprintf(stderr, "Error: invalid file\n");
        return;
    }
    dup2(open_in, STDIN_FILENO); 
   
    pid_t pid;
    int run;
    //if no command, terminate
    if (input[0] == NULL) {
        return;
    } 
    //Parent and Child processes 
    pid = fork();
    if(pid < 0){
        perror(0);
        exit(0);
    } else if(pid == 0){
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        if(execvp(input[0], input) < 0){
            fprintf(stderr, "Error: invalid command\n");
            exit(0);
        }
    } else{
        waitpid(pid, &run, 0);
        dup2(stdin,STDIN_FILENO); 
        close(stdin); 
        close(open_in);
    }
    return;
}

void inputoutput_redirection(char** input, char* in_file, char* out_file, int append){
    int stdin_fd = dup(STDIN_FILENO);
    int stdout_fd = dup(STDOUT_FILENO);
    int open_in = open(in_file, O_RDONLY);
    if (open_in < 0) {
        fprintf(stderr, "Error: cannot open input file\n");
        return;
    }
    dup2(open_in, STDIN_FILENO);
    int open_out;
    if(append){
        open_out = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (open_out < 0) {
            fprintf(stderr, "Error: cannot open output file\n");
            return;
        }
    } else{
        open_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (open_out < 0) {
            fprintf(stderr, "Error: cannot open output file\n");
            return;
        }
    }
    dup2(open_out, STDOUT_FILENO);

    pid_t pid;
    int run;
    //if no command, terminate
    if (input[0] == NULL) {
        return;
    } 
    //Parent and Child processes 
    pid = fork();
    if(pid < 0){
        perror(0);
        exit(0);
    } else if(pid == 0){
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        if(execvp(input[0], input) < 0){
            fprintf(stderr, "Error: invalid command\n");
            exit(0);
        }
    } else{
        waitpid(pid, &run, 0);
        dup2(stdin_fd, STDIN_FILENO);
        dup2(stdout_fd, STDOUT_FILENO);
        close(open_in);
        close(open_out);
        close(stdin_fd);
        close(stdout_fd);
    }
    return;
}


void parse_io(char** tokens, int arg_count){
    char** io_input = malloc(1001 * sizeof(char*));
    int index = 0;
    int in = 0;
    int out = 0;
    int app = 0;
    char* in_file;
    char* out_file;
    char* app_file;
    for(int i = 0; i < arg_count; i++){
        if(!strcmp(tokens[i], "<")){
            if(i + 1 >= arg_count || in + out > 1){
                fprintf(stderr, "Error: invalid command\n");
                return;
            }
            in_file = tokens[i+1];
            in += 1;
            i+=1;
            continue;
        } else if(!strcmp(tokens[i], ">")){
            if(i + 1 >= arg_count || in + out > 1){
                fprintf(stderr, "Error: invalid command\n");
                return;
            }
            out_file = tokens[i+1];
            out += 1;
            i+=1;
            continue;
        } else if(!strcmp(tokens[i], ">>")){
            if(i + 1 >= arg_count || in + out > 1){
                fprintf(stderr, "Error: invalid command\n");
                return;
            }
            app_file = tokens[i+1];
            app += 1;
            i+=1;
            continue;
        } else{
            io_input[index++] = tokens[i];
        }
    }
    io_input[index] = NULL;
    if(in == 1 && out == 0 && app == 0){
        input_redirection(io_input, in_file);
    } else if(out == 1 && in == 0 && app == 0){
        output_redirection(io_input, out_file, 0);
    } else if(in + out == 2 && app == 0){
        inputoutput_redirection(io_input, in_file, out_file, 0);
    } else if(in == 0 && out == 0 && app == 1){
        output_redirection(io_input, app_file, 1);
    } else if(in == 1 && out == 0 && app == 1){
        inputoutput_redirection(io_input, in_file, app_file, 1);
    } else{
        locate_program(tokens);
    }
    return;
}

//Source for processing multiple pipes: https://gist.github.com/sasakiyudai/f7cb5ccc5cd85ae7f23d37748bfb1cfa
void pipes(char ***input, int input_num) {
    pid_t pid;
    int fd[2 * input_num];

    for (int i = 0; i < input_num; i++) {
        if (pipe(fd + i * 2) < 0) {
            fprintf(stderr, "Error: pipe at line 441\n");
            exit(0);
        }
    }
    int j = 0;
    while (*input != NULL) {
        pid = fork();
        if(pid < 0){
            fprintf(stderr, "Error: fork at line 440\n");
            exit(0);
        } else if (pid == 0) {
            if (*(input+1) != NULL) {
                if (dup2(fd[j + 1], 1) < 0) {
                 fprintf(stderr, "Error: fork at line 440\n");
                 exit(0);
                }
            }

            if(j != 0){
                if(dup2(fd[j - 2], 0) < 0){
                 fprintf(stderr, "Error: fork at line 440\n");
                 exit(0);
                }
            }

            for (int i = 0; i < 2 * input_num; i++) {
                close(fd[i]);
            }
            printf(*input[0]);
            if(execvp((*input)[0], *input) < 0){
                fprintf(stderr, "Error: fork at line 440\n");
                exit(0);
            }
        }
        input++;
        j += 2;
    }
    for (int i = 0; i < 2 * input_num; i++) {
        close(fd[i]);
    }
    for (int i = 0; i < input_num; i++){
        wait(NULL);
    } 
}


//Source for working with multiple pipes: https://youtu.be/NkfIUo_Qq4c 
char*** pipe_parse(char** input, int arg_count) {
    char*** new_input = malloc((arg_count + 1) * sizeof(char**));
    int index = 0;
    for (int i = 0; i <= arg_count; i++) {
        new_input[i] = malloc((arg_count + 1) * sizeof(char*));
    }
    int j = 0;
    int pipe_num = 0;
    for (int i = 0; i < arg_count; i++) {
        if (!strcmp(input[i], "|")) {
            if(i == 0){
                fprintf(stderr, "Error: invalid command\n");
                return NULL;
            }
            new_input[index][j] = NULL;
            index++;
            j = 0;
            pipe_num++;
        } else {
            new_input[index][j] = input[i];
            j++;
        }
    }
    new_input[index][j] = NULL;
    new_input[index + 1] = NULL;
    if(pipe_num == 0){
        parse_io(input, arg_count);
    } else{
        pipes(new_input,arg_count);
    }
    return new_input;
}


int main(){
    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    while(1){
        printPrompt();
        char* command_line = readLine();
        if (strlen(command_line) == 0) {
            break;
        }

        char** tokens = splitLine(command_line);
        int arg_count = 0; 

        while(tokens[arg_count] != NULL){
            arg_count++;
        }
        //Built-in commands. Handling exit.
        if(strcmp(tokens[0], "exit") == 0){
            if(tokens[1] != NULL){
                fprintf(stderr, "Error: invalid command\n"); 
                continue;
            } else if(job_list != NULL){
                fprintf(stderr, "Error: there are suspended jobs\n");
                continue;
            }
            break;
        }

        //Built-in commands. Handling cd.
        if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] == NULL || tokens[2] != NULL) {
                fprintf(stderr, "Error: invalid command\n");
                continue;
            } 
            char full_path[1001];
            if (tokens[1][0] == '/') {
                strcpy(full_path, tokens[1]);
            } else {
                char *cwd = getcwd(NULL, 0);
                strcpy(full_path, cwd);
                strcat(full_path, "/");
                strcat(full_path, tokens[1]);
            } 
            if(chdir(full_path) < 0){
                fprintf(stderr, "Error: invalid directory\n");
                continue;
            }
            continue;
        }

        if(strcmp(tokens[0], "jobs") == 0){
            if(tokens[1] != NULL){
                fprintf(stderr, "Error: invalid command\n");
                continue; 
            } else{
                print_jobs();
            }
        }

        //handling fg 
        if(strcmp(tokens[0], "fg") == 0){
            if(tokens[1] == NULL || tokens[2] != NULL){
                fprintf(stderr, "Error: invalid command\n");
                continue; 
            } else{
                resume_job(atoi(tokens[1]));
                continue;
            }
        }
       //char*** parse_pipe_arg = pipe_parse(tokens, arg_count);
        pipe_parse(tokens, arg_count);
        continue;
    }
}
