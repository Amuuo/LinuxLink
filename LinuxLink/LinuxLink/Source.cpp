#include<Windows.h>
#include<Windowsx.h>
#include<tchar.h>
#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<WinSock.h>
#include<string>
#include<iostream>
#include<conio.h>
#include<string>
#pragma comment(lib, "Ws2_32.lib")
#define CHARCODE ((lParam>>16)&0x00ff)
#define KEYDOWN 0x8000
#define KEYUP   0x4000

void setupSocketProtocols();
void listenForConnections();
void acceptConnection();
void bindToSocket();
void connectSocket();
void initializeWinsock();
void initializeConnection(HWND);
LPTHREAD_START_ROUTINE watchSignalThread();

static TCHAR  szWindowClass[] = _T("win32app");
static TCHAR  szTitle[] = _T("LinuxLink");
SOCKADDR_IN   svr;
SOCKADDR_IN   cli;
SOCKADDR_IN   sig;
HINSTANCE     hInst;
const int     local_port = 5600;
SOCKET        mouseSock;
SOCKET        connectSock;
SOCKET        signalSock;
WSADATA       winSock;

char    signl[20];
int     c;      
int16_t x{0};
int16_t y{0};
int16_t prev_x{0};
int16_t prev_y{0};
uint8_t size_of_input{0};

struct input {
  uint8_t  mData[3];
  uint16_t kData = 0;
  uint8_t  mWheel = 0;
} input;

//uint16_t dataToSend;



LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {

  static HANDLE hThread;
  static DWORD threadID;
  static char hex[20];

	switch (Msg) {			

    case WM_CREATE:
      initializeConnection(hwnd);
		  break;
    
    case WM_MOUSEMOVE: 
		  input.mData[1] = (lParam & 0xffff)       - prev_x;
		  input.mData[2] = ((lParam>>16) & 0xffff) - prev_y;
		  break;
	  
    case WM_MOUSEWHEEL  : input.mWheel = GET_WHEEL_DELTA_WPARAM(wParam); break;
    case WM_LBUTTONDOWN : input.mData[0] ^= 0x1; break;
    case WM_LBUTTONUP   : input.mData[0] ^= 0x1; break;
    case WM_RBUTTONDOWN : input.mData[0] ^= 0x2; break;
    case WM_RBUTTONUP   : input.mData[0] ^= 0x2; break;
    case WM_KEYDOWN     : input.kData = CHARCODE+KEYDOWN; break;
    case WM_KEYUP       : input.kData = CHARCODE+KEYUP;   break;
    case WM_SYSKEYDOWN  : input.kData = CHARCODE+KEYDOWN; break;
    case WM_SYSKEYUP    : input.kData = CHARCODE+KEYUP;   break;

    case WM_DESTROY:  
		  send(signalSock, (char*)"exit", 4, 0); 
		  Sleep(1000);
		  PostQuitMessage(0); 
		  break;
		
    case WM_CLOSE:
		  send(signalSock, (char*)"exit", 4, 0); 
		  Sleep(1000);
		  PostQuitMessage(0);
		  break;
    
    default:       
      return DefWindowProc(hwnd, Msg, wParam, lParam); break;
	}
	
  send(mouseSock, (char*)&input, size_of_input, 0);
  prev_x = (lParam & 0xffff);
  prev_y = ((lParam>>16) & 0xffff);
  memset(&input.mData[1], 0, 2);
  memset(&input.kData, 0, 2);
  memset(&input.mWheel, 0, 2);
	
  return 0;
}



