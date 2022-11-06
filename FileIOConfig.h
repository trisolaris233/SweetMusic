#ifndef __FILE_IO_CONFIG_H__
#define __FILE_IO_CONFIG_H__

namespace sweet {

#ifdef WIN32
	typedef __int64 int64;
#else
	typedef long long int int64;
#endif

}

#endif