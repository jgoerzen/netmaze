#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

#include "netmaze.h"

/* if your machine doesn't support I_FLUSH, you have to #undef HAVE_FLUSH
 * in this case you also should reduce the BLOCK_SIZE to 256 or 512 
 */

/* #define SS10_AUDIO */ /* ss10-audio = 16bit signed linear samples at 22khz */
/* #define SIMPLE_AUDIO */ /* simple-audio = 8K-Ulaw-Samples on /dev/audio */
/* #define HP_LINEAR */    /* 8bit signed linear at 8khz on /dev/audioBL */
/* #define NCD_AUDIO */    /* not supported yet */
/* #define SGI_AUDIO */    /* not supported yet */

#ifdef USE_SOUND
 #if (!defined(SS10_AUDIO) && !defined(HP_LINEAR) && !defined(NCD_AUDIO))
  #define SIMPLE_AUDIO
 #endif

/* hp (or general sys5?) doesn't support I_FLUSH/ioctls */
 #ifdef HP_LINEAR
  #undef HAVE_FLUSH
  #define BLOCK_SIZE 256
 #else
   /* if you flush the audio-device, it clears the non-played data */
  #define HAVE_FLUSH
   /* you have to test out the blocksize ... large as possible */
  #define BLOCK_SIZE 4096
 #endif
#endif

#ifdef HAVE_FLUSH
 #include <stropts.h>
#endif

#ifdef SS10_AUDIO
# include <sun/audioio.h>
#endif

   /* depends on soure/destination-sample-rate */
#ifdef SS10_AUDIO
  #define MUL (5)
#else
  #define MUL (2)
#endif

/* test */
#define BLOCKDEVICE O_NDELAY

extern struct shared_struct *sm;

struct sample 
{
  long start;
  long len;
  char name[20];
};

#ifdef USE_SOUND
# ifdef NCD_AUDIO
 static AuServer *aud;
 char *auservername = NULL;
# endif
# ifdef SS10_AUDIO
 audio_info_t ainfo;
# endif
 static int read_data(char *f,int type);
 static int lin_to_au(int lin16);
 static char *data,*pnt;
 static int  plen,len;
 struct sample *sinfo;
#endif

static int audio=-1;
int playit = 0;
static int play_delay=MUL;

enum { AUDIO_8UL , AUDIO_8ULAW , AUDIO_16SL };

void kick_audio(void);

int init_audio(void)
{
#ifdef USE_SOUND
  int type;

  if(!sm->usesound)
    return -1;

 #if defined(HP_LINEAR)
  if(read_data(AUDIOPATH,AUDIO_8UL) )
    audio = open("/dev/audioBL",O_WRONLY|BLOCKDEVICE); /* test */
  if(audio == -1)
    sm->usesound = 0;
 #elif defined(SIMPLE_AUDIO)
  if(read_data(AUDIOPATH,AUDIO_8ULAW) )
    audio = open("/dev/audio",O_WRONLY|BLOCKDEVICE);
  if(audio == -1)
    sm->usesound = 0;
 #elif defined(SS10_AUDIO)
  if(read_data(AUDIOPATH,AUDIO_16SL) )
  {
    audio = open("/dev/audio",O_WRONLY|BLOCKDEVICE);
    if(ioctl(audio, AUDIO_GETDEV, &type))
      audio = -1;
    if(type == AUDIO_DEV_UNKNOWN || type == AUDIO_DEV_AMD)
      audio = -1;
    AUDIO_INITINFO(&ainfo);
    ainfo.play.encoding = AUDIO_ENCODING_LINEAR;
    ainfo.play.channels = 1;
/* 8000, 9600, 11025, 16000, 18900, 22050, 32000, 37800, 44100, 48000 */
    ainfo.play.sample_rate = 22050; /* 44100 */
    ainfo.play.precision = 16;
    if(ioctl(audio, AUDIO_SETINFO, &ainfo))
      audio = -1;
    ioctl(audio, AUDIO_GETINFO, &ainfo);

    printf("rate: %d\n",ainfo.play.sample_rate);
    printf("channels: %d\n",ainfo.play.channels);
    printf("precision: %d\n",ainfo.play.precision);
    printf("encoding: %d\n",ainfo.play.encoding);
  }
  if(audio == -1)
    sm->usesound = 0;
 #elif defined(NCD_AUDIO)
  audio = -1;
  aud = AuOpenServer(auservername, 0, NULL, 0, NULL, NULL);
  if (!aud)
  {
    fprintf(stderr, "Can't connect to audio server\n");
    sm->usesound = 0;
  }
 #endif

  fprintf(stderr,"audioopen: %d\n",audio);
#endif
  return audio;
}

