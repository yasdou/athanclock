# athanclock

I am building an athan clock to remind me of my prayers. Maybe you want one too? 

When I was searching for one to buy I only found ugly clocks from 19. century or very new somewhat good locking clocks, that look like they are made from the cheapest plastic.

So I started to build one myself. It is very simple right now. But I hope that the project gets bigger with time. 

I am no Developer. The code is mainly written by ChatGPT. 

I used a NodeMCU (ESP8266 but might change to ESP32 for bluetooth and more power), a DFPlayer Mini, a 4 GB SD Card and a 8 Watt 3 Ohm speaker. 
I will provide schematics if you want to build it for yourself.

Main Goals: 

1. Show the Prayer Times of the day of my city [DONE]
2. Play the Athan when the Prayer is due [DOING]
3. Play a reminder 15 minutes before each prayer [DOING]
4. Show a countdown 15 minutes before each prayer [DOING]
5. Local Website or App to configure city, athan type etc
6. 


Plans for the future: 
1. Connect with Mawaqit to get the times of each respective mosque
2. Make it updateble OTA (best over internet and automatic each night over GitHub)
3. 

Vision: 
1. Make a lightweight and cheap athan clock that looks nice and is of benefit to its users
2. Build and sell it to mosques so they can resell it, so they have an income


Pin Belegung:

D0 -> Button down
D1 -> DF Player Mini TX
D2 -> DF Player Mini RX
D3 -> Display A0
D4 -> Display Reset
D5 -> Display SCK
D6 -> Button up
D7 -> Display SDA
D8 -> Display CS
