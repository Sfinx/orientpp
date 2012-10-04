
// Copyright (C) 2012, Rus V. Brushkoff, All rights reserved.  

#include "orient.h"

using namespace OrientPP;

class orienttree {
  orient_record_t root;
  int link_class_id;
  json_spirit::wmObject tree;
  map <string, json_spirit::wmObject> rid2slice;
  bool isnode(orient_record_t &r) { return (r.rid.id != link_class_id); }
 public:
  // loosely assuming that first record is root record, class is Vertex, any other class is edge
  orienttree(orientresult_ptr res, int link_class_id_) :
    orienttree(res, res->records[0], link_class_id_) { }
  orienttree(orientresult_ptr res, orient_record_t root_, int link_class_id_) :
   root(root_), link_class_id(link_class_id_) {
    // ������ ������ � ����������� �����
    // ��� �� ��� ��� ������� �� ���� ������ ?
    app_log << "Creating tree with link id " << link_class_id;
    vector <orient_record_t> links;
    uint vertexes = 0, edges = 0;
#ifdef DO_NOT_WORK_WITH_CURRENT_JSON_LIB
    uint connected_edges = 0;
#endif
    uint r = res->records.size();
    while (r--) {
      string type = isnode(res->records[r]) ? "node" : "link";
      if (!isnode(res->records[r])) {
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
        rid2slice.insert(pair<string, json_spirit::wmObject>(res->records[r].rid.str(), slice));
        if (!r)
          type = "root " + type;
        // app_log << type << ": " << string(res->records[r].rid);
      }
#ifdef DO_NOT_WORK_WITH_CURRENT_JSON_LIB
      if (links.size()) {
        if (try_connect_link(links.back())) {
          links.pop_back();
          connected_edges++;
        }
      }
#endif
    }
    app_log << "1 pass: Read " << vertexes << " nodes and " << edges << " links, "
#ifdef DO_NOT_WORK_WITH_CURRENT_JSON_LIB
     << connected_edges << " already connected";
#else
;
#endif
    if (links.size()) {
      uint linked = 0;
      for (vector <orient_record_t>::iterator it = links.begin(); it != links.end();) {
        if (try_connect_link(*it)) {
          links.erase(it);
          linked++;
        } else
             it++;
      }
      app_log << "2 pass: Done " << linked << " links";
      if (links.size())
        throw Exception("Unconnected links remains: " + itoa(links.size()));
    }
    tree[L"slices"] = get_node(root.rid.str());
  }
  bool try_connect_link(orient_record_t &link) {
    if (node_exists(link.get_property("in")) && node_exists(link.get_property("out"))) {
      json_spirit::wmObject &in = get_node(link.get_property("in")),
         &out = get_node(link.get_property("out"));
         // connect slice in (child) to slice out (parent)
         add_child(in, out);
         return true;
    }
    return false;
  }
  json_spirit::wmObject &get_node(string rid) {
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
    json_add_str(obj, "rid", string(rec.rid));
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

orient_record_t create_slice(orientquery &q, string name, string description)
{
 q << "create vertex Slices set name ='" << name << "', description ='" << description << "'";
 dump_result(q.execute(AS_SQL));
 return q.insert_id;
}

orient_record_t create_note(orientquery &q, string title, string url)
{
 q << "create vertex Notes set title ='" << title << "', url ='" << url << "'";
 dump_result(q.execute(AS_SQL));
 return q.insert_id;
}

orient_record_t connect_records(orientquery &q, orient_record_t from, orient_record_t &to)
{
 q << "create edge connected_to from " << string(from) << " to " << string(to);
 dump_result(q.execute(AS_SQL));
 return q.insert_id;
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
// if (!server.dbexists("test"))
//   server.createdb("test", AS_GRAPH_DB, DB_LOCAL_STORAGE);
 orientdb db(server);
 db.open("test2", AS_GRAPH_DB, "admin", "admin");
 app_log << "DB size: " << db.size();
 app_log << "DB records count: " << db.count();

 orientquery q(db);
#ifdef TEST_SELECT
 q << "select * from ouser";
 dump_result(q.execute(AS_SQL));
#endif
 return;
 q << "drop class Slices";
 q.execute(AS_SQL);
 q << "drop class Notes";
 q.execute(AS_SQL);
 q << "drop class connected_to";
 q.execute(AS_SQL);

 q << "create class Slices extends V";
 dump_result(q.execute(AS_SQL));
 q << "create property Slices.name STRING";
 dump_result(q.execute(AS_SQL));
 q << "create property Slices.description STRING";
 dump_result(q.execute(AS_SQL));

 q << "create class Notes extends V";
 dump_result(q.execute(AS_SQL));
 q << "create property Notes.title STRING";
 dump_result(q.execute(AS_SQL));
 q << "create property Notes.url STRING";
 dump_result(q.execute(AS_SQL));

 q << "create class connected_to extends E";
 dump_result(q.execute(AS_SQL));
 
 orient_record_t root_slice = create_slice(q, "Dao", "Root");
 orient_record_t texts_slice = create_slice(q, "Texts", "Texts, Docs");
 orient_record_t edge = connect_records(q, texts_slice, root_slice);
 int link_class_id = edge.rid.id;
 connect_records(q, create_slice(q, "EBooks", "Books in digital form"), texts_slice);
 orient_record_t programming_slice = create_slice(q, "Programming", "All about programming");
 connect_records(q, programming_slice, root_slice);
 orient_record_t languages_slice  = create_slice(q, "Languages", "Programming languages");
 connect_records(q, languages_slice, programming_slice);
 connect_records(q, create_slice(q, "C", "C language"), languages_slice);
 connect_records(q, create_slice(q, "C++", "C++ language"), languages_slice);
 orient_record_t images_slice = create_slice(q, "Images", "Pictures"); 
 connect_records(q, images_slice, root_slice);
 connect_records(q, create_slice(q, "Nature", "Pics of nature"), images_slice);

 orient_record_t note = create_note(q, "Memento Mori", "What are we living for ?");
 connect_records(q, note, texts_slice);

 q << "traverse * from V"; // << string(root_slice);
 orientresult_ptr res = q.execute(AS_SQL);
 dump_result(res);

 orienttree sfinx_objects_tree(res /*, root_slice */ , link_class_id);
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