void play_sound(int num)
{
#ifdef USE_SOUND
  if(sm->usesound)
  {
#ifdef HAVE_FLUSH
    ioctl(audio, I_FLUSH, FLUSHW); /* clear buffer */
#endif
    plen = (play_delay*(sinfo[num].len-sinfo[num].start)) & 0xfffffff0;
    pnt = data+(sinfo[num].start*play_delay & 0xfffffff0);
    kick_audio();
    playit=1;
  }
#endif
}

void kick_audio(void)
{
#ifdef USE_SOUND
  int l;

  if(!playit || audio<0 ) return;

  if(plen > BLOCK_SIZE)
    l = BLOCK_SIZE;
  else
    l = plen;
  plen -= l;
  write(audio,pnt,l);
  pnt += l;

  if(plen == 0) 
    playit = 0;
#endif
}

#ifdef USE_SOUND

static int read_data(char *f,int type)
{
  FILE *fd;
  unsigned char junk[8];
  int  i,j,a,delay;
  long blen,pos;
  unsigned char c;

  delay = play_delay;

  if(type == AUDIO_16SL)
  {
    play_delay<<=1;
  }

  fd = fopen(f,"r");

  if(fd == NULL)
  {
    fprintf(stderr,"Can't load soundsamples.\n");
    return 0;
  }

  fread(junk,1,8,fd);  
  len = ((int)junk[4]<<8)+junk[5];

  sinfo = (struct sample *) calloc(len,sizeof(struct sample));

  for(i=0;i<len;i++)
  {
    fread(junk,1,8,fd);
    sinfo[i].start = ((long)junk[0]<<24)+((long)junk[1]<<16)
                     +((long)junk[2]<<8)+((long)junk[3]);
    sinfo[i].len = ((long)junk[4]<<24)+((long)junk[5]<<16)
                     +((long)junk[6]<<8)+((long)junk[7]);
    fread(sinfo[i].name,1,16,fd);
  }

/*
  for(i=0;i<len;i++)
    printf("SND: %d:%s\n",i,sinfo[i].name);
*/

  pos = ftell(fd);
  fseek(fd,0,2);
  blen = ftell(fd)-pos;
  fseek(fd,pos,0);

  data = (char *) ((malloc(blen*play_delay+16)+15) & 0xfffffff0);
  fread(data,1,blen,fd);
  fclose(fd);

  switch(type)
  {
    case AUDIO_8UL:
      for(i=blen-1;i>=0;i--)
        for(j=0;j<delay;j++)
          data[i*delay+j] = data[i];
      break;
    case AUDIO_8ULAW:
      for(i=blen-1;i>=0;i--)
      {
        a =  lin_to_au( (int) ((unsigned char) (data[i]<<1) - 0x80) << 8) ;
        for(j=0;j<delay;j++)
          data[i*delay+j] = data[i];        
      }
      break;
    case AUDIO_16SL:
      for(i=blen-1;i>=0;i--)
        for(j=0;j<delay;j++)
        {
          a = (int)((unsigned char) (data[i]<<1) - 0x80)<< 8;
          a &= 0x0000ffff;
          data[i*play_delay+j*2] = a>>8;
          data[i*play_delay+j*2+1] = a;
        }
      break;
  }
  return 1;
}

static int lin_to_au(int lin16)
{
  double d,j;
  float m=26;
  int c;

  j = (double) lin16 / 256;
  if(j <0)
  {
    d = log((double)-j)*m;
    c = d;
    c |= 0x80;
  }
  else if(j>0)
  {
    d = log((double)j)*m;
    c = d;
  }
  else c = 0;

  c = (~c) & 0xff;
  
  return c;
}
#endif



