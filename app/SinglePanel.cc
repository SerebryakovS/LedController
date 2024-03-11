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
    MatrixOptions.chain_length = 1;
    MatrixOptions.rows = 16;
    MatrixOptions.cols = 32;
    MatrixOptions.multiplexing=4;
    MatrixOptions.parallel = 1;
    MatrixOptions.show_refresh_rate = true;
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
    Color Color1(255, 0, 0);
    Color Color2(255, 0, 0);

    int XOffset = 0;//1;

    while (!InterruptReceived) {
        OffscreenCanvas->Fill(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b);
        //std::string NewLineText1 = ReadFromPipe(PipeFd1);
        //if (!NewLineText1.empty()) LineText1 = NewLineText1;

//        LineText1 = "↘↙◪◩";
//        rgb_matrix::DrawText(OffscreenCanvas, Font, XOffset, 1 + Font.baseline(), 
//                            Color1, NULL, LineText1.c_str(), LetterSpacing);
        LineText1 = "(┘)";
        rgb_matrix::DrawText(OffscreenCanvas, Font, XOffset, 0/*9*/ + Font.baseline(), 
                            Color2, NULL, LineText1.c_str(), LetterSpacing);
        OffscreenCanvas = Canvas->SwapOnVSync(OffscreenCanvas);
        usleep(100 * 1000); 
    };
    Canvas->Clear();
    delete Canvas;
    close(PipeFd1);
    return 0;
};
