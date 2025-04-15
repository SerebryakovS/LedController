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

int glyph_width = 7;

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


int GetFontHeight(const rgb_matrix::Font& font) {
    return font.height();
}


int main(int argc, char *argv[]) {
    if (LoadConfig() != EXIT_SUCCESS) {
        fprintf(stderr, "[ERR]: Could not load config. Exiting.\n");
        return 1;
    };
    RGBMatrix::Options MatrixOptions;
    MatrixOptions.rows = Config.SinglePanelHeight;
    MatrixOptions.cols = Config.SinglePanelWidth * Config.PanelsChainCount;
    MatrixOptions.pwm_lsb_nanoseconds = Config.PwmLsbNanos;
    MatrixOptions.pwm_bits=1;
    MatrixOptions.show_refresh_rate = true;
    MatrixOptions.led_rgb_sequence = Config.ColorScheme;

    rgb_matrix::RuntimeOptions RuntimeOpt;

    std::string FontsPath = std::string(Config.FontsPath);
    if (FontsPath.back() != '/') {
        FontsPath += "/";
    };

    Color BackgroundColor(0, 0, 0);
    int LetterSpacing = 0;
    int IncomingCommandsPipe = OpenNonBlockingPipe(COMMANDS_PIPE);

    rgb_matrix::Font AFont, BFont, CFont;

    if (MatrixOptions.cols <= 64) {
        std::string commonFont = FontsPath + "7x13.bdf";
        AFont.LoadFont(commonFont.c_str());
        BFont.LoadFont(commonFont.c_str());
        CFont.LoadFont(commonFont.c_str());
    } else {
        AFont.LoadFont((FontsPath +  "8x13.bdf").c_str());
        BFont.LoadFont((FontsPath +  "9x18.bdf").c_str());
        CFont.LoadFont((FontsPath + "10x20.bdf").c_str());
    }
    rgb_matrix::Font *SetFont;

    auto GetFontByName = [&](const std::string& name) -> rgb_matrix::Font* {
        if (MatrixOptions.cols <= 64) {
            glyph_width = 7;
            return &AFont;
        };
        if (name == "tiny"){
            glyph_width = 8;
            return &AFont;
        } else if (name == "medium"){
            glyph_width = 9;
            return &BFont;
        } else {
            glyph_width = 10;
            return &CFont;
        };
    };


    RGBMatrix *Canvas = RGBMatrix::CreateFromOptions(MatrixOptions, RuntimeOpt);
    if (Canvas == NULL) {
        return 1;
    }
    Canvas->SetPWMBits(1);

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    FrameCanvas *OffscreenCanvas = Canvas->CreateFrameCanvas();

    std::vector<std::string> LineTexts = {"", "", ""};
    std::vector<std::string> Fonts = {"huge", "huge", "huge"};
    std::vector<Color> Colors = {
        Color(255, 255, 255),
        Color(255, 255, 255),
        Color(255, 255, 255)
    };
    std::vector<bool> Centered = {false, false, false};

    std::vector<BlinkState> BlinkStates(3);
    std::vector<ScrollState> ScrollStates(3);
    int XOffset = 2;

    SetFont = &CFont;

    while (!InterruptReceived) {
        OffscreenCanvas->Fill(BackgroundColor.r, BackgroundColor.g, BackgroundColor.b);
        std::string NewCommandPacket = ReadFromPipe(IncomingCommandsPipe);
        if (!NewCommandPacket.empty()) {
            std::string CommandName = GetCommandName(NewCommandPacket);
            if (CommandName == "set_line_text") {
                printf("\rNewCommandPacket:%s\n", NewCommandPacket.c_str());
                SetLineTextRequest Request;
                if (ParseSetLineTextRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 3) {
                    int LineIndex = Request.LineNumber - 1;
                    LineTexts[LineIndex] = std::string(Request.LineText);
                    Fonts[LineIndex] = std::string(Request.LineFont);
                    Centered[LineIndex] = Request.Centered;
                    Colors[LineIndex] = Request.LineColor;
                    BlinkStates[LineIndex].IsBlinking = false;
                    if (TimeDisplayLine == Request.LineNumber) {
                        TimeDisplayLine = -1;
                    };
                    UpdateLedTextsFile(LineTexts);
					ScrollStates[Request.LineNumber - 1].IsScrolling = false;
                }
            } else if (CommandName == "set_line_scroll") {
                SetLineScrollRequest Request;
                if (ParseSetLineScrollRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 3) {
                    int LineIndex = Request.LineNumber - 1;
                    ScrollStates[LineIndex].IsScrolling = true;
                    ScrollStates[LineIndex].ScrollSpeed = Request.ScrollSpeed;
                    ScrollStates[LineIndex].CurrentOffset = 0;
					ScrollStates[LineIndex].Text = LineTexts[LineIndex];
                    ScrollStates[LineIndex].LastScrollTime = std::chrono::steady_clock::now();
                }
            } else if (CommandName == "set_line_time") {
                SetLineTimeRequest Request;
                if (ParseSetLineTimeRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 3) {
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
                if (ParseSetLineBlinkRequest(NewCommandPacket.c_str(), &Request) && Request.LineNumber >= 1 && Request.LineNumber <= 3) {
                    int LineIndex = Request.LineNumber - 1;
                    BlinkStates[LineIndex].IsBlinking = true;
                    BlinkStates[LineIndex].BlinkFrequency = Request.LineBlinkFrequency;
                    BlinkStates[LineIndex].BlinkDuration = Request.LineBlinkTimeout;
                    BlinkStates[LineIndex].BlinkStartTime = std::chrono::steady_clock::now();
                }
            }
        }


        int PanelHeight = MatrixOptions.rows;
        std::array<int, 3> LineHeights = {
            GetFontByName(Fonts[0])->height(),
            GetFontByName(Fonts[1])->height(),
            GetFontByName(Fonts[2])->height()
        };

        const int LineSpacing = 2;

        // Центр всей панели
        int PanelCenterY = PanelHeight / 2;

        // Центр 2-й строки
        int MiddleLineIndex = 1;
        int MiddleFontHeight = LineHeights[MiddleLineIndex];
        int MiddleLineY = PanelCenterY - (MiddleFontHeight / 2);

        // Посчитаем Y-позиции для строк 0, 1, 2
        std::array<int, 3> YPositions;
        YPositions[1] = MiddleLineY;
        YPositions[0] = YPositions[1] - LineHeights[0] - LineSpacing;
        YPositions[2] = YPositions[1] + MiddleFontHeight + LineSpacing;


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

            SetFont = GetFontByName(Fonts[Idx]);
            int YPosition = YPositions[Idx];


                if (ScrollStates[Idx].IsScrolling) {
                    int len = utf8_strlen(LineTexts[Idx]);
                    int TextOffsetLimit = len * glyph_width + (len - 1) * LetterSpacing;

                    auto Now = std::chrono::steady_clock::now();
                    auto ElapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            Now - ScrollStates[Idx].LastScrollTime).count();

                    if (ElapsedTimeMs > 100) {
                        ScrollStates[Idx].CurrentOffset -= ScrollStates[Idx].ScrollSpeed;
                        if (ScrollStates[Idx].CurrentOffset < -TextOffsetLimit) {
                            ScrollStates[Idx].CurrentOffset = MatrixOptions.cols;
                        }
                        ScrollStates[Idx].LastScrollTime = Now;
                    }

                    DrawTextSegment(OffscreenCanvas, *SetFont, ScrollStates[Idx].CurrentOffset,
                                    LineTexts[Idx], Colors[Idx], LetterSpacing, YPosition);
                } else {
                int XPos = XOffset;

                if (Centered[Idx]) {
                    // int glyph_width = 8;
                    // if (Fonts[Idx] == "medium") glyph_width = 9;
                    // else if (Fonts[Idx] == "huge") glyph_width = 10;

                    int len = utf8_strlen(LineTexts[Idx]);
                    int TextPixelLength = len * glyph_width + (len - 1) * LetterSpacing;
                    XPos = std::max((MatrixOptions.cols - TextPixelLength) / 2, 0);
                }
                DrawTextSegment(OffscreenCanvas, *SetFont, XPos, LineTexts[Idx], Colors[Idx], LetterSpacing, YPosition);
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
