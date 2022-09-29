#include<stdio.h> 
#include<stdlib.h>
#include<string.h>
#include<windows.h> 
#include<process.h>
#include<conio.h>
#include<time.h>
#define SENSOR_ROW 3
#define SENSOR_COL 3
#define random() ((double)rand() / (RAND_MAX + 1))* 9 // 0 ~ 9.0

#define LEFT 75 // ←
#define RIGHT 77  // →
#define UP 72 // ↑
#define DOWN 80 // ↓

/*커서 숨기기(0) or 보이기(1) */
void CursorView(char show) {
	HANDLE hConsole;
	CONSOLE_CURSOR_INFO ConsoleCursor;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	ConsoleCursor.bVisible = show;
	ConsoleCursor.dwSize = 1;

	SetConsoleCursorInfo(hConsole, &ConsoleCursor);
}

/*콘솔 커서 위치 이동*/
void gotoxy(int x, int y) {
	COORD pos = { x,y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void ErrorHandling(char* msg);
int isContactSensor(int cx, int cy);
unsigned WINAPI generatorSensorValue(void* arg);
void printBoard();
void printMainMenu();
unsigned WINAPI monster(void* arg);

typedef struct sensor {
	double value; // 센서 값
	int isEmergency; // 긴급 상황 여부
	int x; //센서의 2차원 배열 인덱스
	int y;
	int index; // 센서의 1차원 배열 인덱스
	int isShow; // 현재 화면에 센서가 보여졌는지
}sensor;

sensor sensorArr[SENSOR_ROW][SENSOR_COL];
HANDLE tSensors[SENSOR_ROW * SENSOR_COL];
int clntNum;
SOCKET hSock;
HANDLE hMutex;

int board[1000][1000];
long long score = 0;
int main(void) {
	system("mode con cols=120 lines=35 | title 센서 제어 게임"); // 콘솔창 크기 및 제목 설정
	CursorView(FALSE); // 커서 숨기기
	printMainMenu();
	getchar();
	system("cls");
	WSADATA wsaData;
	SOCKADDR_IN servAdr;
	

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");
	hMutex = CreateMutex(NULL, FALSE, NULL);

	hSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9190);

	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");


	for (int i = 0; i < SENSOR_ROW; i++) {
		for (int j = 0; j < SENSOR_COL; j++) {
			sensorArr[i][j].value = random();
			sensorArr[i][j].index = SENSOR_ROW * i + j + 1;
			sensorArr[i][j].isEmergency = FALSE; //초깃값 안전
			sensorArr[i][j].x = i;
			sensorArr[i][j].y = j;

	
		}
	}
	recv(hSock, (char*)&clntNum, sizeof(clntNum), 0);



	for (int i = 0; i < SENSOR_ROW; i++) {
		for (int j = 0; j < SENSOR_COL; j++) {
			tSensors[i*SENSOR_ROW + j] = (HANDLE)_beginthreadex(NULL, 0, generatorSensorValue, (void*)&sensorArr[i][j], 0, NULL);
		}
	}

	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, monster, NULL, 0, NULL);

	while (TRUE);
	WSACleanup();

	

	return 0;
}

void printMainMenu() {
	const int x = 25;
	const int y = 3;
	gotoxy(x, y);
	printf("-------------------------------------------------------------------\n");
	gotoxy(x , y + 1);
	printf("                                                                   \n");
	gotoxy(x, y + 2);
	printf("-----------------------센서 제어하기 게임--------------------------\n");
	gotoxy(x, y + 3);
	printf("                                                                   \n");
	gotoxy(x, y + 4);
	printf("-------------------------------------------------------------------\n");
	gotoxy(x, y + 6);
	printf("               1. 게임방법 : 방향키로 움직여서 센서 제어\n");
	gotoxy(x, y + 8);
	printf("               2. 게임시작 : 엔터키 입력\n");                   

}

/*각 센서는 2초마다 랜덤한 값을 발생한다.*/
unsigned WINAPI generatorSensorValue(void *arg) {
	sensor* target = (sensor*)arg;
	srand((unsigned)time(NULL) * target->index);
	while (TRUE) {
		WaitForSingleObject(hMutex, INFINITE);
		if(!target->isEmergency) target->value = random();
	
		gotoxy(0, 0);
		printf("클라이언트 %02d>\n\n", clntNum + 1);

		for (int i = 0; i < SENSOR_ROW; i++) {
			for (int j = 0; j < SENSOR_COL; j++) {
				if (!sensorArr[i][j].isEmergency) {
					printf("센서%02d: %.1lf              \n", SENSOR_ROW * i + j + 1, sensorArr[i][j].value);
				}
				else {
					printf("센서%02d: %.1lf -> 긴급 상황!\n", SENSOR_ROW * i + j + 1, sensorArr[i][j].value);
				}
			}
		}
		send(hSock, (char*)&target->x, sizeof(target->x), 0);
		send(hSock, (char*)&target->y, sizeof(target->y), 0);
		send(hSock, (char*)target, sizeof(sensor), 0);
		send(hSock, (char*)&score, sizeof(score), 0);

		int control = FALSE;
		
		
		recv(hSock, (char*)&control, sizeof(control), 0);
		
		if (control) {
			recv(hSock, (char*)target, sizeof(sensor), 0);
			control = FALSE;
		}
		printBoard();
		ReleaseMutex(hMutex);
		Sleep(2000);

	}
	return 0;
}

