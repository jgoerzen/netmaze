#define PIX01 *ia1 = pixval1; *ia2 = pixval2;
#define PIX02 PIX01 *(ia1-IMAGEWIDTH)=pixval1; *(ia2+IMAGEWIDTH)=pixval2;
#define PIX03 PIX02 *(ia1-2*IMAGEWIDTH)=pixval1; *(ia2+2*IMAGEWIDTH)=pixval2;
#define PIX04 PIX03 *(ia1-3*IMAGEWIDTH)=pixval1; *(ia2+3*IMAGEWIDTH)=pixval2;
#define PIX05 PIX04 *(ia1-4*IMAGEWIDTH)=pixval1; *(ia2+4*IMAGEWIDTH)=pixval2;
#define PIX06 PIX05 *(ia1-5*IMAGEWIDTH)=pixval1; *(ia2+5*IMAGEWIDTH)=pixval2;
#define PIX07 PIX06 *(ia1-6*IMAGEWIDTH)=pixval1; *(ia2+6*IMAGEWIDTH)=pixval2;
#define PIX08 PIX07 *(ia1-7*IMAGEWIDTH)=pixval1; *(ia2+7*IMAGEWIDTH)=pixval2;
#define PIX09 PIX08 *(ia1-8*IMAGEWIDTH)=pixval1; *(ia2+8*IMAGEWIDTH)=pixval2;
#define PIX0A PIX09 *(ia1-9*IMAGEWIDTH)=pixval1; *(ia2+9*IMAGEWIDTH)=pixval2;
#define PIX0B PIX0A *(ia1-10*IMAGEWIDTH)=pixval1; *(ia2+10*IMAGEWIDTH)=pixval2;
#define PIX0C PIX0B *(ia1-11*IMAGEWIDTH)=pixval1; *(ia2+11*IMAGEWIDTH)=pixval2;
#define PIX0D PIX0C *(ia1-12*IMAGEWIDTH)=pixval1; *(ia2+12*IMAGEWIDTH)=pixval2;
#define PIX0E PIX0D *(ia1-13*IMAGEWIDTH)=pixval1; *(ia2+13*IMAGEWIDTH)=pixval2;
#define PIX0F PIX0E *(ia1-14*IMAGEWIDTH)=pixval1; *(ia2+14*IMAGEWIDTH)=pixval2;
#define PIX10 PIX0F *(ia1-15*IMAGEWIDTH)=pixval1; *(ia2+15*IMAGEWIDTH)=pixval2;
#define PIX11 PIX10 *(ia1-16*IMAGEWIDTH)=pixval1; *(ia2+16*IMAGEWIDTH)=pixval2;
#define PIX12 PIX11 *(ia1-17*IMAGEWIDTH)=pixval1; *(ia2+17*IMAGEWIDTH)=pixval2;
#define PIX13 PIX12 *(ia1-18*IMAGEWIDTH)=pixval1; *(ia2+18*IMAGEWIDTH)=pixval2;
#define PIX14 PIX13 *(ia1-19*IMAGEWIDTH)=pixval1; *(ia2+19*IMAGEWIDTH)=pixval2;
#define PIX15 PIX14 *(ia1-20*IMAGEWIDTH)=pixval1; *(ia2+20*IMAGEWIDTH)=pixval2;
#define PIX16 PIX15 *(ia1-21*IMAGEWIDTH)=pixval1; *(ia2+21*IMAGEWIDTH)=pixval2;
#define PIX17 PIX16 *(ia1-22*IMAGEWIDTH)=pixval1; *(ia2+22*IMAGEWIDTH)=pixval2;
#define PIX18 PIX17 *(ia1-23*IMAGEWIDTH)=pixval1; *(ia2+23*IMAGEWIDTH)=pixval2;
#define PIX19 PIX18 *(ia1-24*IMAGEWIDTH)=pixval1; *(ia2+24*IMAGEWIDTH)=pixval2;
#define PIX1A PIX19 *(ia1-25*IMAGEWIDTH)=pixval1; *(ia2+25*IMAGEWIDTH)=pixval2;
#define PIX1B PIX1A *(ia1-26*IMAGEWIDTH)=pixval1; *(ia2+26*IMAGEWIDTH)=pixval2;
#define PIX1C PIX1B *(ia1-27*IMAGEWIDTH)=pixval1; *(ia2+27*IMAGEWIDTH)=pixval2;
#define PIX1D PIX1C *(ia1-28*IMAGEWIDTH)=pixval1; *(ia2+28*IMAGEWIDTH)=pixval2;
#define PIX1E PIX1D *(ia1-29*IMAGEWIDTH)=pixval1; *(ia2+29*IMAGEWIDTH)=pixval2;
#define PIX1F PIX1E *(ia1-30*IMAGEWIDTH)=pixval1; *(ia2+30*IMAGEWIDTH)=pixval2;
#define PIX20 PIX1F *(ia1-21*IMAGEWIDTH)=pixval1; *(ia2+31*IMAGEWIDTH)=pixval2;

