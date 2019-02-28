#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CMD_LEN 1000
#define MAX_TOKENS 50
#define MAX_INDEX 10

//flags
#define FLAGS 1
#define SINGLE_BACK 2
#define DOUBLE_BACK 4
#define SINGLE_FRONT 8
#define DOUBLE_FRONT 16
#define SINGLE_PIPE 32
#define DOUBLE_PIPE 64
#define TRIPLE_PIPE 128
#define AMPERSAND 256

typedef struct node{
	struct node * next;
	char ** argv;
	int argc;
	int flag;
	char *infile;
	char *outfile;
	int ampersand;
}node;

void check_flags(node* nd);
node table[MAX_INDEX]={};
int scflag=0;
//int intflag=0;

int scanline(char *str);
char ** get_tokens(char *str);
int check_valid(char * tokens);
void exec_stmt(node *cmd_list);
node * get_list(char **tokens);
void mknode(node **here);
void firstnode(node **first, node **tail);
void errExit(char* str);
void shortcut(node * list);
void int_handler(int sig);
void add_entry(int index, node *list);
void free_cmd(char **cmd);
void free_list(node * first);
void put_list(node *first);
void put_tokens(char ** tokens);

/*int main(){
	char ** cmd;
	char str[MAX_CMD_LEN]={};
	node * list;

	while (1){
		printf("$");
		scanf(" %[^\n]s", str);
		str[strlen(str)]=0;
		cmd=get_tokens(str);
		list=get_list(cmd);
		free_list(list);
		free(cmd);
	}
}*/

int main(){
	char ** cmd;
	char str[MAX_CMD_LEN]={};
	node * list;

	while (1){
		printf("$");
		scanline(str);
		str[strlen(str)-1]=0;

		//if (strlen(str)==0) continue;
		
		cmd=get_tokens(str);
		if(cmd[0]==NULL) {
			free_cmd(cmd);
			continue;
		};



		list=get_list(cmd);
		if(strcmp(cmd[0], "sc")==0){
			shortcut(list);
			free_cmd(cmd);
			free_list(list);
			continue;
		}
		exec_stmt(list);
		//put_list(list);
		
		free_cmd(cmd);
		free_list(list);
	}

	printf("EXITING...\n");
}

void check_flags(node* nd) {
	int fd = -1,i;
	FILE* fp;
	if (nd->flag & SINGLE_BACK)
	{
		fp = fopen(nd->infile,"r");
		fd = fileno(fp);
		dup2(fd,STDIN_FILENO);
	}
	if(nd->flag & SINGLE_FRONT) {
		fp = fopen(nd->outfile,"w");
		fd = fileno(fp);
		dup2(fd,STDOUT_FILENO);
	}
	if(nd->flag & DOUBLE_FRONT) {
		fp = fopen(nd->outfile,"a");
		fd = fileno(fp);
		dup2(fd,STDOUT_FILENO);
	}
}

char ** get_tokens(char *str){
	char **ret=(char **) calloc(MAX_TOKENS, sizeof(char*));
	
	if(str[0]==0) return ret;

	char * tok;
	tok=strtok(str," ");
	int i=0;
	while (tok!=NULL){
		ret[i]=(char *) calloc(strlen(tok)+1, sizeof(char));
		if(tok[0]!=0) strcpy(ret[i], tok);
		//ret[i][strlen(tok)] = 'a';
		tok=strtok(NULL," ");
		i++;
	}
	return ret;
}

void put_tokens(char ** tokens){
	int i=0;
	while (tokens[i]!=NULL){
		printf("%s\n", tokens[i]);
		i++;
	}
}

int check_valid(char* tokens) {
	if (strchr(tokens,'/') != NULL){
		if(access(tokens,F_OK) == 0)return 1;
	}

	char* search_path = getenv("PATH");

	if(!search_path)return -1;
		
	char* buf = (char*)malloc(strlen(tokens) + strlen(search_path) + 3);
		
	while (*search_path){
		char* iter = buf;
		for (;*search_path && *search_path != ':'; search_path++,iter++)
		{
			*iter = *search_path;
		}
		if(iter == buf)*iter++ = '.';
		if(iter[-1] != '/')*iter++ = '/';
		strcpy(iter,tokens);
		if(access(buf,F_OK) == 0) {
			free(buf);
			return 1;
		}
		if(!*search_path)break;
		search_path++; 
	}
	free(buf);
	return -1;
}

