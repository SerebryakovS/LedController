Controlling RGB LED display with Allwinner H3
============================================================
This is a total hack of the Raspberry PI RGB Matrix Library.

A library to control commonly available 64x64, 32x32 or 16x32 RGB LED panels.

Supports ONE CHAIN with many panels each.

app/ requires cJSON library to be installed on target system:
https://github.com/DaveGamble/cJSON

don't forget to configure system timezone(if needed):
```
sudo timedatectl set-timezone Asia/Almaty
```

### Rest API

POST set_line_text
```
{
	"line_num" : 1, // available lines: (1,2,3)
	"text" : "123456", // color hex value
	"color" : "FFFFFF" // used colors: (FF0000, 00FF00, 0000FF, FFFF00, FF00FF, 00FFFF)
}
```
POST set_all_lines
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
POST set_line_time
```
{
	"line_num" : 1
}
```
POST set_line_blink
```
{
	"line_num" : 1,
	"blink_freq" : "1",  // [Hz], recommended range: (1,10)
	"blink_time" : "10"  // [Sec]
}
```
POST set_splasher
```
{
	"show_ip" : true | false 
}
```
