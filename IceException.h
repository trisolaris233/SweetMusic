#ifndef __ICE_EXCEPTION_H__
#define __ICE_EXCEPTION_H__

#include <string>
#include <exception>

namespace sweet {

	
	class SweetException : std::exception {
	public:
		SweetException(const std::string &str) :
			_msg(str) {}
		~SweetException() = default;

		const char *what() const { return _msg.c_str(); }

	private:
		std::string _msg;

	};

}


#endif