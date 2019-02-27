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

void put_help();
void login();
void mkgrp();
void logout();
void lsgrp();
void jngrp();
void swgrp();
int isgrp(char *gname);
int getgrp(char *gname);
void sync();
void send();
void recv();
void putmsg(mbuf *m);

int req, resp, mes;
sbuf s;

typedef struct node{
	struct node * next;
	int gqid;
	mbuf m;
}node;

typedef struct usr{
	char uname[MAX_UNAME];
	int online;
	int grpmem[GRP_PER_USR];
	int grpcount;
	int curmsgq;
	int gselflag;
	int offmsgcount;
}usr;

typedef struct grp{
	char gname[GRP_PER_USR];
	int projid;
	int gqid;
	int gkey;
}grp;

grp mgrp[GRP_PER_USR]={};
node *msgs=NULL;
usr me={};

node *head=NULL;
node *tail=NULL;

int main(int argc, const char *argv[]){
	key_t key1, key2;
	rbuf r;
	char str[SMALL_TEXT];

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
	fgets(me.uname, MAX_UNAME, stdin);
	me.uname[strlen(me.uname)-1]=0;
	s.mtype=1;
	s.cmd=UNAME;
	s.pid=getpid();
	strcpy(s.uname, me.uname);
	
	//msgrcv(resp, &r, sizeof(r), getpid(), 0);
	
	printf("Enter 'help' for list of commands.\n");

	while(1){
		printf("$ ");
		fgets(str, SMALL_TEXT, stdin);
		str[strlen(str)-1]='\0';
		if (strcmp(str, "help")==0) {
			put_help();
			continue;
		}
		if (strcmp(str, "exit")==0) {
			logout();
			break;
		}
		if (strcmp(str, "login")==0){
			if(me.online){
				printf("Already online\n");
				continue;
			}
			login();
			sync();
			//send_old(); 										TO DO
			continue;
		}
		if(strcmp(str, "logoff")==0){
			if(!me.online){
				printf("Only valid when online\n");
				continue;
			}
			logout();
		}
		if(strcmp(str, "mkgrp")==0){
			if(!me.online){
				printf("Only valid when online\n");
				continue;
			}
			mkgrp();
			continue;
		}
		if(strcmp(str, "lsgrp")==0){
			if(!me.online){
				printf("Only valid in online mode. Try mygrp.\n");
				continue;
			}
			lsgrp();
			continue;
		}
		if(strcmp(str, "jngrp")==0){
			if(!me.online){
				printf("Only valid in online mode.\n");
				continue;
			}
			jngrp();
			continue;
		}
		if(strcmp(str, "swgrp")==0){
			swgrp();
			continue;
		}
		if(strcmp(str, "send")==0){
			if(!me.gselflag){
				printf("Select a group first. Try swgrp.\n");
				continue;
			}
			send();
			continue;
		}
		if(strcmp(str, "read")==0){
			if(!me.gselflag){
				printf("Select a group first. Try swgrp.\n");
				continue;
			}
			if(!me.online){
				printf("Only valid in online mode\n");
				continue;
			}
			recv();
			continue;
		}

	}

	//printf("%s\n", r.mtext);

	return 0;
}

void put_help(){
	printf("____List of commands____\n");
	printf("help- show this help\n");
	printf("mkgrp- make a new group\n"); 
	printf("lsgrp- list all groups\n");
	printf("jngrp- join a group\n");
	printf("swgrp- switch current group\n");
	printf("rcv- recieve new messages in current group\n");
	printf("rcvall- recieve all messages in current group\n");
	printf("send- send messages to current group\n");
	printf("login- enter passwd to login to server\n");
	printf("logoff- log out from server and enter offline mode\n");
	printf("exit- stop\n");
}

void login(){
	rbuf r;
	s.cmd=LOGIN;
	msgsnd(req, &s, sizeof(s), 0);
	s.cmd=UNAME;
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	printf("%s\n", r.mtext);
	if(strcmp(r.mtext, "full")==0){
		printf("ABORTING... TOO MANY USERS\n");
		raise(SIGTERM);
	}
	char pwd[MAX_PWD];
	printf("passwd: ");
	fgets(pwd, MAX_PWD, stdin);
	s.cmd=PASWD;
	pwd[strlen(pwd)-1]='\0';
	strcpy(s.mtext, pwd);
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	printf("%s\n", r.mtext);
	if(strcmp(r.mtext, "success")==0 || strcmp(r.mtext, "paswd set")==0) {
		me.online = 1;
		printf("ONLINE...\n");
	}
}

void logout(){
	rbuf r;
	s.cmd=LOGOF;
	msgsnd(req, &s, sizeof(s), 0);
	me.online=0;
	printf("OFFLINE...\n");
}

void mkgrp(){
	rbuf r;
	s.mtype=1;
	s.cmd=MKGRP;
	char gname[MAX_GNAME];
	printf("Group name: ");
	fgets(gname, MAX_GNAME, stdin);
	gname[strlen(gname)-1]=0;
	strcpy(s.mtext, gname);
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	if(strcmp(r.mtext, "full")==0){
		printf("Can't create more groups\n");
		return;
	}
	else if(strcmp(r.mtext, "exists")==0){
		printf("Group already exists\n");
		return;
	}
	else printf("Group %s created\n", s.mtext);
}

