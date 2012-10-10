
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved.  

#ifndef _ORIENTPP_H_
#define _ORIENTPP_H_

#include <string>
#include <vector>

#include <boost/thread/mutex.hpp>

using namespace std;

namespace OrientPP {

extern const char *ORIENTPP_CHANGESET;
extern int ORIENTPP_CHANGESET_NUMBER;
extern int ORIENTPP_BUILD_NUMBER;

struct Exception : public std::exception
{
   string e;
   Exception(string s) : e(s) { }
   Exception(const char *s) : e(s) { }
   const char* what() const throw() { return e.c_str(); }
};

static inline string itoa(int v) {
  stringstream ss;
  ss << v;
  return ss.str();
}

string time_str(time_t now = time(0));

#include "log.h"

}; // namespace

#include "orient.h"
#include "json.h"

#endif 
