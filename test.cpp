
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved.  

#include "orient.h"

using namespace OrientPP;

void OrientDBTest()
{
 app_log << "OrientDB test: Start";
 orientsrv server("localhost", "root" , "root");
#if 0
 server.shutdown();
 orientsrv server("localhost");
 if (server.dbexists("test"))
   server.dropdb("test");
 server.createdb("test", AS_GRAPH_DB, DB_LOCAL_STORAGE);
#endif
 if (!server.dbexists("test"))
   server.createdb("test", AS_GRAPH_DB, DB_LOCAL_STORAGE);
 orientdb db(server);
 db.open("tinkerpop", AS_GRAPH_DB, "admin", "admin");
 app_log << "size1: " << db.size();
 app_log << "count1: " << db.count();
 db.close();
 db.open("test", AS_GRAPH_DB, "admin", "admin");
 app_log << "size2: " << db.size();
 app_log << "count2: " << db.count();
 orientquery q(db);
 q << "var r = db.query('select from ouser');print(r);r";
 orientresult_ptr res = q.execute(AS_JAVASCRIPT);
 app_log << "query has " << res->records.size() << " records";
 for (u32 r = 0; r < res->records.size(); r++) {
   stringstream ss;
   // TODO: parse record by fields/types
   // ....
   ss << r << ": " << string(res->records[r]);
   app_log << ss.str();
   ss.str("");
 }
 q << "v = g.addVertex()";
 res = q.execute(AS_GREMLIN);
 app_log << "query has " << res->records.size() << " records";
 q << "g.stopTransaction(SUCCESS)";
 q.execute(AS_GREMLIN);
 q << "select * from ouser";
 res = q.execute(AS_SQL);
 app_log << "query has " << res->records.size() << " records";
 for (u32 r = 0; r < res->records.size(); r++) {
   stringstream ss;
   ss << r << ": " << string(res->records[r]);
   app_log << ss.str();
   ss.str("");
 }
 q << "drop class Slice";
 q.execute(AS_SQL);
 q << "create class Slice extends V";
 q.execute(AS_SQL);
 q << "create vertex Slice";
 q.execute(AS_SQL);
 orient_record_t vertex1 = q.insert_id;
 app_log << "Vertex1: " << string(vertex1);
 q << "create vertex Slice";
 q.execute(AS_SQL);
 orient_record_t vertex2 = q.insert_id;
 app_log << "Vertex2: " << string(vertex2);
 q << "create edge from " << string(vertex1) << " to " << string(vertex2);
 q.execute(AS_SQL);
 app_log << "Edge1: " << string(q.insert_id);
 q << "traverse * from " << string(vertex1);
 q.execute(AS_SQL);
 app_log << "OrientDB test: Done";
}

int main(int argc, char **argv) {
  if ((argc > 1) && (atoi(argv[1]) == 1)) {
    debug.log_to_file = true;
    cout << "[" + time_str() + "] " << "[Init] Logging to " << debug.log_file << endl;
  } else if ((argc > 1) && (atoi(argv[1]) == 2))
    debug.log_to_stdout = true;
  else
    cout << "[" + time_str() + "] " << "[Init] Logging disabled" << endl;

  app_log << "[Init] Changeset " << ORIENTPP_CHANGESET << ", changeset number "
    << ORIENTPP_CHANGESET_NUMBER << ", build number " << ORIENTPP_BUILD_NUMBER;
  try {
    OrientDBTest();
  }
  catch (Exception& e) {
    app_log << "[Init] OrientDB Exception: " << e.what();
  }
  catch (exception& e) {
      app_log << "[Init] Std Exception: " << e.what();
  }    
  catch (...) {
      app_log << "[Init] Unknown Exception";
  }    
  return 0;
}
