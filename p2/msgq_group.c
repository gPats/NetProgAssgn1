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

typedef struct usr{
	char uname[MAX_UNAME];
	int online;
	int sentcount;
}usr;

typedef struct grp{
	char gname[MAX_GNAME];
	int gqid;
	int gkey;
}grp;

typedef struct node{
	struct node * next;
	mbuf m;
}node;

usr usrlist[USR_PER_GRP]={};
int usrcount=0;

node *head=NULL;
node *tail=NULL;
int msgcount=0;;

grp me;
int isc;

void sigusrhandle(int sig);
void mknode(mbuf *m);
void firstnode(mbuf *m);
void recv(char *uname, int pid);
int getusr(char *uname);
void putmsg(mbuf *m);
void addselfmsg(char *str);

int main(int argc, char *argv[]){
	if(argc!=3){
		perror("wrong usage");
		return -1;
	}

	signal(SIGUSR1, sigusrhandle);

	ibuf b;

	key_t keyisc=ftok(GPATH, 0);
	isc=msgget(keyisc, 0);

	int projid=atoi(argv[2]);
	me.gkey=ftok(GPATH, projid+1);
	me.gqid=msgget(me.gkey, IPC_CREAT | 0666);
	strcpy(me.gname, argv[1]);

	char projidstr[5];
	sprintf(projidstr, "%d", projid+1);

	b.mtype=1;
	strcpy(b.mtext, projidstr);
	msgsnd(isc, &b, sizeof(b), 0);

	mbuf m;

	printf("Group %s ready\n", me.gname);
	while(1){
		switch(msgrcv(me.gqid, &m, sizeof(m), 1, 0)){
			case -1: break;
			default:
				switch(m.cmd){
					case SEND:
						//putmsg(&m);
						if(!msgcount) firstnode(&m);
						else mknode(&m);
						msgcount++;
						break;
					case RECV:
						recv(m.uname, m.pid);
				}
				break;
		}
	}
}

void sigusrhandle(int sig){
	ibuf b;
	msgrcv(isc, &b, sizeof(b), getpid(), 0);
	if(strcmp(b.mtext, "join")==0){
		msgrcv(isc, &b, sizeof(b), getpid(), 0);
		strcpy(usrlist[usrcount].uname, b.mtext);
		usrlist[usrcount].online=1;
		char smsg[MAX_TEXT];
		sprintf(smsg, "User %s joined chat", usrlist[usrcount].uname);
		addselfmsg(smsg);
		usrcount++;
		printf("%s joined group %s\n", b.mtext, me.gname);
	}
}

void mknode(mbuf *m){
	node *temp = (node *) calloc(1, sizeof(node));
	tail->next = temp;
	tail = temp;
	strcpy(tail->m.mtext, m->mtext);
	strcpy(tail->m.uname, m->uname);
	tail->m.mtime=m->mtime;
	printf("----storing----\n");
	putmsg(&(tail->m));
}

void firstnode(mbuf *m){
	tail=(node *) calloc(1, sizeof(node));
	head=tail;
	strcpy(tail->m.mtext, m->mtext);
	strcpy(tail->m.uname, m->uname);
	tail->m.mtime=m->mtime;
	printf("----storing----\n");
	putmsg(&(tail->m));
}

void recv(char *uname, int pid){
	int uid=getusr(uname);
	int i;
	node *cur=head;
	for(i=0; i<usrlist[uid].sentcount; i++)
		cur=cur->next;
	while(cur!=NULL){
		(cur->m).mtype=pid;
		(cur->m).cmd=MESG;
		msgsnd(me.gqid, &(cur->m), sizeof(cur->m), 0);
		i++;
		cur=cur->next;
	}
	usrlist[uid].sentcount=i;
	mbuf m;
	m.mtype=pid;
	m.cmd=LAST;
	msgsnd(me.gqid, &m, sizeof(m), 0);
}

int getusr(char *uname){
	int i;
	for(i=0; i<usrcount; i++)
		if(strcmp(usrlist[i].uname, uname)==0) return i;
	return -1;
}

void putmsg(mbuf *m){
	printf("%s", ctime(&(m->mtime)));
	printf("%s: ", m->uname);
	printf("%s\n", m->mtext);
}

void addselfmsg(char *str){
	mbuf m={};
	strcpy(m.mtext, str);
	m.cmd=SEND;
	m.mtype=1;
	strcpy(m.uname, "");
	m.mtime=time(NULL);
	msgsnd(me.gqid, &m, sizeof(m), 0);
}