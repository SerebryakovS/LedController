#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "Controller.h"

ControllerConfig Config;

int ParseConfigRequest(const char* JsonString, ControllerConfig* Config) {
    cJSON *JsonToParse = cJSON_Parse(JsonString);
    if (JsonToParse == NULL) {
        return false;
    };
    const cJSON *SinglePanelWidth = cJSON_GetObjectItemCaseSensitive(JsonToParse, "SinglePanelWidth");
    const cJSON *SinglePanelHeight = cJSON_GetObjectItemCaseSensitive(JsonToParse, "SinglePanelHeight");
    const cJSON *PanelsChainCount = cJSON_GetObjectItemCaseSensitive(JsonToParse, "PanelsChainCount");
    const cJSON *PwmLsbNanos = cJSON_GetObjectItemCaseSensitive(JsonToParse, "PwmLsbNanos");
    const cJSON *FontsPath = cJSON_GetObjectItemCaseSensitive(JsonToParse, "FontsPath");
    const cJSON *ColorScheme = cJSON_GetObjectItemCaseSensitive(JsonToParse, "ColorScheme");
    if (!cJSON_IsNumber(SinglePanelWidth) ||
        !cJSON_IsNumber(SinglePanelHeight) ||
        !cJSON_IsNumber(PanelsChainCount) ||
        !cJSON_IsNumber(PwmLsbNanos) ||
        !cJSON_IsString(FontsPath) ||
        !cJSON_IsString(ColorScheme)) {
        cJSON_Delete(JsonToParse);
        return false;
    };
    Config->SinglePanelWidth = SinglePanelWidth->valueint;
    Config->SinglePanelHeight = SinglePanelHeight->valueint;
    Config->PanelsChainCount = PanelsChainCount->valueint;
    Config->PwmLsbNanos = PwmLsbNanos->valueint;
    strncpy(Config->FontsPath, FontsPath->valuestring, sizeof(Config->FontsPath) - 1);
    strncpy(Config->ColorScheme, ColorScheme->valuestring, sizeof(Config->ColorScheme) - 1);
    cJSON_Delete(JsonToParse);
    return true;
};

int8_t LoadConfig(void) {
    FILE *ConfigFile = fopen("./config.json", "r");
    char ConfigBuffer[1024];
    fread(ConfigBuffer, 1, sizeof(ConfigBuffer), ConfigFile);
    fclose(ConfigFile);
    if (!ParseConfigRequest(ConfigBuffer, &Config)) {
        fprintf(stdout, "[ERR]: Failed to parse config JSON\n");
        fflush(stdout);
        return -EXIT_FAILURE;
    };
    return EXIT_SUCCESS;
};

