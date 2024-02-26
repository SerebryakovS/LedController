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
void DrawTextSegment(FrameCanvas *Canvas, rgb_matrix::Font &Font, 
                     int XOffset, const std::string &Text, Color &_Color, 
                     int LetterSpacing, int YPosition) {
    rgb_matrix::DrawText(Canvas, Font, XOffset, YPosition + Font.baseline(), 
                         _Color, NULL, Text.c_str(), LetterSpacing);
};

int main(int argc, char *argv[]) {
  RGBMatrix::Options MatrixOptions;
  MatrixOptions.chain_length = 6;
  MatrixOptions.rows = 16; 
  MatrixOptions.cols = 32; 
  MatrixOptions.multiplexing=4;
  MatrixOptions.parallel = 1;
  MatrixOptions.show_refresh_rate = true;
  rgb_matrix::RuntimeOptions RuntimeOpt;

  Color BackgroundColor(0, 0, 0);
  
  int LetterSpacing = 0;

  int PipeFd1 = OpenNonBlockingPipe("/tmp/pipe1");
  int PipeFd2 = OpenNonBlockingPipe("/tmp/pipe2");
  int PipeFd3 = OpenNonBlockingPipe("/tmp/pipe3");

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
  std::string LineText2 = "0";
  std::string LineText3 = "0";
  Color Color1(255, 255, 255);
  Color Color2(255, 255, 255);
  Color Color3(255, 255, 255);

  while (!InterruptReceived) {
      OffscreenCanvas->Fill(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b);

      std::string NewLineText1 = ReadFromPipe(PipeFd1);
      if (!NewLineText1.empty()) LineText1 = NewLineText1;
      std::string NewLineText2 = ReadFromPipe(PipeFd2);
      if (!NewLineText2.empty()) LineText2 = NewLineText2;
      std::string NewLineText3 = ReadFromPipe(PipeFd3);
      if (!NewLineText3.empty()) LineText3 = NewLineText3;

      DrawTextSegment(OffscreenCanvas, Font,   1, LineText1, Color1, LetterSpacing, 0); 
      DrawTextSegment(OffscreenCanvas, Font,  65, LineText2, Color2, LetterSpacing, 0);
      DrawTextSegment(OffscreenCanvas, Font, 129, LineText3, Color3, LetterSpacing, 0);

      OffscreenCanvas = Canvas->SwapOnVSync(OffscreenCanvas);
      usleep(100 * 1000); 
  };
  Canvas->Clear();
  delete Canvas;
  close(PipeFd1);
  close(PipeFd2);
  close(PipeFd3);
  return 0;
};
