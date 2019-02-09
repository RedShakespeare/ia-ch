// =============================================================================
// Copyright 2011-2019 Martin Törnqvist <m.tornq@gmail.com>
//
// SPDX-License-Identifier: AGPL-3.0-or-later
// =============================================================================

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
                int year_val,
                int month_val,
                int day_val,
                int hour_val,
                int minute_val,
                int second_val) :

                year(year_val),
                month(month_val),
                day(day_val),
                hour(hour_val),
                minute(minute_val),
                second(second_val) {}

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
