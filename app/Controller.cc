#include "Controller.h"
#include "led-matrix.h"
#include <fstream>
#include <chrono>
#include "utf8.h"

#define COMMANDS_PIPE "/tmp/LedCommandsPipe"
#define USED_TEXTS "/tmp/LedTexts"

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

struct ScrollState {
    bool IsScrolling;
    std::string Text;
    int ScrollSpeed;
    int CurrentOffset;
    std::chrono::steady_clock::time_point LastScrollTime;
    ScrollState() : IsScrolling(false), ScrollSpeed(100), CurrentOffset(0) {}
};

void UpdateLineTextsWithTime(std::vector<std::string>& LineTexts) {
    if (TimeDisplayLine != -1) {
        time_t now = time(0);
        struct tm *ltm = localtime(&now);
        char timeStr[9]; 
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        if (TimeDisplayLine >= 1 && TimeDisplayLine <= (int)LineTexts.size()) {
            LineTexts[TimeDisplayLine - 1] = std::string(timeStr);
        }
    }
}

int OpenNonBlockingPipe(const char* pipeName) {
    if (mkfifo(pipeName, 0666) < 0 && errno != EEXIST) {
        exit(-EXIT_FAILURE);
    }
    int PipeFd = open(pipeName, O_RDONLY | O_NONBLOCK);
    if (PipeFd < 0) {
        exit(-EXIT_FAILURE);
    }
    return PipeFd;
}

void UpdateLedTextsFile(const std::vector<std::string>& LineTexts) {
    std::ofstream File(USED_TEXTS);
    for (const auto& Text : LineTexts) {
        File << Text << std::endl;
    }
    File.close();
}

std::string ReadFromPipe(int PipeFd) {
    char Buffer[128];
    std::string Result;
    ssize_t BytesRead = read(PipeFd, Buffer, sizeof(Buffer) - 1);
    if (BytesRead > 0) {
        Buffer[BytesRead] = '\0';
        Result = std::string(Buffer);
    }
    return Result;
}

void DrawTextSegment(FrameCanvas *Canvas, rgb_matrix::Font &Font, 
                     int XOffset, const std::string &Text, Color &_Color, 
                     int LetterSpacing, int YPosition) {
    rgb_matrix::DrawText(Canvas, Font, XOffset, YPosition + Font.baseline(), 
                         _Color, NULL, Text.c_str(), LetterSpacing);
}

size_t utf8_strlen(const std::string& str) {
    return utf8::distance(str.begin(), str.end());
};

