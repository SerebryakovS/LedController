
#include "Controller.h"
#include "led-matrix.h"
#include <chrono>

volatile bool InterruptReceived = false;
static void InterruptHandler(int signo) {
	InterruptReceived = true;
};

signed int TimeDisplayLine = -1;

struct BlinkState {
    bool IsBlinking;
    float BlinkFrequency;
    float BlinkDuration;
    std::chrono::steady_clock::time_point BlinkStartTime;
    BlinkState() : IsBlinking(false), BlinkFrequency(0), BlinkDuration(0) {}
};

void UpdateLineTextsWithTime(std::vector<std::string>& LineTexts) {
    if (TimeDisplayLine != -1) {
        time_t now = time(0);
        struct tm *ltm = localtime(&now);
        char timeStr[9]; 
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        if (TimeDisplayLine >= 1 && TimeDisplayLine <= (int)LineTexts.size()) {
            LineTexts[TimeDisplayLine - 1] = std::string(timeStr);
        };
    };
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
	//MatrixOptions.show_refresh_rate = true;
	rgb_matrix::RuntimeOptions RuntimeOpt;

	Color BackgroundColor(0, 0, 0);
	int LetterSpacing = 0;
	int IncomingCommandsPipe = OpenNonBlockingPipe("/tmp/LedCommandsPipe");

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

	FrameCanvas *OffscreenCanvas = Canvas->CreateFrameCanvas();

    std::vector<std::string> LineTexts = {"0", "0", "0"};
    std::vector<Color> Colors = {
        Color(255, 255, 255),
        Color(255, 255, 255),
        Color(255, 255, 255)
    };
	std::vector<BlinkState> BlinkStates(3);

	while (!InterruptReceived) {
		OffscreenCanvas->Fill(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b);
		std::string NewCommandPacket = ReadFromPipe(IncomingCommandsPipe);
		if (!NewCommandPacket.empty()) {
			std::string CommandName = GetCommandName(NewCommandPacket);
			if (CommandName == "set_line_text") {
				SetLineTextRequest Request;
				if (ParseSetLineTextRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 3) {
					int LineIndex = Request.LineNumber - 1; 
                    LineTexts[LineIndex] = std::string(Request.LineText);
                    Colors[LineIndex] = Request.LineColor; 
					BlinkStates[LineIndex].IsBlinking = false;
				};
			} else if (CommandName == "set_line_time") {
				SetLineTimeRequest Request;
				if (ParseSetLineTimeRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 3) {
					int LineIndex = Request.LineNumber -1;
					if (TimeDisplayLine > 0){
						LineTexts[LineIndex] = "0";
						Colors[LineIndex] = Color(255, 255, 255);
					}
					if (TimeDisplayLine == Request.LineNumber) {
						TimeDisplayLine = -1;
					} else {
						TimeDisplayLine = Request.LineNumber;
					};
					BlinkStates[LineIndex].IsBlinking = false;
				};
			} else if (CommandName == "set_line_blink") {
				SetLineBlinkRequest Request;
				if (ParseSetLineBlinkRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 3) {
					int LineIndex = Request.LineNumber - 1;
					BlinkStates[LineIndex].IsBlinking = true;
					BlinkStates[LineIndex].BlinkFrequency = Request.LineBlinkFrequency;
					BlinkStates[LineIndex].BlinkDuration = Request.LineBlinkTimeout;
					BlinkStates[LineIndex].BlinkStartTime = std::chrono::steady_clock::now();
				};
			};
        };
		UpdateLineTextsWithTime(LineTexts);		
		for (int Idx = 0; Idx < (int)LineTexts.size(); ++Idx) {
			if (BlinkStates[Idx].IsBlinking) {
				auto Now = std::chrono::steady_clock::now();
				auto ElapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(Now - BlinkStates[Idx].BlinkStartTime).count();
				float BlinkIntervalMs = 1000 / BlinkStates[Idx].BlinkFrequency; 
				if (ElapsedTimeMs / BlinkIntervalMs > BlinkStates[Idx].BlinkDuration) {
					BlinkStates[Idx].IsBlinking = false;
				} else {
					bool BlinkOnOff = static_cast<int>((ElapsedTimeMs / BlinkIntervalMs)) % 2 == 0;
					if (!BlinkOnOff) {
						continue;
					};
				};
			};
			int XOffset = 1 + 64 * Idx;
			DrawTextSegment(OffscreenCanvas, Font, XOffset, LineTexts[Idx], Colors[Idx], LetterSpacing, 0);
		};
		
		OffscreenCanvas = Canvas->SwapOnVSync(OffscreenCanvas);
		usleep(100 * 1000); 
	};
	Canvas->Clear();
	delete Canvas;
	close(IncomingCommandsPipe);
	return 0;
};
