#pragma once
#include <iostream>

#define RTC_LOG(sev) if(0) std::cerr
#define RTC_LOG_V(sev) if(0) std::cerr
#define RTC_DLOG(sev) if(0) std::cerr
#define RTC_DCHECK(x) ((void)(x))
#define RTC_DCHECK_EQ(a,b) ((void)0)
#define RTC_DCHECK_NE(a,b) ((void)0)
#define RTC_DCHECK_GE(a,b) ((void)0)
#define RTC_DCHECK_GT(a,b) ((void)0)
#define RTC_DCHECK_LE(a,b) ((void)0)
#define RTC_DCHECK_LT(a,b) ((void)0)

namespace rtc {
enum LoggingSeverity {
    LS_VERBOSE, LS_INFO, LS_WARNING, LS_ERROR, LS_NONE
};
} // namespace rtc

#define LS_VERBOSE rtc::LS_VERBOSE
#define LS_INFO rtc::LS_INFO
#define LS_WARNING rtc::LS_WARNING
#define LS_ERROR rtc::LS_ERROR
