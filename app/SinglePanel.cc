#include "led-matrix.h"
#include "graphics.h"
#include <errno.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

using namespace rgb_matrix;

volatile bool InterruptReceived = false;
static void InterruptHandler(int signo) {
    InterruptReceived = true;
};
int OpenNonBlockingPipe(const char* pipeName) {
    if (mkfifo(pipeName, 0666) < 0 && errno != EEXIST) {
        exit(-EXIT_FAILURE);
    };
    int PipeFd = open(pipeName, O_RDONLY | O_NONBLOCK);
    if (PipeFd < 0) {
        exit(-EXIT_FAILURE);
    };
    return PipeFd;
};
std::string ReadFromPipe(int PipeFd) {
    char Buffer[128];
    std::string Result;
    ssize_t BytesRead = read(PipeFd, Buffer, sizeof(Buffer) - 1);
    if (BytesRead > 0) {
        Buffer[BytesRead] = '\0';
        Result = std::string(Buffer);
    };
    return Result;
};

int main(int argc, char *argv[]) {	
    RGBMatrix::Options MatrixOptions;
	MatrixOptions.rows = 64; 
	MatrixOptions.cols = 64;
	MatrixOptions.multiplexing=0;
	MatrixOptions.parallel = 1;	
	MatrixOptions.chain_length = 1; 
    MatrixOptions.row_address_type = 0; // ABC-addressed panels
    MatrixOptions.pwm_bits = 1;			
	MatrixOptions.show_refresh_rate = true;
	MatrixOptions.pwm_lsb_nanoseconds = 1000;
	MatrixOptions.pwm_dither_bits = 2;
    rgb_matrix::RuntimeOptions RuntimeOpt;

    Color BackgroundColor(0,0,0);

    int LetterSpacing = 0;

    int PipeFd1 = OpenNonBlockingPipe("/tmp/pipe1");

    const char *DefaultFont = "../fonts/8x13.bdf";
    rgb_matrix::Font Font;
    if (!Font.LoadFont(DefaultFont)) {
        fprintf(stderr, "Couldn't load font '%s'\n", DefaultFont);
        return -EXIT_FAILURE;
    };

    RGBMatrix *Canvas = RGBMatrix::CreateFromOptions(MatrixOptions, RuntimeOpt);
    if (Canvas == NULL){
        return 1;
    };
    Canvas->SetPWMBits(1);

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    printf("CTRL-C for exit.\n");

    FrameCanvas *OffscreenCanvas = Canvas->CreateFrameCanvas();

    std::string LineText1 = "0";
    Color Color1(255, 255, 255);
    Color Color2(255, 255, 255);

    int XOffset = 0;//1;

    while (!InterruptReceived) {
        OffscreenCanvas->Fill(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b);
        //std::string NewLineText1 = ReadFromPipe(PipeFd1);
        //if (!NewLineText1.empty()) LineText1 = NewLineText1;

        LineText1 = "HELLO";
        rgb_matrix::DrawText(OffscreenCanvas, Font, XOffset, 1 + Font.baseline(), 
							 Color1, NULL, LineText1.c_str(), LetterSpacing);
        rgb_matrix::DrawText(OffscreenCanvas, Font, XOffset, 15 + Font.baseline(), 
							 Color1, NULL, LineText1.c_str(), LetterSpacing);
        rgb_matrix::DrawText(OffscreenCanvas, Font, XOffset, 30 + Font.baseline(), 
							 Color1, NULL, LineText1.c_str(), LetterSpacing);
        rgb_matrix::DrawText(OffscreenCanvas, Font, XOffset, 45 + Font.baseline(), 
							 Color1, NULL, LineText1.c_str(), LetterSpacing);
        OffscreenCanvas = Canvas->SwapOnVSync(OffscreenCanvas);
        usleep(100 * 1000); 
    };
    Canvas->Clear();
    delete Canvas;
    close(PipeFd1);
    return 0;
};

