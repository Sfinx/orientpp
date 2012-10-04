
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved.  

#include "orient.h"

using namespace OrientPP;

class orienttree {
  orient_record_t root;
  json_spirit::wmObject tree;
  map <string, json_spirit::wmObject> rid2slice;
 public:
  // loosely assuming that first record is root record, class is Vertex, any other class is edge
  orienttree(orientresult_ptr res) : orienttree(res, res->records[0]) { }
  orienttree(orientresult_ptr res, orient_record_t root_) : root(root_) {
    // строим дерево в направлении корня
    // как бы это все сделать за один проход ?
    app_log << "Creating tree";
    vector <orient_record_t> links;
    uint vertexes = 0, edges = 0;
    uint r = res->records.size();
    while (r--) {
      bool link = (res->records[r].rid.id != root.rid.id); 
      string type = link ? "link" : "node";
      if (link) {
        edges++;
        links.push_back(res->records[r]);
        // app_log << type << ": " << string(res->records[r].rid) << ", from: "
        //  << string(res->records[r].get_property("in")) << ", to: "
        //  << string(res->records[r].get_property("out"));
      } else {
        vertexes++;
        json_spirit::wmObject slice;
        fill_node(slice, res->records[r]);
        // add slice to map
        string rid(string(res->records[r].rid).c_str() + 1);
        rid2slice.insert(pair<string, json_spirit::wmObject>(rid, slice));
        if (!r)
          type = "root " + type;
        // app_log << type << ": " << string(res->records[r].rid);
      }
    }
    app_log << "1 pass: Read " << vertexes << " nodes and " << edges << " links";
    uint linked = 0;
    for (vector <orient_record_t>::iterator it = links.begin(); it != links.end();) {
      orient_record_t link = *it;
      if (node_exists(link.get_property("in")) && node_exists(link.get_property("out"))) {
        json_spirit::wmObject &in = get_node(link.get_property("in")),
          &out = get_node(link.get_property("out"));
        // connect slice in (child) to slice out (parent)
        add_child(in, out);
        links.erase(it);
        linked++;
      } else
           it++;
    }
    app_log << "2 pass: Done " << linked << " links";
    if (links.size())
      throw Exception("Unconnected links remains: " + itoa(links.size()));
    string root_rid(string(root.rid).c_str() + 1);
    json_spirit::wmObject &root_slice = get_node_str(root_rid);
    tree[L"slices"] = root_slice;
  }
  json_spirit::wmObject &get_node_str(string &rid) {
    map <string, json_spirit::wmObject>::iterator it = rid2slice.find(rid);
    if (it == rid2slice.end())
      throw Exception("Node absent: " + rid);
    return it->second;
  }
  json_spirit::wmObject &get_node(property_t p) {
    if (p.type != ORIENT_RECORD_TYPE_LINK)
      throw Exception("Wrong node type: " + string(p));
    map <string, json_spirit::wmObject>::iterator it = rid2slice.find(string(p));
    if (it == rid2slice.end())
      throw Exception("Node absent: " + string(p));
    return it->second;
  }    
  bool node_exists(property_t p) {
    if (p.type != ORIENT_RECORD_TYPE_LINK)
      throw Exception("Wrong node type: " + string(p));
    map <string, json_spirit::wmObject>::iterator it = rid2slice.find(string(p));
    if (it == rid2slice.end())
      return false;
    return true;
  }    
  void fill_node(json_spirit::wmObject &obj, orient_record_t &rec) {
    for (property_iterator it = rec.properties.begin(); it != rec.properties.end(); it++) {
      property_t p = it->second;
      if (p.name == "in" || p.name == "out")
        continue;
      json_add_str(obj, p.name, string(p));
    }
  }
  void add_child(json_spirit::wmObject &parent, json_spirit::wmObject &child) {
    json_spirit::wmArray children;
    json_get(parent, "children", children);
    children.push_back(child);
    parent[L"children"] = children;
  }
  string tojson() { return json_write(tree); }
};

void dump_result(orientresult_ptr res)
{
 app_log << "Result has " << res->records.size() << " records";
 for (u32 r = 0; r < res->records.size(); r++) {
   stringstream ss;
   if (res->records[r].rid.valid) {
     if (res->records[r].is_document()) {
       ss << "#" << r << " " << string(res->records[r].rid) << " class:" << res->records[r].classof();
       for (property_iterator it = res->records[r].properties.begin();
         it != res->records[r].properties.end(); it++) {
           property_t p = it->second;
           ss << ", t: " << p.type2str() << ", n:" << p.name << ", v:" << string(p);
       }
     } else
       ss << "#" << r << " " << string(res->records[r].rid) << " " << string(res->records[r]);
   } else
     ss << "#" << r << " " << string(res->records[r]);
   app_log << ss.str();
   ss.str("");
 }
}

