#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_CMD_LEN 1000
#define MAX_TOKENS 50

char ** get_tokens(char *str);
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

char ** get_tokens(char *str){
	char **ret=(char **) calloc(MAX_TOKENS, sizeof(char*));
	char * tok;
	tok=strtok(str," ");
	int i=0;
	while (tok!=NULL){
		ret[i]=(char *) calloc(strlen(tok)+1, sizeof(char));
		if(tok[0]!=0) strcpy(ret[i], tok);
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