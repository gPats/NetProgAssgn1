#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CMD_LEN 1000
#define MAX_TOKENS 50

//flags
#define SINGLE_BACK 1
#define DOUBLE_BACK 2
#define SINGLE_FRONT 4
#define DOUBLE_FRONT 8
#define SINGLE_PIPE 16
#define DOUBLE_PIPE 32
#define TRIPLE_PIPE 64
#define AMPERSAND 128

typedef struct node{
	struct node * next;
	char ** argv;
	int argc;
	int flag;
}node;

char ** get_tokens(char *str);
int check_valid(char ** tokens);
void put_tokens(char ** tokens);
void free_list(node * first);
node * get_list(char **tokens);
void put_list(node *first);
void mknode(node **here);


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
		put_tokens(cmd);
		list=get_list(cmd);
		put_list(list);
		free(cmd);
	//}
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
int exec_command() {


}

//reads array of tokens to make a linked list of commands
node * get_list(char **tokens){
	node *ret=NULL;
	int i=0;
	while (tokens[i]!=NULL){
		mknode(&ret);
		while(
			(strcmp(tokens[i],"<")  !=0) &&
			(strcmp(tokens[i],"<<") !=0) &&
			(strcmp(tokens[i],">")  !=0) &&
			(strcmp(tokens[i],">>") !=0) &&
			(strcmp(tokens[i],"|")  !=0) &&
			(strcmp(tokens[i],"||") !=0) &&
			(strcmp(tokens[i],"|||")!=0) &&
			(strcmp(tokens[i],"&")  !=0) 
			){

			ret->argc=ret->argc+1;
			ret->argv=realloc(ret->argv, (ret->argc)*sizeof(char*));
			ret->argv[ret->argc-1]=calloc(strlen(tokens[i])+1, sizeof(char));
			strcpy(ret->argv[ret->argc-1], tokens[i]);
			
			if (tokens[i+1]==NULL) break;
			i++;

		}
		if (strcmp(tokens[i],"<") == 0) ret->flag |= SINGLE_BACK;
		if (strcmp(tokens[i],">") == 0) ret->flag |= SINGLE_FRONT;
		if (strcmp(tokens[i],"<<") == 0) ret->flag |= DOUBLE_BACK;
		if (strcmp(tokens[i],">>") == 0) ret->flag |= DOUBLE_FRONT;
		if (strcmp(tokens[i],"|") == 0) ret->flag |= SINGLE_PIPE;
		if (strcmp(tokens[i],"||") == 0) ret->flag |= DOUBLE_PIPE;
		if (strcmp(tokens[i],"|||") == 0) ret->flag |= TRIPLE_PIPE;
		if (strcmp(tokens[i],"&") == 0) ret->flag |= AMPERSAND;
		
		i++;
	}

	return ret;
}

//creates a null initiated node
void mknode(node **here){
	node *temp = (node *) calloc(1, sizeof(node));
	temp->next=*here;
	*here=temp;
}


void put_list(node *first){
	int i;
	if (first->next != NULL){
		put_list(first->next);
	}

	printf("____Command____\n");
	printf("argc: %d\n", first->argc);
	for (i=0; i<first->argc; i++){
		printf("arg: %s\n", first->argv[i]);
	}
	printf("flag: %d\n", first->flag);

}
