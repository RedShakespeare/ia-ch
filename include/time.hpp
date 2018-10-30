#ifndef TIME_HPP
#define TIME_HPP

#include <string>

enum class TimeType
{
        year,
        month,
        day,
        hour,
        minute,
        second
};

struct TimeData
{
        TimeData() {}

        TimeData(
                int year,
                int month,
                int day,
                int hour,
                int minute,
                int second) :

                year(year),
                month(month),
                day(day),
                hour(hour),
                minute(minute),
                second(second) {}

        std::string time_str(
                const TimeType lowest,
                const bool add_separators) const;

        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
};

TimeData current_time();

#endif // TIME_HPP
