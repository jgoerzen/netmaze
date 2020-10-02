/* BetterBot(BB) Version 0.6beta  20.2.94
**
** vom Frankfurt-Netmaze-Support-Team
**
** Jens Kurlanda, Benjamin Baermann
**
** kurlanda@informatik.uni-frankfurt.de
** benni@informatik.uni-frankfurt.de
**
**
** There are NO bugs... only unwanted Features ;-)
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "netmaze.h"
#include "better.h"

#define BIGGEST			/* groesster int-wert */
#define NERVOUS 50000000 	/* ab hier sucht BB sein Opfer */
#define TODESRADIUS1 7000000
#define TODESRADIUS 15000000   
extern int walktab[320];	/* static-declaration in allmove.c
				muss nicht-static werden */
static int ownnumber;
extern struct shared_struct *sm;


static int enemy_touch(PLAYER *player,PLAYER *opfer){
  int xd,yd;

      xd = (player->x - opfer->x);
      yd = (player->y - opfer->y);

      if(xd < 0) xd = -xd;
      if(yd < 0) yd = -yd;

      if((xd > 0x800000) || (yd > 0x800000)) return 0;

      xd>>=12; yd>>=12;

      if( (xd*xd + yd*yd) < 0x560000)  return 1;
  return 0;
}

/*
static int see_him(PLAYER* player){
sm->std_maze.vwalls[yc][xc+1]
	
};
*/
/* typedef enum {LINKS, GERADE, RECHTS} richtung;
*/

/*******************
 * the init_robot is called one time right after the start
 */

robodat_t robodat={0,GERADE,0,0,0,0,0,-2,"",0,-1,-1};


/* ich finde die verf***te standardfunktion nicht :-( */

int signum(int eingabe)
{
	if (eingabe==0)
		return 0;
	else if(eingabe>0)
		return 1;
	else
		return -1;
}



/* umwandlung von quadranten nach ints und umgekehrt */

int ltoq(int eingabe)
{
	return (int) ( (eingabe & 0xff000000) >> 24);
}

int qtol(int eingabe)
{
	int ausgabe;

	ausgabe = (int) eingabe;
	return (eingabe << 24);
}

void laber(char *ausgabe)
{
	fprintf(stderr,ausgabe);
}

/* diese funktion soll ueberpruefen, ob von x1,y1 nach x2,y2 eine
direkte verbindung existiert (ohne waende dazwischen)

sicher nicht besonders schoen, effizient oder gar korrekt ;-) aber es
geht so einigermassen... */

int k_sichtbar(int x1,int y1,int  x2,int y2)
{
	int x1q,y1q,x2q,y2q,xqdiff,yqdiff;
	int x_count,y_count,vstep,hstep,hstep1,vstep1;
	int xdiff,ydiff,xl_count,yl_count,xl_step,yl_step;
	float xy,yx;

	
	/* in welchen quadranten? */
	x1q = ltoq(x1);
	y1q = ltoq(y1);
	x2q = ltoq(x2);
	y2q = ltoq(y2);
	
	xqdiff=x1q-x2q;
	yqdiff=y1q-y2q;
	xdiff=x1-x2;
	ydiff=y1-y2;
	xy=(float)xdiff/ydiff;
	yx=(float)ydiff/xdiff;
	vstep=signum(xqdiff);
	hstep=signum(yqdiff);
		
/* 	hier versuche ich mal, die neun faelle auf zwei zu reduzieren... */

	/* horizontale waende (dunkelgrau) auf der verbindungslinie? */
	if(hstep!=0)
	{
		x_count=x1q;
		xl_count=x1;
		xl_step= (hstep) * (int) (xy * 0x01000000);

		for(y_count=y1q+((hstep==-1)?1:0);
		    (hstep==-1)?(y_count<=y2q):(y_count>y2q);
		    (hstep==-1)?(y_count++):(y_count--) )
		{
			xl_count-=xl_step;
			x_count=ltoq(xl_count);
			/*fprintf(stderr,"%d:%d=",x_count,y_count);*/
			if((sm->std_maze.hwalls[y_count][x_count]))
			{
				return FALSE;
			}
		}
	}

	/* vertikale waende (hellgrau) auf der verbindungslinie? */
	if (vstep!=0)
	{
		y_count=y1q;
		yl_count=y1;
		
		yl_step=-(vstep)*(int) (yx * 0x01000000);

		for(x_count=x1q+((vstep==-1)?1:0);
		    (vstep==-1)?(x_count<=x2q):(x_count>x2q);
		    (vstep==-1)?(x_count++):(x_count--) )
		{
			if(hstep!=1)
				yl_count+=yl_step;
			else
				yl_count-=yl_step;
				
			y_count=ltoq(yl_count);
			if((sm->std_maze.vwalls[y_count][x_count]))
			{

				return FALSE;
			}
		}		
	}
	return TRUE;
}

/* ist das opfer sichtbar? */

int sichtbar(int opfer)
{
	return k_sichtbar(sm->playfeld[ownnumber].x,sm->playfeld[ownnumber].y,
	       sm->playfeld[opfer].x,sm->playfeld[opfer].y);
}



 
void init_robot(void)
{
  srand48(258245);
  strcpy(sm->owncomment,"Ich weiss, dass ich besser bin!");
  strcpy(sm->ownname,"Betterbot");
}

