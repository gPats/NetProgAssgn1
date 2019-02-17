#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CMD_LEN 1000
#define MAX_TOKENS 50

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
}node;

void check_flags(node* nd);
char ** get_tokens(char *str);
int check_valid(char ** tokens);
void exec_command(char** tokens);
void put_tokens(char ** tokens);
void free_list(node * first);
node * get_list(char **tokens);
void put_list(node *first);
void mknode(node **here);
void firstnode(node **first, node **tail);
void free_cmd(char **cmd);
void errExit(char* str);

/*int main(){
	char ** cmd;
	char str[MAX_CMD_LEN]={};
	node * list;

	while (1){
		printf("$");
		scanf(" %[^\n]s", str);
		str[strlen(str)]=0;
		//printf("%s\n", str);
		cmd=get_tokens(str);
		put_tokens(cmd);
		free(cmd);
	}
}*/

int main(){
	char ** cmd;
	char str[MAX_CMD_LEN]={};
	node * list;

	//while (1){
		printf("$");
		scanf(" %[^\n]s", str);
		str[strlen(str)]=0;
		//printf("%s\n", str);
		cmd=get_tokens(str);
		//put_tokens(cmd);
		list=get_list(cmd);
		//put_list(list);
		exec_command(cmd);
		free_cmd(cmd);
		free_list(list);
	//}
}

void check_flags(node* nd) {
	int fd = -1,i;
	FILE* fp;
	if (nd->flag & SINGLE_BACK)
	{
		for(i = 0;i < nd->argc;i++) {
			if(strcmp((nd->argv)[i],"<") == 0) {
				fp = fopen((nd->argv)[i+1],"r");
				fd = fileno(fp);
				dup2(fd,STDIN_FILENO);
				break;
			}
		}
	}
	if(nd->flag & SINGLE_FRONT) {
		for(i = 0;i < nd->argc;i++) {
			if(strcmp((nd->argv)[i],">") == 0) {
				fp = fopen((nd->argv)[i+1],"w");
				if(fd == -1) fd = fileno(fp);
				dup2(fd,STDOUT_FILENO);
				break;
			}
		}
	}
	if(nd->flag & DOUBLE_FRONT) {
		for(i = 0;i < nd->argc;i++) {
			if(strcmp((nd->argv)[i],">>") == 0) {
				fp = fopen((nd->argv)[i+1],"a");
				if(fd == -1) fd = fileno(fp);
				dup2(fd,STDOUT_FILENO);
			}
		}
	}
}

char ** get_tokens(char *str){
	char **ret=(char **) calloc(MAX_TOKENS, sizeof(char*));
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

int check_valid(char** tokens) {
	if (strchr(tokens[0],'/') != NULL){
		if(access(tokens[0],F_OK) == 0)return 1;
	}

	char* search_path = getenv("PATH");

	if(!search_path)return -1;
		
	char* buf = (char*)malloc(strlen(tokens[0]) + strlen(search_path) + 3);
		
	while (*search_path){
		char* iter = buf;
		for (;*search_path && *search_path != ':'; search_path++,iter++)
		{
			*iter = *search_path;
		}
		if(iter == buf)*iter++ = '.';
		if(iter[-1] != '/')*iter++ = '/';
		strcpy(iter,tokens[0]);
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
void exec_command(char** tokens) {
	int pid,std_out,std_in;
	node* cmd_list = get_list(tokens);
	node* iter = cmd_list;
	node* iter_next = cmd_list->next;
	int p1[2],p2[2];
	
	std_out = dup(STDOUT_FILENO);
	std_in = dup(STDIN_FILENO);
	
	if (iter_next == NULL)
	{
		check_flags(iter);
		if ((pid = fork()) == 0)
		{
			check_flags(iter);
			if(iter->flag & FLAGS)	execlp((iter->argv)[0],(iter->argv)[0],(iter->argv)[1],NULL);
			else execlp((iter->argv)[0],(iter->argv)[0],NULL);
		}
		else wait(NULL);
	}
	else
	{
		if (pipe(p1) == -1) errExit("pipe 1");
		if (pipe(p2) == -1) errExit("pipe 2");
		
		if(iter->flag & DOUBLE_PIPE) {
			
			if ((pid = fork()) == 0)
			{
				check_flags(iter);
				close(p1[0]);
				close(p2[0]);
				if(iter->flag & FLAGS) execlp((iter->argv)[0],(iter->argv)[0],(iter->argv)[1],NULL);
				else execlp((iter->argv)[0],(iter->argv)[0],NULL);
			}
			if ((pid = fork()) == 0)
			{
				check_flags(iter->next);
				close(p1[1]);
				close(p2[0]);
				close(p2[1]);
				if(iter->next->flag & FLAGS) execlp((iter->next->argv)[0],(iter->next->argv)[0],(iter->next->argv)[1],NULL);
				else execlp((iter->next->argv)[0],(iter->next->argv)[0],NULL);
			}
			if ((pid = fork()) == 0)
			{
				check_flags(iter->next->next);
				close(p1[0]);
				close(p1[1]);
				close(p2[1]);
				if(iter->next->next->flag & FLAGS) execlp((iter->next->next->argv)[0],(iter->next->next->argv)[0],(iter->next->next->argv)[1],NULL);
				else execlp((iter->next->next->argv)[0],(iter->next->next->argv)[0],NULL);
			}
			close(p1[0]);
			close(p1[1]);
			close(p2[0]);
			close(p2[1]);
		}
		else if (iter->flag & TRIPLE_PIPE)
		{
			
		}
		else
		{
			
		}
	}
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
			(strcmp(tokens[i],"|||")!=0)
			){

			if (strcmp(tokens[i], ",") == 0) break;

			tail->argc=tail->argc+1;
			tail->argv=realloc(tail->argv, (tail->argc)*sizeof(char*));
			tail->argv[tail->argc-1]=calloc(strlen(tokens[i])+1, sizeof(char));
			strcpy(tail->argv[tail->argc-1], tokens[i]);
			
			if(strchr(tokens[i],'-') != NULL) tail->flag |= FLAGS;
			if (strcmp(tokens[i],"<") == 0) tail->flag |= SINGLE_BACK;
			if (strcmp(tokens[i],">") == 0) tail->flag |= SINGLE_FRONT;
			if (strcmp(tokens[i],"<<") == 0) tail->flag |= DOUBLE_BACK;
			if (strcmp(tokens[i],">>") == 0) tail->flag |= DOUBLE_FRONT;
			if (strcmp(tokens[i],"&") == 0) tail->flag |= AMPERSAND;
			
			if (tokens[i+1]==NULL) break;
			i++;

		}
		if (strcmp(tokens[i],"|") == 0) tail->flag |= SINGLE_PIPE;
		if (strcmp(tokens[i],"||") == 0) {tail->flag |= DOUBLE_PIPE; flag=2;}
		if (strcmp(tokens[i],"|||") == 0) {tail->flag |= TRIPLE_PIPE; flag=3;}
		
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

		first=first->next;
	}
}

void firstnode(node **first, node **tail){
	*tail=(node *) calloc(1, sizeof(node));
	*first=*tail;
}

void free_list(node *first){
	int i;
	node *temp;
	while(first!=NULL){
		for (i=0; i<first->argc; i++)
			free(first->argv[i]);
		free(first->argv);
		temp=first->next;
		free(first);
		first=temp;
	}

	/*if (first -> next != NULL) free_list(first->next);

	for (i=0; i<first->argc; i++)
		free(first->argv[i]);

	free(first->argv);
	free(first);*/
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
