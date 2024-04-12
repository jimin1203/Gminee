#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <termios.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "colors.h"
#include "Matrix.h"

using namespace std;


/**************************************************************/
/**************** Linux System Functions **********************/
/**************************************************************/


char saved_key = 0;
int tty_raw(int fd);	/* put terminal into a raw mode */
int tty_reset(int fd);	/* restore terminal's mode */
 
  
/* Read 1 character - echo defines echo mode */
char getch() {
  char ch;
  int n;
  while (1) {
    tty_raw(0);
    n = read(0, &ch, 1);
    tty_reset(0);
    if (n > 0)
      break;
    else if (n < 0) {
      if (errno == EINTR) {
        if (saved_key != 0) {
          ch = saved_key;
          saved_key = 0;
          break;
        }
      }
    }
  }
  return ch;
}

void sigint_handler(int signo) {
  // cout << "SIGINT received!" << endl;
  // do nothing;
}

void sigalrm_handler(int signo) {
  alarm(1);
  saved_key = 's';
}

void registerInterrupt() {
  struct sigaction act, oact;
  act.sa_handler = sigint_handler;
  sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
  act.sa_flags = SA_INTERRUPT;
#else
  act.sa_flags = 0;
#endif
  if (sigaction(SIGINT, &act, &oact) < 0) {
    cerr << "sigaction error" << endl;
    exit(1);
  }
}

void registerAlarm() {
  struct sigaction act, oact;
  act.sa_handler = sigalrm_handler;
  sigemptyset(&act.sa_mask);
#ifdef SA_INTERRUPT
  act.sa_flags = SA_INTERRUPT;
#else
  act.sa_flags = 0;
#endif
  if (sigaction(SIGALRM, &act, &oact) < 0) {
    cerr << "sigaction error" << endl;
    exit(1);
  }
  alarm(1);
}

/**************************************************************/
/**************** Tetris Blocks Definitions *******************/
/**************************************************************/
#define MAX_BLK_TYPES 7
#define MAX_BLK_DEGREES 4

int T0D0[] = { 1, 1, 1, 1, -1 };
int T0D1[] = { 1, 1, 1, 1, -1 };
int T0D2[] = { 1, 1, 1, 1, -1 };
int T0D3[] = { 1, 1, 1, 1, -1 };

int T1D0[] = { 0, 1, 0, 1, 1, 1, 0, 0, 0, -1 };
int T1D1[] = { 0, 1, 0, 0, 1, 1, 0, 1, 0, -1 };
int T1D2[] = { 0, 0, 0, 1, 1, 1, 0, 1, 0, -1 };
int T1D3[] = { 0, 1, 0, 1, 1, 0, 0, 1, 0, -1 };

int T2D0[] = { 1, 0, 0, 1, 1, 1, 0, 0, 0, -1 };
int T2D1[] = { 0, 1, 1, 0, 1, 0, 0, 1, 0, -1 };
int T2D2[] = { 0, 0, 0, 1, 1, 1, 0, 0, 1, -1 };
int T2D3[] = { 0, 1, 0, 0, 1, 0, 1, 1, 0, -1 };

int T3D0[] = { 0, 0, 1, 1, 1, 1, 0, 0, 0, -1 };
int T3D1[] = { 0, 1, 0, 0, 1, 0, 0, 1, 1, -1 };
int T3D2[] = { 0, 0, 0, 1, 1, 1, 1, 0, 0, -1 };
int T3D3[] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, -1 };

int T4D0[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 };
int T4D1[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };
int T4D2[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 };
int T4D3[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };

int T5D0[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 };
int T5D1[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };
int T5D2[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 };
int T5D3[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };

int T6D0[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1 };
int T6D1[] = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1 };
int T6D2[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1 };
int T6D3[] = { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1 };
  
int *setOfBlockArrays[] = {
  T0D0, T0D1, T0D2, T0D3,
  T1D0, T1D1, T1D2, T1D3,
  T2D0, T2D1, T2D2, T2D3,
  T3D0, T3D1, T3D2, T3D3,
  T4D0, T4D1, T4D2, T4D3,
  T5D0, T5D1, T5D2, T5D3,
  T6D0, T6D1, T6D2, T6D3,
};


