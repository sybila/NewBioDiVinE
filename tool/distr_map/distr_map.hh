#include "divine.h"

static divine::size_int_t nid;

static divine::size_int_t bfs_number=1; //number of recently visited states (per workstation)

struct map_value_t
{ divine::size_int_t bfs_order; //number corresponding to BFS ordering || id in divine::state_ref_t
  divine::size_int_t nid; //ID of computer (unique)
  divine::size_int_t id;
};

//old_map of the state visited during extracting accepting cycle
const divine::size_int_t EXTRACT_CYCLE_VISIT = divine::MAX_ULONG_INT-2;

//old_map of the state visited during extracting a prefix to accepting cycle
const divine::size_int_t EXTRACT_PATH_VISIT = divine::MAX_ULONG_INT-3;

/* !!! values of EXTRACT_CYCLE_VISIT and EXTRACT_PATH_VISIT can not be used
   as map_value_t.id anywhere !!! */

/* !!! map_value_t.nid can NOT be set to divine::MAX_ULONG_INT anywhere in NULL_MAP_* !!! */

const map_value_t NULL_MAP_BFS_LOW = { 0,
                                       0,
                                       0 };

const map_value_t NULL_MAP_VISIT_BFS_LOW = { 0,
                                             divine::MAX_ULONG_INT-1,
                                             0 };

const map_value_t NULL_MAP_BFS_HIGH = { divine::MAX_ULONG_INT,
                                        divine::MAX_ULONG_INT-1,
                                        0 };

const map_value_t NULL_MAP_VISIT_BFS_HIGH = { divine::MAX_ULONG_INT,
                                              divine::MAX_ULONG_INT-1,
                                              0 };

const map_value_t NULL_MAP_RND = { divine::MAX_ULONG_INT,
                                   divine::MAX_ULONG_INT-1,
                                   divine::MAX_ULONG_INT };

const map_value_t NULL_MAP_VISIT_RND = { divine::MAX_ULONG_INT-1,
                                         divine::MAX_ULONG_INT-1,
                                         divine::MAX_ULONG_INT };

bool operator== (map_value_t val1, map_value_t val2)
{ return ((val1.bfs_order == val2.bfs_order)&&
          (val1.nid       == val2.nid)&&
          (val1.id        == val2.id));
}

bool cmp_map_bfs_low (map_value_t val1, map_value_t val2) //first visited state is lower
{ return ((val1.bfs_order<val2.bfs_order)||
          ((val1.bfs_order==val2.bfs_order)&&(val1.nid<val2.nid)));
}

bool cmp_map_bfs_high (map_value_t val1, map_value_t val2) //first visited state is higher
{ return ((val1.bfs_order>val2.bfs_order)||
          ((val1.bfs_order==val2.bfs_order)&&(val1.nid>val2.nid)));
}

void vypis_map_value(map_value_t val, std::ostream &outs)
{ outs << '[' << val.nid << ',' << val.bfs_order;
//  if (val.id!=0)
    outs << ',' << val.id;
  outs << ']';
}
        

bool cmp_map_rnd_hip (map_value_t val1, map_value_t val2)
{ 
  return ((val2.id != divine::MAX_ULONG_INT)&&
	  ((val1.id == divine::MAX_ULONG_INT)||
	   (val1.bfs_order < val2.bfs_order)||
	   ((val1.bfs_order == val2.bfs_order)&&
	    (val1.id         < val2.id))||
	   ((val1.bfs_order == val2.bfs_order)&&
	    (val1.id        == val2.id)&&
	    (val1.nid     < val2.nid))));
}
                                            
map_value_t prirad_novy_map_bfs(divine::state_ref_t *ref)
{ map_value_t val;
  val.bfs_order = bfs_number++;
  val.nid = nid;
  val.id = 0;
  return val;
}

map_value_t prirad_novy_map_rnd(divine::state_ref_t *ref)
{ map_value_t val;
  val.bfs_order = ref->hres;
  val.nid = nid;
  val.id = ref->id;
  /*  vypis_map_value(val, cerr);
      cerr << endl;*/
  return val;
}

void reset_new_map_val(void)
{ bfs_number = 1;
}

map_value_t (*prirad_novy_map) (divine::state_ref_t *ref) = prirad_novy_map_rnd;
bool (*cmp_map) (map_value_t val1, map_value_t val2) = cmp_map_rnd_hip;
map_value_t NULL_MAP = NULL_MAP_RND;
map_value_t NULL_MAP_VISIT = NULL_MAP_VISIT_RND;

//bool (* operator <) (map_value_t val1, map_value_t val2) = cmp_map_rnd_hip;







