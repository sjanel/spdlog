// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/details/os.h>
#include <spdlog/formatter.h>
#include <spdlog/details/fmt_helper.h>
#include <iterator>
#include <algorithm>

namespace spdlog {

class SPDLOG_API default_formatter final : public formatter
{
public:
    default_formatter() = default;
    ~default_formatter() = default;

    std::unique_ptr<formatter> clone() const override
    {
        return details::make_unique<default_formatter>();
    }

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4127) // consider using 'if constexpr' instead
#endif // _MSC_VER
    static const char *basename(const char *filename)
    {
        // if the size is 2 (1 character + null terminator) we can use the more efficient strrchr
        // the branch will be elided by optimizations
        if (sizeof(details::os::folder_seps) == 2)
        {
            const char *rv = std::strrchr(filename, details::os::folder_seps[0]);
            return rv != nullptr ? rv + 1 : filename;
        }
        else
        {
            const std::reverse_iterator<const char *> begin(filename + std::strlen(filename));
            const std::reverse_iterator<const char *> end(filename);

            const auto it = std::find_first_of(begin, end, std::begin(details::os::folder_seps), std::end(details::os::folder_seps) - 1);
            return it != end ? it.base() : filename;
        }
    }
#ifdef _MSC_VER
#    pragma warning(pop)
#endif // _MSC_VER

    void format(const details::log_msg &msg, memory_buf_t &dest) override
    {
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        using std::chrono::seconds;

        // cache the date/time part for the next second.
        auto duration = msg.time.time_since_epoch();
        auto secs = duration_cast<seconds>(duration);

        using namespace details;        
        if (cache_timestamp_ != secs || cached_datetime_.size() == 0)
        {
            auto tm_time = os::localtime(log_clock::to_time_t(msg.time));
            cached_datetime_.clear();
            cached_datetime_.push_back('[');
            fmt_helper::append_int(tm_time.tm_year + 1900, cached_datetime_);
            cached_datetime_.push_back('-');

            fmt_helper::pad2(tm_time.tm_mon + 1, cached_datetime_);
            cached_datetime_.push_back('-');

            fmt_helper::pad2(tm_time.tm_mday, cached_datetime_);
            cached_datetime_.push_back(' ');

            fmt_helper::pad2(tm_time.tm_hour, cached_datetime_);
            cached_datetime_.push_back(':');

            fmt_helper::pad2(tm_time.tm_min, cached_datetime_);
            cached_datetime_.push_back(':');

            fmt_helper::pad2(tm_time.tm_sec, cached_datetime_);
            cached_datetime_.push_back('.');

            cache_timestamp_ = secs;
        }
        dest.append(cached_datetime_.begin(), cached_datetime_.end());

        auto millis = fmt_helper::time_fraction<milliseconds>(msg.time);
        fmt_helper::pad3(static_cast<uint32_t>(millis.count()), dest);
        dest.push_back(']');
        dest.push_back(' ');

        // append logger name if exists
        if (msg.logger_name.size() > 0)
        {
            dest.push_back('[');
            fmt_helper::append_string_view(msg.logger_name, dest);
            dest.push_back(']');
            dest.push_back(' ');
        }

        dest.push_back('[');
        // wrap the level name with color
        msg.color_range_start = dest.size();
        // fmt_helper::append_string_view(level::to_c_str(msg.level), dest);
        fmt_helper::append_string_view(level::to_string_view(msg.level), dest);
        msg.color_range_end = dest.size();
        dest.push_back(']');
        dest.push_back(' ');

        // add source location if present
        if (!msg.source.empty())
        {
            dest.push_back('[');
            const char *filename = basename(msg.source.filename);
            fmt_helper::append_string_view(filename, dest);
            dest.push_back(':');
            fmt_helper::append_int(msg.source.line, dest);
            dest.push_back(']');
            dest.push_back(' ');
        }

        fmt_helper::append_string_view(msg.payload, dest);
        fmt_helper::append_string_view(os::default_eol, dest);
    }

private:
    std::chrono::seconds cache_timestamp_{0};
    memory_buf_t cached_datetime_;        
};
} // namespace spdlog
