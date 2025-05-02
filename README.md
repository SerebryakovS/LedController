Controlling RGB LED display with Allwinner H3
============================================================
This is a total hack of the Raspberry PI RGB Matrix Library.

Supports ONE CHAIN with many panels each.
### Installation
```
sudo timedatectl set-timezone Asia/Almaty
sudo apt-get update
sudo apt-get install socat jq cmake -y
git clone https://github.com/SerebryakovS/LedController.git
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON/
make && sudo make install
sudo ldconfig
cd ../LedController/
make
cd app/
make Controller Splasher
cd ../
CURRENT_DIR=$(pwd)/app
sed -i "s|<PATH_TO_SERVER_SH>|$CURRENT_DIR|g" "./lc.service"
sudo cp lc.service /etc/systemd/system/lc.service
sudo systemctl daemon-reload
sudo systemctl enable lc.service
sudo systemctl start lc.service
reboot
```
HTTP Server works on port: 13222

### Rest API

#### POST set_line_text
```
{
  "line_num": 1,             // Номер строки (1-3)
  "text": "Hello!",          // Текст для отображения
  "color": "FF0000",         // Цвет текста (HEX, 6 символов)
  "font": "huge"             // [Опц.] Размер шрифта: tiny | medium | huge (по умолчанию: huge)
  "center" : true | false    // Центрировать строку по ширине табло
}
```
Supported colors:
```
"FF0000" — красный
"00FF00" — зеленый
"0000FF" — синий
"FFFF00" — желтый
"FF00FF" — фиолетовый
"00FFFF" — голубой
"FFFFFF" — белый
```
#### POST set_all_lines
```
{
	"lines_out" : [
		{
			"line_num" : 1,
			"text" : "123456", 
			"color" : "FFFFFF" 
		},
		...
	]
}
```
#### POST set_line_time
```
{
	"line_num" : 1
}
```
#### POST set_line_blink
```
{
	"line_num" : 1,
	"blink_freq" : "1",  // [Hz], recommended range: (1,10)
	"blink_time" : "10"  // [Sec]
}
```
#### POST set_splasher
```
{
	"show_ip" : true | false,
	"logo_type" : 
	
}
```
#### POST set_colored_status
```
{
	"color" : "FFFFFF",
	"text" : "123456"
}
```
\_(°‿°)_/