//execs and completes requirement of one command
void exec_stmt(node *cmd_list) {
	int pid,std_out,std_in;
	//node* cmd_list = get_list(tokens);
	int status;
	node* iter = cmd_list;
	node* iter_next = cmd_list->next;
	node* prev_iter = NULL;
	int p[2][2];
	int tpipe[3][2];
	
	std_out = dup(STDOUT_FILENO);
	std_in = dup(STDIN_FILENO);
	
	if (iter_next == NULL)
	{
		if ((pid = fork()) == 0)
		{
			if(check_valid(iter->argv[0]) == -1)
			{
				errExit("program not in PATH");
			}
			check_flags(iter);
			setpgid(0,0);
			execvp((iter->argv)[0],iter->argv);
		}
		signal(SIGTTOU,SIG_IGN);
		if(!(iter->flag & AMPERSAND)) {
			tcsetpgrp(0,pid);
			wait(&status);
			tcsetpgrp(0,getpid());
		}
		printf("pid: %d\nstatus: %d\n",pid,status);
	}
	else
	{
		
		if(iter->flag & DOUBLE_PIPE) {
			
			if (pipe(p[0]) == -1) errExit("pipe 1");
			if (pipe(p[1]) == -1) errExit("pipe 2");
			
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter);
				setpgid(0,0);
				close(p[0][0]);
				close(p[1][0]);
				close(p[1][1]);
				dup2(p[0][1],STDOUT_FILENO);
				execvp((iter->argv)[0],iter->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter);
				setpgid(0,0);
				close(p[0][0]);
				close(p[1][0]);
				close(p[0][1]);
				dup2(p[1][1],STDOUT_FILENO);
				execvp((iter->argv)[0],iter->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter->next);
				setpgid(0,0);
				close(p[0][1]);
				close(p[1][0]);
				close(p[1][1]);
				dup2(p[0][0],STDIN_FILENO);
				execvp((iter->next->argv)[0],iter->next->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter->next->next);
				setpgid(0,0);
				close(p[0][0]);
				close(p[0][1]);
				close(p[1][1]);
				dup2(p[1][0],STDIN_FILENO);
				execvp((iter->next->next->argv)[0],iter->next->next->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			close(p[0][0]);
			close(p[0][1]);
			close(p[1][0]);
			close(p[1][1]);
		}
		else if (iter->flag & TRIPLE_PIPE)
		{
			if (pipe(tpipe[0]) == -1) errExit("pipe 1");
			if (pipe(tpipe[1]) == -1) errExit("pipe 2");
			if (pipe(tpipe[2]) == -1) errExit("pipe 3");
			
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter);
				setpgid(0,0);
				close(tpipe[0][0]);
				close(tpipe[1][0]);
				close(tpipe[1][1]);
				close(tpipe[2][0]);
				close(tpipe[2][1]);
				dup2(tpipe[0][1],STDOUT_FILENO);
				execvp((iter->argv)[0],iter->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter);
				setpgid(0,0);
				close(tpipe[0][0]);
				close(tpipe[0][1]);
				close(tpipe[1][0]);
				close(tpipe[2][0]);
				close(tpipe[2][1]);
				dup2(tpipe[1][1],STDOUT_FILENO);
				execvp((iter->argv)[0],iter->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter);
				setpgid(0,0);
				close(tpipe[0][0]);
				close(tpipe[0][1]);
				close(tpipe[1][0]);
				close(tpipe[1][1]);
				close(tpipe[2][0]);
				dup2(tpipe[2][1],STDOUT_FILENO);
				execvp((iter->argv)[0],iter->argv);				
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter->next);
				setpgid(0,0);
				close(tpipe[0][1]);
				close(tpipe[1][0]);
				close(tpipe[1][1]);
				close(tpipe[2][0]);
				close(tpipe[2][1]);
				dup2(tpipe[0][0],STDIN_FILENO);
				execvp((iter->next->argv)[0],iter->next->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter->next->next);
				setpgid(0,0);
				close(tpipe[0][0]);
				close(tpipe[0][1]);
				close(tpipe[1][1]);
				close(tpipe[2][0]);
				close(tpipe[2][1]);
				dup2(tpipe[1][0],STDIN_FILENO);
				execvp((iter->next->next->argv)[0],iter->next->next->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			if ((pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter->next->next->next);
				setpgid(0,0);
				close(tpipe[0][0]);
				close(tpipe[0][1]);
				close(tpipe[1][0]);
				close(tpipe[1][1]);
				close(tpipe[2][1]);
				dup2(tpipe[2][0],STDIN_FILENO);
				execvp((iter->next->next->next->argv)[0],iter->next->next->next->argv);
			}
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
		close(tpipe[0][0]);
		close(tpipe[0][1]);
		close(tpipe[1][0]);
		close(tpipe[1][1]);
		close(tpipe[2][0]);
		close(tpipe[2][1]);
		}
		else
		{
			int cpipe = 0;
			if (pipe(p[0]) == -1) errExit("pipe 1");
			if (pipe(p[1]) == -1) errExit("pipe 2");
			
			if (iter && (pid = fork()) == 0)
			{
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				check_flags(iter);
				setpgid(0,0);
				close(p[0][0]);
				close(p[1][1]);
				if(!prev_iter)close(p[1][0]);
				else if(prev_iter->flag & SINGLE_PIPE)dup2(p[1][0],STDIN_FILENO);
				
				if(iter->flag & SINGLE_PIPE)dup2(p[0][1],STDOUT_FILENO);
				else close(p[0][1]);
				
				execvp((iter->argv)[0],iter->argv);
			}
				waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
		
			if(iter->next && (pid = fork()) == 0)
			{		
				if(check_valid(iter->argv[0]) == -1)
				{
					errExit("program not in PATH");
				}
				
				if(!(iter->next)) check_flags(iter);
				else if(iter->flag & (SINGLE_FRONT | DOUBLE_FRONT | SINGLE_BACK)) errExit("invalid syntax.\n redirection only allowed in first or last command\n");
				
				setpgid(0,0);
				close(p[0][1]);
				close(p[1][0]);
				
				dup2(p[0][0],STDIN_FILENO);
				
				if(iter->next->flag & SINGLE_PIPE)dup2(p[1][1],STDOUT_FILENO);
				else close(p[1][1]);
				
				execvp((iter->next->argv)[0],iter->next->argv);
			}
			
			waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
			
			prev_iter = iter->next;
			iter = iter->next->next;
			
			close(p[cpipe][0]);
			close(p[cpipe][1]);
			
			cpipe = (cpipe == 0)?1:0;
			wait(NULL);
			
			while (iter)
			{	
				if(pipe(p[1-cpipe]) == -1)errExit("pipe");
				
				if ((pid = fork()) == 0)
				{
					if(!(iter->next)) check_flags(iter);
					else if(iter->flag & (SINGLE_FRONT | DOUBLE_FRONT | SINGLE_BACK)) errExit("invalid syntax.\n redirection only allowed in first or last command\n");
					setpgid(0,0);
					if(close(p[cpipe][1]) == -1)errExit("close");
					if(close(p[1-cpipe][0]) == -1)errExit("close");
					
					dup2(p[cpipe][0],STDIN_FILENO);
					
					if (iter->next && iter->flag & SINGLE_PIPE)
					{
						dup2(p[1-cpipe][1],STDOUT_FILENO);
					}
					else {
						if(close(p[1-cpipe][1]) == -1) 
							errExit("close"); 
						check_flags(iter);
					}
					if(execvp((iter->argv)[0],iter->argv) == -1)errExit("execv");
				}
				waitpid(-1,&status,WNOHANG);
		printf("pid: %d\nstatus: %d\n",pid,status);
		
				close(p[cpipe][0]);
				close(p[cpipe][1]);
				cpipe = (cpipe == 0)?1:0;
				iter = iter->next;
				wait(NULL);
			}
		}
	}
	wait(&status);
	dup2(std_out,STDOUT_FILENO);
	dup2(std_in,STDIN_FILENO);
}

