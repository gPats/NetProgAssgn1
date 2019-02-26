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

typedef struct grp{
	char gname[20];
	int gpid;
	int gkey;
	struct grp *next;
}grp;

typedef struct usr{
	char uname[20];
	grp *ugrp;
}usr;

grp *grplist=0;
int grpcount=0;
usr usrlist[10]={};
int usrcount=0;

int mkgrp(sbuf *s);
void put_req(sbuf *s);
void respond(char *msg, int pid);
int isusr(char *uname);
void addusr(char *uname);

int req, resp;

int main(){
	key_t key1, key2;
	
	sbuf s;

	if((key1=ftok(SPATH, 0))==-1){
		perror("ftok\n");
		return -1;
	}
	if((key2=ftok(SPATH, 1))==-1){
		perror("ftok\n");
		return -2;
	}

	if((req=msgget(key1,IPC_CREAT | 0666))==-1){
		perror("msgget\n");
		return -2;
	}
	if((resp=msgget(key2,IPC_CREAT | 0666))==-1){
		perror("msgget\n");
		return -2;
	}

	int fd;

	printf("server ready. press ^C to stop server and delete all groups\n");
	while(1){
		if(msgrcv(req, &s, sizeof(s), 1, 0)==-1){
			printf("recieve error\n");
			return -3;
		}
		//printf("recieved. type:%d cmd:%d str:%s\n", (int)g.mtype, g.cmd, g.mtext);
		switch(s.cmd){
			case UNAME: 
				if(!isusr(s.uname)) {
					addusr(s.uname);
					respond("new user", s.pid);
				}
				else respond("exists", s.pid);
				break;
			default: respond("invalid", s.pid); break;
		}
	}
}

int mkgrp(sbuf *s){
	return 0;
}

void put_req(sbuf *s){
	printf("____Request____\n");
	printf("type: %ld\n", s->mtype);
	printf("uname: %s\n", s->uname);
	printf("gnum: %d\n", s->gnum);
	printf("pid: %d\n", s->pid);
	printf("text: %s\n", s->mtext);
}

void respond(char *msg, int pid){
	rbuf r;
	r.mtype=pid;
	strcpy(r.mtext, msg);
	msgsnd(resp, &r, sizeof(r), 0);
}

int isusr(char *uname){
	int ret=0;
	for(int i=0; i<usrcount; i++)
		if (strcmp(uname, usrlist[i].uname)==0) ret=1;
	return ret;
}

void addusr(char *uname){
	strcpy(usrlist[usrcount].uname, uname);
	usrlist[usrcount].ugrp=NULL;
	usrcount++;
}