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
void addnode(mbuf *m);
void send_old();

#define RED_BOLD printf("\033[1;31m")
#define BLUE_BOLD printf("\033[1;34m")
#define GREEN_BOLD printf("\033[1;32m")
#define BLUE printf("\033[0;34m")
#define CYAN printf("\033[0;36m")
#define RESET printf("\033[0m")

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

	BLUE; printf("Username: "); RESET;
	fgets(me.uname, MAX_UNAME, stdin);
	me.uname[strlen(me.uname)-1]=0;
	s.mtype=1;
	s.cmd=UNAME;
	s.pid=getpid();
	strcpy(s.uname, me.uname);
	
	//msgrcv(resp, &r, sizeof(r), getpid(), 0);
	
	printf("Enter 'help' for list of commands.\n");

	while(1){
		BLUE_BOLD; printf("$ "); RESET;
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
				RED_BOLD; printf("Already online\n"); RESET;
				continue;
			}
			login();
			sync();
			//send_old(); 										TO DO
			continue;
		}
		if(strcmp(str, "logoff")==0){
			if(!me.online){
				RED_BOLD; printf("Only valid when online\n"); RESET;
				continue;
			}
			logout();
		}
		if(strcmp(str, "mkgrp")==0){
			if(!me.online){
				RED_BOLD; printf("Only valid when online\n"); RESET;
				continue;
			}
			mkgrp();
			continue;
		}
		if(strcmp(str, "lsgrp")==0){
			if(!me.online){
				RED_BOLD; printf("Only valid in online mode. Try mygrp.\n"); RESET;
				continue;
			}
			lsgrp();
			continue;
		}
		if(strcmp(str, "jngrp")==0){
			if(!me.online){
				RED_BOLD; printf("Only valid in online mode.\n"); RESET;
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
				RED_BOLD; printf("Select a group first. Try swgrp.\n"); RESET;
				continue;
			}
			send();
			continue;
		}
		if(strcmp(str, "read")==0){
			if(!me.gselflag){
				RED_BOLD; printf("Select a group first. Try swgrp.\n"); RESET;
				continue;
			}
			if(!me.online){
				RED_BOLD; printf("Only valid in online mode\n"); RESET;
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
		RED_BOLD; printf("ABORTING... TOO MANY USERS\n"); RESET;
		raise(SIGTERM);
	}
	char pwd[MAX_PWD];
	BLUE; printf("passwd: "); RESET;
	fgets(pwd, MAX_PWD, stdin);
	s.cmd=PASWD;
	pwd[strlen(pwd)-1]='\0';
	strcpy(s.mtext, pwd);
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	printf("%s\n", r.mtext);
	if(strcmp(r.mtext, "success")==0 || strcmp(r.mtext, "paswd set")==0) {
		me.online = 1;
		GREEN_BOLD; printf("ONLINE...\n"); RESET;
	}
}

void logout(){
	rbuf r;
	s.cmd=LOGOF;
	msgsnd(req, &s, sizeof(s), 0);
	me.online=0;
	GREEN_BOLD; printf("OFFLINE...\n"); RESET;
}

