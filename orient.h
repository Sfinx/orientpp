
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved

#ifndef _ORIENTPP_DB_H_
#define _ORIENTPP_DB_H_

#include <stdint.h>
#include <errno.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/thread/mutex.hpp>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;
using boost::lambda::bind;
using boost::lambda::var;
using boost::lambda::_1;

#define ORIENTPP_DEBUG	1

namespace OrientPP {

enum {
#ifdef ORIENTPP_DEBUG
  ORIENTPP_DEFAULT_VERBOSE_LEVEL = 1,
#else
  ORIENTPP_DEFAULT_VERBOSE_LEVEL = 0,
#endif
  ORIENTPP_DEFAULT_OPS_TIMEOUT = 5
};

#define ORIENTPP_DRIVER_NAME		"OrientPP"
#define ORIENTPP_DRIVER_VERSION		"v0.1"
#define ORIENTPP_DRIVER_PROTO_VERSION	0xc
#define ORIENTDB_SERVER_PORT		"2424"
#define ORIENT_NULL			0xFFFFFFFF

enum { // binary protocol commands
  ORIENTDB_SHUTDOWN = 1,
  ORIENTDB_CONNECT,
  ORIENTDB_DB_OPEN,
  ORIENTDB_DB_CREATE,
  ORIENTDB_DB_CLOSE,
  ORIENTDB_DB_EXIST,
  ORIENTDB_DB_DELETE,
  ORIENTDB_DB_SIZE,
  ORIENTDB_DB_COUNTRECORDS,
  ORIENTDB_DATACLUSTER_ADD,
  ORIENTDB_DATACLUSTER_REMOVE,
  ORIENTDB_DATACLUSTER_COUNT,
  ORIENTDB_DATACLUSTER_DATARANGE,
  ORIENTDB_DATASEGMENT_ADD = 20,
  ORIENTDB_DATASEGMENT_REMOVE,
  ORIENTDB_RECORD_LOAD = 30,
  ORIENTDB_RECORD_CREATE,
  ORIENTDB_RECORD_UPDATE,
  ORIENTDB_RECORD_DELETE,
  ORIENTDB_COUNT = 40,
  ORIENTDB_COMMAND,
  ORIENTDB_TX_COMMIT = 60,
  ORIENTDB_CONFIG_GET = 70,
  ORIENTDB_CONFIG_SET,
  ORIENTDB_CONFIG_LIST,
  ORIENTDB_DB_RELOAD
};

enum { // db types
  AS_DOCUMENT_DB,
  AS_GRAPH_DB
};

enum { // storage types
  DB_LOCAL_STORAGE,
  DB_MEMORY_STORAGE
};

enum { // query types
  AS_SQL,
  AS_JAVASCRIPT,
  AS_ECMASCRIPT,
  AS_GREMLIN,
  AS_GROOVY,
  AS_GREMLIN_GROOVY
};

enum { // record types
 ORIENT_NULL_RECORD = 0,
 ORIENT_RECORD_ID,
 ORIENT_SERIALIZED_RECORD,
 ORIENT_DOCUMENT_RECORD = 'd',
 ORIENT_FLAT_DATA_RECORD = 'f',
 ORIENT_RAW_BYTES_RECORD = 'b',
};

class tcp_client {
  int timeout_seconds;
  bool verbose_;
public:
  void verbose(bool v) { verbose_ = v; }
  void timeout(int seconds) { timeout_seconds = seconds; }
  tcp_client() : socket_(io_service_), deadline_(io_service_) {
    deadline_.expires_at(boost::posix_time::pos_infin);
    check_deadline();
    timeout_seconds = ORIENTPP_DEFAULT_OPS_TIMEOUT;
    verbose_ = false;
  }
  ~tcp_client() { socket_.close(); io_service_.stop(); }
  void close() { socket_.close(); }
  void connect(const string& host, const string &port) {
    tcp::resolver::query query(host, port);
    tcp::resolver::iterator iter = tcp::resolver(io_service_).resolve(query);
    deadline_.expires_from_now(boost::posix_time::seconds(timeout_seconds));
    boost::system::error_code ec = boost::asio::error::would_block;
    boost::asio::async_connect(socket_, iter, var(ec) = _1);
    do io_service_.run_one(); while (ec == boost::asio::error::would_block);
    if (ec || !socket_.is_open())
      throw boost::system::system_error(
          ec ? ec : boost::asio::error::operation_aborted);
  }
  void do_read(size_t len = 1) {
    deadline_.expires_from_now(boost::posix_time::seconds(timeout_seconds));
    boost::system::error_code ec = boost::asio::error::would_block;
    boost::asio::async_read(socket_, buf_, boost::asio::transfer_at_least(len), var(ec) = _1);
    do io_service_.run_one(); while (ec == boost::asio::error::would_block);
    if (ec)
      throw boost::system::system_error(ec);
  }
  void read_data(u8 *buf, size_t len) {
    if (len > buf_.size())
      do_read(len - buf_.size());
    const char *data = boost::asio::buffer_cast<const char*>(buf_.data());
    if (verbose_) {
      app_log << "read " << len << " bytes";
      for (unsigned int i = 0; i < len; i++) {
       if (data[i] < 0x20)
         app_log << "r:" << i << ":0x" << hex << int(u8(data[i]));
       else
         app_log << "r:" << i << ":" << char(data[i]) << " | 0x" << hex << int(u8(data[i]));
      }
    }
    memcpy(buf, data, len);
    buf_.consume(len);
  }
  size_t size() { return buf_.size(); }
  void flush() { buf_.consume(buf_.size()); }
  void write_data(const string& data) {
    if (verbose_) {
      for (unsigned int i = 0; i < data.size(); i++) {
        if (data[i] < 0x20)
          app_log << "w:" << i << ":0x" << hex << int(u8(data[i]));
        else
          app_log << "w:" << i << ":" << char(data[i]) << " | 0x" << hex << int(u8(data[i]));
        }
    }
    deadline_.expires_from_now(boost::posix_time::seconds(timeout_seconds));
    boost::system::error_code ec = boost::asio::error::would_block;
    boost::asio::async_write(socket_, boost::asio::buffer(data), var(ec) = _1);
    do io_service_.run_one(); while (ec == boost::asio::error::would_block);
    if (ec)
      throw boost::system::system_error(ec);
  }
private:
  void check_deadline() {
    if (deadline_.expires_at() <= deadline_timer::traits_type::now()) {
      boost::system::error_code ignored_ec;
      socket_.close(ignored_ec);
      deadline_.expires_at(boost::posix_time::pos_infin);
    }
    deadline_.async_wait(bind(&tcp_client::check_deadline, this));
  }