void start_robot(int num)
{
int i;
    ownnumber = num;

	/* hier wird festgestellt, welcher spieler nicht getoetet werden
	darf*/
	robodat.freund=-2;
	strcpy(sm->owncomment,"Ich weiss, dass ich besser bin!");
	for(i=0;i<sm->anzplayers;i++){
		if (!strcmp(robodat.freundname,sm->playfeld[i].name)){
			robodat.freund=i;
			fprintf(stderr,"Du bist mein Freund, %s!",sm->playfeld[robodat.freund].name);
			strcpy(sm->owncomment,robodat.freundname);
			strcat(sm->owncomment,"'s Feinde sind auch meine Feinde");
		};
	}
        send_owncomment();
}



/* gibt die koordinate des abstandes zum opfer zurueck, die groesser ist */

int OpferDistanz(int opfer){
	int	x_dist, y_dist;
	if (opfer ==  -1) return BIGGEST;
	x_dist=labs((sm->playfeld[ownnumber].x)-(sm->playfeld[opfer].x));
	y_dist=labs((sm->playfeld[ownnumber].y)-(sm->playfeld[opfer].y));

	if (x_dist >= y_dist)
		return x_dist;
	else
		return y_dist;
}

/* ermittelt das naechste opfer mit hilfe von OpferDistanz() */

int Opfer(){
	int BestOpfer=-1;
	int WeissesindenAugen=NERVOUS;
	int i;
	if (robodat.freund == -1 ) return robodat.exfreund;
	/* schleife ueber alle spieler */
	for (i=0;i<sm->anzplayers;i++){
		/* nur, wenn distanz kuerzer, nicht eigener spieler,
		nur lebende gegner, keine teampartner (funktioniert
		nicht immer :-(  )
		keine spieler, die in der kommandozeile angegeben waren*/
		if(    (WeissesindenAugen>OpferDistanz(i))
		    && (i!=ownnumber)
		    && (sm->playfeld[i].alive)
		    && ((sm->playfeld[i].team)!=(sm->playfeld[ownnumber].team))
		    && (robodat.freund!=i) )
		{
			WeissesindenAugen = OpferDistanz(i);
			BestOpfer = i;
		};	
	};
	return BestOpfer; 	
}

/* setzen der richtungen */
void geradeaus(void)
{
	robodat.ret     = JOY_UP;
	robodat.zustand = GERADE;
	robodat.counter = 0;
}
void rechts(void)
{
	robodat.ret     = JOY_RIGHT;
	robodat.zustand = RECHTS;
	robodat.counter = 0;
}
void links(void)
{
	robodat.ret     = JOY_LEFT;
	robodat.zustand = LINKS;
	robodat.counter = 0;
}


static int bewegt(){
	return (!((robodat.oldy==sm->playfeld[ownnumber].y)&&(robodat.oldx==sm->playfeld[ownnumber].x)));
}


static int deg(double x,double y){
  int winkel;
  if (x){ /* muss Berechnet Werden */
    winkel =  (int)(atan(y/x)/(2.0*M_PI) * 265 );
    if (winkel > 0 ){
      if (y > 0){
      }else{
	winkel += 128;
      };
    }else{
      if (y > 0){
	winkel += 64 + 64 ; /* 2. Quadrant offset 64 + umdrehen des Winkels 64 */
      }else{
	winkel += 192 + 64 ; /* 2. Quadrant offset 192 + umdrehen des Winkels 64 */
      };
    };
  }else{ /* x=0 => einfache Bestimmung von Winkel, gell ? */
    if (y > 0) {
      winkel = 64;
    }else{
      winkel = 192;
    };
  };
  return (winkel + 128) % 256  ;
}

static int target_angle(PLAYER* them){
    int mx,my;
    int tx,ty;
    int dx,dy;
    int angle, diff;
    PLAYER* me= &(sm->playfeld[ownnumber]);

/*  mx = (int) ((me->x) >> 24);
    my = (int) ((me->y) >> 24);
    tx = (int) ((them->x) >> 24);
    ty = (int) ((them->y) >> 24);
*/
    mx = me->x;
    my = me->y; 
    tx = them->x;
    ty = them->y;

    dx = mx-tx;
    dy = my-ty;
/*		fprintf(stderr,"%li:%li:",dx,dy);
*/
     return (int)deg((double)dy,(double)dx);

}



/* testen, was besser ist, rechts oder links fahren */
void angl(int opfer){
	int ownwinkel=sm->playfeld[ownnumber].winkel;
	int angle = target_angle(&(sm->playfeld[opfer]));
	int wonkel;
	wonkel=angle - ownwinkel;
/*	fprintf (stderr,"\neigenwinkel %li winkel zu anderem  %li diff %li ",ownwinkel,angle,wonkel);
*/	if ((wonkel)>0){
		if (wonkel>128)
			links();
		else
			rechts();
	}else{
		if (wonkel > (-128))
			links();
		else
			rechts();
	};
}



