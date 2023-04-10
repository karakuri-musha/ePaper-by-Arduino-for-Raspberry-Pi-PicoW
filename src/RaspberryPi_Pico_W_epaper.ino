// -----------------------------------------------------------------------
// Raspberry Pi Pico W e-Paper Test Program
//
//　2.13インチ3色電子ペーパーへの画像表示プログラム
//
// 2023.04.05
// Author   : げんろく@karakuri-musha
// Device   : Raspberry Pi Pico W
//            Raspberry Pi Pico用 2.13インチ e-Paper ディスプレイ（白黒赤）212×104
//            
// License:
//    See the license file for the license.
// -----------------------------------------------------------------------
// ------------------------------------------------------------
// ライブラリインクルード部 Library include section.
// ------------------------------------------------------------
#include <WiFi.h>                                             // Wifi制御用ライブラリ
#include <time.h>                                             // 時刻制御用ライブラリ
#include <TimeLib.h>                                          // 時刻変換用ライブラリ
#include <stdio.h>                                            // 標準入出力処理ライブラリ

#include <HTTPClient.h>                                       // Http制御クライアントライブラリ
#include <WiFiClientSecure.h>                                 // Https制御クライアントライブラリ

#include <ArduinoJson.h>                                      // JSONデータ制御用ライブラリ

#include <SPI.h>                                              // SPI通信用ライブラリ

#include "epd2in13b.h"                                        // e-Paper 用ライブラリ
#include "epdpaint.h"                                         // e-Paper 用ライブラリ

#include "Original_Icon_src.h"                                // 画面表示用の画像データ

// ------------------------------------------------------------
// 定数/変数　定義部　Constant / variable definition section.
// ------------------------------------------------------------ 
#define COLORED     0
#define UNCOLORED   1

const char* ssid = "your-ssid";                          // ご自分のWi-FiルータのSSIDを記述します。 "your-ssid"
const char* password = "your-password";                            // ご自分のWi-Fiルータのパスワードを記述します。"your-password"

// NTP接続情報　NTP connection information.
const char* NTPSRV      = "ntp.jst.mfeed.ad.jp";              // NTPサーバーアドレス NTP server address.
const long  GMT_OFFSET  = 9;                                  // GMT-TOKYO(時差９時間）9 hours time difference.
int current_hour;

// OpenWeatherへのhttpsクエリ生成用
const String sitehost = "api.openweathermap.org";             // OpenWeatherのサイトホスト名
const String Api_KEY  = "Please correct it to the value that suits your environment.";   // API-KEY（ユーザにより変わります。）
const String lang = "ja";                                     // 言語設定
char geo_lat[20] = "35.6828";                                 // 緯度（東京）
char geo_lon[20] = "139.759";                                 // 経度（東京）

// JSON用
const size_t mem_cap = 2048;                                  // JSON形式データ格納用メモリサイズ
StaticJsonDocument<mem_cap> n_jsondata;                       // JSON形式データ格納用

// 天気ステータス格納用
uint8_t Weather_Stats = 0;                                    // 0:初期化 1:雨 2:雪 3:注意 4:晴れ 5:曇り

// OpenWeatherのルート証明書を定義
const char* ow_rootca = \
"-----BEGIN CERTIFICATE-----\n" \
//"Please correct it to the value that suits your environment." 
"-----END CERTIFICATE-----\n";

Epd epd;

// ------------------------------------------------------------
// 時刻同期 関数　Time synchronization function.
// ------------------------------------------------------------ 
void setClock(const char* ntpsrv, const long gmt_offset) {
  char buf[64];                                                 // 日時出力用

  NTP.begin(ntpsrv);                                            // NTPサーバとの同期
  NTP.waitSet();                                                // 同期待ち

  time_t now = time(nullptr);                                   // 時刻の取得
  struct tm timeinfo;                                           // tm（時刻）構造体の生成
  gmtime_r(&now, &timeinfo);                                    // time_tからtmへ変換

  setTime(timeinfo.tm_hour,                                     // 時刻表示用の領域に時間をセット
          timeinfo.tm_min, 
          timeinfo.tm_sec, 
          timeinfo.tm_mday, 
          timeinfo.tm_mon+1, 
          timeinfo.tm_year+1900); 
  adjustTime(gmt_offset * SECS_PER_HOUR);                       // 時刻を日本時間へ変更
  Serial.print("Current time: ");                               // シリアル出力
  sprintf(buf,                                                  // シリアル出力用に日時データの編集
          " %04d-%02d-%02d %02d:%02d:%02d\n", 
          year(), 
          month(), 
          day(), 
          hour(), 
          minute(), 
          second());
  Serial.print(buf);                                             // 日時データのシリアル出力
}