int main(int argc, char *argv[]) {
    std::string FontsPath = argv[1];
    if (FontsPath.back() != '/') {
        FontsPath += "/";
    };
    RGBMatrix::Options MatrixOptions;
    MatrixOptions.rows = 64; 
    const char* panelWidthEnv = getenv("PANEL_WIDTH");
    if (panelWidthEnv != nullptr) {
        int panelWidth = std::atoi(panelWidthEnv);
        if (panelWidth > 0) {
            MatrixOptions.cols = panelWidth;
        }else {
			MatrixOptions.cols = 64;
		};
    };
    MatrixOptions.multiplexing = 0;
    MatrixOptions.parallel = 1;	
    MatrixOptions.chain_length = 1; 
    MatrixOptions.row_address_type = 0;
    MatrixOptions.pwm_bits = 1;
    MatrixOptions.show_refresh_rate = true;
    MatrixOptions.pwm_lsb_nanoseconds = 700;
    MatrixOptions.pwm_dither_bits = 2;
    //MatrixOptions.led_rgb_sequence = "BRG";
	
    rgb_matrix::RuntimeOptions RuntimeOpt;
	
    Color BackgroundColor(0, 0, 0);
    int LetterSpacing = 0;
    int IncomingCommandsPipe = OpenNonBlockingPipe(COMMANDS_PIPE);
    rgb_matrix::Font AFont; AFont.LoadFont((FontsPath + "8x13.bdf").c_str());
    rgb_matrix::Font BFont; BFont.LoadFont((FontsPath + "7x13.bdf").c_str());
    rgb_matrix::Font CFont; CFont.LoadFont((FontsPath + "6x13.bdf").c_str());
    rgb_matrix::Font *SetFont;
	
    RGBMatrix *Canvas = RGBMatrix::CreateFromOptions(MatrixOptions, RuntimeOpt);
    if (Canvas == NULL) {
        return 1;
    }
    Canvas->SetPWMBits(1);

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    FrameCanvas *OffscreenCanvas = Canvas->CreateFrameCanvas();

    std::vector<std::string> LineTexts = {"", "", "", ""};
    std::vector<Color> Colors = {
        Color(255, 255, 255),
        Color(255, 255, 255),
        Color(255, 255, 255),
        Color(255, 255, 255)
    };
    std::vector<BlinkState> BlinkStates(4);
    std::vector<ScrollState> ScrollStates(4);
    int XOffset = 2;

    while (!InterruptReceived) {
        OffscreenCanvas->Fill(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b);
        std::string NewCommandPacket = ReadFromPipe(IncomingCommandsPipe);
        if (!NewCommandPacket.empty()) {
            std::string CommandName = GetCommandName(NewCommandPacket);
            if (CommandName == "set_line_text") {
                printf("\rNewCommandPacket:%s\n", NewCommandPacket.c_str());
                SetLineTextRequest Request;
                if (ParseSetLineTextRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 4) {
                    int LineIndex = Request.LineNumber - 1;
                    LineTexts[LineIndex] = std::string(Request.LineText);
                    Colors[LineIndex] = Request.LineColor;
                    BlinkStates[LineIndex].IsBlinking = false;
                    if (TimeDisplayLine == Request.LineNumber) {
                        TimeDisplayLine = -1;
                    }
                    UpdateLedTextsFile(LineTexts);
					ScrollStates[Request.LineNumber - 1].IsScrolling = false;
                }
            } else if (CommandName == "set_line_scroll") {
                SetLineScrollRequest Request;
                if (ParseSetLineScrollRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 4) {
                    int LineIndex = Request.LineNumber - 1;
                    ScrollStates[LineIndex].IsScrolling = true;
                    ScrollStates[LineIndex].ScrollSpeed = Request.ScrollSpeed;
                    ScrollStates[LineIndex].CurrentOffset = 0;
					ScrollStates[LineIndex].Text = LineTexts[LineIndex];
                    ScrollStates[LineIndex].LastScrollTime = std::chrono::steady_clock::now();
                }
            } else if (CommandName == "set_line_time") {
                SetLineTimeRequest Request;
                if (ParseSetLineTimeRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 4) {
                    int LineIndex = Request.LineNumber - 1;
                    if (TimeDisplayLine > 0) {
                        LineTexts[TimeDisplayLine - 1] = "";
                    }
                    if (TimeDisplayLine == Request.LineNumber) {
                        TimeDisplayLine = -1;
                    } else {
                        TimeDisplayLine = Request.LineNumber;
                        Colors[LineIndex] = Color(255, 255, 255);
                    }
                    BlinkStates[LineIndex].IsBlinking = false;
                }
            } else if (CommandName == "set_line_blink") {
                SetLineBlinkRequest Request;
                if (ParseSetLineBlinkRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 4) {
                    int LineIndex = Request.LineNumber - 1;
                    BlinkStates[LineIndex].IsBlinking = true;
                    BlinkStates[LineIndex].BlinkFrequency = Request.LineBlinkFrequency;
                    BlinkStates[LineIndex].BlinkDuration = Request.LineBlinkTimeout;
                    BlinkStates[LineIndex].BlinkStartTime = std::chrono::steady_clock::now();
                }
            }
        }
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
                    }
                }
            }
            int YPosition = 2 + 15 * Idx;
            SetFont = &CFont;
			if (ScrollStates[Idx].IsScrolling) {
				int TextOffsetLimit = utf8_strlen(LineTexts[Idx]) * 6;
				auto Now = std::chrono::steady_clock::now();
				auto ElapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(Now - ScrollStates[Idx].LastScrollTime).count();
				if (ElapsedTimeMs > 100) {
					ScrollStates[Idx].CurrentOffset -= ScrollStates[Idx].ScrollSpeed;
					if (ScrollStates[Idx].CurrentOffset < -TextOffsetLimit) {
						ScrollStates[Idx].CurrentOffset = MatrixOptions.cols;
						printf("HERE\n");
					};
					ScrollStates[Idx].LastScrollTime = Now;
				};
				//for (int x = ScrollStates[Idx].CurrentOffset; x < MatrixOptions.cols; x += TextOffsetLimit) {
					DrawTextSegment(OffscreenCanvas, *SetFont, ScrollStates[Idx].CurrentOffset, LineTexts[Idx], Colors[Idx], LetterSpacing, YPosition);
				//};
			} else {
				DrawTextSegment(OffscreenCanvas, *SetFont, XOffset, LineTexts[Idx], Colors[Idx], LetterSpacing, YPosition);
			};
        };
        OffscreenCanvas = Canvas->SwapOnVSync(OffscreenCanvas);
        usleep(100 * 1000);
    };
    Canvas->Clear();
    delete Canvas;
    close(IncomingCommandsPipe);
    return 0;
}
