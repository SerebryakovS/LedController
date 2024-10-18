#include "led-matrix.h"
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "graphics.h"
#include <cmath>

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

std::string FontsPath;

using namespace rgb_matrix;

std::string GetIpAddress() {
    struct ifaddrs *InterfaceAddressList, *CurrentInterface;
    std::string IpAddress;
    if (getifaddrs(&InterfaceAddressList) == -1) {
        perror("GetIPAddressesError");
        return "";
    };
    for (CurrentInterface = InterfaceAddressList; CurrentInterface != NULL; CurrentInterface = CurrentInterface->ifa_next) {
        if (CurrentInterface->ifa_addr == NULL) {
            continue;
        };
        int Family = CurrentInterface->ifa_addr->sa_family;
        if (Family == AF_INET) {
            char AddressBuffer[INET_ADDRSTRLEN];
            void *TempAddressPointer = &((struct sockaddr_in *)CurrentInterface->ifa_addr)->sin_addr;
            if (!inet_ntop(Family, TempAddressPointer, AddressBuffer, sizeof(AddressBuffer))) {
                continue;
            };
            if (strcmp(CurrentInterface->ifa_name, "lo") != 0) {
                IpAddress = AddressBuffer;
                break;
            };
        };
    };
    freeifaddrs(InterfaceAddressList);
    return IpAddress;
};

void DrawIPAddress(RGBMatrix *Matrix, RGBMatrix::Options *MatrixOptions, const std::string &IpAddress, FrameCanvas *OffscreenCanvas) {
    rgb_matrix::Font Font; 
	if (!Font.LoadFont((FontsPath + "4x6.bdf").c_str())){
		printf("_____[ERR]: Could not load font..\n");
	};
    int TextLength = 4 * IpAddress.length(); 
	int XOffset = Matrix->width()/2 - TextLength / 2;
    Color _Color(0, 255, 0);
	rgb_matrix::DrawText(OffscreenCanvas, Font, XOffset+1, MatrixOptions->rows * 4/5 , _Color, nullptr, IpAddress.c_str(), 0);
};

volatile bool InterruptReceived = false;
static void InterruptHandler(int signo) {
    InterruptReceived = true;
};

struct RGB {
    uint8_t Red, Green, Blue;
};

bool DisplayLogoPatternCenter(RGBMatrix *Matrix, RGBMatrix::Options *MatrixOptions, FrameCanvas *OffscreenCanvas) {
    OffscreenCanvas->Clear();
    RGB ColorOne = {21, 127, 255}; 
    RGB ColorTwo = {0,  255, 145}; 
    const int LogoPattern[16][64] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    };

    int YOffset = Matrix->height() / 2 - 8;
	int XOffset = 0;
	if (MatrixOptions->cols > 64){
		XOffset = (MatrixOptions->cols-64)/2;
	};
    for (int Y = 0; Y < 16; ++Y) {
        for (int X = 0; X < 64; ++X) {
            if (LogoPattern[Y][X] == 0) {
                OffscreenCanvas->SetPixel(X + XOffset, Y + YOffset, ColorOne.Red, ColorOne.Green, ColorOne.Blue);
            } else if (LogoPattern[Y][X] == 2) {
                OffscreenCanvas->SetPixel(X + XOffset, Y + YOffset, ColorTwo.Red, ColorTwo.Green, ColorTwo.Blue);
            } else {
                OffscreenCanvas->SetPixel(X + XOffset, Y + YOffset, 0, 0, 0);
            };
        };
    };
    return true;

};

void DrawCircleCenter(RGBMatrix *Matrix, FrameCanvas *OffscreenCanvas, int radius, unsigned char Red, unsigned char Green, unsigned char Blue) {
    int XCenter = Matrix->width() / 2;
    int YCenter = Matrix->height() / 2;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if ((x * x + y * y) <= (radius * radius + radius)) {
                OffscreenCanvas->SetPixel(XCenter + x, YCenter + y, Red, Green, Blue);
            }
        }
    }
}

int ViewSplashScreen(bool ShowIP, bool ShowLogo, bool ShowSemaphore, unsigned char Red, unsigned char Green, unsigned char Blue) {
    RGBMatrix::Options MatrixOptions;
	MatrixOptions.rows = 64; 

  //   const char* panelWidthEnv = getenv("PANEL_WIDTH");
  //   if (panelWidthEnv != nullptr) {
  //       int panelWidth = std::atoi(panelWidthEnv);
  //       if (panelWidth > 0) {
  //           MatrixOptions.cols = panelWidth;
  //       }else {
		// 	MatrixOptions.cols = 64;
		// };
  //   };
	
    MatrixOptions.cols = 64*4;

	MatrixOptions.multiplexing = 0;
	MatrixOptions.parallel = 1;
	MatrixOptions.chain_length = 1; 
    MatrixOptions.row_address_type = 0;
    MatrixOptions.pwm_bits = 1;
	MatrixOptions.show_refresh_rate = true;
	MatrixOptions.pwm_lsb_nanoseconds = 600;
	// MatrixOptions.pwm_dither_bits = 2;
	

	
    rgb_matrix::RuntimeOptions RuntimeOpt;
	RGBMatrix *Matrix = RGBMatrix::CreateFromOptions(MatrixOptions, RuntimeOpt);
    if (Matrix == NULL) {
        std::cerr << "Could not create matrix object.\n";
        return 1;
    };
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    FrameCanvas *OffscreenCanvas = Matrix->CreateFrameCanvas();
		
    if (ShowLogo) {
        if (!DisplayLogoPatternCenter(Matrix, &MatrixOptions, OffscreenCanvas)) {
            std::cerr << "Displaying image failed.\n";
            delete Matrix;
            return -EXIT_FAILURE;
        };
    };
    if (ShowIP) {
        std::string IpAddress = GetIpAddress();
        if (!IpAddress.empty()) {
            DrawIPAddress(Matrix, &MatrixOptions, IpAddress, OffscreenCanvas);
        };
    };
    if (ShowSemaphore) {
        DrawCircleCenter(Matrix, OffscreenCanvas, 20, Red, Green, Blue);
    };
	
    Matrix->SwapOnVSync(OffscreenCanvas);
    while (!InterruptReceived) {
        sleep(1);
    };
    std::cout << "Exiting splash viewer\n";
    delete Matrix;
    return 0;
};

int main(int argc, char *argv[]) {
    bool ShowIP = false;
    bool ShowLogo = false;
    bool ShowSemaphore = false;
    unsigned char Red = 0, Green = 0, Blue = 0;
    
    FontsPath = argv[1];
    if (FontsPath.back() != '/') {
        FontsPath += "/";
    }

    for (int Idx = 2; Idx < argc; ++Idx) {
        if (strcmp(argv[Idx], "-l") == 0) {
            ShowLogo = true;
        };
		if (strcmp(argv[Idx], "-a") == 0) {
            ShowIP = true;
        };
        if (strcmp(argv[Idx], "-s") == 0) {
            ShowSemaphore = true;
            if (Idx + 1 < argc) {
                std::string ColorStr = argv[++Idx];
                if (sscanf(ColorStr.c_str(), "%hhu,%hhu,%hhu", &Red, &Green, &Blue) != 3) {
                    std::cerr << "Invalid color format. Use R,G,B format.\n";
                    return 1;
                }
            } else {
                std::cerr << "Missing color for semaphore mode.\n";
                return 1;
            };
        };
    };
    ViewSplashScreen(ShowIP, ShowLogo, ShowSemaphore, Red, Green, Blue);
    return 0;
};
