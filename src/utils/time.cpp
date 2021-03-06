#include "matador/utils/time.hpp"
#include "matador/utils/string.hpp"
#include "matador/utils/strptime.hpp"

#include <stdexcept>
#include <cstring>
#include <cmath>
#include <vector>

#ifndef _MSC_VER
#include <sys/time.h>
#endif

namespace matador {

namespace detail {

  void localtime(const time_t &in, struct tm &out)
  {
#ifdef _MSC_VER
    errno_t err = localtime_s(&out, &in);
#else
    localtime_r(&in, &out);
#endif
  }

#ifdef _MSC_VER
  /* FILETIME of Jan 1 1970 00:00:00. */
  static const unsigned __int64 epoch = ((unsigned __int64)116444736000000000ULL);
#endif

  int gettimeofday(struct timeval * tp, struct timezone *)
  {
#ifdef _MSC_VER
    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

    return 0;
#else
    return ::gettimeofday(tp, 0);
#endif
  }
}

void throw_invalid_time(int h, int m, int s, long ms)
{
  if (!time::is_valid_time(h, m, s, ms)) {
    throw std::logic_error("time isn't valid");
  }
}

time::time()
{
  if (detail::gettimeofday(&time_, 0) != 0) {
    throw std::logic_error("couldn' get time of day");
  }
  auto temp = this->time_.tv_sec;
  detail::localtime(temp, tm_);
}

time::time(time_t t)
{
  set(t, 0);
}

time::time(struct timeval tv)
{
  set(tv);
}

time::time(int year, int month, int day, int hour, int min, int sec, long millis)
{
  set(year, month, day, hour, min, sec, millis);
}

//time::time(uint64_t microseconds)
//{
//  time_.tv_sec = microseconds / 1000000;
//  time_.tv_usec = microseconds % 1000000;
//  localtime_r(&time_.tv_sec, &tm_);
//}

time::time(const time &x)
  : time_(x.time_)
  , tm_(x.tm_)
{}

time &time::operator=(const time &x)
{
  time_ = x.time_;
  tm_ = x.tm_;
  return *this;
}

time::~time()
{
}

bool time::operator==(const time &x) const
{
  return time_.tv_sec == x.time_.tv_sec &&
    time_.tv_usec == x.time_.tv_usec;
}

bool time::operator!=(const time &x) const
{
  return !(*this == x);
}

bool time::operator<(const time &x) const
{
  return timercmp(&time_, &x.time_, <);
}

bool time::operator<=(const time &x) const
{
  return *this == x || *this < x;
}

bool time::operator>(const time &x) const
{
  return !(*this <= x);
}

bool time::operator>=(const time &x) const
{
  return !(*this < x);
}

time time::now()
{
  return time();
}

bool time::is_valid_time(int hour, int min, int sec, long millis) {
  if (hour < 0 || hour > 23) {
    return false;
  }
  if (min < 0 || min > 59) {
    return false;
  }
  if (sec < 0 || sec > 59) {
    return false;
  }
  return !(millis < 0 || millis > 9999);
}

time time::parse(const std::string &tstr, const char *format)
{
  /*
  * find the %f format token
  * and split the string to parse
  */
  const char *pch = strstr(format, "%f");

  std::string part(format, (pch ? pch-format : strlen(format)));
  struct tm tm;
  memset(&tm, 0, sizeof(struct tm));
  const char *endptr = detail::strptime(tstr.c_str(), part.c_str(), &tm);
  unsigned long usec = 0;
  if (endptr == nullptr && pch != nullptr) {
    // parse error
    throw std::logic_error("error parsing time");
  } else if (pch != nullptr) {
    char *next;
    usec = std::strtoul(endptr, &next, 10);
    // calculate precision
    unsigned digits = (unsigned int) (next - endptr);
    usec *= (unsigned long)pow(10.0, 6 - digits);
    if ((size_t)(next - format) != strlen(format)) {
      // still time string to parse
      detail::strptime(next, pch+2, &tm);
    }
  }

  tm.tm_isdst = date::is_daylight_saving(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  struct timeval tv;
#ifdef _MSC_VER
  tv.tv_sec = (long)mktime(&tm);
  tv.tv_usec = usec;
#else
  tv.tv_sec = mktime(&tm);
  tv.tv_usec = usec;
#endif
  return matador::time(tv);
}

void time::set(int year, int month, int day, int hour, int min, int sec, long millis)
{
  throw_invalid_date(day, month, year);
  throw_invalid_time(hour, min, sec, millis);

  time_t rawtime;
  ::time(&rawtime);

  struct tm t;
  detail::localtime(rawtime, t);
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  t.tm_isdst = date::is_daylight_saving(year, month, day);

#ifdef _MSC_VER
  time_t tt = mktime(&t);
  this->time_.tv_sec = (long)tt;
#else
  this->time_.tv_sec = mktime(&t);
#endif
  this->time_.tv_usec = millis * 1000;

  auto temp = this->time_.tv_sec;
  detail::localtime(temp, this->tm_);
}

void time::set(time_t t, long millis)
{
#ifdef _MSC_VER
  time_.tv_sec = (long)t;
#else
  time_.tv_sec = t;
#endif
  time_.tv_usec = millis * 1000;
  auto temp = this->time_.tv_sec;
  detail::localtime(temp, tm_);
}

void time::set(const date &d)
{
  set(d.year(), d.month(), d.day(), 0, 0, 0, 0);
}

void time::set(timeval tv)
{
  time_ = tv;
  auto temp = this->time_.tv_sec;
  detail::localtime(temp, tm_);
}

int time::year() const
{
  return tm_.tm_year + 1900;
}

int time::month() const
{
  return tm_.tm_mon + 1;
}

int time::day() const
{
  return tm_.tm_mday;
}

int time::hour() const
{
   return tm_.tm_hour;
}

int time::minute() const
{
  return tm_.tm_min;
}

int time::second() const
{
  return tm_.tm_sec;
}

int time::milli_second() const
{
  return (int)(time_.tv_usec / 1000);
}

time &time::year(int y)
{
  sync_year(y);
  return *this;
}

time &time::month(int m)
{
  sync_month(m);
  return *this;
}

time &time::day(int d)
{
  sync_day(d);
  return *this;
}

time &time::hour(int h)
{
  sync_hour(h);
  return *this;
}

time &time::minute(int m)
{
  sync_minute(m);
  return *this;
}

time &time::second(int s)
{
  sync_second(s);
  return *this;
}

time &time::milli_second(int ms)
{
  sync_milli_second(ms);
  return *this;
}

int time::day_of_week() const
{
  return tm_.tm_wday;
}

int time::day_of_year() const
{
  return tm_.tm_yday;
}

bool time::is_leapyear() const
{
  return date::is_leapyear(tm_.tm_year + 1900);
}

bool time::is_daylight_saving() const
{
  return tm_.tm_isdst == 1;
}

struct timeval time::get_timeval() const
{
  return time_;
}

struct tm time::get_tm() const
{
  return tm_;
}

date time::to_date() const
{
  return matador::date(tm_.tm_mday, tm_.tm_mon + 1,  tm_.tm_year + 1900);
}

std::ostream& operator<<(std::ostream &out, const time &x)
{
  out << to_string(x);
  return out;
}

void time::sync_day(int d)
{
  sync_time(tm_.tm_year + 1900, tm_.tm_mon + 1, d, tm_.tm_hour, tm_.tm_min, tm_.tm_sec, time_.tv_usec / 1000);
}

void time::sync_month(int m)
{
  sync_time(tm_.tm_year + 1900, m, tm_.tm_mday, tm_.tm_hour, tm_.tm_min, tm_.tm_sec, time_.tv_usec / 1000);
}

void time::sync_year(int y)
{
  sync_time(y, tm_.tm_mon + 1, tm_.tm_mday, tm_.tm_hour, tm_.tm_min, tm_.tm_sec, time_.tv_usec / 1000);
}

void time::sync_hour(int h)
{
  sync_time(tm_.tm_year + 1900, tm_.tm_mon + 1, tm_.tm_mday, h, tm_.tm_min, tm_.tm_sec, time_.tv_usec / 1000);
}

void time::sync_minute(int m)
{
  sync_time(tm_.tm_year + 1900, tm_.tm_mon + 1, tm_.tm_mday, tm_.tm_hour, m, tm_.tm_sec, time_.tv_usec / 1000);
}

void time::sync_second(int s)
{
  sync_time(tm_.tm_year + 1900, tm_.tm_mon + 1, tm_.tm_mday, tm_.tm_hour, tm_.tm_min, s, time_.tv_usec / 1000);
}

void time::sync_milli_second(int ms)
{
  sync_time(tm_.tm_year + 1900, tm_.tm_mon + 1, tm_.tm_mday, tm_.tm_hour, tm_.tm_min, tm_.tm_sec, ms);
}

void time::sync_time(int y, int m, int d, int h, int min, int s, long ms)
{
  set(y, m, d, h, min, s, ms);
}

}
