#!/bin/bash

SCRIPT_PATH=$(realpath "${BASH_SOURCE[0]}")
REST_PORT=13222
FONTS_PATH="$(dirname "$0")"/../fonts/
LOGO_PATH="$(dirname "$0")/logo.png"
COMMANDS_PIPE="/tmp/LedCommandsPipe"

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
    if [[ $LineNum -ge 1 && $LineNum -le 3 ]]; then
        Command="{\"cmd\":\"set_line_text\",\"line_num\":$LineNum,\"text\":$LineText,\"color\":$LineColor}"
        echo "$Command" > "$COMMANDS_PIPE"
        echo "success"
    else
        echo "error: Invalid line number"
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
		read -r -d '' -n "$ContentLength" Body <&0
	fi
	echo $Body > /tmp/tempo;
	
	
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
				echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"error\": \"$ProcessLineResult\"}"
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
        "/set_splasher")
			KillProcess Splasher;
			ShowIP=$(echo "$Body" | jq ".show_ip")
            if [[ "$ShowIP" == "true" ]];then
				echo "HERE"
				KillStart Controller Splasher -a
			else
				KillStart Controller Splasher
			fi;
            echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
            ;;
        *)
            echo -ne "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"status\": \"error\", \"message\": \"Endpoint not found\"}"
            ;;
    esac
};

Main(){
	sleep 10;
    eval "$(dirname "$SCRIPT_PATH")/Splasher" $FONTS_PATH -a &
	RunHTTPServer 
};

if [ $# -eq 0 ]; then
    echo "Running server on ports: $REST_PORT"
    Main;
elif [ $1 = "APIRequestsHandler" ];then
    APIRequestsHandler $2;
fi;
