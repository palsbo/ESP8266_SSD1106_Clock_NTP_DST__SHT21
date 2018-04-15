#ifndef MYNTP_H
#define MYNTP_H

#include <TimeLib.h>
#include <simpleDSTadjust.h>

#define NTP_SERVERS "us.pool.ntp.org", "pool.ntp.org", "time.nist.gov"
#define timezone +1 // 
struct dstRule StartRule = {"CET", Last, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT -4 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 2, 0};       // Standard time = UTC/GMT -5 hour
simpleDSTadjust dstAdjusted(StartRule, EndRule);

class {
  public:
    void set() {
      configTime(timezone * 3600, 0, NTP_SERVERS);
      while (time(nullptr) < 100000) delay(200);
      setTime(dstAdjusted.time(nullptr));
    }

    String twoDigits(int digits) {
      if (digits > 9) return String(digits);
      return '0' + String(digits);
    }

    String timeNow() {
      setTime(dstAdjusted.time(nullptr));
      return twoDigits(hour()) + ":" + twoDigits(minute()) + ":" + twoDigits(second());
    }

    String dateNow() {
      setTime(dstAdjusted.time(nullptr));
      return twoDigits(day()) + "-" + twoDigits(month()) + "-" + twoDigits(year());
    }
} MyNTP;

#endif

