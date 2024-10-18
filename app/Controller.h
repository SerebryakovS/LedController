
#include <stdbool.h>
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

typedef struct {
	int  LineNumber;
	char LineText[64];
	struct Color LineColor;
} SetLineTextRequest;

typedef struct {
	int LineNumber;
} SetLineTimeRequest;

typedef struct {
	int  LineNumber;
	float LineBlinkFrequency; 
	float LineBlinkTimeout; 
} SetLineBlinkRequest;

typedef struct {
    int LineNumber;
    int ScrollSpeed; // in milliseconds
} SetLineScrollRequest;

std::string GetCommandName(std::string RawJsonString);
int ParseSetLineTextRequest(const char *JsonString, SetLineTextRequest *Request);
int ParseSetLineTimeRequest(const char *JsonString, SetLineTimeRequest *Request);
int ParseSetLineBlinkRequest(const char *JsonString, SetLineBlinkRequest* Request);
int ParseSetLineScrollRequest(const char *JsonString, SetLineScrollRequest* Request);


typedef struct {
    int  SinglePanelWidth;
    int  SinglePanelHeight;
    int  PanelsChainCount;
    int  PwmLsbNanos;
	char ColorScheme[6];
    char FontsPath[256];
} ControllerConfig;

extern ControllerConfig Config;

int8_t LoadConfig(void);
