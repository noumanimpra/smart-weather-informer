
#include <Deneyap_SicaklikNemBasincOlcer.h> 
#include <Deneyap_YagmurAlgilama.h>
#include "WiFi.h"
#include "WiFiClientSecure.h"
//#include "mp34dt05.h"
#include "time.h"
#include <UniversalTelegramBot.h>

#define WIFI_SSID "WiFi_NAME"                                     // Bağlantı kurulacak Wi-Fi ağı adı
#define WIFI_PASSWORD "WiFi_NAME"                                 // Bağlantı kurulacak Wi-Fi ağı şifresi
#define BOT_TOKEN "BOT_TOKEN"                                     // Telegram BOT Token
#define PRIVATE_CHAT_ID "CHAT_ID"

const unsigned long BOT_MTBS = 1000;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;

SHT4x TempHum; 
Rain YagmurSensor;

float sicaklik;
float nem;
float temp_x;
String temp_y;
int ledStatus = 0;
int currentHour = -1;
int lastHour = -1;
bool f_muteMode = false;
int rainProc()
{

  bool yagmurDurum = YagmurSensor.ReadRainDigital();
  if (yagmurDurum)
    return 0;
  else
    return 1;
}
int tempProc()
{
  sicaklik = TempHum.TtoDegC();
  return sicaklik;
}
void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/yagmur")
    {

      if (rainProc())
        bot.sendMessage(chat_id, "🌧 yağmur var", "");
      else
        bot.sendMessage(chat_id, "😎 yağmur yok", "");
    }

    if (text == "/sicaklik")
    {
      temp_x = tempProc();
      temp_y = String(temp_x);
      bot.sendMessage(chat_id, "🌡️ Dış Sıcaklık [" + temp_y + " °C]", "");
    }
    if (text == "/mute")
    {
      if (f_muteMode)
        f_muteMode = false;
      else
        f_muteMode = true;
    }

    if (text == "/status")
    {
      temp_x = tempProc();
      temp_y = String(temp_x);
      bot.sendMessage(chat_id, "🌡️ Dış Sıcaklık [" + temp_y + " °C]", "");
      if (rainProc())
        bot.sendMessage(chat_id, "🌧 yağmur var. Şemsiyeni al", "");
      else
        bot.sendMessage(chat_id, "😎 yağmur yok", "");
    }

    if (text == "/start")
    {
      String welcome = "Deneyap Telegram Botuna hosgeldin " + from_name + ".\n\n";
      welcome += "/sicaklik : Dış Sıcaklık Bilgisi\n";
      welcome += "/yagmur : Yagmur Bilgisi\n";
      welcome += "/mute : Sessiz Mod\n";
      welcome += "/status : Status get\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}
void setup()
{
  Serial.begin(115200); 
  if (!TempHum.begin(0X44))
  {             
     delay(3000);
    Serial.println("I2C bağlantısı başarısız temp "); 
    while (1)
      ;
  }
  if (!YagmurSensor.begin(0x2E))
  { 
    delay(3000);
    Serial.println("I2C bağlantısı başarısız2 "); 
    while (1)
      ;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
 
}

void loop()
{
 
  TempHum.measure();
  sicaklik = TempHum.TtoDegC();
  nem = TempHum.RHtoPercent();

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
  if (get_current_time() != lastHour && get_current_time() != 1327 && !f_muteMode)
  {
    lastHour = currentHour;
    bot.sendMessage(PRIVATE_CHAT_ID, "⏰ [Ev] konumu saatlik [Dış Mekan] bilgileri.", "");   
    temp_x = tempProc();
    temp_y = String(temp_x);
    bot.sendMessage(PRIVATE_CHAT_ID, "🌡️ Dış Sıcaklık [" + temp_y + " °C]", "");
    if (rainProc())
      bot.sendMessage(PRIVATE_CHAT_ID, "🌧 yağmur var. Şemsiyeni al", "");
    else
       bot.sendMessage(PRIVATE_CHAT_ID, "😎 yağmur yok", "");
  }
}
int get_current_time()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return 1327;
  }
  currentHour = timeinfo.tm_hour;
  //Serial.println(currentHour);
  return currentHour;
}