void drawScreen(Matrix *screen, int wall_depth)
{
  int dy = screen->get_dy();
  int dx = screen->get_dx();
  int dw = wall_depth;
  int **array = screen->get_array();

  for (int y = 0; y < dy - dw + 1; y++) {
    for (int x = dw - 1; x < dx - dw + 1; x++) {
      if (array[y][x] == 0)
	      cout << "□ ";
      else if (array[y][x] == 1 || array[y][x] == 2)
	      cout << "■ ";
      else if (array[y][x] == 10)
	      cout << "◈ ";
      else if (array[y][x] == 20)
	      cout << "★ ";
      else if (array[y][x] == 30)
	      cout << "● ";
      else if (array[y][x] == 40)
	      cout << "◆ ";
      else if (array[y][x] == 50)
	      cout << "▲ ";
      else if (array[y][x] == 60)
	      cout << "♣ ";
      else if (array[y][x] == 70)
	      cout << "♥ ";
      else
	      cout << "X ";
    }
    cout << endl;
  }
}

  		
  

/**************************************************************/
/******************** Tetris Main Loop ************************/
/**************************************************************/

#define SCREEN_DY  15
#define SCREEN_DX  14
#define SCREEN_DW  3

#define ARRAY_DY (SCREEN_DY + 1)
#define ARRAY_DX (SCREEN_DX + 2)

int arrayScreen[ARRAY_DY][ARRAY_DX] = {
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
  };


void deleteFullLines()
{
  Matrix *iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
  Matrix *full_check[13];
  Matrix *a_line_Blk;
  Matrix *top_line_Matrix = new Matrix(1, 16);
  
  for (int k=0; k<13; k++) {
  	a_line_Blk = new Matrix(arrayScreen[k], 1, 16);
  	full_check[k] = a_line_Blk->clip(0, 0, 1, 16);
  	delete a_line_Blk;
  	
  	if (full_check[k] -> sum() == 16) {
  		for (int j=k; j>=1; j--) {
  			iScreen -> paste(full_check[j-1], j, 0); }	
		for (int i = 0; i < 16; i++) {
  			top_line_Matrix->get_array()[0][i] = (i < 3 || i > 12) ? 1 : 0; }
		iScreen->paste(top_line_Matrix, 0, 0);
  		for (int i=0; i<ARRAY_DY; i++) {
        		for (int j=0; j<ARRAY_DX; j++) {
          			arrayScreen[i][j] = iScreen->get_array()[i][j]; }	
         	}
  	}
  }
  for (int i=0; i<13;i++){
    	delete full_check[i];
  }
  delete iScreen;
  delete top_line_Matrix;
}



