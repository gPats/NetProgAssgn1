#include <stdio.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "msgq.h"

void put_help();

int main(int argc, const char *argv[]){
	key_t key1, key2, keyg;
	int req, resp;
	sbuf s;
	rbuf r;

	char uname[20];
	char str[10];

	if((key1=ftok(SPATH, 0))==-1){
		perror("ftok\n");
		return -1;
	}
	if((key2=ftok(SPATH, 1))==-1){
		perror("ftok\n");
		return -1;
	}

	if((req=msgget(key1,0))==-1){
		perror("msgget\n");
		return -2;
	}
	if((resp=msgget(key2,0))==-1){
		perror("msgget\n");
		return -2;
	}

	printf("Username: ");
	scanf(" %s", uname);
	s.mtype=1;
	s.cmd=UNAME;
	strcpy(s.uname, uname);
	strcpy(s.mtext, uname);
	s.pid=getpid();

	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	
	printf("Enter 'help' for list of commands.\n");

	while(1){
		fgets(str, 20, stdin);
		str[strlen(str)-1]='\0';
		if (strcmp(str, "help")==0) {
			put_help();
			continue;
		}
		if (strcmp(str, "exit")==0) break;
		if (strcmp(str, ""))
	}

	printf("%s\n", r.mtext);

	return 0;
}

void put_help(){
	printf("____List of commands____\n");
	printf("help- show this help\n");
	printf("mkgrp- make a new group\n"); 
	printf("jngrp- join a group\n");
	printf("swgrp- switch current group\n");
	printf("rcv- recieve new messages in current group\n");
	printf("rcvall- recieve all messages in current group\n");
	printf("send- send messages to current group\n");
	printf("exit- stop\n");
}