//reads array of tokens to make a linked list of commands
node * get_list(char **tokens){
	int flag=0;
	node *ret=NULL;
	node *tail=NULL;
	int i=0;
	while (tokens[i]!=NULL){
		if (i==0) firstnode(&ret, &tail);
		else mknode(&tail);
		while(
			(strcmp(tokens[i],"|")  !=0) &&
			(strcmp(tokens[i],"||") !=0) &&
			(strcmp(tokens[i],"|||")!=0) &&
			(strcmp(tokens[i],"&") !=0)){

			if (strcmp(tokens[i], ",") == 0) break;

			if (strcmp(tokens[i],"<") == 0){
				tail->flag |= SINGLE_BACK;
				tail->infile = (char *)calloc(strlen(tokens[i+1])+1, sizeof(char));
				strcpy(tail->infile, tokens[i+1]);
				i++;
				if (tokens[i+1]==NULL) break;
				i++;
				continue;
			}

			if (strcmp(tokens[i],">") == 0){
				tail->flag |= SINGLE_FRONT;
				tail->outfile = (char *)calloc(strlen(tokens[i+1])+1, sizeof(char));
				strcpy(tail->outfile, tokens[i+1]);
				i++;
				if (tokens[i+1]==NULL) break;
				i++;
				continue;
			}

			if (strcmp(tokens[i],">>") == 0){
				tail->flag |= DOUBLE_FRONT;
				tail->outfile = (char *)calloc(strlen(tokens[i+1])+1, sizeof(char));
				strcpy(tail->outfile, tokens[i+1]);
				i++;
				if (tokens[i+1]==NULL) break;
				i++;
				continue;
			}

			tail->argc=tail->argc+1;
			tail->argv=realloc(tail->argv, (tail->argc+1)*sizeof(char*));
			tail->argv[tail->argc-1]=calloc(strlen(tokens[i])+1, sizeof(char));
			strcpy(tail->argv[tail->argc-1], tokens[i]);
			
			if (strchr(tokens[i],'-') != NULL) tail->flag |= FLAGS;
			if (strcmp(tokens[i],"<") == 0) tail->flag |= SINGLE_BACK;
			if (strcmp(tokens[i],">") == 0) tail->flag |= SINGLE_FRONT;
			if (strcmp(tokens[i],"<<") == 0) tail->flag |= DOUBLE_BACK;
			if (strcmp(tokens[i],">>") == 0) tail->flag |= DOUBLE_FRONT;
			if (strcmp(tokens[i],"&") == 0) tail->flag |= AMPERSAND;
			
			if (tokens[i+1]==NULL) break;
			i++;

		}

		tail->argv[tail->argc]=0;

		//tail->argv=realloc(tail->argv, (tail->argc+1)*sizeof(char*));
		//tail->argv[tail->argc]=calloc(1,sizeof(char));
		//if (strcmp(tokens[i],"<") == 0) tail->flag |= SINGLE_BACK;
		//if (strcmp(tokens[i],">") == 0) tail->flag |= SINGLE_FRONT;
		if (strcmp(tokens[i],"<<") == 0) tail->flag |= DOUBLE_BACK;
		//if (strcmp(tokens[i],">>") == 0) tail->flag |= DOUBLE_FRONT;
		if (strcmp(tokens[i],"|") == 0) tail->flag |= SINGLE_PIPE;
		if (strcmp(tokens[i],"||") == 0) {tail->flag |= DOUBLE_PIPE; flag=2;}
		if (strcmp(tokens[i],"|||") == 0) {tail->flag |= TRIPLE_PIPE; flag=3;}
		if (strcmp(tokens[i],"&") == 0) tail->flag |= AMPERSAND;

		i++;
	}

	return ret;
}

