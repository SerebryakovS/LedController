Controlling RGB LED display with Allwinner H3
============================================================
This is a total hack of the Raspberry PI RGB Matrix Library.

A library to control commonly available 64x64, 32x32 or 16x32 RGB LED panels.

Supports ONE CHAIN with many panels each.

### Rest API

'''
POST set_line_text{
	"line_num" : 1, // 1,2,3
	"text" : "AX444E",
	"color" : "default" // color hex value
}
'''
'''
POST set_line_time{
	"line_num" : 1
}
'''
'''
POST set_line_blink{
	"line_num" : 1,
	"blink_freq" : "default", // or number in [Hz]
	"blink_time" : "forever" // "none", or number of [Sec]
}
'''
'''
POST set_splasher{
	"splash" : true | false,
	"show_ip" : true | false 
}
'''