/* wird bei jedem 'tick' aufgerufen. das herz von BB. bb - Falsch BB hat kein Herz . JK*/

int own_action(void){
  static touchie=0;
  int opfer, winkel, owinkel, hwinkel, lwinkel, alt_opfer;
  int x_dist,y_dist,nx_dist,ny_dist,lx_dist,ly_dist,hx_dist,hy_dist;
  int i;

  if (!(sm->playfeld[ownnumber].alive)){ /* I'm dead , wooouueeehhhh */
    touchie=0;
    if (!(robodat.tod)){ /* killed right now - look at the killer, and if its the friend ...*/
      robodat.tod=1;/* I'm dead*/
      if (sm->playfeld[ownnumber].ownkiller==robodat.freund){ /* ts, ts */
	robodat.exfreund=robodat.freund;
	robodat.freund=-1;
	fprintf(stderr,"No more Mister Nice Guy, %s!",sm->playfeld[robodat.freund].name);
	sprintf(sm->owncomment,"%s, you fucking traitor pig : die scum!",robodat.freundname);
	send_owncomment();
      };
    };
  }else{/*not dead*/
    if (robodat.tod){ /* if was dead, reset deadflag */
      robodat.tod=0;
    };
    touchie=0;
    /* JA! BB hat ein Opfer gefunden und ist nicht in einem
       Ausweichmanoever */
    opfer = Opfer(); /* Get a matching victim */
    if (opfer != robodat.alt_opfer ){ /* Is it a new victim  ?*/
      if (opfer > -1){
	robodat.jagd=1;
	fprintf(stderr,"Die Jagd auf %s beginnt!\n",sm->playfeld[opfer].name);
      }else{
	robodat.jagd=0;	
	fprintf(stderr,"Ende der Jagd auf %s!\n",sm->playfeld[robodat.alt_opfer].name);
      };
      robodat.alt_opfer = opfer;
    };
    /* turn your face to the black man */
    if ((!(robodat.ausweichen))&&(opfer>-1)) angl(opfer); 
    /* if no move , evade */
    if 	((robodat.ausweichen) || (!bewegt() )){
      if ( robodat.ausweichen==0){ /* if not in evade mode, look at the situation */ 
	if (robodat.jagd && sichtbar(opfer)){/* chasing a victim and have eye contact */
	  if (enemy_touch(&(sm->playfeld[ownnumber]),&sm->playfeld[opfer])){ 
	    /* if touchin victim no more action */
	    touchie=2;
	    robodat.ausweichen = 0;
	    fprintf(stderr,"%s : enemy touch\n",sm->ownname);	
	  }else{
	    if (touchie){/* if touched until know, no action */
	      touchie--;
	      fprintf(stderr,"%s : the pig flees! (no touch)\n",sm->ownname);
	    }else{
	      /* I seem to be blocked by a wall, but see the victim ->evade a short time*/
	      i = (int)(drand48() * 2);
	      switch (i) {
	      case 0 :
		robodat.ret = JOY_RIGHT;
		break;
	      case 1 :
		robodat.ret = JOY_LEFT;
		break;
	      };
	      robodat.ausweichen = 10; 
	      fprintf(stderr,"%s : evade! (no touch)\n",sm->ownname);
	    };
	  };
	}else{
	  /* I seem to be blocked by a wall ->evade a int time */
	  i = (int)(drand48() * 2);
	  switch (i) {
	  case 0 :
	    robodat.ret = JOY_RIGHT;
	    break;
	  case 1 :
	    robodat.ret = JOY_LEFT;
	    break;
	  };
	  
	  touchie=0;					
	  if (!robodat.jagd) robodat.ausweichen = 15;
	  fprintf(stderr,"%s : evade!\n",sm->ownname);
	};
      }else
	robodat.ausweichen--; /* count down evading */
    }else if (!robodat.jagd){ /* no victim , no evade */
      i = (int)(drand48() * 40);
      touchie=0;
      robodat.counter++;
      switch(i){
	case 0:
	  if ((robodat.zustand!=LINKS)
	      &&
	      ((robodat.counter>5)&&(robodat.zustand!=GERADE)||(robodat.counter>40)))
	    links();				
	  break;
	case 1:
	  if ((robodat.zustand!=RECHTS)
	      &&
	      ((robodat.counter>5)&&(robodat.zustand!=GERADE)||(robodat.counter>40)))
	    rechts;
	  break;
	}
      
      if ((robodat.counter>10)&&(robodat.zustand!=GERADE)){
	  /*				fprintf(stderr,"gerade aus!\n");
	   */		
	  robodat.ret &=(( ~JOY_LEFT) && (~JOY_RIGHT) );
	  robodat.zustand=GERADE;
	  robodat.counter=0;
	}
    };
    
    if(!(sm->gamemode & GM_WEAKINGSHOTS) || (robodat.jagd))
      robodat.ret |= JOY_BUTTON;
  };
  robodat.ret |= JOY_UP;
  robodat.oldy=sm->playfeld[ownnumber].y;
  robodat.oldx=sm->playfeld[ownnumber].x;
  return robodat.ret;
}