void lsgrp(){
	rbuf r;
	s.cmd=LSGRP;
	msgsnd(req, &s, sizeof(s), 0);
	int gcount;
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	gcount=atoi(r.mtext);
	printf("NAME:\t\tMember?\n");
	for(int i=0; i<gcount; i++){
		msgrcv(resp, &r, sizeof(r), getpid(), 0);
		printf("%s\t\t", r.mtext);
		msgrcv(resp, &r, sizeof(r), getpid(), 0);
		printf("%s\n", r.mtext);
	}
}

void jngrp(){
	rbuf r;
	s.cmd=JNGRP;
	char gname[MAX_GNAME];
	printf("Group name: ");
	fgets(gname, MAX_GNAME, stdin);
	gname[strlen(gname)-1]=0;
	strcpy(s.mtext, gname);
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	char serverresp[SMALL_TEXT];
	strcpy(serverresp, r.mtext);
	
	if(strcmp(serverresp, "alreadyin")==0){
		printf("You are already a member. Try swgrp.\n");
		return;
	}
	if(strcmp(serverresp, "noexist")==0){
		printf("Group does not exist. Create using mkgrp or check group list using lsgrp.\n");
		return;
	}

	int projid=atoi(r.mtext);
	strcpy(mgrp[me.grpcount].gname, s.mtext);
	mgrp[me.grpcount].projid=projid;
	mgrp[me.grpcount].gkey=ftok(GPATH, mgrp[me.grpcount].projid);
	mgrp[me.grpcount].gqid=msgget(mgrp[me.grpcount].gkey, 0);
	printf("Joined group %s\n", mgrp[me.grpcount].gname);

	me.grpcount=me.grpcount+1;
}

void swgrp(){
	char gname[MAX_GNAME];
	printf("Group name: ");
	fgets(gname, MAX_GNAME, stdin);
	gname[strlen(gname)-1]=0;
	if(!isgrp(gname)){
		printf("Either group doesn't exist or you are not a member.\n");
		return;
	}
	me.gselflag=1;
	int i=getgrp(gname);
	me.curmsgq=mgrp[i].gqid;
	printf("Switching to group %s\n", mgrp[i].gname);
}

int isgrp(char *gname){
	int ret=0;
	int i;
	for (i=0; i<me.grpcount; i++)
		if(strcmp(mgrp[i].gname, gname)==0) ret =1;
	return ret;
}

int getgrp(char *gname){
	int i;
	for(i=0; i<me.grpcount; i++)
		if(strcmp(mgrp[i].gname, gname)==0) return i;
}

void sync(){
	rbuf r;
	s.cmd=SYNC;
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	me.grpcount=atoi(r.mtext);
	for (int i=0; i<me.grpcount; i++){
		msgrcv(resp, &r, sizeof(r), getpid(), 0);
		strcpy(mgrp[i].gname, r.mtext);
		msgrcv(resp, &r, sizeof(r), getpid(), 0);
		mgrp[i].projid=atoi(r.mtext);
		mgrp[i].gkey=ftok(GPATH, mgrp[i].projid);
		mgrp[i].gqid=msgget(mgrp[i].gkey, 0);
	}
}

void send(){
	mbuf m;
	printf("Enter lines of text. ^D to end.\n");
	while(fgets(m.mtext, MAX_TEXT, stdin), !feof(stdin)){
		m.mtype=1;
		strcpy(m.uname, me.uname);
		m.pid=getpid();
		m.mtime=time(NULL);
		m.cmd=SEND;
		if(me.online){
			msgsnd(me.curmsgq, &m, sizeof(m), 0);
		}
		else{
			addmode(&m);
		}
	}
}

void recv(){
	mbuf m;
	m.cmd=RECV;
	m.mtype=1;
	strcpy(m.uname, me.uname);
	m.pid=getpid();
	msgsnd(me.curmsgq, &m, sizeof(m), 0);
	while(1){
		msgrcv(me.curmsgq, &m, sizeof(m), getpid(), 0);
		if(m.cmd==LAST) break;
		putmsg(&m);
	}
}

void putmsg(mbuf *m){
	printf("%s", ctime(&(m->mtime)));
	printf("%s: ", m->uname);
	printf("%s\n", m->mtext);
}

void mknode(mbuf *m){
	node *temp = (node *) calloc(1, sizeof(node));
	tail->next = temp;
	tail = temp;
	strcpy(tail->m.mtext, m->mtext);
	strcpy(tail->m.uname, m->uname);
	tail->m.mtime=m->mtime;
	//printf("----storing----\n");
	//putmsg(&(tail->m));
}

void firstnode(mbuf *m){
	tail=(node *) calloc(1, sizeof(node));
	head=tail;
	strcpy(tail->m.mtext, m->mtext);
	strcpy(tail->m.uname, m->uname);
	tail->m.mtime=m->mtime;
	//printf("----storing----\n");
	//putmsg(&(tail->m));
}