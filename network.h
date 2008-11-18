
#define SUBSERVMODE 1
#define PLAYERMODE 2
#define CAMMODE 3

/*
#define CAMMAGIC     0x47115d1eL
#define SUBSERVMAGIC 0x08151234L
#define PLAYERMAGIC  0x17121970L
*/

/* V0.8 - magics: this avoids bugs, because of different versions */
#define CAMMAGIC    0x4d696b60L
#define SUBSERVMAGIC 0x78531930L
#define PLAYERMAGIC 0x77554712L

/*
void pushlong(char *s,long w)
{
  s[0] = ((unsigned long) w)>>24;
  s[1] = ((unsigned long) w)>>16;
  s[2] = ((unsigned long) w)>>8;
  s[3] = ((unsigned long) w);
}

long poplong(char *s)
{
  w = ((unsigend long) s[0]<<24) + ((unsigend long) s[1]<<16) +
      ((unsigend long) s[2]<<8) + ((unsigend long) s[3])
  return((long) w);

}
*/

struct robot_functions {
  int  valid;
  void (*init)(void);
  void (*start)(int);
  int (*action)(void);
};

