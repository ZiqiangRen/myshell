/* See Chapter 5 of Advanced UNIX Programming:  http://www.basepath.com/aup/
 *   for further related examples of systems programming.  (That home page
 *   has pointers to download this chapter free.
 *
 * Copyright (c) Gene Cooperman, 2006; May be freely copied as long as this
 *   copyright notice remains.  There is no warranty.
 */

/* To know which "includes" to ask for, do 'man' on each system call used.
 * For example, "man fork" (or "man 2 fork" or man -s 2 fork") requires:
 *   <sys/types.h> and <unistd.h>
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#define MAXLINE 200  /* This is how we declare constants in C */
#define MAXARGS 20
#define PIPE_READ 0
#define PIPE_WRITE 1
int myshell_flag = 0;
int wait_flag = 1;
int pipe_flag = 0;
int fd1;
int fd2;
char * argv2[MAXARGS];
pid_t childpid = -1;
static char * getword(char * begin, char **end_ptr) {
    char * end = begin;

    while ( *begin == ' ' )
        begin++;  /* Get rid of leading spaces. */
    end = begin;
    while ( *end != '\0' && *end != '\n' && *end != ' ' && *end!='#')
        end++;  /* Keep going. */
    if (*end == '#')
    	return NULL;
    if ( end == begin )
        return NULL;  /* if no more words, return NULL */
    *end = '\0';  /* else put string terminator at end of this word. */
    *end_ptr = end;
    if (begin[0] == '$') { /* if this is a variable to be expanded */
        begin = getenv(begin+1); /* begin+1, to skip past '$' */
	    if (begin == NULL) {
	        perror("getenv");
	        begin = "UNDEFINED";
        }
    }
    return begin; /* This word is now a null-terminated string.  return it. */
}

static void getargs(char cmd[], int *argcp, char *argv[])
{
    char *cmdp = cmd;
    char *end;
    int i = 0;
    //printf("getargs:    ");
    if (fgets(cmd, MAXLINE, stdin) == NULL && feof(stdin)) {
        if (myshell_flag == 1)
            {dup2(fd1, fileno(stdin));myshell_flag = 0;fflush(stdin);fflush(stdout);return;}
        else
        {printf("Couldn't read from standard input. End of file? Exiting ...\n");
        exit(1);}  /* any non-zero value for exit means failure. */
    }
    while ( (cmdp = getword(cmdp, &end)) != NULL ) { /* end is output param */
        argv[i++] = cmdp;
	    cmdp = end + 1;
        if (strcmp(argv[i-1],">")==0)
        {
            printf("matched '>' \n");
            fd2 = dup(fileno(stdout));
            cmdp = getword(cmdp, &end);
            freopen(cmdp, "w", stdout);
            argv[i-1] = NULL;
            *argcp = i-1;
            return;
            }
        else if (strcmp(argv[i-1],"<")==0)
        {
            printf("matched '<' \n");
            fd1 = dup(fileno(stdin));
            cmdp = getword(cmdp, &end);
            freopen(cmdp, "r", stdin);
            argv[i-1] = NULL;
            *argcp = i-1;
            return;
            }
        else if (strcmp(argv[i-1],"&")==0)
        {
            printf("matched '&' \n");
            wait_flag = 0;
            argv[i-1] = NULL;
            *argcp = i-1;
            return; 
            }
        else if(strcmp(argv[i-1], "|")==0)
        {
            printf("matched '|' \n");
            pipe_flag = 1;
            argv[i-1] = NULL;
            *argcp = i-1;
            i = 0;
            while ( (cmdp = getword(cmdp, &end)) != NULL ) 
                    {argv2[i++] = cmdp;cmdp = end + 1;}
                    printf("1st: %s ", argv2[0]);
                    printf("2nd: %s\n", argv2[1]);
                    argv2[i] = NULL;
            return;
        }
    }
    argv[i] = NULL; /* Create additional null word at end for safety. */
    *argcp = i;
    if (i>1 && strcmp(argv[0], "./myshell")==0)
    {   
        myshell_flag = 1;
        fd1 = dup(fileno(stdin));
        freopen(argv[1], "r", stdin);
    }
}