// ------------------------------------------------------------
// https要求実行 関数　
// ------------------------------------------------------------
bool Https_GetRes(String url_str, 
                  String *payload) {
  
  if (WiFi.status() != WL_CONNECTED)                              // Wi-Fi接続の確認
    return false;                                                 // 未接続の場合Falseを戻す

  // OpenWeather に対して天気データを要求
  // https通信を行い、JSON形式データを取得する
  WiFiClientSecure *client = new WiFiClientSecure;                // インスタンス生成
  if(client) {
    client -> setCACert(ow_rootca);                               // ルート証明書のセット
    {
      HTTPClient https;                                           // HTTPClientのhttpsスコープブロックを追加

      Serial.print("[HTTPS] begin...\n");                         // シリアルポート出力
      if (https.begin(*client, url_str)) {                        // HTTPS要求セット
        Serial.print("[HTTPS] GET...\n");                         // シリアルポート出力
        int httpCode = https.GET();
  
        if (httpCode > 0) {
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);   // シリアルポート出力
  
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            *payload = https.getString();                         // OpenWeatherから取得したデータの格納
            Serial.println(*payload);                             // シリアルポート出力
            return true;                                          // 取得成功
          } else {
            return false;                                         // 取得失敗
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
          return false;                                           // 取得失敗
        }
        https.end();                                              // Https通信の終了
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");             // シリアルポート出力
        return false;                                             // 取得失敗
      }
    }
    delete client;                                                // Clientの削除
  } else {
    Serial.println("Unable to create client");                    // シリアルポート出力
    return false;                                                 // 取得失敗
  }  
}

// ============================================================
// OpenWeatherデータ取得：描画 関数　
// ============================================================
void OpenWeather_get_print() {

  // 天気情報取得用URLの生成
  String url = "https://" + String(sitehost);                     // 要求先ホストの指定
  url += "/data/2.5/weather?";                                    // APIの指定
  url += "lat=" + String(geo_lat) + "&lon=" + String(geo_lon);    // 経度、緯度の指定
  url += "&units=metric&lang=" + lang;                            // 言語設定の指定
  url += "&appid=" + Api_KEY;                                     // APIキーの指定
  Serial.println(url);                                            // シリアルポート出力

  // 天気情報の取得とJSONデータからの値の取得
  String payload;                                                 // 取得データ格納用

  // OpenWeatherへのデータを要求が正常に行えた場合
  if (Https_GetRes(url, &payload)){                               
    DeserializationError error = deserializeJson(n_jsondata, payload); // JSONデータの格納
                                                                  
    if (error) {                                                  // JSONデータ格納に失敗した場合
      Serial.print(F("deserializeJson() failed: "));              // シリアルモニタに出力
      Serial.println(error.f_str());                              // シリアルモニタにエラーを出力
    } else {                                                      // 正常な場合はJSON形式データから値を取得して画面表示
                                                                  // JSON形式データから値の取得
      const int weather_id = n_jsondata["weather"][0]["id"];      // 天気IDの取得
      Serial.println("WeatherID : "+ String(weather_id));                                     
      // 画面表示内容の生成
      if (weather_id >= 200 && weather_id < 600) {                // 雨と判定
        Weather_Stats = 1;                                        // 天気ステータス設定
        epaper_print(Weather_Stats);                              // e-Paper画面表示
      }
      else if (weather_id >= 600 && weather_id < 700) {           // 雪と判定
        Weather_Stats = 2;                                        // 天気ステータス設定
        epaper_print(Weather_Stats);                              // e-Paper画面表示
      }
      else if (weather_id >= 700 && weather_id < 800) {           // その他の天気と判定
        Weather_Stats = 3;                                        // 天気ステータス設定
        epaper_print(Weather_Stats);                              // e-Paper画面表示
      }
      else if (weather_id == 800) {                               // 晴と判定
        Weather_Stats = 4;                                        // 天気ステータス設定
        epaper_print(Weather_Stats);                              // e-Paper画面表示
      }
      else {                                                      // 曇りと判定
        Weather_Stats = 5;                                        // 天気ステータス設定
        epaper_print(Weather_Stats);                              // e-Paper画面表示
      }    
    }
  }
  // OpenWeatherへのデータを要求が失敗した場合
  else {
    Serial.println("Connection failed (OpenWeather)");            // シリアルモニタに出力
  }
}