int CALLBACK WinMain(HINSTANCE hInstance, 
                     HINSTANCE hPrevInstance, 
                     LPSTR lpCmdLine, 
                     int nCmdShow) {
	                                            
	WNDCLASSEX wc;
	
	size_of_input = sizeof(input);


	// SET UP WINDOW CLASS
	wc.cbSize         = sizeof(WNDCLASSEX);
	wc.style          = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc    = WndProc;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hInstance      = hInstance;
	wc.hIcon          = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName   = NULL;
	wc.lpszClassName  = szWindowClass;
	wc.hIconSm        = LoadIcon(wc.hInstance, IDI_APPLICATION);

	// REGISTER WINDOW CLASS
	if (!RegisterClassEx(&wc)) {
		MessageBox(
			NULL, 
			_T("Call to RegisterClassEx failed!"),
			_T("Window Desktop Guided Tour"), 
			MB_OK);

		return 1;
	}
	
	hInst = hInstance;
	
	// CREATE WINDOW
	HWND hwnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_POPUP,
		500, 500,
		400, 150,
		NULL,
		NULL,
		hInstance,
		NULL
	);

  if (!hwnd) {
		MessageBox(
			NULL,
			_T("Call to CreateWindow failed!"),
			_T("Windows Desktop Guided Tour"),
			MB_OK);

		return 1;
	}

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage (&msg);
	}

	return (int)msg.wParam;

}


void bindToSocket() {
	if (bind(connectSock, (SOCKADDR*)&svr, sizeof(svr)) == SOCKET_ERROR)
		printf("\nBind failed with error code: %d", WSAGetLastError());
}
void connectSocket() {
	if ((connectSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("\n>> Count not create socket : \n");
  printf("\n>> Connected to Socket...");
}
void initializeWinsock() {	
	if (WSAStartup(MAKEWORD(2, 2), &winSock) != 0)
		printf("\nFailed to initialize");
  printf("\n>> Initialized Winsock...");
}
void initializeConnection(HWND hwnd) {
  static PAINTSTRUCT ps;
  static HDC hdc;
  static const char* tmp_message = "\n\n\n       Run WinLink on Linux";
  static RECT clientRect;
  static HBRUSH  hBrush;
  static HRGN    bgRgn;
  
  ShowWindow  (hwnd, SW_SHOW);	    
      
  initializeWinsock();	
	setupSocketProtocols();	
	connectSocket();
	bindToSocket();
	listenForConnections();      
      
  GetClientRect(hwnd, &clientRect);
  hdc = BeginPaint(hwnd, &ps);
  bgRgn = CreateRectRgnIndirect(&clientRect);
  hBrush = CreateSolidBrush(RGB(75, 53, 118));
  FillRgn(hdc, bgRgn, hBrush);
  DrawText(hdc, tmp_message, strlen(tmp_message), &clientRect, 0);
  EndPaint(hwnd, &ps);
      
  acceptConnection();

  Beep(523, 75);
  Beep(987, 75);
  Beep(783, 250);
  Beep(1567, 750);
  
  MoveWindow  (hwnd, 3000, 500, 500, 500, true);
  ShowWindow  (hwnd, SW_MAXIMIZE);
	UpdateWindow(hwnd);      

}
LPTHREAD_START_ROUTINE watchSignalThread() {
  recv(signalSock, signl, 20, 0);
  exit(1);
}
void setupSocketProtocols() {      
	memset(&svr, 0, sizeof(SOCKADDR_IN));
	svr.sin_addr.s_addr  = inet_addr("192.168.1.4");
	svr.sin_family       = AF_INET;
	svr.sin_port         = htons(local_port);
  printf("\n>> Protocols established...");
}
void listenForConnections() {
	if(listen(connectSock, 2) < 0){
    printf("\n>> Failed to listen. Exiting...");
    exit(1);
  }
	printf("\n>> Waiting for incoming connections...");
}
void acceptConnection() {
	c = sizeof(SOCKADDR_IN);
  
	if ((mouseSock = accept(connectSock, (SOCKADDR*)&cli, &c)) == INVALID_SOCKET)
		printf("\nAccept failed with error code: %d", WSAGetLastError());
  printf("\n>> ");

  if ((signalSock = accept(connectSock, (SOCKADDR*)&sig, &c)) == INVALID_SOCKET)
		printf("\nAccept failed with error code: %d", WSAGetLastError());
  printf("\n>> ");
  
  
  //Sleep(500);
  FreeConsole();
}

