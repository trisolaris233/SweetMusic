#include "Date.h"

#include <memory>
#include <sstream>

namespace sweet {
	
	Date::Formatter Date::defaultDateFormatter = [](const Date &date)->Sstring {
		std::wostringstream oss;
		oss << date.year() << "-" << date.month() << "-" << date.day();
		return oss.str();
	};

	Date::Formatter Date::defaultDateTimeFormatter = [](const Date &date)->Sstring {
		std::wostringstream oss;
		oss
			<< date.year() << "-" << date.month() << "-" << date.day() << ", "
			<< date.hour() << ":" << date.min();
		return oss.str();
	};

	Date::Date() : _year(0)
		,_month(0)
		,_day(0)
		, _hour(0)
		, _min(0)
		, _sec(0)
	{}

	Date::Date(time_t t) {
#ifdef WIN32
		std::unique_ptr<tm> ptime(new tm);
		localtime_s(ptime.get(), &t);
#else
		tm* ptime(localtime(&t));
#endif
		_year	= 1900 + ptime->tm_year;
		_month	= 1 + ptime->tm_mon;
		_day	= ptime->tm_mday;
		_hour	= ptime->tm_hour;
		_min	= ptime->tm_min;
		_sec	= ptime->tm_sec;
	}

	inline int Date::day() const {
		return _day;
	}

	inline int Date::month() const {
		return _month;
	}

	inline int Date::year() const {
		return _year;
	}

	inline int Date::hour() const {
		return _hour;
	}

	inline int Date::min() const {
		return _min;
	}

	inline int Date::sec() const {
		return _sec;
	}

	inline void Date::setDay(int d) {
		_day = d;
	}

	inline void Date::setMonth(int m) {
		_month = m;
	}

	inline void Date::setYear(int y) {
		_year = y;
	}

	inline void Date::setHour(int h) {
		_hour = h;
	}

	inline void Date::setMin(int m) {
		_min = m;
	}

	inline void Date::setSec(int s) {
		_sec = s;
	}

	inline bool Date::isValid() const {
		return(_year > 0 && _month > 0 && _day > 0 &&
			_hour >= 0 && _hour <= 23 && _min >= 0 && _min <= 59 &&
			_sec >= 0 && _sec <= 59);
	}

	inline bool Date::isNull() const {
		return(_year == 0 || _month == 0 || _day == 0);
	}

	Date Date::today() {
		time_t tt = time(NULL);
		return Date(tt);
	}

	Sstring Date::toString(Date::Formatter formatter) const {
		return formatter(*this);
	}

}