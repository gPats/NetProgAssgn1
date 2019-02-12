#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CMD_LEN 1000
#define MAX_TOKENS 50


char ** get_tokens(char *str,char* delim);
int check_valid(char ** tokens);
void put_tokens(char ** tokens);

int main(){
	char ** cmd;
	char str[MAX_CMD_LEN]={};
	while (1){
		printf("$");
		scanf(" %[^\n]s", str);
		str[strlen(str)]=0;
		//printf("%s\n", str);
		cmd=get_tokens(str);
		put_tokens(cmd);
		free(cmd);
	}
}

char ** get_tokens(char *str,char* delim){

	char **ret=(char **) calloc(MAX_TOKENS, sizeof(char*));
	char * tok;
	tok=strtok(str,delim);
	int i=0;
	while (tok!=NULL){
		ret[i]=(char *) calloc(strlen(tok)+1, sizeof(char));
		if(tok[0]!=0) strcpy(ret[i], tok);
		ret[i][strlen(tok)] = 'a';
		tok=strtok(NULL,delim);
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

int main(){
	char ** cmd;
	char ** indi;
	char str[MAX_CMD_LEN]={};
	//while (1){
		printf("$");
		scanf("%[^\n]s", str);
		str[strlen(str)]=0;
		//printf("%s\n", str);
		cmd=get_tokens(str,"|");
		indi = get_tokens(cmd[0]," ");
		put_tokens(cmd);
		//put_tokens(indi);
		int ret = check_valid(cmd);
		int pid;
		//if(ret > 0) {
			pid = fork();
			if(pid == 0)execvp(indi[0],indi);
			else wait(NULL);
		//}
		free(indi);
		free(cmd);
	//}
}
