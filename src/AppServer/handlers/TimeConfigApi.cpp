#include "TimeConfigApi.h"

#include <M5StickC.h>

#include "../../Time/TimeManager.h"
#include "ArduinoJson.h"
#include "HttpConstants.h"
#include "HttpUtil.h"

using namespace std;

void TimeConfigApi::setTime(HTTPRequest* req, HTTPResponse* res) {
  unique_ptr<char[]> reqBodyText = HttpUtil::readRequestBody(req, 100);

  if (reqBodyText == nullptr) {
    res->setStatusCode(413);
    res->println("Request entity too large");
    return;
  }

  JsonDocument reqBody;
  JsonDocument filter;
  filter["timestamp"] = true;
  deserializeJson(reqBody, reqBodyText.get(),
                  DeserializationOption::Filter(filter));

  if (reqBody["timestamp"].isUnbound()) {
    res->setStatusCode(400);
    res->setHeader(HttpHeader::ContentType, MimeType::JSON);
    res->println("{ \"success\": false, \"error\": \"Missing timestamp\" }");
    return;
  }

  // Get Unix timestamp (seconds since epoch)
  long long timestamp = reqBody["timestamp"].as<long long>();

  // Convert Unix timestamp to struct tm
  time_t rawtime = (time_t)timestamp;
  struct tm* timeinfo = gmtime(&rawtime);

  if (timeinfo == nullptr) {
    res->setStatusCode(400);
    res->setHeader(HttpHeader::ContentType, MimeType::JSON);
    res->println("{ \"success\": false, \"error\": \"Invalid timestamp\" }");
    return;
  }

  // Set RTC date
  RTC_DateTypeDef date;
  date.Date = timeinfo->tm_mday;
  date.Month = timeinfo->tm_mon + 1;
  date.Year = timeinfo->tm_year + 1900;
  date.WeekDay = timeinfo->tm_wday;

  // Set RTC time
  RTC_TimeTypeDef time;
  time.Hours = timeinfo->tm_hour;
  time.Minutes = timeinfo->tm_min;
  time.Seconds = timeinfo->tm_sec;

  M5.Rtc.SetTime(&time);
  M5.Rtc.SetData(&date);

  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  res->println("{ \"success\": true }");
}

void TimeConfigApi::getTime(HTTPRequest* req, HTTPResponse* res) {
  TimeManager* timeManager = TimeManager::getInstance();
  TimeData currentTime = timeManager->getTime();

  JsonDocument respBody;
  respBody["year"] = currentTime.year;
  respBody["month"] = currentTime.month;
  respBody["day"] = currentTime.day;
  respBody["hour"] = currentTime.hour;
  respBody["minute"] = currentTime.minute;
  respBody["second"] = currentTime.second;

  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  serializeJson(respBody, *res);
}
