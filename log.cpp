
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved.  

#include "orient.h"
#include <fstream>

string OrientPP::time_str(time_t now)
{
 string t(ctime(&now));
 t.erase(t.size() - 1);
 return t;
}

OrientPP::debug_t OrientPP::debug;

OrientPP::app_logger::~app_logger()
{
 boost::mutex::scoped_lock scoped_lock(debug.m);
 stringstream m;
 m << "[" + time_str() + "] " << buf.str();
 if (debug.log_to_stdout)
   cout << m.str() << endl;
 if (debug.log_to_file) {
   ofstream flog;
   flog.open(debug.log_file.c_str(), ios::app);
   // ios_base::sync_with_stdio(false);
   clog.rdbuf(flog.rdbuf());
   clog << m.str() << endl;
   flog.close();
 }
}
