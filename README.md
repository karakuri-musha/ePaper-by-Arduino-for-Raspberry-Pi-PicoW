# ePaper by Arduino for Raspberry Pi PicoW

## OverView
Arduino を使って Raspberry Pi Pico W からWaveShareのe-Paper（電子ペーパー）を制御するプログラムを作りました。
プログラムを実行するとWi-Fi接続し、OpenWeatherサービスから天気情報を取得後、e-Paperに結果を描画します。

詳しくは、次のブログで紹介していますので確認してください。

I made a program to control WaveShare's e-Paper (electronic paper) from Raspberry Pi Pico W using Arduino.
Run the program to connect to Wi-Fi, get weather information from the OpenWeather service, and draw the results on e-Paper.

For details, please check the following blog.

### Blog URL
https://karakuri-musha.com/inside-technology/arduino-raspberrypi-picow-epaper01/

## Src File
"src"フォルダにソース一式が入っています。Arduino IDEをインストールしたパソコンにフォルダごとダウンロードして、「RaspberryPi_Pico_W_epaper.ino」を開いてください。

The "src" folder contains a set of sources. Download each folder to a computer with Arduino IDE installed, and open "RaspberryPi_Pico_W_epaper.ino".

## Operating environment
動作確認環境は以下の通りです。

### 1.HARDWARE
- Raspberry Pi Pico W
- WaveShare Raspberry Pi Pico用 2.13 e-Paper 3 Color 212×104

### 2.IDE
- Arduino IDE (ver:2.0.4）

### 3.BoardManager
- [Name] Raspberry Pi Pico/RP2040
- [BoardManager URL] https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

### 4.Additional library
- ArduinoJson
- Time By Michael Margolis　（1.6.1）
- epd2in13b (e-Paper) *1

*1 このライブラリはArduino IDEではインストールできません。GitHubからダウンロードして追加する必要があります。その際の手順は上記ブログURLにて説明しています。

This library cannot be installed with the Arduino IDE. You need to download it from GitHub and add it. The procedure for doing so is explained in the blog URL above.

### 3.Other
 ライブラリやボードマネージャなどの構成は、上記のBlog URLを参照してください。
 
 Please refer to the blog URL above for configuration of libraries, boards manager, etc.