void OrientDBTest()
{
 app_log << "OrientDB test: Start";
 orientsrv server("localhost", "root" , "root");
#if TEST_SHUTDOWN
 server.shutdown();
#endif
#if TEST_DROP
 if (server.dbexists("test"))
   server.dropdb("test");
 server.createdb("test", AS_GRAPH_DB, DB_LOCAL_STORAGE);
#endif
 if (!server.dbexists("test"))
   server.createdb("test", AS_GRAPH_DB, DB_LOCAL_STORAGE);
 orientdb db(server);
 db.open("test", AS_GRAPH_DB, "admin", "admin");
 app_log << "DB size: " << db.size();
 app_log << "DB records count: " << db.count();

 orientquery q(db);
#ifdef TEST_SELECT
 q << "select * from ouser";
 dump_result(q.execute(AS_SQL));
#endif

 q << "drop class Slices";
 q.execute(AS_SQL);
 q << "drop class connected_to";
 q.execute(AS_SQL);

 q << "create class Slices extends V";
 dump_result(q.execute(AS_SQL));
 q << "create property Slices.name STRING";
 dump_result(q.execute(AS_SQL));
 q << "create property Slices.description STRING";
 dump_result(q.execute(AS_SQL));
 q << "create class connected_to extends E";
 dump_result(q.execute(AS_SQL));
 
 q << "create vertex Slices create vertex Slices set name='Dao', description='Root'";
 dump_result(q.execute(AS_SQL));
 orient_record_t root_slice = q.insert_id;
 q << "create vertex Slices set name='Texts', description='Texts, Docs'";
 dump_result(q.execute(AS_SQL));
 orient_record_t texts_slice = q.insert_id;
 q << "create edge connected_to from " << string(texts_slice) << " to " << string(root_slice);
 dump_result(q.execute(AS_SQL));
 q << "create vertex Slices set name='Books', description='Books'";
 dump_result(q.execute(AS_SQL));
 q << "create edge connected_to from " << string(q.insert_id) << " to " << string(texts_slice);
 dump_result(q.execute(AS_SQL));

 q << "create vertex Slices set name='Programming', description='All about programming'";
 dump_result(q.execute(AS_SQL));
 orient_record_t programming_slice = q.insert_id;
 q << "create edge connected_to from " << string(q.insert_id) << " to " << string(root_slice);
 dump_result(q.execute(AS_SQL));
 q << "create vertex Slices set name='Languages', description='All languages'";
 dump_result(q.execute(AS_SQL));
 orient_record_t languages_slice = q.insert_id;
 q << "create edge connected_to from " << string(q.insert_id) << " to " << string(programming_slice);
 dump_result(q.execute(AS_SQL));
 q << "create vertex Slices set name='C', description='C language'";
 dump_result(q.execute(AS_SQL));
 q << "create edge connected_to from " << string(q.insert_id) << " to " << string(languages_slice);
 dump_result(q.execute(AS_SQL));
 q << "create vertex Slices set name='C++', description='C++ language'";
 dump_result(q.execute(AS_SQL));
 q << "create edge connected_to from " << string(q.insert_id) << " to " << string(languages_slice);
 dump_result(q.execute(AS_SQL));
 
 q << "create vertex Slices set name='Images', description='Pictures'";
 dump_result(q.execute(AS_SQL));
 orient_record_t images_slice = q.insert_id;
 q << "create edge connected_to from " << string(q.insert_id) << " to " << string(root_slice);
 dump_result(q.execute(AS_SQL));
 q << "create vertex Slices set name='Nature', description='Pics of nature'";
 dump_result(q.execute(AS_SQL));
 q << "create edge connected_to from " << string(q.insert_id) << " to " << string(images_slice); 
 dump_result(q.execute(AS_SQL));
 // db.verbose(2);
 q << "traverse * from V"; // << string(root_slice);
 orientresult_ptr res = q.execute(AS_SQL);
 dump_result(res);

 orienttree sfinx_objects_tree(res /*, root_slice */);
 app_log << "JSON: " << sfinx_objects_tree.tojson();

#ifdef WHEN_JS_SOMETIME_WILL_WORK_IN_SNAPSHOT
 q << "var r = db.query('select from ouser');print(r);r";
 res = q.execute(AS_JAVASCRIPT);
#endif
#ifdef TEST_GREMLIN
 q << "v = g.addVertex()";
 dump_result(q.execute(AS_GREMLIN));
 q << "g.stopTransaction(SUCCESS)";
 q.execute(AS_GREMLIN);
#endif
 db.close();
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
