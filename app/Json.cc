
#include "Controller.h"
#include <cjson/cJSON.h>

void HexToRgb(const char *HexColorStr, unsigned char *Red, unsigned char *Green, unsigned char *Blue) {
    if (HexColorStr[0] == '#') {
        sscanf(HexColorStr + 1, "%02x%02x%02x", (int *)Red, (int *)Green, (int *)Blue);
    } else {
        sscanf(HexColorStr, "%02x%02x%02x", (int *)Red, (int *)Green, (int *)Blue);
    };
};

int ParseSetLineTextRequest(const char* JsonString, SetLineTextRequest* Request) {
    cJSON *JsonToParse = cJSON_Parse(JsonString);
    if (JsonToParse == NULL) {
        return false;
    };
    const cJSON *LineNumber = cJSON_GetObjectItemCaseSensitive(JsonToParse, "line_num");
    const cJSON *LineText = cJSON_GetObjectItemCaseSensitive(JsonToParse, "text");
    const cJSON *LineColor = cJSON_GetObjectItemCaseSensitive(JsonToParse, "color");
    if (!cJSON_IsNumber(LineNumber) || !cJSON_IsString(LineText) || !cJSON_IsString(LineColor)) {
        cJSON_Delete(JsonToParse);
        return false; 
    };
    Request->LineNumber = LineNumber->valueint;
    strncpy(Request->LineText, LineText->valuestring, sizeof(Request->LineText)-1);
	HexToRgb(LineColor->valuestring, 
		&Request->LineColor.r,
		&Request->LineColor.g,
		&Request->LineColor.b
	);
    cJSON_Delete(JsonToParse);
    return true;
};

std::string GetCommandName(std::string RawJsonString){
	std::string CommandName;
	cJSON *JsonToParse = cJSON_Parse(RawJsonString.c_str());
	if (JsonToParse != NULL) {
		const cJSON *CommandType = cJSON_GetObjectItemCaseSensitive(JsonToParse, "cmd");
		if (cJSON_IsString(CommandType) && CommandType->valuestring != NULL) {
			CommandName = CommandType->valuestring;
		};
		cJSON_Delete(JsonToParse);
	};
	return CommandName;
};

int ParseSetLineTimeRequest(const char* JsonString, SetLineTimeRequest* Request) {
    cJSON *JsonToParse = cJSON_Parse(JsonString);
    if (JsonToParse == NULL) {
        return false;
    };
    const cJSON *LineNumber = cJSON_GetObjectItemCaseSensitive(JsonToParse, "line_num");
    if (!cJSON_IsNumber(LineNumber)) {
        cJSON_Delete(JsonToParse);
        return false;
    };
    Request->LineNumber = LineNumber->valueint;
    cJSON_Delete(JsonToParse);
    return true;
};

int ParseSetLineBlinkRequest(const char* JsonString, SetLineBlinkRequest* Request) {
    cJSON* JsonToParse = cJSON_Parse(JsonString);
    if (JsonToParse == NULL) return false;

    const cJSON* LineNumber = cJSON_GetObjectItemCaseSensitive(JsonToParse, "line_num");
    const cJSON* BlinkFreq = cJSON_GetObjectItemCaseSensitive(JsonToParse, "blink_freq");
    const cJSON* BlinkTime = cJSON_GetObjectItemCaseSensitive(JsonToParse, "blink_time");

    if (!cJSON_IsNumber(LineNumber) || !cJSON_IsNumber(BlinkFreq) || !cJSON_IsNumber(BlinkTime)) {
        cJSON_Delete(JsonToParse);
        return false;
    };
    Request->LineNumber = LineNumber->valueint;
    Request->LineBlinkFrequency = static_cast<float>(BlinkFreq->valuedouble);
    Request->LineBlinkTimeout = static_cast<float>(BlinkTime->valuedouble);

    cJSON_Delete(JsonToParse);
    return true;
};

int ParseSetLineScrollRequest(const char* JsonString, SetLineScrollRequest* Request) {
    cJSON *JsonToParse = cJSON_Parse(JsonString);
    if (JsonToParse == NULL) {
        return false;
    };
    const cJSON *LineNumber = cJSON_GetObjectItemCaseSensitive(JsonToParse, "line_num");
    const cJSON *ScrollSpeed = cJSON_GetObjectItemCaseSensitive(JsonToParse, "scroll_speed");

    if (!cJSON_IsNumber(LineNumber) || !cJSON_IsNumber(ScrollSpeed)) {
        cJSON_Delete(JsonToParse);
        return false; 
    };
    Request->LineNumber = LineNumber->valueint;
    Request->ScrollSpeed = ScrollSpeed->valueint;

    cJSON_Delete(JsonToParse);
    return true;
}