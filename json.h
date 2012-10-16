
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved

#ifndef _ORIENT_JSON_H_
#define _ORIENT_JSON_H_

#include "json_spirit.h"
#include <utf8.h>

using namespace std;

namespace json {

static inline wstring s2w(const string &s)
{
 wstring res;
 try {
   utf8::utf8to32(s.begin(), s.end(), back_inserter(res));
 } catch (...) {
    throw OrientPP::Exception("utf8to32: Invalid chars !");
 }
 return res;
}

static inline wstring s2w(const char *_s) { string s(_s); return s2w(s); }

static inline string w2s(wstring s)
{
 string res;
 try {
   utf8::utf32to8(s.begin(), s.end(), back_inserter(res));
 } catch (...) {
   throw OrientPP::Exception("utf32to8: Invalid chars !");
 }
 return res;
}
    static inline string json_write(json_spirit::wmObject &vals) {
       string res = w2s(json_spirit::write(json_spirit::wmValue(vals), json_spirit::raw_utf8 | json_spirit::remove_trailing_zeros));
       return res;
    }
    static inline bool json_get(json_spirit::wmObject &vals, const char *key, json_spirit::wmObject &res) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      if (i == vals.end())
        return false;
      json_spirit::wmValue v = i->second;
      if (v.type() != json_spirit::obj_type)
        return false;
      res = v.get_obj();
      return true;
    }
    static inline bool json_get(json_spirit::wmObject &vals, const char *key, json_spirit::wmArray &res) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      if (i == vals.end())
        return false;
      json_spirit::wmValue v = i->second;
      if (v.type() != json_spirit::array_type)
        return false;
      res = v.get_array();
      return true;
    }
    static inline bool json_get(json_spirit::wmObject &vals, const char *key, string &res) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      if (i == vals.end())
        return false;
      json_spirit::wmValue v = i->second;
      if (v.type() != json_spirit::str_type)
        return false;
      res = w2s(v.get_str());
      return true;
    }
    static inline bool json_get(json_spirit::wmObject &vals, const char *key, int &res) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      if (i == vals.end())
        return false;
      json_spirit::wmValue v = i->second;
      if (v.type() != json_spirit::int_type)
        return false;
      res = v.get_int();
      return true;
    }
    static inline bool json_get(json_spirit::wmObject &vals, const char *key, bool &res) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      if (i == vals.end())
        return false;
      json_spirit::wmValue v = i->second;
      if (v.type() != json_spirit::bool_type)
        return false;
      res = v.get_bool();
      return true;
    }
    static inline string json_get_str(json_spirit::wmObject &vals, const char *key, const char *default_val = "") {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      string res(default_val);
      if (i != vals.end()) {
        json_spirit::wmValue v = i->second;
        if (v.type() == json_spirit::str_type)
          res = w2s(v.get_str());
      }
      return res;
    }
    static inline int json_get_int(json_spirit::wmObject &vals, const char *key, int default_val = -1) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      int res = default_val;
      if (i != vals.end()) {
        json_spirit::wmValue v = i->second;
        if (v.type() == json_spirit::int_type)
          res = v.get_int();
      }
      return res;
    }
    static inline double json_get_double(json_spirit::wmObject &vals, const char *key, double default_val = 0.0) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      double res = default_val;
      if (i != vals.end()) {
        json_spirit::wmValue v = i->second;
        if (v.type() == json_spirit::real_type)
          res = v.get_real();
        else if (v.type() == json_spirit::int_type)
          res = v.get_int();
      }
      return res;
    }
    static inline float json_get_float(json_spirit::wmObject &vals, const char *key, float default_val = 0.0) {
      return json_get_double(vals, key, default_val);
    }
    static inline int json_get_bool(json_spirit::wmObject &vals, const char *key, bool default_val = false) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      bool res = default_val;
      if (i != vals.end()) {
        json_spirit::wmValue v = i->second;
        if (v.type() == json_spirit::bool_type)
          res = v.get_bool();
     }
      return res;
    }
    static inline string json_get_string_int(json_spirit::wmObject &vals, const char *key, int default_val = -1) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      stringstream ss;
      if (i != vals.end()) {
        json_spirit::wmValue v = i->second;
        if (v.type() == json_spirit::int_type)
          ss << v.get_int();
      } else
          ss << default_val;
      return ss.str();
    }
    static inline void json_add_bool(json_spirit::wmObject &vals, const char *key, bool v) {
      vals[s2w(key)] = v;
    }
    static inline void json_add_int(json_spirit::wmObject &vals, const char *key, int v) {
      vals[s2w(key)] = v;
    }
    static inline void json_add_float(json_spirit::wmObject &vals, const char *key, float v) {
      vals[s2w(key)] = v;
    }
    static inline void json_add_double(json_spirit::wmObject &vals, const char *key, double v) {
      vals[s2w(key)] = v;
    }
    static inline void json_add_str(json_spirit::wmObject &vals, string key, string v) {
      vals[s2w(key.c_str())] = s2w(v);
    }
    static inline bool json_remove(json_spirit::wmObject &vals, const char *key) {
      json_spirit::wmObject::iterator i = vals.find(s2w(key));
      if (i == vals.end())
        return false;
      vals.erase(i);
      return true;
    }
};

using namespace json;

#endif