static void execute(int argc, char *argv[])
{
    //pid_t childpid; /* child process ID */
    if (pipe_flag == 1)
    {
        int pipe_fd[2]; 
        int fd;
        pid_t child1,child2;
        if (-1 == pipe(pipe_fd)) {
                  perror("pipe_fd");}
        child1 = fork();
        if(child1>0)
            child2=fork();
        if(child1 == 0)
        {
            if (-1 == close(STDOUT_FILENO)) {
                perror("close");}
            fd = dup(pipe_fd[PIPE_WRITE]);
            if (-1 == fd) {
                perror("dup");}
            if (fd != STDOUT_FILENO) {
                fprintf(stderr, "Pipe output not at STDOUT.\n");} 
            close(pipe_fd[PIPE_READ]); 
            close(pipe_fd[PIPE_WRITE]); 
            
            if (-1 == execvp(argv[0], argv)) {
                            perror("execvp");}
        }
        else if (child2 == 0){
            if (-1 == close(STDIN_FILENO)) {
                perror("close");}
            fd = dup(pipe_fd[PIPE_READ]);
            if (-1 == fd) {perror("dup");}
            if (fd != STDIN_FILENO) {
                fprintf(stderr, "Pipe input not at STDIN.\n");}
            close(pipe_fd[PIPE_READ]); 
            close(pipe_fd[PIPE_WRITE]);
            
            if (-1 == execvp(argv2[0], argv2)) {
                            perror("execvp");}
            }
        else
        {
            int status;
            close(pipe_fd[PIPE_READ]);
            close(pipe_fd[PIPE_WRITE]);
            if (-1 == waitpid(child1,&status,0)) {perror("waitpid");}
            if (WIFEXITED(status) == 0) {
                printf("child1 returned w/ error code %d\n", WEXITSTATUS(status));}
            if (-1 == waitpid(child2, &status, 0)) {perror("waitpid");}
            if (WIFEXITED(status) == 0) {printf("child2 returned w/ error code %d\n", WEXITSTATUS(status));}
            }
        pipe_flag = 0;
        return;  
    }

    else{
    childpid = fork();
    if (childpid == -1) { /* in parent (returned error) */
        perror("fork"); /* perror => print error string of last system call */
        printf("  (failed to execute command)\n");
    }
    if (childpid == 0) { /* child:  in child, childpid was set to 0 */
	if (-1 == execvp(argv[0], argv)) {
          perror("execvp");
          printf("  (couldn't find command)\n");
        }
    exit(1);
        }  /* parent:  in parent, childpid was set to pid of child process */
    else if(wait_flag == 1)  
        {waitpid(childpid, NULL, 0);  /* wait until child process finishes */
         }
    wait_flag = 1;
    childpid = -1;
    return;
    }
}
static void handler(int sig)
{   if (childpid > 0)
        {kill(childpid, SIGTERM);
        printf("\n");}
    else
        exit(0);
}
int main(int argc, char *argv[])
{
    char cmd[MAXLINE];
    char *childargv[MAXARGS];
    int childargc;    
    fd1 = dup(fileno(stdin));
    fd2 = dup(fileno(stdout));

    signal(SIGINT, handler);
	while (1) {
        dup2(fd2, fileno(stdout));
        if(myshell_flag!=1)
            dup2(fd1, fileno(stdin));
          
        printf("%% "); 
        fflush(stdout); 
	    getargs(cmd, &childargc, childargv); 	
	if ( childargc > 0 && strcmp(childargv[0], "exit") == 0 )
            exit(0);
	else if ( childargc > 0 && strcmp(childargv[0], "logout") == 0 )
            exit(0);
    else
	    execute(childargc, childargv);
    }
    /* NOT REACHED */
}
