#ifndef __DATE_H__
#define __DATA_H__

#include <ctime>
#include <functional>

#include "Sstring.h"

namespace sweet {

	class Date {
	public:
		typedef std::function<Sstring(const Date&)> Formatter;

		static Formatter defaultDateFormatter;
		static Formatter defaultDateTimeFormatter;

	public:
		Date();
		Date(time_t t);

		int day() const;
		int month() const;
		int year() const;
		int hour() const;
		int min() const;
		int sec() const;
		void setDay(int d);
		void setMonth(int m);
		void setYear(int y);
		void setHour(int h);
		void setMin(int m);
		void setSec(int s);

		bool isValid() const;
		bool isNull() const;

		static Date today();

		Sstring toString(Formatter formatter = defaultDateFormatter) const;


	private:
		int _year;
		int _month;
		int _day;
		int _hour;
		int _min;
		int _sec;

	};
}

#endif