#define LOOPINIT for(j=tex->divtabh[hi][0];j;j--) { pixval1 = *t1++; pixval2 = *--t2;

#define LOOPEXIT }
#define JMPDEF if(*jmpt++) { ia1 -= IMAGEWIDTH; *ia1  = pixval1; *ia2  = pixval2; ia2 += IMAGEWIDTH; }
 
#define CASE01 LOOPINIT PIX01 ia1-=IMAGEWIDTH; ia2+=IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE02 LOOPINIT PIX02 ia1-=2*IMAGEWIDTH; ia2+=0x2*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE03 LOOPINIT PIX03 ia1-=3*IMAGEWIDTH; ia2+=0x3*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE04 LOOPINIT PIX04 ia1-=4*IMAGEWIDTH; ia2+=0x4*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE05 LOOPINIT PIX05 ia1-=5*IMAGEWIDTH; ia2+=0x5*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE06 LOOPINIT PIX06 ia1-=6*IMAGEWIDTH; ia2+=0x6*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE07 LOOPINIT PIX07 ia1-=7*IMAGEWIDTH; ia2+=0x7*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE08 LOOPINIT PIX08 ia1-=8*IMAGEWIDTH; ia2+=0x8*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE09 LOOPINIT PIX09 ia1-=9*IMAGEWIDTH; ia2+=0x9*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE0A LOOPINIT PIX0A ia1-=0xa*IMAGEWIDTH; ia2+=0xa*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE0B LOOPINIT PIX0B ia1-=0xb*IMAGEWIDTH; ia2+=0xb*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE0C LOOPINIT PIX0C ia1-=0xc*IMAGEWIDTH; ia2+=0xc*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE0D LOOPINIT PIX0D ia1-=0xd*IMAGEWIDTH; ia2+=0xd*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE0E LOOPINIT PIX0E ia1-=0xe*IMAGEWIDTH; ia2+=0xe*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE0F LOOPINIT PIX0F ia1-=0xf*IMAGEWIDTH; ia2+=0xf*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE10 LOOPINIT PIX10 ia1-=0x10*IMAGEWIDTH; ia2+=0x10*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE11 LOOPINIT PIX11 ia1-=0x11*IMAGEWIDTH; ia2+=0x11*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE12 LOOPINIT PIX12 ia1-=0x12*IMAGEWIDTH; ia2+=0x12*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE13 LOOPINIT PIX13 ia1-=0x13*IMAGEWIDTH; ia2+=0x13*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE14 LOOPINIT PIX14 ia1-=0x14*IMAGEWIDTH; ia2+=0x14*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE15 LOOPINIT PIX15 ia1-=0x15*IMAGEWIDTH; ia2+=0x15*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE16 LOOPINIT PIX16 ia1-=0x16*IMAGEWIDTH; ia2+=0x16*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE17 LOOPINIT PIX17 ia1-=0x17*IMAGEWIDTH; ia2+=0x17*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE18 LOOPINIT PIX18 ia1-=0x18*IMAGEWIDTH; ia2+=0x18*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE19 LOOPINIT PIX19 ia1-=0x19*IMAGEWIDTH; ia2+=0x19*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE1A LOOPINIT PIX1A ia1-=0x1a*IMAGEWIDTH; ia2+=0x1a*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE1B LOOPINIT PIX1B ia1-=0x1b*IMAGEWIDTH; ia2+=0x1b*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE1C LOOPINIT PIX1C ia1-=0x1c*IMAGEWIDTH; ia2+=0x1c*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE1D LOOPINIT PIX1D ia1-=0x1d*IMAGEWIDTH; ia2+=0x1d*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE1E LOOPINIT PIX1E ia1-=0x1e*IMAGEWIDTH; ia2+=0x1e*IMAGEWIDTH; JMPDEF LOOPEXIT
#define CASE1F LOOPINIT PIX1F ia1-=0x1f*IMAGEWIDTH; ia2+=0x1f*IMAGEWIDTH; JMPDEF LOOPEXIT



