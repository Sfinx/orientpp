
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved

#include "db.h"

namespace OrientPP {

unique_counter client_id;
// connect lock, needed if multiple client/threaded reconnect occures
boost::mutex c_lock;

void orientsrv::connect(string _url, string _user, string _pass)
{
 boost::unique_lock<boost::mutex> lock(c_lock);
 if (isconnected())
   close();
 url = _url;
 // get host:port
 size_t pos = strcspn(_url.c_str(), ":");
 if (pos != _url.size()) {
   host = string(_url.c_str(), _url.c_str() + pos);
   port = string(_url.c_str() + pos + 1);
   if (!port.size())
     port = ORIENTDB_SERVER_PORT;
 } else
     host = _url;
 if (verbose() > 1)
   app_log << "orientsrv::connect(): Connecting to " << host << ":" << port;
 try {
   tc.connect(host, port);
   tc.read_data((u8 *)&protocol, sizeof(u16));
   protocol = ntohs(protocol);
   if (_user.size() && _pass.size()) {
     user = _user;
     pass = _pass;
     // (driver-name:string)(driver-version:string)(protocol-version:short)(client-id:string)
     // (user-name:string)(user-password:string)
     orientsrv_buf r(ORIENTPP_DRIVER_NAME);
     r.append(ORIENTPP_DRIVER_VERSION);
     r.append((u16)ORIENTPP_DRIVER_PROTO_VERSION);
     r.append(itoa(int(client_id.next())));
     r.append(_user);
     r.append(_pass);
     orientrsp rsp = send(ORIENTDB_CONNECT, r);
     rsp.parse(&session.id);
     if (verbose())
       app_log << "orientsrv::connect(): Connected to " << host << ":" << port << ", protocol: "
         << protocol << ", SRV session [0x" << hex << session.id << "]";
   } else
       if (verbose())
         app_log << "orientsrv::connect(): Connected to " << host << ":" << port << ", protocol: "
           << protocol;   
   session.connected = true;
 }
 catch (std::exception &e) {
   error("orientsrv::connect(): " + string(e.what()));
 }
 catch (...) {
   error("orientsrv::connect(): Connection failed");
 }
}

u64 orientdb::size()
{
 orientrsp rsp = send(ORIENTDB_DB_SIZE);
 u64 sz;
 rsp.parse(&sz);
 if (verbose() > 1)
   app_log << "DB " << db << " size is " << sz;
 return sz;
}

u64 orientdb::count()
{
 orientrsp rsp = send(ORIENTDB_DB_COUNTRECORDS);
 u64 recs;
 rsp.parse(&recs);
 if (verbose() > 1)
   app_log << "DB " << db << " has " << recs << " records";
 return recs;
}

void orientdb::close()
{
 if (!isconnected())
   return;
 if (verbose() > 1)
   app_log << "Closing " << db << " for user " << user;
 orientrsp rsp = send(ORIENTDB_DB_CLOSE);
 // no result returned (?!)
 session.connected = false;
 session.id = -1;
 if (verbose())
   app_log << "DB " << db << " closed for user " << user;
}

void orientdb::open(string db_, int db_type_, string u, string p)
{
 if (!srv->isconnected())
   reconnect();
 if (isconnected())
   close();
 bool reconnecting = false;
restart:
 try {
 if (verbose() > 1) {
   stringstream ss;
   ss << "orientdb::open(): Opening DB " << db_ << " on " << srv->host << ":"
     << srv->port;
   if (srv->session.id != -1)
     ss << " [0x" << hex << srv->session.id << "]";
   app_log << ss.str();
 }
 db = db_;
 db_type = db_type_;
 user = u;
 pass = p;
 // (driver-name:string)(driver-version:string)(protocol-version:short)(client-id:string)
 // (database-name:string)(database-type:string)(user-name:string)(user-password:string)
 orientsrv_buf r(ORIENTPP_DRIVER_NAME);
 r.append(ORIENTPP_DRIVER_VERSION);
 r.append((u16)ORIENTPP_DRIVER_PROTO_VERSION);
 r.append(itoa(int(client_id.next())));
 r.append(db);
 r.append((db_type == AS_DOCUMENT_DB) ? "document" : "graph");
 r.append(user);
 r.append(pass);
 orientrsp rsp = send(ORIENTDB_DB_OPEN, r);
 rsp.parse(&session.id);
 session.connected = true;
 // (session-id:int)(num-of-clusters:short)[(cluster-name:string)(cluster-id:short)
 // (cluster-type:string)(cluster-dataSegmentId:short)](cluster-config:bytes)
 // кластер у них что-то типа таблицы
 // несколько проблем:
 //  - 1.2.0-snapshot иногда выдает num_of_clusters > чем реальное количество в пакете
 //  - cluster_id может быть > чем num_of_clusters (?!) исходя из исходника OStorageRemote.java:1822
 //  - в свете выявленного нифига непонятно где же кончало в пакете
 u16 num_of_clusters;
 rsp.parse(&num_of_clusters);
 u16 real_num_of_clusters = 0;
 while (real_num_of_clusters < num_of_clusters) {
   string cl_name, cl_type;
   u16 cl_id, cl_data_segment_id;
   rsp.parse(&cl_name);
   if (!cl_name.size())
     break;
   real_num_of_clusters++;
   rsp.parse(&cl_id);
   if (cl_id > num_of_clusters)
     error("Too big cluster number !");
   rsp.parse(&cl_type);
   rsp.parse(&cl_data_segment_id);
   if (verbose() > 1)
     app_log << "id: " << cl_id << ", name: " << cl_name << ", type: " << cl_type
       << ", data_segment_id :" << cl_data_segment_id;
   // теоретически нужно бы эту всю хрень куда-то сохранять, шобы потом красиво рекорды выводить
   // ...
 }
 if (real_num_of_clusters == num_of_clusters) {
   // read (cluster-config:bytes)
   orientsrv_buf cluster_config;
   rsp.parse(&cluster_config);
   if (cluster_config.size())
     app_log << "TODO: parse cluster_config";
 }
 if (verbose() > 1) {
   if (real_num_of_clusters != num_of_clusters)
     app_log << "DB has " << real_num_of_clusters << " clusters [reported " << num_of_clusters << "]";
   else {
     app_log << "DB has " << real_num_of_clusters << " clusters";
   }
  }
 if (verbose())
   app_log << "orientdb::open(): Opened DB " << db << ", DB session [0x" << hex << session.id
     << "] for user " << user;
 }
 catch (boost::system::system_error &e) {
   if (!reconnecting && ((e.code() == boost::asio::error::eof) ||
     (e.code() == boost::asio::error::broken_pipe))) {
       reconnecting = true;
       reconnect();
       goto restart;
   } else
      error(string("orientdb::open(): ") + e.what());
 } 
}

bool orientsrv::dbexists(string db)
{
 orientsrv_buf r(db);
 orientrsp rsp = send(ORIENTDB_DB_EXIST, r);
 u8 exist;
 rsp.parse(&exist);
 if (verbose() > 1)
   app_log << "orientsrv::exists(" << db << ") = " << (exist ? "true" : "false");
 return exist;
}

void orientsrv::dropdb(string db)
{
 if (verbose() > 1)
   app_log << "Dropping DB " << db;
 orientsrv_buf r(db);
 orientrsp rsp = send(ORIENTDB_DB_DELETE, r);
 // reply is sid & protoversion
 // ...
 rsp.check_result();
 if (verbose())
   app_log << "DB " << db << " dropped";
}

void orientsrv::createdb(string db, int db_type, int db_engine)
{
#if 0
 if (exists(db))
   error("DB " + db + " already exists !");
#endif
 if (verbose() > 1)
   app_log << "Creating DB " << db;
 // (database-name:string)(database-type:string)(storage-type:string)
 orientsrv_buf r(db);
 r.append((db_type == AS_DOCUMENT_DB) ? "document" : "graph");
 r.append((db_engine == DB_LOCAL_STORAGE) ? "local" : "memory");
 orientrsp rsp = send(ORIENTDB_DB_CREATE, r);
 rsp.check_result();
 if (verbose())
   app_log << "DB " << db << " created";
}

orientsrv::~orientsrv()
{
 if (isconnected() && (verbose() > 1))
   app_log << "~orientsrv(): Disconnected from " << host << ":" << port;
 close();
}

orientdb::~orientdb()
{
 if (verbose() > 1)
   app_log << "~orientdb_t(): Closing database " << db;
}

orient_record_t orientquery::parse_record(orientrsp &rsp)
{
 u8 record_type;
 s16 record_header, cluster_id;
 s64 cluster_pos;
 s32 record_version;
 orientsrv_buf record_content;
 rsp.parse(&record_header);
 switch (record_header) {
   case -2: // NULL
     if (db->verbose() > 1)
       app_log << "got Null";
     break;
   case -3: // RID
     rsp.parse(&cluster_id);
     rsp.parse(&cluster_pos);
     if (db->verbose() > 1)
       app_log << "got RID: " << cluster_id << ":" << cluster_pos;
     return orient_record_t(cluster_id, cluster_pos);
   case 0: // record
     rsp.parse(&record_type);
     rsp.parse(&cluster_id);
     rsp.parse(&cluster_pos);
     rsp.parse(&record_version);
     rsp.parse(&record_content);
     if (db->verbose() > 1)
       app_log << "got record: " << cluster_id << ":" << cluster_pos << ", type: " << record_type
         << ", ver: " << record_version << ", len: " << record_content.size();
     return orient_record_t(record_type, cluster_id, cluster_pos, record_version, record_content.data);
   default:
     throw Exception("parse_record(): unknown record_header [" + itoa(record_header) + "]");
 }
 return orient_null;;
}

u32 orientquery::parse_records_collection(orientrsp &rsp, vector <orient_record_t> *records)
{
 u32 n_records;
 rsp.parse(&n_records);
 if (!n_records)
   return 0;
 for (u32 i = 0; i < n_records; i++)
   records->push_back(parse_record(rsp));
 return n_records;
}

#ifdef ORIENTPP_DEBUG
#undef execute
orientresult_ptr orientquery::execute_debug(const char *file, int line, const char *func,
  const char *qs, int query_type)
#else
orientresult_ptr orientquery::execute(const char *qs, int query_type)
#endif
{
 bool reconnecting = false;
 orientresult_ptr result(new orientresult);
restart:
 try {
  if (!prepared) {
    if (qs)
      q = qs;
    else if (buf.str().size())
      q = buf.str();
    buf.str("");
  }
  if (!q.size()) // empty query
    return result;
  orientsrv_buf r;
  // (mode:byte)(command-serialized:bytes)
  // 'a' - async, 's' - sync
  u8 mode = 's';
  r.append(mode);  
  orientsrv_buf command_serialized;
  // command-serialized: (class-name:string)(command-payload)
  // q - com.orientechnologies.orient.core.sql.query.OSQLSynchQuery: query (select)
  // c - com.orientechnologies.orient.core.sql.OCommandSQL: SQL commands (insert, update)
  // s or 'com.orientechnologies.orient.core.command.script.OCommandScript' : Script commands
  string class_name;
  if (query_type == AS_SQL) {
    // check for select or insert/update
    if (!strncasecmp("select", q.c_str(), 6))
      class_name = "q"; // com.orientechnologies.orient.core.sql.query.OSQLSynchQuery
    else
      class_name = "c"; // com.orientechnologies.orient.core.sql.OCommandSQL
  } else if (query_type == AS_JAVASCRIPT)
      class_name = "s";
  else if (query_type == AS_GREMLIN)
    class_name = "com.orientechnologies.orient.graph.gremlin.OCommandGremlin";
  else
    db->error("Unsupported script language !");
#ifdef ORIENTPP_DEBUG
  string qtype_str;
  if (query_type == AS_SQL)
    qtype_str = "SQL";
  else if (query_type == AS_JAVASCRIPT)
    qtype_str = "JS";
  else
    qtype_str = "GREMLIN";
  app_log << "OrientPP::query [" << qtype_str << "] [" << func << "():" << file << ":" << line << "] "
    << q;
#endif 
  command_serialized.append(class_name);
  // SQL Command
  // (text:string)(non-text-limit:int)[(fetchplan:string)](serialized-params:bytes)
  // SQL Script Command
  // (language:string)(text:string)(non-text-limit:int)[(fetchplan:string)](serialized-params:bytes)
  if (query_type == AS_JAVASCRIPT) {
    string language = "javascript";
    command_serialized.append(language);
  }
  command_serialized.append(q);
  s32 non_text_limit = -1;
  command_serialized.append(non_text_limit);
  // string fetchplan;
  // command_serialized.append(fetchplan);
  //if (prepared_statement) {
  // orientsrv_buf serialized_params;
  // command_serialized.append(serialized_params);
  // } else // no params
  command_serialized.append((s32)0);
  r.append(command_serialized);
  orientrsp rsp = db->send(ORIENTDB_COMMAND, r);
//  db->verbose(2);
  // [(payload-status:byte)[(content:?)]*]+
  u8 payload_status;
  rsp.parse(&payload_status);
  switch (payload_status) {
    case 'l': // collection of records
      parse_records_collection(rsp, &(result->records));
      break;
    case 'r': // single record returned
      result->records.push_back(parse_record(rsp));
      break;
    case 0:   // no records
    case 'n': // null result
      break;
    case 1:   // record is returned as a resultset
    case 2:   // record is returned as pre-fetched to be loaded in client's cache only
              // It's not part of the result set but the client knows that it's available for
              // later access
    case 'a': // serialized result
     {
        string res;
        rsp.parse(&res);
        if (db->verbose() > 1)
          app_log << "Got serialized result [" << res << "]";
        result->records.push_back(orient_record_t(res));
     }
      break;
    default:
      db->error("Unsupported query result [" + itoa(payload_status) + "]");
  }
  if (!strncasecmp("create", q.c_str(), 6) || !strncasecmp("insert", q.c_str(), 6)) {
    insert_id = result->records[0];
    insert_id.type = ORIENT_RECORD_ID;
  }
  if (db->verbose() > 1)
    app_log << "Query returns " << result->records.size() << " records";
 }
 catch (boost::system::system_error &e) {
   if (!reconnecting && ((e.code() == boost::asio::error::eof) ||
     (e.code() == boost::asio::error::broken_pipe))) {
       reconnecting = true;
       db->reopen();
       goto restart;
   } else
      db->error(string("orientquery::execute(): ") + e.what());
 }
 return result;
}

orient_record_t orient_null;

};