// ============================================================
// e-Paper表示 関数　
// ============================================================
void epaper_print(int weather_stats) {
  switch (weather_stats) {                                        // Weather＿Statsの値によって表示するアイコンを変更
    case 1:
      Serial.println("WeatherStats : 1");
      epd.ClearFrame();
      epd.DisplayFrame(epd_bitmap_plated01_BLACK, epd_bitmap_plated01_RED);     // 背景ロゴの表示
      epd.SetPartialWindowBlack(epd_bitmap_Rain01_g, 0, 80, 80, 40);            // 文字画像の表示（メモリ）                                                
      epd.SetPartialWindowBlack(epd_bitmap_Rain01, 0, 0, 80, 80);               // 天気画像の表示（メモリ）
      epd.DisplayFrame();                                                       // メモリ描画内容の画面表示
      break;
    case 2:
      Serial.println("WeatherStats : 2");
      epd.ClearFrame();
      epd.DisplayFrame(epd_bitmap_plated01_BLACK, epd_bitmap_plated01_RED);     // 背景ロゴの表示
      epd.SetPartialWindowBlack(epd_bitmap_Snow01_g, 0, 80, 80, 40);            // 文字画像の表示（メモリ） 
      epd.SetPartialWindowBlack(epd_bitmap_Snow01, 0, 0, 80, 80);               // 天気画像の表示（メモリ）
      epd.DisplayFrame();                                                       // メモリ描画内容の画面表示
      break;
    case 3:
      Serial.println("WeatherStats : 3");
      epd.ClearFrame();
      epd.DisplayFrame(epd_bitmap_plated01_BLACK, epd_bitmap_plated01_RED);     // 背景ロゴの表示
      epd.SetPartialWindowBlack(epd_bitmap_Caut01_g, 0, 80, 80, 40);            // 文字画像の表示（メモリ）
      epd.SetPartialWindowBlack(epd_bitmap_Caut01, 0, 0, 80, 80);               // 天気画像の表示（メモリ）
      epd.DisplayFrame();                                                       // メモリ描画内容の画面表示
      break;
     case 4:
      Serial.println("WeatherStats : 4");
      epd.ClearFrame();
      epd.DisplayFrame(epd_bitmap_plated01_BLACK, epd_bitmap_plated01_RED);     // 背景ロゴの表示
      epd.SetPartialWindowBlack(epd_bitmap_Sun01_g, 0, 80, 80, 40);             // 文字画像の表示（メモリ）
      epd.SetPartialWindowBlack(epd_bitmap_Sun01, 0, 0, 80, 80);                // 天気画像の表示（メモリ）
      epd.DisplayFrame();                                                       // メモリ描画内容の画面表示
      break;
    case 5:
      Serial.println("WeatherStats : 5");
      epd.ClearFrame();
      epd.DisplayFrame(epd_bitmap_plated01_BLACK, epd_bitmap_plated01_RED);     // 背景ロゴの表示
      epd.SetPartialWindowBlack(epd_bitmap_Cloud01_g, 0, 80, 80, 40);           // 文字画像の表示（メモリ）
      epd.SetPartialWindowBlack(epd_bitmap_Cloud01, 0, 0, 80, 80);              // 天気画像の表示（メモリ）
      epd.DisplayFrame();                                                       // メモリ描画内容の画面表示
      break;
  }
}

// ============================================================
// 初期設定 関数　
// ============================================================
void setup() {
  // put your setup code here, to run once:
    // シリアルコンソールの開始　Start serial console.
  Serial.begin(9600); 
  delay(3000); 

  // Wi-Fi接続に向けたシリアルモニタ出力
  Serial.println();                                               // シリアルポート出力 
  Serial.println();                                               // シリアルポート出力 
  Serial.print("Connecting to ");                                 // シリアルポート出力 
  Serial.println(ssid);                                           // シリアルポート出力 

  WiFi.begin(ssid, password);                                     // Wi-Fi接続開始
  
  while (WiFi.status() != WL_CONNECTED) {                         // Wi-Fi接続の状況を監視（WiFi.statusがWL_CONNECTEDになるまで繰り返し 
    delay(500); 
    Serial.print("."); } 
  
  // Wi-Fi接続結果をシリアルモニタへ出力 
  Serial.println("");                                             // シリアルモニタに出力
  Serial.println("WiFi connected");                               // シリアルモニタに出力
  Serial.println("IP address: ");                                 // シリアルモニタに出力
  Serial.println(WiFi.localIP());                                 // シリアルモニタに出力

  // 時刻同期関数
  setClock(NTPSRV, GMT_OFFSET);

  // e-Paperの初期化
  if (epd.Init() != 0) {
    Serial.print("e-Paper init failed");
    return;
  }
  // 初期起動時ロゴの描画
  epd.ClearFrame();
  epd.DisplayFrame(epd_bitmap_plated01_BLACK, epd_bitmap_plated01_RED);
}

// ============================================================
//  Loop 関数　
// ============================================================
void loop() {
  // 一時間おきにOpenWeatherからのデータ取得と描画
  if (current_hour != hour()) {
    OpenWeather_get_print();  
    current_hour = hour();
  }
}
