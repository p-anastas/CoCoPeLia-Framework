///
/// \author Anastasiadis Petros (panastas@cslab.ece.ntua.gr)
///
/// \brief The "Asset" related function implementations.
///

#include "Asset.hpp"
#include "unihelpers.hpp"

int Tile1D_num = 0;
double link_used_1D[LOC_NUM][LOC_NUM] = {0};

template class Tile1D<double>;

template<typename dtype>  Tile1D<dtype>::Tile1D(void * in_addr, int in_dim,
  int in_inc, int inGrid, CBlock_p init_loc_block_p)
{
  short lvl = 3;

  #ifdef DEBUG
    lprintf(lvl-1, "|-----> Tile1D(%d)::Tile1D(in_addr(%d), %d, %d, %d)\n",
      Tile1D_num, CoCoGetPtrLoc(in_addr), in_dim, in_inc, inGrid);
  #endif

  dim = in_dim;
  GridId = inGrid;
  id = Tile1D_num;
  Tile1D_num++;
  short init_loc = CoCoGetPtrLoc(in_addr);
  short init_loc_idx = idxize(init_loc);
  WriteBackLoc = init_loc;
  for (int iloc = 0; iloc < LOC_NUM; iloc++){
    RunTileMap[iloc] = 0;
    if (iloc == init_loc_idx){
      WriteBackBlock = init_loc_block_p;
      StoreBlock[iloc] = init_loc_block_p;
      StoreBlock[iloc]->Adrs = in_addr;
      StoreBlock[iloc]->set_owner((void**)&StoreBlock[iloc]);
      inc[iloc] = in_inc;
      StoreBlock[iloc]->Available->record_to_queue(NULL);
    }
    else{
      StoreBlock[iloc] = NULL;
      inc[iloc] = in_inc;
    }
  }
  W_flag = R_flag = W_total = 0;
  RW_lock = -42;
  RW_lock_holders = 0;

  RW_master = init_loc;
  #ifdef DEBUG
  	lprintf(lvl-1, "<-----|\n");
  #endif
}

template<typename dtype>  Tile1D<dtype>::~Tile1D()
{
  short lvl = 3;
#ifdef DEBUG
  lprintf(lvl-1, "|-----> Tile1D(%d)::~Tile1D()\n", Tile1D_num);
#endif
  Tile1D_num--;
}

template<typename dtype> short Tile1D<dtype>::getWriteBackLoc(){
  return WriteBackLoc;
}

template<typename dtype> short Tile1D<dtype>::isLocked(){
#ifdef ENABLE_MUTEX_LOCKING
		error("Not sure how to do this correctly\n");
    return 0;
#else
		if(RW_lock) return 1;
    else return 0;
#endif
}

// TODO: to make this more sophisticated, we have to add prediction data in it.
// For now: closest = already in device, then other GPUs, then host (since devices in order etc, host after in CacheLocId)
template<typename dtype> short Tile1D<dtype>::getClosestReadLoc(short dev_id_in){
  short lvl = 5;
#ifdef DDEBUG
  lprintf(lvl-1, "|-----> Tile1D(%d)::getClosestReadLoc(%d)\n", id, dev_id_in);
#endif
  int dev_id_in_idx = idxize(dev_id_in);
  int pos_min = LOC_NUM;
  double link_cost_1D_min = 10000000;
  for (int pos =0; pos < LOC_NUM; pos++){
    if (pos == dev_id_in_idx || StoreBlock[pos] == NULL) {
      //if (StoreBlock[pos]!= NULL)
      //  error("Tile2D(%d)::getClosestReadLoc(%d): Should not be called, Tile already available in %d.\n",  id, dev_id_in, dev_id_in);
      continue;
    }
    //StoreBlock[pos]->update_state(false);
    state temp = StoreBlock[pos]->State;
    if (temp == AVAILABLE || temp == SHARABLE || temp == NATIVE){
      event_status block_status = StoreBlock[pos]->Available->query_status();
      if(block_status == COMPLETE || block_status == CHECKED || block_status == RECORDED){
        double current_link_cost = link_cost_1D[dev_id_in_idx][pos];
        if (block_status == RECORDED) current_link_cost+=current_link_cost*FETCH_UNAVAILABLE_PENALTY;
        if (current_link_cost < link_cost_1D_min){
          link_cost_1D_min = current_link_cost;
          pos_min = pos;
        }
        else if (current_link_cost == link_cost_1D_min &&
        link_used_1D[dev_id_in_idx][pos] < link_used_1D[dev_id_in_idx][pos_min]){
          link_cost_1D_min = current_link_cost;
          pos_min = pos;
        }
      }
    }
  }
#ifdef DEBUG
  lprintf(lvl, "|-----> Tile1D(%d)::getClosestReadLoc(%d): Selecting cached tile in loc =%d \n", id, dev_id_in, pos_min);
#endif
  if (pos_min >= LOC_NUM) error("Tile1D(%d)::getClosestReadLoc(%d): No location found for tile - bug.", id, dev_id_in);
  //Global_Cache[pos_min]->lock();
  CBlock_p temp_outblock = StoreBlock[pos_min];
  if(temp_outblock != NULL){
    temp_outblock->lock();
    //temp_outblock->update_state(true);
    state temp = temp_outblock->State;
    event_status block_status = temp_outblock->Available->query_status();
    if ((temp == AVAILABLE || temp == SHARABLE || temp == NATIVE) &&
    (block_status == COMPLETE || block_status == CHECKED || block_status == RECORDED)){
      temp_outblock->add_reader(true);
      //Global_Cache[pos_min]->unlock();
      temp_outblock->unlock();
      #ifdef DDEBUG
        lprintf(lvl-1, "<-----|\n");
      #endif
      link_used_1D[dev_id_in_idx][pos_min]++;
      return deidxize(pos_min);
    }
    else error("Tile1D(%d)::getClosestReadLoc(%d): pos_min = %d selected,\
      but something changed after locking its cache...fixme\n", id, dev_id_in, pos_min);
  }
  else error("Tile1D(%d)::getClosestReadLoc(%d): pos_min = %d selected,\
    but StoreBlock[pos_min] was NULL after locking its cache...fixme\n", id, dev_id_in, pos_min);
  return -666;
}

template<typename dtype> double Tile1D<dtype>::getMinLinkCost(short dev_id_in){
  short lvl = 5;
  int dev_id_in_idx = idxize(dev_id_in);
  double link_cost_1D_min = 10000000;
  for (int pos =0; pos < LOC_NUM; pos++){
    if(StoreBlock[pos] == NULL) continue;
    //StoreBlock[pos]->update_state(false);
    state temp = StoreBlock[pos]->State;
    if (temp == AVAILABLE || temp == SHARABLE || temp == NATIVE){
      event_status block_status = StoreBlock[pos]->Available->query_status();
      if(block_status == COMPLETE || block_status == CHECKED || block_status == RECORDED){
        double current_link_cost = link_cost_1D[dev_id_in_idx][pos];
        if (block_status == RECORDED) current_link_cost+=current_link_cost*FETCH_UNAVAILABLE_PENALTY;
        if (current_link_cost < link_cost_1D_min) link_cost_1D_min = current_link_cost;
      }
    }
  }
  return link_cost_1D_min;
}
