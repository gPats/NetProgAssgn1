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
#include <stdlib.h>
#include <signal.h>
#include "msgq.h"

typedef struct grp{
	char gname[MAX_GNAME];
	int gpid;
	int projid;
}grp;

typedef struct usr{
	char uname[MAX_UNAME];
	char paswd[MAX_PWD];
	int online;
	int grpmem[GRP_PER_USR];
}usr;

grp grplist[NUM_GRP]={};
int grpcount=0;
usr usrlist[NUM_USER]={};
int usrcount=0;

int mkgrp(char *gname, int pid);
void put_req(sbuf *s);
void respond(const char msg[], int pid);
int isusr(char *uname);
void addusr(char *uname);
int getusr(char *uname);
void login();
void logout(char *uname);
int isgrp(char *gname);
void lsgrp(char *uname, int pid);
void jngrp(char *gname, char *uname, int pid);
void synch(char *uname, int pid);

int req, resp, isc;

int main(){
	key_t key1, key2, keyisc;
	
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

	keyisc=ftok(GPATH, 0);
	isc=msgget(keyisc, IPC_CREAT | 0666);

	int fd;

	printf("server ready. press ^C to stop server and delete all groups\n");
	while(1){
		if(msgrcv(req, &s, sizeof(s), 1, 0)==-1){
			printf("recieve error\n");
			return -3;
		}
		//printf("recieved. type:%d cmd:%d str:%s\n", (int)g.mtype, g.cmd, g.mtext);
		switch(s.cmd){
			case LOGIN:
				login();
				break;
			case LOGOF:
				logout(s.uname);
				break;
			case MKGRP:
				mkgrp(s.mtext, s.pid);
				break;
			case LSGRP:
				lsgrp(s.uname, s.pid);
				break;
			case JNGRP:
				jngrp(s.mtext, s.uname, s.pid);
				break;
			case SYNC:
				synch(s.uname, s.pid);
				break;

			default: respond("invalid", s.pid); break;
		}
	}
}

int mkgrp(char *gname, int pid){
	ibuf b;

	if(grpcount>=NUM_GRP){
		respond("full", pid);
		return -1;
	}
	else if(isgrp(gname)){
		respond("exists", pid);
		return -1;
	}

	respond("success", pid);
	strcpy(grplist[grpcount].gname, gname);
	
	int gpid;
	if((gpid=fork())==0){
		char str[5];
		sprintf(str, "%d", grpcount);
		execl("./group", "./group", gname, str, (char*)NULL);
	}
	grplist[grpcount].gpid=gpid;

	msgrcv(isc, &b, sizeof(b), 1, 0);
	grplist[grpcount].projid=atoi(b.mtext);

	grpcount++;
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

void respond(const char msg[], int pid){
	rbuf r={};
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
	usrlist[usrcount].paswd[0]='\0';
	usrcount++;
}

void login(){
	sbuf s;
	msgrcv(req, &s, sizeof(s), 1, 0);	//recieve username
	if(usrcount>=NUM_USER){
		respond("full", s.pid);
		return;
	}
	else if (isusr(s.uname)) respond("old user", s.pid);
	else {
		respond("new user", s.pid);
		addusr(s.uname);
	}


	int i=getusr(s.uname);
	msgrcv(req, &s, sizeof(s), 1, 0); 	//recieve password
	if(usrlist[i].paswd[0]==0){		//new user, add password
		strcpy(usrlist[i].paswd, s.mtext);
		usrlist[i].online=1;
		respond("paswd set", s.pid);
	}
	else{
		if(strcmp(usrlist[i].paswd, s.mtext)==0){
			usrlist[i].online=1;
			respond("success", s.pid);
		}
		else respond("invalid paswd", s.pid);
	}
}

int getusr(char *uname){
	int i;
	for(i=0; i<usrcount; i++)
		if(strcmp(usrlist[i].uname, uname)==0) return i;
}

void logout(char *uname){
	int i=getusr(uname);
	usrlist[i].online=0;
	//printf("%s logged out\n", uname);
}

int isgrp(char *gname){
	int ret=0;
	int i;
	for (i=0; i<grpcount; i++)
		if(strcmp(grplist[i].gname, gname)==0) ret =1;
	return ret;
}

int getgrp(char *gname){
	int i;
	for(i=0; i<grpcount; i++)
		if(strcmp(grplist[i].gname, gname)==0) return i;
}

void lsgrp(char *uname, int pid){
	char str[SMALL_TEXT];
	sprintf(str, "%d", grpcount);
	respond(str, pid);
	int i;
	int uid=getusr(uname);
	for (i=0; i<grpcount; i++){
		respond(grplist[i].gname, pid);
		if(usrlist[uid].grpmem[i]) respond("yes", pid);
		else respond("no", pid);
	}
}

void jngrp(char *gname, char *uname, int pid){
	if(!isgrp(gname)){
		respond("noexist", pid);
		return;
	}
	ibuf b;
	int i=getgrp(gname);
	int uid=getusr(uname);
	if(usrlist[uid].grpmem[i]==1){
		respond("alreadyin", pid);
		return;
	}
	char str[5];
	sprintf(str, "%d", grplist[i].projid);
	respond(str,pid);
	
	usrlist[uid].grpmem[i]=1;
	
	kill(grplist[i].gpid, SIGUSR1);
	
	b.mtype=grplist[i].gpid;
	strcpy(b.mtext, "join");
	msgsnd(isc, &b, sizeof(b), 0);

	b.mtype=grplist[i].gpid;
	strcpy(b.mtext, uname);
	msgsnd(isc, &b, sizeof(b), 0);
}

void synch(char *uname, int pid){
	int i;
	int uid=getusr(uname);
	int sum=0;
	for(i=0; i<grpcount; i++)
		sum+=usrlist[uid].grpmem[i];
	char str[5];
	sprintf(str, "%d", sum);
	respond(str, pid);
	for(i=0; i<sum; i++){
		if(usrlist[uid].grpmem[i]){
			respond(grplist[i].gname, pid);
			sprintf(str, "%d", grplist[i].projid);
			respond(str, pid);
		}
	}
}