void mkgrp(){
	rbuf r;
	s.mtype=1;
	s.cmd=MKGRP;
	char gname[MAX_GNAME];
	BLUE; printf("Group name: "); RESET;
	fgets(gname, MAX_GNAME, stdin);
	gname[strlen(gname)-1]=0;
	strcpy(s.mtext, gname);
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	if(strcmp(r.mtext, "full")==0){
		RED_BOLD; printf("Can't create more groups\n"); RESET;
		return;
	}
	else if(strcmp(r.mtext, "exists")==0){
		RED_BOLD; printf("Group already exists\n"); RESET;
		return;
	}
	GREEN_BOLD; printf("Group %s created\n", s.mtext); RESET;
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
	BLUE; printf("Group name: "); RESET;
	fgets(gname, MAX_GNAME, stdin);
	gname[strlen(gname)-1]=0;
	strcpy(s.mtext, gname);
	msgsnd(req, &s, sizeof(s), 0);
	msgrcv(resp, &r, sizeof(r), getpid(), 0);
	char serverresp[SMALL_TEXT];
	strcpy(serverresp, r.mtext);
	
	if(strcmp(serverresp, "alreadyin")==0){
		RED_BOLD; printf("You are already a member. Try swgrp.\n"); RESET;
		return;
	}
	if(strcmp(serverresp, "noexist")==0){
		RED_BOLD; printf("Group does not exist. Create using mkgrp or check group list using lsgrp.\n"); RESET;
		return;
	}

	int projid=atoi(r.mtext);
	strcpy(mgrp[me.grpcount].gname, s.mtext);
	mgrp[me.grpcount].projid=projid;
	mgrp[me.grpcount].gkey=ftok(GPATH, mgrp[me.grpcount].projid);
	mgrp[me.grpcount].gqid=msgget(mgrp[me.grpcount].gkey, 0);
	GREEN_BOLD; printf("Joined group %s\n", mgrp[me.grpcount].gname); RESET;

	me.grpcount=me.grpcount+1;
}

void swgrp(){
	char gname[MAX_GNAME];
	BLUE; printf("Group name: "); RESET;
	fgets(gname, MAX_GNAME, stdin);
	gname[strlen(gname)-1]=0;
	if(!isgrp(gname)){
		RED_BOLD; printf("Either group doesn't exist or you are not a member.\n"); RESET;
		return;
	}
	me.gselflag=1;
	int i=getgrp(gname);
	me.curmsgq=mgrp[i].gqid;
	GREEN_BOLD; printf("Switching to group %s\n", mgrp[i].gname); RESET;
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
	send_old();
}

void send(){
	mbuf m;
	ssize_t size=MAX_TEXT;
	char *c=(char *)malloc(sizeof(char)*MAX_TEXT);
	GREEN_BOLD; printf("Enter lines of text. ^D to end.\n"); RESET;
	while(getline(&c, &size, stdin)!=1){
		strcpy(m.mtext, c);
		m.mtype=1;
		strcpy(m.uname, me.uname);
		m.pid=getpid();
		m.mtime=time(NULL);
		m.cmd=SEND;
		if(me.online){
			msgsnd(me.curmsgq, &m, sizeof(m), 0);
		}
		else{
			addnode(&m);
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
	CYAN;
	printf("%s", ctime(&(m->mtime)));
	printf("%s: ", m->uname);
	RESET;
	printf("%s\n", m->mtext);
}

void mknode(mbuf *m){
	node *temp = (node *) calloc(1, sizeof(node));
	tail->next = temp;
	tail = temp;
	strcpy(tail->m.mtext, m->mtext);
	strcpy(tail->m.uname, m->uname);
	tail->m.mtime=m->mtime;
	tail->m.cmd=m->cmd;
	tail->m.mtype=m->mtype;
	tail->gqid=me.curmsgq;
	//printf("----storing----\n");
	//putmsg(&(tail->m));
}

void firstnode(mbuf *m){
	tail=(node *) calloc(1, sizeof(node));
	head=tail;
	strcpy(tail->m.mtext, m->mtext);
	strcpy(tail->m.uname, m->uname);
	tail->m.mtime=m->mtime;
	tail->m.cmd=m->cmd;
	tail->m.mtype=m->mtype;
	tail->gqid=me.curmsgq;
	//printf("----storing----\n");
	//putmsg(&(tail->m));
}

void addnode(mbuf *m){
	if(me.offmsgcount==0) firstnode(m);
	else mknode(m);
	me.offmsgcount++;
}

void send_old(){
	node *cur=head;
	node *next=NULL;
	while(cur!=NULL){
		cur->m.mtime=time(NULL);
		msgsnd(cur->gqid, &(cur->m), sizeof(mbuf), 0);
		me.offmsgcount--;
		next=cur->next;
		free(cur);
		cur=next;
	}
	head=tail=NULL;
}