//creates a null initiated node
void mknode(node **here){
	node *temp = (node *) calloc(1, sizeof(node));
	(*here)->next = temp;
	*here = temp;
}

//prints list of commands in reverse order
void put_list(node *first){
	int i;
	while(first!=NULL)
	{
		printf("____Command____\n");
		printf("argc: %d\n", first->argc);
		for (i=0; i<first->argc; i++){
			printf("arg: %s\n", first->argv[i]);
		}
		printf("flag: %d\n", first->flag);

		printf("infile: %s\n", first->infile);
		printf("outfile: %s\n", first->outfile);

		first=first->next;
	}
}

void firstnode(node **first, node **tail){
	*tail=(node *) calloc(1, sizeof(node));
	*first=*tail;
}

void free_list(node *first){
	int i;
	node *temp=first;
	while (first!=NULL){
		for(i=0; i<first->argc; i++)
			free(first->argv[i]);
		free(first->argv);
		temp=first;
		first=first->next;
		free(temp);
	}
}

void free_cmd(char **cmd){
	int i=0;
	while (cmd[i]!=NULL){
		free(cmd[i]);
		i++;
	}
	
	free(cmd);
}


void errExit(char* str) {
	perror(str);
	exit(0);
}

void shortcut(node *list){
	if (list->argc == 1){
		signal(SIGINT, int_handler);
		scflag=1;
		printf("STARTING SC MODE. PRESS CTRL-C TO USE SHORTCUT COMMANDS...\n");
		return;
	}

	char *endptr;
	int index=(int) strtol(list->argv[2], &endptr, 0);

	int tableflag=0;

	if(strcmp(list->argv[1],"-i")==0) tableflag=1;
	if(strcmp(list->argv[1],"-d")==0) tableflag=2;

	if((list->argc<4)||tableflag==0||(endptr==list->argv[2])){
		printf("Shortcut usage: sc -i/-d <index> <command>\n");
		return;
	}

	if(index<0 || index>MAX_INDEX-1){
		printf("Index must be between 0 and %d (both included)\n", MAX_INDEX-1);
		return;
	}

	int flag=0;
	int contflag=0;
	char c;

	if(tableflag==2){
		int i;
		for (i=0; i<table[index].argc; i++){
			free(table[index].argv[i]);
		}
		table[index].argc=0;
	}

	if(table[index].argc!=0){
		printf("Index already in use by the following command:\n");
		put_list(&table[index]);
		printf("Are you sure you want to continue? (y or n)");
		while(flag==0){
			c=fgetc(stdin);
			switch(c){
				case 'y':
				case 'Y':
					contflag=1;
				case 'n':
				case 'N':
					flag=1;
					break;
				default :
					printf("retry:");
					break;
			}
		}

		if(contflag==1)
			add_entry(index, list);

		return;
	}

	add_entry(index, list);
}

