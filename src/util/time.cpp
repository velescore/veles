// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2018 FXTC developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <util/time.h>

#include <atomic>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <ctime>
#include <tinyformat.h>
#include <mutex>

static std::atomic<int64_t> nMockTime(0); //!< For unit testing
static std::mutex gmtime_mutex;

int64_t GetTime()
{
    int64_t mocktime = nMockTime.load(std::memory_order_relaxed);
    if (mocktime) return mocktime;

    time_t now = time(nullptr);
    assert(now > 0);
    return now;
}

void SetMockTime(int64_t nMockTimeIn)
{
    nMockTime.store(nMockTimeIn, std::memory_order_relaxed);
}

int64_t GetMockTime()
{
    return nMockTime.load(std::memory_order_relaxed);
}

int64_t GetTimeMillis()
{
    int64_t now = (boost::posix_time::microsec_clock::universal_time() -
                   boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_milliseconds();
    assert(now > 0);
    return now;
}

int64_t GetTimeMicros()
{
    int64_t now = (boost::posix_time::microsec_clock::universal_time() -
                   boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_microseconds();
    assert(now > 0);
    return now;
}

int64_t GetSystemTimeInSeconds()
{
    return GetTimeMicros()/1000000;
}

void MilliSleep(int64_t n)
{

/**
 * Boost's sleep_for was uninterruptible when backed by nanosleep from 1.50
 * until fixed in 1.52. Use the deprecated sleep method for the broken case.
 * See: https://svn.boost.org/trac/boost/ticket/7238
 */
#if defined(HAVE_WORKING_BOOST_SLEEP_FOR)
    boost::this_thread::sleep_for(boost::chrono::milliseconds(n));
#elif defined(HAVE_WORKING_BOOST_SLEEP)
    boost::this_thread::sleep(boost::posix_time::milliseconds(n));
#else
//should never get here
#error missing boost sleep implementation
#endif
}

// Thread-safe wrapper for gmtime
struct tm *safe_gmtime(const time_t *timep) {
    std::lock_guard<std::mutex> lock(gmtime_mutex);
    return gmtime(timep); // This is safe because of the mutex
}

std::string FormatISO8601DateTime(int64_t nTime) {
    time_t time_val = nTime;
    const struct tm *ts = safe_gmtime(&time_val); // Use safe_gmtime here
    if (!ts) return ""; // Handle error if ts is nullptr

    return strprintf("%04i-%02i-%02iT%02i:%02i:%02iZ",
                     ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
                     ts->tm_hour, ts->tm_min, ts->tm_sec);
}

std::string FormatISO8601Date(int64_t nTime) {
    time_t time_val = nTime;
    const struct tm *ts = safe_gmtime(&time_val); // Use safe_gmtime here
    if (!ts) return ""; // Handle error if ts is nullptr

    return strprintf("%04i-%02i-%02i", ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday);
}

std::string FormatISO8601Time(int64_t nTime) {
    time_t time_val = nTime;
    const struct tm *ts = safe_gmtime(&time_val); // Use safe_gmtime here
    if (!ts) return ""; // Handle error if ts is nullptr

    return strprintf("%02i:%02i:%02iZ", ts->tm_hour, ts->tm_min, ts->tm_sec);
}

// Dash
std::string DurationToDHMS(int64_t nDurationTime)
{
    int seconds = nDurationTime % 60;
    nDurationTime /= 60;
    int minutes = nDurationTime % 60;
    nDurationTime /= 60;
    int hours = nDurationTime % 24;
    int days = nDurationTime / 24;
    if(days)
        return strprintf("%dd %02dh:%02dm:%02ds", days, hours, minutes, seconds);
    if(hours)
        return strprintf("%02dh:%02dm:%02ds", hours, minutes, seconds);
    return strprintf("%02dm:%02ds", minutes, seconds);
}
//