int main(int argc, char *argv[]) {

  char key;
  int blkType;
  int top=0, left=4;
  
  Matrix *setOfBlockObjects[MAX_BLK_TYPES][MAX_BLK_DEGREES];

  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 4; j++) {
      if ((i*4+j)>=0 && (i*4+j)<=3) {
        setOfBlockObjects[i][j] = new Matrix(setOfBlockArrays[i*4+j],2,2);
      }
      else if ((i*4+j)>=4 && (i*4+j)<=23) {
        setOfBlockObjects[i][j] = new Matrix(setOfBlockArrays[i*4+j],3,3);
      }
      else if ((i*4+j)>=24 && (i*4+j)<=27) {
        setOfBlockObjects[i][j] = new Matrix(setOfBlockArrays[i*4+j],4,4);
      }
    }
  }
  
  srand((unsigned int)time(NULL)); 
  int idxBlockType = rand() % MAX_BLK_TYPES; 
  int idxBlockDegree=0; 


  Matrix *iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
  Matrix *currBlk = setOfBlockObjects[idxBlockType][idxBlockDegree]; 		
  Matrix *tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
  Matrix *tempBlk2 = tempBlk->add(currBlk);
  delete tempBlk;
  Matrix *oScreen = new Matrix(iScreen);
  oScreen->paste(tempBlk2, top, left);
  delete tempBlk2; 
  drawScreen(oScreen, SCREEN_DW);
  delete oScreen;
  
  while ((key = getch()) != 'q') {
    int prevBlock = idxBlockDegree;
    switch (key) {
      case 'a': left--; break;
      case 'd': left++; break;
      case 's': top++; break;
      case 'w': idxBlockDegree = (idxBlockDegree + 1) % 4; 
      	break;
      case ' ': 
      		tempBlk=iScreen->clip(top, left, top+currBlk->get_dy(), left+currBlk->get_dx());
    		tempBlk2=tempBlk->add(currBlk);
    		delete tempBlk;
    		
      		while (!tempBlk2->anyGreaterThan(1)) {
      			top++;
        		tempBlk = iScreen->clip(top, left, top+currBlk->get_dy(), left+currBlk->get_dx());
    			delete tempBlk2;
    			tempBlk2 = tempBlk->add(currBlk);
    			delete tempBlk;
        		} 
        		delete tempBlk2; 
        		break;
      default: cout << "wrong key input" << endl;
    }
    
    currBlk = setOfBlockObjects[idxBlockType][idxBlockDegree];
    tempBlk=iScreen->clip(top, left, top+currBlk->get_dy(), left+currBlk->get_dx());
    tempBlk2=tempBlk->add(currBlk);
    delete tempBlk;
    
    if (tempBlk2->anyGreaterThan(1)) {
      
      switch (key) {
        case 'a': left++; break;
        case 'd': left--; break;
        case 's':
          	top--;
          	for(int i=0; i<currBlk->get_dy(); i++){
            		for(int j=0; j<currBlk->get_dx(); j++){
              			if (currBlk->get_array()[i][j] == 1) {
                			if(arrayScreen[top+i][left+j] != 1 || arrayScreen[top+i][left+j] != 2){
                  				arrayScreen[top+i][left+j] = 1; }
              			}
            		}
          	}

          deleteFullLines();

          top=0;
          left=4;
          idxBlockType = rand() % MAX_BLK_TYPES;
          idxBlockDegree = 0;
          break;
        case 'w': 
           idxBlockDegree=prevBlock; break;
        case ' ': 
           top--;
           
           for(int i=0; i<currBlk->get_dy(); i++){
           	for(int j=0; j<currBlk->get_dx(); j++){
              		if (currBlk->get_array()[i][j] == 1) {
                		if(arrayScreen[top+i][left+j] != 1 || arrayScreen[top+i][left+j] != 2){
                  			arrayScreen[top+i][left+j] = 1;
                		}
              		}
            	}
           }

          deleteFullLines();

          top=0;
          left=4;
          idxBlockType = rand() % MAX_BLK_TYPES;
          idxBlockDegree = 0;
          break;
        default: cout << "wrong key input" << endl;
      }
      delete tempBlk2;
      currBlk = setOfBlockObjects[idxBlockType][idxBlockDegree];
      tempBlk=iScreen->clip(top, left, top+currBlk->get_dy(), left+currBlk->get_dx());
      tempBlk2=tempBlk->add(currBlk);
      delete tempBlk;
    }
    delete iScreen;
    iScreen = new Matrix((int *) arrayScreen, ARRAY_DY, ARRAY_DX);
    oScreen = new Matrix(iScreen);
    Matrix *o;
    o = new Matrix(oScreen);
    o->mulc(0);
    o->paste(tempBlk2, top, left);
    delete tempBlk2;
    Matrix *tempScreen;
    tempScreen = oScreen->add(o);
    delete o;
    delete oScreen;
    drawScreen(tempScreen, SCREEN_DW);
    delete tempScreen;
    cout << "__________________________________________________" << endl;
    
    }
  delete iScreen;
  delete currBlk;
  
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 4; j++) {
    	if (i != idxBlockType || j !=idxBlockDegree ) {
    		delete setOfBlockObjects[i][j];
    	}
    }
  }

  cout << "(nAlloc, nFree) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ")" << endl;  
  cout << "Program terminated!" << endl;

  return 0;
}