  boost::asio::io_service io_service_;
  tcp::socket socket_;
  deadline_timer deadline_;
  boost::asio::streambuf buf_;
};

struct orientsrv_buf {
  string data;
  size_t size() { return data.size(); }
  const char *buf() { return data.c_str(); }
  orientsrv_buf() { }
  orientsrv_buf(string s) { append(s); }
  void append(orientsrv_buf &b) {
    u32 len = b.size();
    if (len) {
      append((s32)len);
      data.append(b.buf(), len);
    } else {
      len = ORIENT_NULL;
      append((s32)len);
    }
  }
  void append(s32 val) {
    val = htonl(val);
    data.append((const char *)&val, sizeof(val));
  }
  void append(u16 val) {
    val = htons(val);
    data.append((const char *)&val, sizeof(val));
  }
  void append(u8 val) {
    data.append((const char *)&val, 1);
  }
  string safe_quote(string s) {
   for (uint i = 0; i < s.size(); i++) {
     if ((s[i] == '"') || (s[i] == '\\'))
       s.insert(i++, 1, '\\');
   }
   return s;
  }
  void append(string s) {
    string quoted(s);
    u32 len = quoted.size();
    if (len) {
      append((s32)len);
      data.append(quoted.c_str(), len);
    } else {
      len = ORIENT_NULL;
      append((s32)len);
    }
  }
};

struct orientsession {
  s32 id;
  bool connected;
  orientsession() : id(-1), connected(false) { }
};

struct orientrsp {
  tcp_client *tc;
  orientsession *session;
  s8 res;
  ~orientrsp() {
    if (tc->size())
      app_log << "*** Ignoring " << tc->size() << " non-parsed bytes of the response";
    tc->flush();
  }
  orientrsp(tcp_client *tc_, orientsession *session_) : tc(tc_), session(session_), res(-1) { }
  void check_result() {
    if (res >= 0)
      return;
    tc->read_data((u8 *)&res, 1);
    s32 session_id;
    tc->read_data((u8 *)&session_id, 4);
    session_id = ntohl(session_id);
    if (session->connected && (session->id != session_id))
      throw Exception("orientsrv::Error: Wrong Server Session ID");
    if (res)
      throw Exception("orientsrv::Error: " + parse_error());
  }
  // long
  void parse(s64 *dst) { parse((u64 *)dst); } 
  void parse(u64 *dst) {
    check_result();
    tc->read_data((u8 *)dst, 8);
    *dst = be64toh(*dst);
  }
  // int
  void parse(s32 *dst) { parse((u32 *)dst); } 
  void parse(u32 *dst) {
    check_result();
    tc->read_data((u8 *)dst, 4);
    *dst = ntohl(*dst);
  }
  // short
  void parse(s16 *dst) { parse((u16 *)dst); } 
  void parse(u16 *dst) {
    check_result();
    tc->read_data((u8 *)dst, 2);
    *dst = ntohs(*dst);
  }
  // byte
  void parse(u8 *dst) {
    check_result();
    tc->read_data((u8 *)dst, 1);
  }
  // string
  void parse(string *dst) {
    check_result();
    u32 len;
    parse(&len);
    if (len && (len != ORIENT_NULL)) {
      u8 buf[len];
      tc->read_data(buf, len);
      *dst = string((const char*)buf, len);
    }
  }
  // bytes
  void parse(orientsrv_buf *dst) {
    check_result();
    u32 len;
    parse(&len);
    if (len && (len != ORIENT_NULL)) { // WTF ? the NULL value must have 0 len, not -1
      u8 buf[len];
      tc->read_data(buf, len);
      dst->data = string((const char*)buf, len);
    }
  }
  string parse_error() {
    string err;
    while (1) {
      u8 next_error;
      parse(&next_error);
      if (!next_error)
        break;
      string exception_class;
      parse(&exception_class);
      string exception_message;
      parse(&exception_message);
      if (err.size())
        err += ",";
      err += (exception_class + ": " + exception_message);
    }
    return err;
  }
};

class orientsrv {
  boost::mutex m_lock;
  int verbose_level;
  orientsession session;
  u16 protocol;
  string url, user, pass, host, port;
  tcp_client tc;
  void init() {
    verbose(ORIENTPP_DEFAULT_VERBOSE_LEVEL);
    tc.timeout(ORIENTPP_DEFAULT_OPS_TIMEOUT);
    port = ORIENTDB_SERVER_PORT;
    protocol = 0;
  }
  orientrsp send(u8 cmd, orientsrv_buf &r, orientsession *s = 0) {
    boost::unique_lock<boost::mutex> lock(m_lock);
    string req((const char *)&cmd, 1);
    s32 sid = htonl(s ? s->id : session.id);
    req.append((const char *)&sid, sizeof(sid));
    if (r.size())
      req.append(r.buf(), r.size());
    tc.write_data(req);
    return orientrsp(&tc, s ? s : &session);
  }
  void error(string err) {
    if (verbose())
      app_log << err;
    throw Exception(err);
  }
  void verbose(int v) { verbose_level = v; if (v > 1) tc.verbose(1); else tc.verbose(0); }
  int verbose() { return verbose_level; }
  void close() {
    tc.close();
    session.connected = false;
    session.id = -1;
  }
  void reopen() { connect(url, user, pass); }
 public:
  bool dbexists(string db);
  void dropdb(string db);
  bool isconnected() { return session.connected; }
  void timeout(int t) { tc.timeout(t); }
  void createdb(string db, int db_type, int db_engine);
  void connect(string _url, string user = "", string pass = "");
  orientsrv(string _url, string user = "", string pass = "") { init(); connect(_url, user, pass); }
  void shutdown() {
    if (verbose())
      app_log << "orientsrv::shutdown() called for [" << session.id << "]";
    if (!isconnected())
      throw Exception("orientsrv::shutdown(): Connect to server first");
    orientsrv_buf r(user);
    r.append(pass);
    orientrsp rsp = send(ORIENTDB_SHUTDOWN, r);
    // wait for successfull responce
    rsp.check_result();
    throw Exception("orientsrv::shutdown(): Completed");
  }
  orientsrv() { init(); }
  ~orientsrv();
  friend class orientdb;
  friend class orientrsp;
};

class orientdb {
  orientsrv *srv;
  int db_type;
  string db, user, pass;
  orientsession session;
  void reconnect() {
    app_log << "Lost SRV connection, reconnecting";
    session.connected = false;
    session.id = -1;
    srv->reopen();
  }
 public:
  void reopen() {
    app_log << "Lost DB connection, reopening";
    reconnect();
    open(db, db_type, user, pass);
  }
  void open(string db_, int db_type_, string u, string p);
  u64 count();
  u64 size();
  void close();
  orientrsp send(u8 cmd) { orientsrv_buf dummy; return srv->send(cmd, dummy, &session); }
  orientrsp send(u8 cmd, orientsrv_buf &r, orientsession *s = 0) { return srv->send(cmd, r, s ? s : &session); }
  void error(string err) { srv->error(err); }
  int verbose() { return srv->verbose(); }
  void verbose(int v) { srv->verbose(v); }
  bool isconnected() { return session.connected; }
  orientdb(orientsrv &s) : srv(&s) { }
  orientdb(orientsrv *s) : srv(s) { }
  ~orientdb();
};

enum {
 ORIENT_RECORD_TYPE_UNKNOWN = 0,
 ORIENT_RECORD_TYPE_BOOL,
 ORIENT_RECORD_TYPE_BYTE,
 ORIENT_RECORD_TYPE_SHORT,
 ORIENT_RECORD_TYPE_INT,
 ORIENT_RECORD_TYPE_LONG,
 ORIENT_RECORD_TYPE_STRING,
 ORIENT_RECORD_TYPE_BINARY,
 ORIENT_RECORD_TYPE_FLOAT,
 ORIENT_RECORD_TYPE_DOUBLE,
 ORIENT_RECORD_TYPE_BIGDECIMAL,
 ORIENT_RECORD_TYPE_DATE,
 ORIENT_RECORD_TYPE_DATETIME,
 ORIENT_RECORD_TYPE_LINK,
 ORIENT_RECORD_TYPE_LINKSET,
 ORIENT_RECORD_TYPE_EMBEDDED,
 ORIENT_RECORD_TYPE_COLLECTION,
 ORIENT_RECORD_TYPE_MAP,
 ORIENT_RECORD_TYPE_NULL
};

// рекурсия при обходе => use boost:graph
struct property_t {
  string name, data;
  u8 type;
  string type2str() {
    switch (type) {
      case ORIENT_RECORD_TYPE_BOOL:
        return "bool";
      case ORIENT_RECORD_TYPE_BYTE:
        return "byte";
      case ORIENT_RECORD_TYPE_SHORT:
        return "short";
      case ORIENT_RECORD_TYPE_INT:
        return "int";
      case ORIENT_RECORD_TYPE_LONG:
        return "long";
      case ORIENT_RECORD_TYPE_STRING:
        return "string";
      case ORIENT_RECORD_TYPE_BINARY:
        return "binary";
      case ORIENT_RECORD_TYPE_FLOAT:
        return "float";
      case ORIENT_RECORD_TYPE_DOUBLE:
        return "double";
      case ORIENT_RECORD_TYPE_BIGDECIMAL:
        return "bigdecimal";
      case ORIENT_RECORD_TYPE_DATE:
        return "date";
      case ORIENT_RECORD_TYPE_DATETIME:
        return "datetime";
      case ORIENT_RECORD_TYPE_LINK:
        return "link";
      case ORIENT_RECORD_TYPE_LINKSET:
        return "linkset";
      case ORIENT_RECORD_TYPE_EMBEDDED:
        return "embedded";
      case ORIENT_RECORD_TYPE_COLLECTION:
        return "collection";
      case ORIENT_RECORD_TYPE_MAP:
        return "map";
      case ORIENT_RECORD_TYPE_NULL:
        return "null";
    }
      return "unknown:" + itoa(type);
  }
  // union val { ... }
  // vector <rid_t> links;
  vector <property_t> embedded; // for arrays, maps, etc.
  property_t(string n) : name(n), type(ORIENT_RECORD_TYPE_UNKNOWN) { }
  property_t() : type(ORIENT_RECORD_TYPE_UNKNOWN) { }
  operator string () {
    return data;
  }
  operator int() {
    if (type == ORIENT_RECORD_TYPE_INT)
      return atoi(data.c_str());
    throw Exception("property_t::int(): Invalid type [" + itoa(type) + "] !");
  }
  // TODO: operator bool, float, ...
};

typedef map <string, property_t>::iterator property_iterator;

struct rid_t {
  s16 id;
  s64 pos;
  bool valid;
  rid_t () : id(-1), pos(-1), valid(false) { }
  rid_t (s16 id_, s64 pos_) : id(id_), pos(pos_), valid(true) { }
  string str() {
    if (!valid)
      return "";
    stringstream ss;
    ss << id << ":" << pos;
    return ss.str();
  }
  operator string() {
    if (!valid)
      return "";
    stringstream ss;
    ss << "#" << id << ":" << pos;
    return ss.str();
  }
};

struct orient_record_t {
  u8 type;
  rid_t rid;
  s32 version;
  string content;
  bool parsed;
  string class_;
  // and what if property can be without the name ?
  // replace with vector <property_t> ?
  map <string, property_t> properties;
  orient_record_t(string &serialized) :
    type(ORIENT_SERIALIZED_RECORD), content(serialized), parsed(false) { }
  orient_record_t(s16 id_, s64 pos_) :
    type(ORIENT_RECORD_ID), rid(id_, pos_), parsed(false) { }
  orient_record_t(u8 type_, s16 id_, s64 pos_, s32 version_, string &content_) :
    type(type_),
    rid(id_, pos_),
    version(version_),
    content(content_),
    parsed(false) { }
  orient_record_t() : type(ORIENT_NULL_RECORD) { }
  bool has_property(string n) {
    parse();
    property_iterator it = properties.find(n);
    return (it == properties.end()) ? false : true;
  }
  property_t get_property(string n) {
    parse();
    property_iterator it = properties.find(n);
    if (it == properties.end())
      throw Exception("No such property: " + n);
    return it->second;
  }
  size_t add_property(size_t pos, property_t &p) {
    size_t collection_start;
    bool done;
    switch (content[pos]) {
      case '"':
        // string 
        while ((pos < (content.size() - 1)) && content[++pos] != '"') {
          if (content[pos] == '\\')
            pos++;
          p.data += content[pos];
        }
        pos++;
        p.type = ORIENT_RECORD_TYPE_STRING;
        break;
      case '#':
        // link
        pos++;
        while ((pos < content.size()) && content[pos] != ',' && content[pos] != ')'
          && content[pos] != ']' && content[pos] != '*') {
            if ((content[pos] != ':') && !(content[pos] >= '0' && content[pos] <= '9'))
              throw Exception("add_property(): Invalid link data: " + itoa(content[pos]));
            p.data += content[pos++];
        }
        p.type = ORIENT_RECORD_TYPE_LINK;
        break;
      case 't':
      case 'f':
        // bool: true or false
        p.type = ORIENT_RECORD_TYPE_BOOL;
        p.data = string(content.c_str() + pos, (content[pos] == 't') ? 4 : 5);
        pos += ((content[pos] == 't') ? 4 : 5);
        break;
      case '[':
        // array
        pos++;
        collection_start = pos;
        while ((pos < content.size() - 1) && content[pos] != ',' && content[pos] != ')'
          && content[pos] != ']' && content[pos] != '*') {
            property_t emb;
            pos = add_property(pos, emb);
            p.embedded.push_back(emb);
        }
        p.type = ORIENT_RECORD_TYPE_COLLECTION;
        p.data = string(content.c_str() + collection_start, pos - collection_start);
        if (content[pos] == ']')
          pos++;
        break;
      case '{':
        // map
        done = false;
        while (!done && (pos++ < (content.size() - 1))) {
          switch (content[pos]) {
            case '}':
              p.type = ORIENT_RECORD_TYPE_MAP;
              pos++;
              done = true;
              break;
            default:
              p.data += content[pos];
          }
        }
        break;
      case '(':
        // embedded
      case '*':
      case ')':
      case ']':
      case ',':
        throw Exception("add_property(): Unimplemented type: " + itoa(content[pos]));
      default:
        // plain value
        done = false;
        p.type = ORIENT_RECORD_TYPE_INT; // default type
        while (!done && (pos < content.size())) {
          switch (content[pos]) {
            case ')':
            case ']':
            case ',':
//              if (p.data.size()) {
//                double number = atof(p.data);
//              }
              done = true;
              break;
            case 'b':
              p.type = ORIENT_RECORD_TYPE_BYTE;
//              if (p.data.size()) {
//              }
              pos++;
              done = true;
              break;
            case 's':
              p.type = ORIENT_RECORD_TYPE_SHORT;
              pos++;
              done = true;
              break;
            case 'i':
              p.type = ORIENT_RECORD_TYPE_INT;
              pos++;
              done = true;
              break;
            case 'l':
              p.type = ORIENT_RECORD_TYPE_LONG;
              pos++;
              done = true;
              break;
            case 'f':
              p.type = ORIENT_RECORD_TYPE_FLOAT;
              pos++;
              done = true;
              break;
            case 'd':
              p.type = ORIENT_RECORD_TYPE_DOUBLE;
              pos++;
              done = true;
              break;
            case 't':
              p.type = ORIENT_RECORD_TYPE_DATETIME;
              pos++;
              done = true;
              break;
            case 'a':
              p.type = ORIENT_RECORD_TYPE_DATE;
              pos++;
              done = true;
              break;
            default:
              if ((content[pos] < '0' || content[pos] > '9') && content[pos] != '.')
                throw Exception("add_property(): Invalid digit !");
              p.data += content[pos++];
          }
        }
    }
    if (content[pos] == ',')
      pos++;
    return pos;
  }
  void parse() {
    if (parsed || !is_document())
      return;
    size_t curr_pos = 0;
    string t;
    // app_log << "full: " << content;
    // parse property
    do {
      // try parse class
      if (!properties.size() && !class_.size() && content[curr_pos] == '@') {
        class_ = t;
        t.clear();
        curr_pos++;
      }
      // parse value
      if (content[curr_pos] == ':') {
        curr_pos++;
        property_t p(t);
        // app_log << "adding prop n: " << p.name << " pos: " << curr_pos;
        curr_pos = add_property(curr_pos, p);
        properties.insert(pair<string, property_t>(p.name, p));
        t.clear();
      } else if (content[curr_pos] != ')' && content[curr_pos] != ']' && content[curr_pos] != ',' &&
          content[curr_pos] != '*')
            t += content[curr_pos++]; // or collect name
    } while ((curr_pos < content.size()) &&
        content[curr_pos -1] != ')' && content[curr_pos -1] != '*');
    parsed = true;
  }
  string classof() {
    parse();
    return class_;
  }
  bool is_document() { return (type == ORIENT_DOCUMENT_RECORD); }
  operator string() { // parse record to generic string
    stringstream ss;
    switch (type) {
      case ORIENT_SERIALIZED_RECORD:
      case ORIENT_DOCUMENT_RECORD:
        return content;
      case ORIENT_RECORD_ID:
        ss << "#" << rid.id << ":" << rid.pos;
        return ss.str();
      case ORIENT_NULL_RECORD:
      default:
        return "null";
    }
  }
};
  
extern orient_record_t orient_null;

struct orientresult {
  vector <orient_record_t> records;
};

typedef boost::shared_ptr<orientresult> orientresult_ptr;

class orientquery {
  orientdb *db;
  // orientpreparestatement *ps;
  string q;
  bool prepared;
  bool autocommit_;
  u64 update_counter_;
  ostringstream buf;
  orientquery& operator= (const orientquery&) = delete;
  orientquery& operator== (const orientquery&) = delete;
  orientquery(const orientquery &)  = delete;
  orient_record_t parse_record(orientrsp &rsp);
  u32 parse_records_collection(orientrsp &rsp, vector <orient_record_t> *records);
public:
  orient_record_t insert_id;
  const char *str() { return buf.str().size() ? buf.str().c_str() : q.c_str(); }
  template <typename T> orientquery& operator<< (const T &value) {
    buf << value;
    if (prepared) { // close prev statement
    //  if (ps) {
    //    ps->close();
    //    ps = 0;
    //  }
      prepared = false;
    }
    return *this;
  }
  void autocommit(bool ac) { autocommit_ = ac; }
  // prepared stuff
  void set(uint pos, string &val); // throw(Exception);
  u64 affected_rows() { return update_counter_; }
  orientquery(orientdb &db_, const char *qs = 0) : db(&db_), q(qs ? qs : ""), prepared(false),
    autocommit_(true) { }
  orientquery(orientdb *db_, const char *qs = 0) : db(db_), q(qs ? qs : ""), prepared(false),
    autocommit_(true) { }
  orientquery(orientdb &db_, string qs) : db(&db_), q(qs), prepared(false), autocommit_(true) { }
  ~orientquery() {
    //if (ps)
    //  ps->close();
  }
#ifdef ORIENTPP_DEBUG
  orientresult_ptr execute_debug(const char *file, int line, const char *func, const char *qs = 0,
     int query_type = AS_SQL);
#define execute(x)       execute_debug(__FILE__, __LINE__, __FUNCTION__, 0, x)
#define val(x)          val_debug(__FILE__, __LINE__, __FUNCTION__, x)
#else
  orientresult_ptr execute(const char *qs = 0, int query_type = AS_SQL);
#endif
};

// used for client_id generation
class unique_counter
{
 boost::mutex mutex;
 unsigned long count;
public:
 unique_counter() : count(0) { }
 unsigned long next() {
   boost::mutex::scoped_lock scoped_lock(mutex);
   return ++count;
 }
};

}; // namespace

#endif
