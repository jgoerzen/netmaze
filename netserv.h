/*****************
 * exmenu controll
 */

enum { MENU_NOP , MENU_LOADMAZE , MENU_RUNGAME , MENU_STOPGAME , MENU_LIST ,
       MENU_DISCONNECT , MENU_SETMODE , MENU_SETDIVIDER , MENU_ERROR ,
       MENU_RUNTEAMGAME , MENU_SETTEAMS };

/**********************************************
 * new structures
 * they are replacing the old slot-struct ...
 */

struct cqueue;
struct pqueue;
struct gqueue;

/* player */
struct pqueue
{
  struct cqueue *connection; /* connection */
  struct gqueue *group;      /* group */
  struct pqueue *cnext; /* connection chain */
  struct pqueue *gnext; /* group chain */
  struct pqueue *clast; /* connection chain */
  struct pqueue *glast; /* group chain */
  struct pqueue *nextgf; /* group-first-chain */
  char name[MAXNAME+1];
  char comment[MAXCOMMENT+1];
  int  mode; /* player/cam */
  int  playing;
  int  joydata;
  int  cnumber; /* connectionnumber: (client) */
  int  number;  /* game number */
  int  groupnumber;
  int  team;
  int  master;
  int  checked; /* only checked players can receive personal messages */
};

/* connections */
struct cqueue
{
  struct cqueue *next; /* global chain */
  struct cqueue *last; /* global chain */
  struct pqueue *players; /* player-chain on the connection */
  int plnum; /* number of players from this connection */
  int mode; /* flags */
  int socket;
  unsigned long lasttick;
  struct sockaddr remoteaddr;  /* remote socket address */
  char hostname[256];
  int response;
};

/* groups */
struct gqueue
{
  struct gqueue *next;
  char groupname[16];
  int groupno;
  int numplayers; /* numplay */
  int mode;
  struct pqueue *first;
  int gameflag;
  int playing;
  int gamemode;
  int numgamers; /* number of PLAYING players */
  int numjoy;
  int numwait;
  int response;
  int numteams;
  int timeout;
  int divider;
  int dividercnt;
  MAZE maze;
  int nomaze;
};

#define PLAYING 1
#define SINGLEPLAYER 2
#define SINGLECAMERA 4

