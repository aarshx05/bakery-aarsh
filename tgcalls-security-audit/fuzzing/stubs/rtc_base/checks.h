#pragma once
#include <cstdlib>

#define RTC_CHECK(x) do { if(!(x)) std::abort(); } while(0)
#define RTC_CHECK_EQ(a,b) RTC_CHECK((a)==(b))
#define RTC_CHECK_NE(a,b) RTC_CHECK((a)!=(b))
#define RTC_CHECK_GE(a,b) RTC_CHECK((a)>=(b))
#define RTC_CHECK_GT(a,b) RTC_CHECK((a)>(b))
#define RTC_CHECK_LE(a,b) RTC_CHECK((a)<=(b))
#define RTC_CHECK_LT(a,b) RTC_CHECK((a)<(b))
#define RTC_FATAL() std::abort()
#define RTC_DCHECK_IS_ON 0
