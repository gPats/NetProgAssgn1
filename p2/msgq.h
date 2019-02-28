#define SPATH "./msgq_server.c"
#define GPATH "./msgq_group.c"
#define MAX_TEXT 100
#define SMALL_TEXT 20
#define MAX_UNAME 20
#define MAX_PWD 20
#define MAX_GNAME 20
#define NUM_USER 10
#define NUM_GRP 10
#define USR_PER_GRP 10
#define GRP_PER_USR 10
#define MKGRP 1
#define JNGRP 2
#define UNAME 3
#define LOGIN 4
#define LOGOF 5
#define PASWD 6
#define LSGRP 7
#define SYNC  8

#define SEND  9
#define RECV 10
#define LAST 11
#define MESG 12

typedef struct sbuf{
	long mtype;
	int cmd;
	char uname[MAX_UNAME];
	int pid;
	char mtext[SMALL_TEXT];
}sbuf;

typedef struct rbuf{
	long mtype;
	char mtext[SMALL_TEXT];
}rbuf;

typedef struct mbuf{
	long mtype;
	int cmd;
	char mtext[MAX_TEXT];
	char uname[MAX_UNAME];
	int pid;
	time_t mtime;
}mbuf;

typedef struct ibuf{
	long mtype;
	char mtext[SMALL_TEXT];
}ibuf;