#!/bin/bash

SCRIPT_PATH=$(realpath "${BASH_SOURCE[0]}")
REST_PORT=13222
LOGO_PATH="$(dirname "$0")/logo.png"

RunHTTPServer() {
    socat TCP-LISTEN:$REST_PORT,fork,reuseaddr SYSTEM:"$SCRIPT_PATH APIRequestsHandler"
}

ServeHTMLPage() {
    echo -ne "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n"
    cat "$(dirname "$SCRIPT_PATH")/Index.html"
}

KillStart(){
    if pgrep -x "$1" >/dev/null; then
        pkill $1
    fi;
    if ! pgrep -x "$2" >/dev/null; then
        "$(dirname "$SCRIPT_PATH")/$2" -a &
        sleep 0.5;
    fi;
}

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
        IFS= read -r -n "$ContentLength" Body
    fi;

    case "$RequestPath" in
        "/set_line_text")
            KillStart Splasher Controller
            LineNum=$(echo "$Body" | jq ".line_num")
            LineText=$(echo "$Body" | jq ".text")
            LineColor=$(echo "$Body" | jq ".color")
            if [[ $LineNum -ge 1 && $LineNum -le 3 ]]; then
                PipePath="/tmp/pipe$LineNum"
		printf ${LineText//\"/} > "$PipePath"
                echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\"}"
            else
                echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"error\": \"Invalid Line Number\"}"
            fi;
            ;;
        "/set_line_time")
            KillStart Splasher Controller
            LineNum=$(echo "$Body" | jq ".line_num")
            # Implement your logic for setting line time here
            echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\", \"message\": \"Time set on line $LineNum\"}"
            ;;
        "/set_line_blink")
            KillStart Splasher Controller
            LineNum=$(echo "$Body" | jq ".line_num")
            BlinkFreq=$(echo "$Body" | jq ".blink_freq")
            BlinkTime=$(echo "$Body" | jq ".blink_time")
            # Implement your logic for setting line blink here
            echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\", \"message\": \"Blink set on line $LineNum with frequency $BlinkFreq and duration $BlinkTime\"}"
            ;;
        "/set_splasher")
            KillStart Controller Splasher
            Splash=$(echo "$Body" | jq ".splash")
            ShowIP=$(echo "$Body" | jq ".show_ip")
            # Implement your logic for splash on boot here
            echo -ne "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"success\", \"message\": \"Splash on boot set to $Splash, show IP set to $ShowIP\"}"
            ;;
        *)
            echo -ne "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"status\": \"error\", \"message\": \"Endpoint not found\"}"
            ;;
    esac
}

Main(){
    "$(dirname "$SCRIPT_PATH")/Splasher" -a &
    RunHTTPServer 
};

if [ $# -eq 0 ]; then
    echo "Running server on ports: $REST_PORT"
    Main;
elif [ $1 = "APIRequestsHandler" ];then
    APIRequestsHandler $2;
fi;
