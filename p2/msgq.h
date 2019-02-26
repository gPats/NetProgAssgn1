#define SPATH "./msgq_server.c"
#define GPATH "./msgq_group.c"
#define MAX_TEXT 100
#define MKGRP 1
#define JNGRP 2
#define UNAME 3

typedef struct sbuf{
	long mtype;
	int cmd;
	char uname[20];
	int gnum;
	int pid;
	char mtext[MAX_TEXT];
}sbuf;

typedef struct rbuf{
	long mtype;
	char mtext[10];
}rbuf;
