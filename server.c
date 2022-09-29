#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<windows.h>
#include<process.h>
#include<time.h>
#define SENSOR_ROW 3
#define SENSOR_COL 3
#define MAX_CLNT 256
#define SAFE_START 0.6 // ���� ���� ����
#define SAFE_END 8.8  // ���� ���� ��
#define random() ((double)rand() / (RAND_MAX + 1))* 1000 // 0 ~ 20.0
/*Ŀ�� �����(0) or ���̱�(1) */
void CursorView(char show) {
	HANDLE hConsole;
	CONSOLE_CURSOR_INFO ConsoleCursor;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	ConsoleCursor.bVisible = show;
	ConsoleCursor.dwSize = 1;

	SetConsoleCursorInfo(hConsole, &ConsoleCursor);
}
/*�ܼ� Ŀ�� ��ġ �̵�*/
void gotoxy(int x, int y) {
	COORD pos = { x,y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
void ErrorHandling(char* msg);
unsigned WINAPI getClntSensor(void* arg);
unsigned WINAPI printSensor(void* arg);
typedef struct sensor {
	double value; // ���� ��
	int isEmergency; // ��� ��Ȳ ����
	int x; //������ 2���� �迭 �ε���
	int y;
	int index; // ������ 1���� �迭 �ε���
	int isShow; // ���� ȭ�鿡 ������ ����������
}sensor;

/*���� ����*/
typedef struct hClntSockInfo {
	SOCKET hSock;
	int index;
}hClntSockInfo;

sensor clntSensorList[MAX_CLNT][SENSOR_ROW][SENSOR_COL];
hClntSockInfo clntSocks[MAX_CLNT];
long long scores[MAX_CLNT];
HANDLE hMutex;

int clntCnt = 0;

int main(void) {
	system("mode con cols=120 lines=35 | title ���� ���� ���� ����"); // �ܼ�â ũ�� �� ���� ����

	CursorView(FALSE); // Ŀ�� �����
	WSADATA wsaData;
	SOCKET hServSock;
	SOCKADDR_IN servAdr, clntAdr;
	HANDLE hThread;
	hClntSockInfo temp;
	int clntAdrSz;


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hMutex = CreateMutex(NULL, FALSE, NULL);
	
	hServSock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9190);

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() error");

	hThread = (HANDLE)_beginthreadex(NULL, 0, printSensor, NULL, 0, NULL);
	printf("Server >> Ŭ���̾�Ʈ ���� ��û ���\n");
	while (1) {

		clntAdrSz = sizeof(clntAdr);
		temp.hSock = accept(hServSock, (SOCKADDR*)&clntAdr, &clntAdrSz);
		if (temp.hSock == -1)
			ErrorHandling("accept() error");


		WaitForSingleObject(hMutex, INFINITE);

		send(temp.hSock, (char*)&clntCnt, sizeof(clntCnt), 0);
		clntSocks[clntCnt].hSock = temp.hSock;
		temp.index = clntCnt;
		clntSocks[clntCnt].index = clntCnt++;
		ReleaseMutex(hMutex);

		hThread = (HANDLE)_beginthreadex(NULL, 0, getClntSensor, (void*)&clntSocks[temp.index], 0, NULL);
		
	
		
	}

	Sleep(10000);

	closesocket(hServSock);
	WSACleanup();


	return 0;
}



unsigned WINAPI getClntSensor(void* arg) {
	hClntSockInfo clntSock = *((hClntSockInfo*)arg);
	while (TRUE) {
		int x, y;
		int flag;
		
		int ret = recv(clntSock.hSock, (char*)&x, sizeof(x), 0);
		if (ret <= 0) {
			
			closesocket(clntSocks[clntSock.index].hSock);
			clntSocks[clntSock.index].hSock = NULL;
			
			return 0;
		}
		
		recv(clntSock.hSock, (char*)&y, sizeof(y), 0);
		recv(clntSock.hSock, (char*)&clntSensorList[clntSock.index][x][y], sizeof(sensor), 0);
		recv(clntSock.hSock, (char*)&scores[clntSock.index], sizeof(long long), 0);
	
		double val = clntSensorList[clntSock.index][x][y].value;
		int control = FALSE;
		if (val <= SAFE_START || val >= SAFE_END) { //�������� �Ǻ�
			clntSensorList[clntSock.index][x][y].isEmergency = TRUE;
			control = TRUE;
			send(clntSock.hSock, (char*)&control, sizeof(control), 0);
			send(clntSock.hSock, (char*)&clntSensorList[clntSock.index][x][y], sizeof(sensor), 0);
		}
		else {
			send(clntSock.hSock, (char*)&control, sizeof(control), 0);
		}
	}
	return 0;
}

unsigned WINAPI printSensor(void* arg) {
	while (clntCnt == 0);
	Sleep(500);
	while (TRUE) {
		
		time_t timer;
		struct tm* t;
		timer = time(NULL); // 1970�� 1�� 1�� 0�� 0�� 0�ʺ��� �����Ͽ� ��������� ��
		t = localtime(&timer); // �������� ���� ����ü�� �ֱ�
		printf("[�α� �ð�] %d�� %d�� %d�� %d�� %d�� %d��\n",
			t->tm_year + 1900,
			t->tm_mon + 1,
			t->tm_mday,
			t->tm_hour,
			t->tm_min,
			t->tm_sec);
		for (int k = 0; k < clntCnt; k++) {
			if (clntSocks[k].hSock == NULL) continue; // ���ŵ� Ŭ���̾�Ʈ�� ����
			printf("Ŭ���̾�Ʈ %02d>   Game Score : %lld \n\n", k + 1, scores[k]);
			for (int i = 0; i < SENSOR_ROW; i++) {
				for (int j = 0; j < SENSOR_COL; j++) {
					if (!clntSensorList[k][i][j].isEmergency) {
						printf("����%02d: %.1lf\n", SENSOR_ROW * i + j + 1, clntSensorList[k][i][j].value);
					}
					else {
						printf("����%02d: %.1lf -> control ��ɾ� ����\n", SENSOR_ROW * i + j + 1, clntSensorList[k][i][j].value);
					}
				}
			}
			printf("\n\n");
		}
		
		Sleep(2000);
	}
}

void ErrorHandling(char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}