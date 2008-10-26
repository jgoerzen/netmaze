
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
void pushint(char *s,int w)
{
  s[0] = ((unsigned int) w)>>24;
  s[1] = ((unsigned int) w)>>16;
  s[2] = ((unsigned int) w)>>8;
  s[3] = ((unsigned int) w);
}

int popint(char *s)
{
  w = ((unsigend int) s[0]<<24) + ((unsigend int) s[1]<<16) +
      ((unsigend int) s[2]<<8) + ((unsigend int) s[3])
  return((int) w);

}
*/

struct robot_functions {
  int  valid;
  void (*init)(void);
  void (*start)(int);
  int (*action)(void);
};

