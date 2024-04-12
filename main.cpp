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
int tty_raw(int fd);   /* put terminal into a raw mode */
int tty_reset(int fd);   /* restore terminal's mode */
  
/* Read 1 character - echo defines echo mode */
char getch() { // 사용자로부터 키 입력에 바로 반응.
  char ch; 
  int n;
  while (1) { // 무한루프
    tty_raw(0); // raw mode로 설정
    n = read(0, &ch, 1); // 한 바이트를 읽어서 ch 변수에 저장. 읽은 바이트 수를 n에 저장
    tty_reset(0); // 터미널 저장
    if (n > 0) // 한 바이트 이상을 성공적으로 읽었는지 확인
      break;
    else if (n < 0) { // read함수 호출 실패 시
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

// 프로그램이 Ctrl+C를 눌러도 멈추지 않게 하는 역할
// sigint 신호 발생 시 이 함수가 호출되어 프로그램 계속 실행
void sigint_handler(int signo) {
  // cout << "SIGINT received!" << endl;
  // do nothing;
}
// alarm함수를 사용한 타이머 만료되었을 때
// 시간에 따라 상태 갱신
void sigalrm_handler(int signo) {
  alarm(1);
  saved_key = 's'; // 사용자가 s 키를 누른 것처럼 처리
}

void registerInterrupt() {
  struct sigaction act, oact; //act:새로운 신호처리설정, oact:이전 설정 저장
  act.sa_handler = sigint_handler; //SIGINT신호 수신 시 함수 호출
  sigemptyset(&act.sa_mask); //act.sa_mask 초기화하여 sigint_handler 
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

int T0D0[] = { 1, 1, 1, 1, -1 };  // -모양
int T0D1[] = { 1, 1, 1, 1, -1 };
int T0D2[] = { 1, 1, 1, 1, -1 };
int T0D3[] = { 1, 1, 1, 1, -1 };

int T1D0[] = { 0, 1, 0, 1, 1, 1, 0, 0, 0, -1 }; // T모양
int T1D1[] = { 0, 1, 0, 0, 1, 1, 0, 1, 0, -1 };
int T1D2[] = { 0, 0, 0, 1, 1, 1, 0, 1, 0, -1 };
int T1D3[] = { 0, 1, 0, 1, 1, 0, 0, 1, 0, -1 };

int T2D0[] = { 1, 0, 0, 1, 1, 1, 0, 0, 0, -1 }; // L모양
int T2D1[] = { 0, 1, 1, 0, 1, 0, 0, 1, 0, -1 };
int T2D2[] = { 0, 0, 0, 1, 1, 1, 0, 0, 1, -1 };
int T2D3[] = { 0, 1, 0, 0, 1, 0, 1, 1, 0, -1 };

int T3D0[] = { 0, 0, 1, 1, 1, 1, 0, 0, 0, -1 }; // L반대모양
int T3D1[] = { 0, 1, 0, 0, 1, 0, 0, 1, 1, -1 };
int T3D2[] = { 0, 0, 0, 1, 1, 1, 1, 0, 0, -1 };
int T3D3[] = { 1, 1, 0, 0, 1, 0, 0, 1, 0, -1 };

int T4D0[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 }; // Z모양 
int T4D1[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };
int T4D2[] = { 0, 1, 0, 1, 1, 0, 1, 0, 0, -1 };
int T4D3[] = { 1, 1, 0, 0, 1, 1, 0, 0, 0, -1 };

int T5D0[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 }; // S모양 
int T5D1[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };
int T5D2[] = { 0, 1, 0, 0, 1, 1, 0, 0, 1, -1 };
int T5D3[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, -1 };

int T6D0[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1 }; // 4개씩 보기 ㅁ모양
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
      else if (array[y][x] == 1) 
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

#define SCREEN_DY  10
#define SCREEN_DX  10
#define SCREEN_DW  4

#define ARRAY_DY (SCREEN_DY + SCREEN_DW)
#define ARRAY_DX (SCREEN_DX + 2*SCREEN_DW)

int arrayScreen[ARRAY_DY][ARRAY_DX] = { // 높이 14, 가로 18
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
};

void deleteFullLines()
{
  Matrix *iScreen = new Matrix((int*) arrayScreen, ARRAY_DY,ARRAY_DX);
  Matrix *full_check[13];
  Matrix *a_line_Blk;
  Matrix *top_line_Matrix = new Matrix(1, 16);

  for (int k=0; k<13; k++) {
    a_line_Blk = new Matrix(arrayScreen[k], 1, 16);
    full_check[k] = a_line_Blk->clip(0,0,1,16);
    delete a_line_Blk;

    if (full_check[k]->sum()==16) {
      for (int j=k; j>=1; j--) {
        iScreen -> paste(full_check[j-1], j, 0); }
    for (int i=0; i<16;i++) {
      top_line_Matrix->get_array()[0][i] = (i<3 || i>12) ? i : 0; }
    iScreen -> paste(top_line_Matrix,0,0);
    for (int i=0; i<ARRAY_DY;i++) {
      for (int j=0; j<ARRAY_DX; j++) {
          arrayScreen[i][j] = iScreen->get_array()[i][j]; }
      }
    }
  }
  for (int i=0; i<13;i++) {
    delete full_check[i]; }

  delete iScreen;
  delete top_line_Matrix;
}

int main(int argc, char *argv[]) {
  char key;
  int top = 0, left = 8;
  bool newBlockNeeded = false;

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
  // iScreen에서 currBlk이 차지할 영역을 잘라내서 tempBlk라는 객체 생성
  Matrix *tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
  // 화면에서 블록을 그리는 작업 잘라낸 부분+추가할 블록
  Matrix *tempBlk2 = tempBlk->add(currBlk);
  delete tempBlk;
  //iScreen의 복사해서 oScreen에 할당
  //최종화면 준비
  Matrix *oScreen = new Matrix(iScreen);
  //최종 화면에 현재 블록 표시
  oScreen->paste(tempBlk2, top, left);
  delete tempBlk2;
  drawScreen(oScreen, SCREEN_DW);
  delete oScreen;
  //delete iScreen;

  while ((key = getch()) != 'q') {
    int prevBlock = idxBlockDegree;
    switch (key) {
      case 'a': left--; break;
      case 'd': left++; break;
      case 's': top++; break;
      case 'w': idxBlockDegree=(idxBlockDegree+1)%4;
       break;
      case ' ': 
        tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
        tempBlk2 = tempBlk->add(currBlk);
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
    tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
    tempBlk2 = tempBlk->add(currBlk);
    delete tempBlk;
  
    if (tempBlk2->anyGreaterThan(1)) {
      switch (key) {
        case 'a': left++; 
         cout << "(nAlloc, nFree) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ")" << endl;  
        break;
        case 'd': left--; break;
        case 's': 
          top--;
          newBlockNeeded=true;
          for (int i=0; i<currBlk->get_dy();i++){
            for (int j=0; j<currBlk->get_dx();j++){
              	if (currBlk->get_array()[i][j] == 1) {
                	if(arrayScreen[top+i][left+j] != 1 || arrayScreen[top+i][left+j] != 2){
                  		arrayScreen[top+i][left+j] = 1; }
                }
            }
          }

          deleteFullLines();
          break;
        case 'w': 
            idxBlockDegree = prevBlock;
            cout << "부딪"<<idxBlockType << endl ;  // 회전 취소
          break;
        case ' ':
          top--;
           cout << "(nAlloc, nFree) = (" << Matrix::get_nAlloc() << ',' << Matrix::get_nFree() << ")" << endl;  
          newBlockNeeded=true;
          for (int i=0; i<currBlk->get_dy();i++){
            for (int j=0; j<currBlk->get_dx();j++){
              	if (currBlk->get_array()[i][j] == 1) {
                	if(arrayScreen[top+i][left+j] != 1 || arrayScreen[top+i][left+j] != 2){
                  		arrayScreen[top+i][left+j] = 1;
                		}
              	 }
            	}
           }
          deleteFullLines();

          break;

        }
        delete tempBlk2;
        currBlk = setOfBlockObjects[idxBlockType][idxBlockDegree];
        tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
        tempBlk2 = tempBlk->add(currBlk);    
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
    delete iScreen;
    // delete oScreen;
    drawScreen(tempScreen, SCREEN_DW);
    //delete tempScreen;
    cout << "__________________________________________________" << endl;
    
    if (newBlockNeeded) {
      iScreen = new Matrix(oScreen);
      newBlockNeeded = false;
      // 난수 발생 코드 추가
      idxBlockType = rand() % MAX_BLK_TYPES; // 블록 종류를 무작위로 선택
      idxBlockDegree = rand() % MAX_BLK_DEGREES; // 블록의 회전 상태를 무작위로 선택 (예: 0~3)

      top = 0;
      left = 8;
      currBlk = new Matrix((int *) setOfBlockObjects, 3, 3);

      tempBlk = iScreen->clip(top, left, top + currBlk->get_dy(), left + currBlk->get_dx());
      tempBlk2 = tempBlk->add(currBlk);
      delete tempBlk;
      oScreen = new Matrix(iScreen);
      oScreen->paste(tempBlk2, top, left);
      delete iScreen;
      drawScreen(oScreen, SCREEN_DW);
      delete oScreen;
      
    }

  }

  delete iScreen;
  delete currBlk;

    // 프로그램 종료 전 메모리 해제
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
