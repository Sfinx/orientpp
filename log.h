
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved.  

#ifndef _ORIENTPP_LOG_H_
#define _ORIENTPP_LOG_H_

struct debug_t {
  boost::mutex m;
  string log_file;
  bool log_to_file, log_to_console, log_to_stdout;  
  debug_t() : log_file("/var/log/orientpp.log"), log_to_file(false),
    log_to_console(false), log_to_stdout(false) { }
};

extern debug_t debug;

class app_logger
{
public:
    template <typename T> app_logger& operator<< (const T &value) {
        buf << value;
        return *this;
    }
    ~app_logger();
    app_logger(long _c = -1) : cid(_c) { }
private:
    ostringstream buf;
    long cid;
};

#define app_log app_logger()

#endif