/* find the right code, (in asm this would be a simple thing) */
/* works like a switch(ln) { case 1: ...... case 0x1f: };
/* btw: the code generated by the gcc with switch is better than this */

#define REGVAL 0x1

if(ln &= REGVAL) /* 0x1 */
{
  if(ln>>=REGVAL)
  {
    if(ln &= REGVAL) /* 0x2 */
    {
      if(ln>>=REGVAL) 
      {
        if(ln &= REGVAL) /* 0x4 */
        {
          if(ln>>=REGVAL)
          {
            if(ln &= REGVAL) /* 0x8 */
            {
              if(ln>>=REGVAL)
              {
                CASE1F
              }
              else
              {
                CASE0F 
              }
            }
            else /* !0x80 */
            {
              CASE17
            } 
          }
          else
          {
            CASE07
          }
        }
        else /* !0x40 */
        {
          ln>>=REGVAL;
          if(ln &= REGVAL)
          {
            if(ln>>=REGVAL)
            {
              CASE1B
            }
            else
            {
              CASE0B
            }
          }
          else
          {
            CASE13
          }
        }
      }
      else
      {
        CASE03
      }
    }
    else /* !0x2 */
    {
      ln>>=REGVAL;
      if(ln &= REGVAL) /* 0x4 */
      {
        if(ln>>=REGVAL)
        {
          if(ln &= REGVAL)
          {
            if(ln>>=PIXVAL)
            {
              CASE1C
            }
            else
            {
              CASE0C
            }
          }
          else
          {
            CASE15
          }
        }
        else
        {
          CASE05
        }
      }
      else /* !0x4 */
      {
        ln>>=REGVAL;
        if(ln &= REGVAL)
        {
          if(ln>>=REGVAL)
          {
            CASE19
          }
          else
          {
            CASE09
          }
        }
        else
        {
          CASE11
        }
      }
    }
  }
  else
  {
    CASE01
  }
}
else /* !0x1 */
{
  ln>>=REGVAL;
  if(ln &= REGVAL) /* 0x2 */
  {
    if(ln>>=REGVAL)
    {
      if(ln &= REGVAL) /* 0x4 */
      {
        if(ln>>=REGVAL)
        {
          if(ln &= REGVAL)
          {
            if(ln>>=REGVAL)
            {
              CASE1E
            }
            else
            {
              CASE0E
            }
          }
          else
          {
            CASE16
          }
        }
        else
        {
          CASE06
        }
      }
      else /* !0x4 */
      {
        
      }
    }
    else
    {
      CASE02
    }
  }
  else /* !0x2 */
  {
  }
}