void int_handler(int sig){
	printf(":");
	int flag=0;
	int c;
	while(flag==0){
		scanf(" %d", &c);
		if(c>=0 && c<MAX_INDEX){
			if(table[c].argc!=0)
				flag=1;
			else{
				printf("Table not initialised at Index=%d\n", c);
				printf("Retry:");
			}
		}
		else{
			printf("Index must be between 0 and %d (both included)\n", MAX_INDEX-1);
			printf("Retry:");
		}
	}
	
	exec_stmt(&table[c]);
	printf("Press ENTER to continue.\n");
}

//Do not call this anywhere else except in shortcut
void add_entry(int index, node * list){
	table[index].argc=list->argc-3;
	table[index].flag=list->flag;
	table[index].argv=(char**) calloc((list->argc-2), sizeof(char*));
	int i;
	for (i=0; i<list->argc-3; i++){
		table[index].argv[i]=(char*) calloc((strlen(list->argv[i+3])+1), sizeof(char));
		strcpy(table[index].argv[i], list->argv[i+3]);
	}
	table[index].argv[i]=0;
	table[index].next=NULL;
}

int scanline(char *str){
	char c=0;
	int i=0;
	while(c!='\n'){
		c=fgetc(stdin);
		str[i]=c;
		i++;
	}
	str[i]=0;
	return i;
}

