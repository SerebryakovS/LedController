#!/bin/bash

SCRIPT_PATH=$(realpath "${BASH_SOURCE[0]}")
CONFIG_PATH="./config.json"
FONTS_PATH="../fonts/"
LOGO_PATH="./logo.png"
COMMANDS_PIPE="/tmp/LedCommandsPipe"
COMMANDS_RETRIES=3
USED_TEXTS="/tmp/LedTexts"

if [[ ! -f "$CONFIG_PATH" ]]; then
    echo "Creating config file with default values..."
    cat > "$CONFIG_PATH" <<EOL
{
"SinglePanelWidth": 64,
"SinglePanelHeight": 64,
"PanelsChainCount": 1,
"PwmLsbNanos": 300,
"FontsPath": "../fonts/",
"ColorScheme": "RGB",
"RestPort": 13222,
"SelectedLogo": "parqour"
}
EOL
fi

SinglePanelWidth=$(jq ".SinglePanelWidth" "$CONFIG_PATH")
SinglePanelHeight=$(jq ".SinglePanelHeight" "$CONFIG_PATH")
PanelsChainCount=$(jq ".PanelsChainCount" "$CONFIG_PATH")
PwmLsbNanos=$(jq ".PwmLsbNanos" "$CONFIG_PATH")
FontsPath=$(jq -r ".FontsPath" "$CONFIG_PATH")
ColorScheme=$(jq -r ".ColorScheme" "$CONFIG_PATH")
REST_PORT=$(jq ".RestPort" "$CONFIG_PATH")
SelectedLogo=$(jq -r ".SelectedLogo" "$CONFIG_PATH")

UpdateConfig() {
    KEY=$1; VALUE=$2;
    jq --argjson val "$VALUE" ".$KEY = \$val" "$CONFIG_PATH" > tmp.$$.json && mv tmp.$$.json "$CONFIG_PATH"
}

RunHTTPServer() {
    socat TCP-LISTEN:$REST_PORT,fork,reuseaddr SYSTEM:"$SCRIPT_PATH APIRequestsHandler"
};

ServeHTMLPage() {
    echo -ne "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
    cat "$(dirname "$SCRIPT_PATH")/Index.html"
};

KillProcess(){
	if pgrep -x "$1" >/dev/null; then
        pkill $1
    fi;
};

KillStart(){
    if pgrep -x "$1" >/dev/null; then
        pkill $1 
    fi;
    if ! pgrep -x "$2" >/dev/null; then
        "$(dirname "$SCRIPT_PATH")/$2" $FONTS_PATH $3 &
        sleep 0.5;
    fi;
};

ProcessLine() {
    local LineNum="$1"; local LineText="$2"; local LineColor="$3";
	local Retries=COMMANDS_RETRIES;
    local Success=false;
    if [[ $LineNum -ge 1 && $LineNum -le 4 ]]; then
        while [[ $Retries -gt 0 && $Success == false ]]; do
            Command="{\"cmd\":\"set_line_text\",\"line_num\":$LineNum,\"text\":$LineText,\"color\":$LineColor}"
            echo "$Command" > "$COMMANDS_PIPE"
            sleep 1;
			if VerifyLineText "$LineNum" "$LineText"; then
                Success=true;
                echo "success";
            else
                ((Retries--));
            fi;
        done;
        if [[ $Success == false ]]; then
            echo "error: Failed to set text after retries"
        fi;
    else
        echo "error: Invalid line number"
    fi;
};

