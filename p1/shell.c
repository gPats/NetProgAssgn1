#include <stdlib.h>
#include <string.h>

char ** get_tokens(char *str);
int check_valid(char ** tokens);
void put_tokens(char ** tokens);

int main(){
	char ** cmd;
	char str[100]={};
	while (1){
		printf("$");
		scanf("%[^\n]s", str);
		cmd=get_tokens(str);
		put_tokens(cmd);
	}
}

char ** get_tokens(char *str){
	char **ret
	char * tok;
	tok=strtok(str," ");
	while (tok!=NULL){

	}
}