/*
 * load and transform maze-datas
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "netmaze.h"
#include "defmaze.h"

static int work_maze(MAZE *mazeadd,char* cbuffer);

/************************************/
/* Speicher fuer Maze bereitstellen */
/************************************/

int create_maze(MAZE *mazeadd)
{
  int i;
  char *cbuffer,*buffer;

  mazeadd->xdim  = defmazelen>>1;
  mazeadd->ydim  = defmazelen>>1;
  mazeadd->setlist = NULL;
  mazeadd->bitlist = NULL;

  if ((buffer = cbuffer = (char *) malloc(10000U)) == NULL) return FALSE;

  for(i=0;i<defmazelen;i++,buffer += BPL)
  {
     strcpy(buffer,defmazedata[i]);
  }

  if( work_maze(mazeadd,cbuffer) == TRUE)
  {
    printf("Defaultmaze is ok!\n");
    free(cbuffer);
    return TRUE;
  }
  else
  {
    free(cbuffer);
    return FALSE;
  }
}

/******************************/
/* Maze laden & bearbeiten    */
/******************************/

int load_maze(char *name,MAZE *mazeadd)
{
  char *cbuffer,*buffer;
  FILE *handle;
  int dim,i,flag = !EOF,ret=FALSE;

  if ((cbuffer = buffer = (char *) malloc(16384U)) == NULL) return FALSE;

  if ( (handle = fopen(name,"r")) != NULL)
  {
    fscanf(handle,"%d\n",&dim);
    if(dim % 2 == 0) dim++;
    if ((dim >= 3) && (dim <= BPL-1) /* && (dim % 2 == 1) */ )
    {
      for(i=0;i<=dim;i++,cbuffer += BPL)
      {
         if ((flag = fscanf(handle,"%s",cbuffer)) == EOF) break;
      }

      fclose(handle);
      if(i == dim)
      {
        mazeadd->xdim = dim/2;
        mazeadd->ydim = dim/2;
        ret = work_maze(mazeadd,buffer);
      }
    }
  }
  free(buffer);
  return ret;
}

/*************************************************
 * translate ASCII-definition into internal format
 */

static int work_maze(MAZE *mazeadd,char* buffer)
{
  int (*hfeld)[MAZEDIMENSION],(*vfeld)[MAZEDIMENSION];
  int dimension,i,j,i1,j1,err_x=0,err_y=0,c;

  hfeld = mazeadd->hwalls;
  vfeld = mazeadd->vwalls;
  dimension = (mazeadd->xdim)*2+1;

  for(j1=0,j=0;(j<dimension/2+1) && (!err_x);j1+=2,j++)
  {
    for(i1=0,i=0;(i<dimension/2) && (!err_x);i1+=2,i++)
    {
      c = *(buffer+i1+1+j1*BPL);
      switch(c)
      {
        case 'X':
          if ( (*(buffer+i1+j1*BPL) != 'X') || (*(buffer+i1+2+j1*BPL) != 'X'))
          {
            err_x = i1+2; err_y = j1+1;
            break;
          }
          hfeld[j][i] = 0xf | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
          break;
        case '.':
          hfeld[j][i] = 0;
          break;
        default:
          if(c >= '1' && c <= '9')
          {
            if ( (*(buffer+i1+j1*BPL) != 'X') || (*(buffer+i1+2+j1*BPL) != 'X'))
            {
              err_x = i1+2; err_y = j1+1;
              break;
            }
            hfeld[j][i] = (c - '0') | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
          }
          else
          {
            err_x = i1+2; err_y = j1+1;
          }
          break;
      }

      c = *(buffer+j1+(i1+1)*BPL); 
      switch(c) 
      { 
        case 'X': 
          if((*(buffer+j1+i1*BPL) != 'X') || (*(buffer+j1+(i1+2)*BPL) != 'X')) 
          { 
            err_x = j1+1; err_y = i1+2; 
            break; 
          } 
          vfeld[i][j] = 0xe | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT; 
          break; 
        case '.': 
          vfeld[i][j] = 0; 
          break; 
        default: 
          if(c >= '1' && c <= '9') 
          { 
            if((*(buffer+j1+i1*BPL) != 'X') || (*(buffer+j1+(i1+2)*BPL) != 'X'))            
            { 
              err_x = j1+1; err_y = i1+2; 
              break; 
            }
            vfeld[i][j] = (c - '0') | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
          } 
          else 
          { 
            err_x = j1+1; err_y = i1+2; 
          } 
          break; 
      }
    }
  }

  if(err_x == 0)
    return TRUE;

  printf("mazeerror at: x(%d) y(%d)\n",err_x,err_y);
  return FALSE;
}


int random_maze(MAZE *mazeadd,int size,int wlen)
{
  int m[4][2] = { { 0,1 }, {1,0} , {-1,0} , {0,-1} };
  int (*hfeld)[MAZEDIMENSION],(*vfeld)[MAZEDIMENSION];
  int r,i,j,cnt;
  char bitlist[MAZEDIMENSION][MAZEDIMENSION];

  if((size < 2) || (size >= MAZEDIMENSION))
    return FALSE;

  hfeld = mazeadd->hwalls;
  vfeld = mazeadd->vwalls;

  memset((char*) hfeld,0,MAZEDIMENSION*MAZEDIMENSION*sizeof(int));
  memset((char*) vfeld,0,MAZEDIMENSION*MAZEDIMENSION*sizeof(int));
  memset(bitlist,0,MAZEDIMENSION*MAZEDIMENSION*sizeof(char));

  mazeadd->xdim  = size;
  mazeadd->ydim  = size;
  mazeadd->setlist = NULL;
  mazeadd->bitlist = NULL;

  for(i=0;i<size;i++)
  {
    vfeld[i][0] = vfeld[i][size] = 0xe | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
    hfeld[0][i] = hfeld[size][i] = 0xf | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;

    bitlist[i][0] = bitlist[0][i] =
    bitlist[size][i] = bitlist[i][size] = 1;
  }

  if(wlen > 0)
  {
    int x=0,y=0,l=0;
    r = (size-1)*(size-1);
    cnt = 30000;
    while( (r>1) && (--cnt>0) )
    {
      if(l == 0)
      {
        x = (rand() % (size-1)) + 1;
        y = (rand() % (size-1)) + 1;
        if(!bitlist[x][y])
        {
          bitlist[x][y] = 1; r--;
          l = wlen;
        }
      }
      if(l > 0)
      {
        int m1[4][2];
        for(i=0,j=0;i<4;i++)
        {
          m1[j][0] = x+m[i][0]; m1[j][1] = y+m[i][1];
          if(!bitlist[m1[j][0]][m1[j][1]]) j++;
        }
        if(j)
        {
          l--;
          j = rand() % j;
          if(x != m1[j][0])
          {
            if(x<m1[j][0])
              hfeld[y][x] = 0xf | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
            else
              hfeld[y][m1[j][0]] = 0xf | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
          }
          else
          {
            if(y<m1[j][1])
              vfeld[y][x] = 0xe | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
            else
              vfeld[m1[j][1]][x] = 0xe | MAZE_BLOCK_PLAYER | MAZE_BLOCK_SHOT;
          }
          x = m1[j][0]; y = m1[j][1];
          bitlist[x][y] = 1; r--;
        }
        else
          l = 0;
      }
    }
  }

  return TRUE;
}