VerifyLineText() {
    local LineNum="$1"; local ExpectedText="$2";
    local ActualText=$(sed "${LineNum}q;d" "$USED_TEXTS")
    ExpectedText=${ExpectedText//\"/}
    if [[ "$ActualText" == "$ExpectedText" ]]; then
        return 0 # true
    else
        return 1 # false
    fi;
};

APIRequestsHandler() {
    read -r RequestMethod RequestPath RequestProtocol
    ContentLength=0
    while IFS= read -r Header; do
        [[ $Header == $'\r' ]] && break
        if [[ $Header =~ ^Content-Length: ]]; then
            ContentLength="${Header#*: }"
            ContentLength="${ContentLength//[^0-9]/}"
        fi
    done

    if [[ $RequestMethod == "GET" && $RequestPath == "/" ]]; then
        ServeHTMLPage
        return
    fi;

    if [[ $RequestMethod == "GET" && $RequestPath == "/logo.png" ]]; then
        echo -ne "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\n\r\n"
        cat "$LOGO_PATH"
        return
    fi;

	if [[ $ContentLength -gt 0 ]]; then
		Body=$(timeout 1 cat)
		if [ $? -ne 0 ]; then
			echo "Reading request body timed out" >&2
		fi;
	fi;
	echo $Body > /tmp/LastRequestBody;
    case "$RequestPath" in
        "/set_line_text")
            KillStart Splasher Controller
            LineNum=$(echo "$Body" | jq ".line_num")
            LineText=$(echo "$Body" | jq ".text")
            LineColor=$(echo "$Body" | jq ".color")			
			ProcessLineResult=$(ProcessLine "$LineNum" "$LineText" "$LineColor")
			if [[ "$ProcessLineResult" == "success" ]]; then
				echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
			else
				echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"error\": \"$Body\"}"
			fi
			;;
		"/set_all_lines")
			KillStart Splasher Controller
			LinesOut=$(echo "$Body" | jq -c ".lines_out[]");
			Status="success"; ErrorMessage="";
			echo "$LinesOut" | while read -r Line; do
				LineNum=$(echo "$Line" | jq ".line_num")
				LineText=$(echo "$Line" | jq ".text")
				LineColor=$(echo "$Line" | jq ".color")
				ProcessLineResult=$(ProcessLine "$LineNum" "$LineText" "$LineColor");
				if [[ "$ProcessLineResult" != "success" ]]; then
					Status="error"
					ErrorMessage=$ProcessLineResult;
					break;
				else
					sleep 0.2; # some time for panel to refresh
				fi;
			done
			if [[ "$Status" == "success" ]]; then
				echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
			else
				echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"error\", \"message\": \"$ErrorMessage\"}"
			fi
			;;
        "/set_line_time")
			KillStart Splasher Controller
			LineNum=$(echo "$Body" | jq ".line_num")
			PipePath="/tmp/LedCommandsPipe"
			printf '{"cmd":"set_line_time", "line_num":%s}' "$LineNum" > "$PipePath"
			echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
			;;
			"/set_line_blink")
				KillStart Splasher Controller
				LineNum=$(echo "$Body" | jq ".line_num");
				BlinkFreq=$(echo "$Body" | jq ".blink_freq");
				BlinkDuration=$(echo "$Body" | jq -r ".blink_time")

				# Check if BlinkFreq is a valid number, otherwise set a default value
				if ! [[ $BlinkFreq =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
					BlinkFreq=1 # Set a default frequency value if necessary
				else
					BlinkFreq=$(echo "$BlinkFreq" | bc -l) # Ensure BlinkFreq is treated as a decimal number
				fi

				# Check if BlinkDuration is a valid number, otherwise set a default value
				if ! [[ $BlinkDuration =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
					BlinkDuration=10 # Default duration if input is not a valid number
				else
					BlinkDuration=$(echo "$BlinkDuration" | bc -l) # Ensure BlinkDuration is treated as a decimal number
				fi
				
				# Now, handle the command with valid BlinkFreq and BlinkDuration
				Command="{\"cmd\":\"set_line_blink\",\"line_num\":$LineNum,\"blink_freq\":$BlinkFreq,\"blink_time\":$BlinkDuration}"
				echo "$Command" > "$COMMANDS_PIPE"
				echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
				;;
		"/set_line_scroll")
			KillStart Splasher Controller
			LineNum=$(echo "$Body" | jq ".line_num")
			ScrollSpeed=$(echo "$Body" | jq ".scroll_speed")
			if [[ ! $LineNum =~ ^[0-9]+$ || ! $ScrollSpeed =~ ^[0-9]+$ ]]; then
				echo -ne "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\": \"Invalid input data\"}"
			else
				# Execute the command to scroll text on the specified line with the given speed
				Command="{\"cmd\":\"set_line_scroll\",\"line_num\":$LineNum,\"scroll_speed\":$ScrollSpeed}"
				echo "$Command" > "$COMMANDS_PIPE"
				echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
			fi
			;;
        "/set_splasher")
            KillProcess Splasher;
            ShowIP=$(echo "$Body" | jq ".show_ip")
            LogoType=$(echo "$Body" | jq -r ".logo_type")  # Use -r to remove quotes
            Argument="-l"
            if [[ "$ShowIP" == "true" ]]; then
                Argument="-l -a"
            fi
            Argument="$Argument -logo $SelectedLogo"
            KillStart Controller Splasher "$Argument"
            echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
            ;;
        "/set_colored_status")
            KillProcess Splasher
            ColorHex=$(echo "$Body" | jq -r ".color")
            Text=$(echo "$Body" | jq -r ".text")
            if [[ ! $ColorHex =~ ^#[0-9A-Fa-f]{6}$ ]]; then
                echo -ne "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n{\"error\": \"Invalid color format. Use #RRGGBB.\"}"
                return
            fi
            Red=$((16#${ColorHex:1:2}))
            Green=$((16#${ColorHex:3:2}))
            Blue=$((16#${ColorHex:5:2}))
            KillStart Controller Splasher "-s $Red,$Green,$Blue -t $Text"
            echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
            ;;
        "/set_config")
            KillProcess Splasher
            KillProcess Controller
            SinglePanelWidth=$(echo "$Body" | jq ".panel_width // $SinglePanelWidth")
            SinglePanelHeight=$(echo "$Body" | jq ".panel_height // $SinglePanelHeight")
            PanelsChainCount=$(echo "$Body" | jq ".panels_chain_count // $PanelsChainCount")
            PwmLsbNanos=$(echo "$Body" | jq ".pwm_lsb_nanos // $PwmLsbNanos")
            FontsPath=$(echo "$Body" | jq -r ".fonts_path // \"$FontsPath\"")
            ColorScheme=$(echo "$Body" | jq -r ".color_scheme // \"$ColorScheme\"")
            SelectedLogo=$(echo "$Body" | jq -r ".selected_logo // \"$SelectedLogo\"")
            cat > "$CONFIG_PATH" <<EOL
{
    "SinglePanelWidth": $SinglePanelWidth,
    "SinglePanelHeight": $SinglePanelHeight,
    "PanelsChainCount": $PanelsChainCount,
    "PwmLsbNanos": $PwmLsbNanos,
    "FontsPath": "$FontsPath",
    "ColorScheme": "$ColorScheme",
    "RestPort": $REST_PORT,
    "SelectedLogo" : "$SelectedLogo"
}
EOL
            echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"config_updated\"}"
            eval "$(dirname "$SCRIPT_PATH")/Splasher" $FontsPath -l -logo $SelectedLogo -a &
            ;;
        *)
            echo -ne "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"status\": \"error\", \"message\": \"Endpoint not found\"}"
            ;;
    esac
};

Main() {
    if [[ -z "$COMMANDS_PIPE" ]]; then
        echo "Error: COMMANDS_PIPE is not set."
        return 1
    fi
    sleep 5
    eval "$(dirname "$SCRIPT_PATH")/Splasher" $FONTS_PATH -l -logo $SelectedLogo -a &
    RunHTTPServer
}

if [ $# -eq 0 ]; then
    echo "Running server on port: $REST_PORT"
    Main
elif [ $1 = "APIRequestsHandler" ]; then
    APIRequestsHandler $2
fi