/*센서 누르기 게임 보드를 출력한다.*/
void printBoard() {
	const int targetX = 50;
	const int targetY = 3;
	
	for (int i = 0; i <= 32; i++) {
		gotoxy(30, i);
		printf("|\n");
	}

	gotoxy(targetX, 0);

	printf("[센서 제어하기 게임]\n");
	
	for (int i = 0; i < SENSOR_ROW; i++) {
		for (int j = 0; j < SENSOR_COL; j++) {
			if (sensorArr[i][j].isEmergency && !sensorArr[i][j].isShow) {
				int randomX = targetX + random() * 2;
				int randomY = targetY + random() * 3;
				board[randomX][randomY] = sensorArr[i][j].index;
				gotoxy(
					randomX, randomY
				);
				printf("%d ", sensorArr[i][j].index);
				sensorArr[i][j].isShow = TRUE;
			}
		}
	}

	gotoxy(targetX * 2, 0);
	printf("  <SCORE>");
	gotoxy(targetX * 2, 1);
	printf("-----------");
	gotoxy(targetX * 2, 2);
	printf("     %lld   ", score);
	gotoxy(targetX * 2, 3);
	printf("-----------");



	printf("\n");
}

int isContactSensor(int cx, int cy) {
	if (board[cx][cy]) {
		return TRUE;
	}

	return FALSE;
}

unsigned WINAPI monster(void* arg) {
	int monsterX = 32;
	int monsterY = 15;
	WaitForSingleObject(hMutex, INFINITE);
	gotoxy(monsterX, monsterY);
	printf("C");
	ReleaseMutex(hMutex);
	while (TRUE) { // 방향키 입력 이벤트
		int nSelect;
		if (_kbhit()) {
			nSelect = _getch();
			if (nSelect == 224) {
				nSelect = _getch();
				switch (nSelect) {

				case UP: // 화살표 위 눌렀을 때
					WaitForSingleObject(hMutex, INFINITE);
					gotoxy(monsterX, monsterY);
					printf(" ");
					monsterY--;
					gotoxy(monsterX, monsterY);
					printf("C");
					if (isContactSensor(monsterX, monsterY)) { //만약 몬스터와 센서가 접촉했으면
						for (int i = 0; i < SENSOR_ROW; i++) {
							for (int j = 0; j < SENSOR_COL; j++) {
								if (board[monsterX][monsterY] == sensorArr[i][j].index) { //해당 센서 처리
									board[monsterX][monsterY] = 0;
									sensorArr[i][j].isEmergency = FALSE;
									sensorArr[i][j].isShow = FALSE;
									score += sensorArr[i][j].index;
									
								}
							}
						}

					}
					ReleaseMutex(hMutex);
					break;
				case DOWN: // 화살표 아래 눌렀을 때
					WaitForSingleObject(hMutex, INFINITE);
					gotoxy(monsterX, monsterY);
					printf(" ");
					monsterY++;
					gotoxy(monsterX, monsterY);
					printf("C");
					if (isContactSensor(monsterX, monsterY)) { //만약 몬스터와 센서가 접촉했으면
						for (int i = 0; i < SENSOR_ROW; i++) {
							for (int j = 0; j < SENSOR_COL; j++) {
								if (board[monsterX][monsterY] == sensorArr[i][j].index) { //해당 센서 처리
									board[monsterX][monsterY] = 0;
									sensorArr[i][j].isEmergency = FALSE;
									sensorArr[i][j].isShow = FALSE;
									score += sensorArr[i][j].index;

								}
							}
						}

					}
					ReleaseMutex(hMutex);
					break;
				case LEFT: // 화살표 왼쪽 눌렀을 떄
					WaitForSingleObject(hMutex, INFINITE);
					gotoxy(monsterX, monsterY);
					printf(" ");
					monsterX--;
					gotoxy(monsterX, monsterY);
					printf("C");
					if (isContactSensor(monsterX, monsterY)) { //만약 몬스터와 센서가 접촉했으면
						for (int i = 0; i < SENSOR_ROW; i++) {
							for (int j = 0; j < SENSOR_COL; j++) {
								if (board[monsterX][monsterY] == sensorArr[i][j].index) { //해당 센서 처리
									board[monsterX][monsterY] = 0;
									sensorArr[i][j].isEmergency = FALSE;
									sensorArr[i][j].isShow = FALSE;
									score += sensorArr[i][j].index;

								}
							}
						}

					}
					ReleaseMutex(hMutex);
					break;
				case RIGHT: // 화살표 오른쪽 눌렀을 때
					WaitForSingleObject(hMutex, INFINITE);
					gotoxy(monsterX, monsterY);
					printf(" ");
					monsterX++;
					gotoxy(monsterX, monsterY);
					printf("C");
					if (isContactSensor(monsterX, monsterY)) { //만약 몬스터와 센서가 접촉했으면
						for (int i = 0; i < SENSOR_ROW; i++) {
							for (int j = 0; j < SENSOR_COL; j++) {
								if (board[monsterX][monsterY] == sensorArr[i][j].index) { //해당 센서 처리
									board[monsterX][monsterY] = 0;
									sensorArr[i][j].isEmergency = FALSE;
									sensorArr[i][j].isShow = FALSE;
									score += sensorArr[i][j].index;

								}
							}
						}

					}
					ReleaseMutex(hMutex);
					break;
				default:
					break;
				}
			}
		}
	}


	return 0;
}
void ErrorHandling(char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}