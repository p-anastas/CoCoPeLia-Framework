///
/// \author Anastasiadis Petros (panastas@cslab.ece.ntua.gr)
///
/// \brief The "Asset" related function implementations.
///

#include "Asset.hpp"
#include "unihelpers.hpp"

int Tile2D_num = 0;
double link_used_2D[LOC_NUM][LOC_NUM] = {0};

template class Tile2D<double>;

template<typename dtype>  Tile2D<dtype>::Tile2D(void * in_addr, int in_dim1, int in_dim2,
  int in_ldim, int inGrid1, int inGrid2, CBlock_p init_loc_block_p)
{
  short lvl = 3;

  #ifdef DDEBUG
    lprintf(lvl-1, "|-----> Tile2D(%d)::Tile2D(in_addr(%d),%d,%d,%d, %d, %d)\n",
      Tile2D_num, CoCoGetPtrLoc(in_addr), in_dim1, in_dim2, in_ldim, inGrid1, inGrid2);
  #endif

  dim1 = in_dim1;
  dim2 = in_dim2;
  GridId1 = inGrid1;
  GridId2 = inGrid2;
  id = Tile2D_num;
  Tile2D_num++;
  short init_loc = CoCoGetPtrLoc(in_addr);
  short init_loc_idx = idxize(init_loc);
  WriteBackLoc = init_loc;
  for (int iloc = 0; iloc < LOC_NUM; iloc++){
    RunTileMap[iloc] = 0;
    if (iloc == init_loc_idx){
      WriteBackBlock = init_loc_block_p;
      StoreBlock[iloc] = init_loc_block_p;
      StoreBlock[iloc]->Adrs = in_addr;
      StoreBlock[iloc]->set_owner((void**)&StoreBlock[iloc],false);
      ldim[iloc] = in_ldim;
      StoreBlock[iloc]->Available->record_to_queue(NULL);
    }
    else{
      StoreBlock[iloc] = NULL;
      ldim[iloc] = in_dim1;
    }
  }
  W_flag = R_flag = W_total = 0;

  RW_lock = -42;
  RW_lock_holders = 0;

  RW_master = init_loc;
  #ifdef DDEBUG
  	lprintf(lvl-1, "<-----|\n");
  #endif
}

template<typename dtype>  Tile2D<dtype>::~Tile2D()
{
  short lvl = 3;
  Tile2D_num--;
}

template<typename dtype> short Tile2D<dtype>::getWriteBackLoc(){
  return WriteBackLoc;
}

template<typename dtype> short Tile2D<dtype>::isLocked(){
#ifdef ENABLE_MUTEX_LOCKING
		error("Not sure how to do this correctly\n");
    return 0;
#else
		if(RW_lock) return 1;
    else return 0;
#endif
}

template<typename dtype> short Tile2D<dtype>::getClosestReadLoc(short dev_id_in){
  short lvl = 5;
#ifdef DDEBUG
  lprintf(lvl-1, "|-----> Tile2D(%d)::getClosestReadLoc(%d)\n", id, dev_id_in);
#endif
  int dev_id_in_idx = idxize(dev_id_in);
  int pos_min = LOC_NUM;
  double link_cost_2D_min = 10000000;
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
#ifdef ENABLE_TRANSFER_HOPS
        double current_link_cost = link_cost_hop_2D[dev_id_in_idx][pos];
#else
        double current_link_cost = link_cost_2D[dev_id_in_idx][pos];
#endif
        if (block_status == RECORDED) current_link_cost+=current_link_cost*FETCH_UNAVAILABLE_PENALTY;
        if (current_link_cost < link_cost_2D_min){
          link_cost_2D_min = current_link_cost;
          pos_min = pos;
        }
        else if (current_link_cost == link_cost_2D_min &&
        link_used_2D[dev_id_in_idx][pos] < link_used_2D[dev_id_in_idx][pos_min]){
          link_cost_2D_min = current_link_cost;
          pos_min = pos;
        }
      }
    }
  }
#ifdef DEBUG
  lprintf(lvl, "|-----> Tile2D(%d)::getClosestReadLoc(%d): Selecting cached tile in loc =%d \n", id, dev_id_in, pos_min);
#endif
  if (pos_min >= LOC_NUM) error("Tile2D(%d)::getClosestReadLoc(%d): No location found for tile - bug.", id, dev_id_in);
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
      link_used_2D[dev_id_in_idx][pos_min]++;
      return deidxize(pos_min);
    }
    else error("Tile2D(%d)::getClosestReadLoc(%d): pos_min = %d selected,\
      but something changed after locking its cache...fixme\n", id, dev_id_in, pos_min);
  }
  else error("Tile2D(%d)::getClosestReadLoc(%d): pos_min = %d selected,\
    but StoreBlock[pos_min] was NULL after locking its cache...fixme\n", id, dev_id_in, pos_min);
  return -666;
}

template<typename dtype> double Tile2D<dtype>::getMinLinkCost(short dev_id_in){
  short lvl = 5;
  int dev_id_in_idx = idxize(dev_id_in);
  double link_cost_2D_min = 10000000;
  for (int pos =0; pos < LOC_NUM; pos++){
    CBlock_p temp_outblock = StoreBlock[pos];
    if(temp_outblock == NULL) continue;
    //StoreBlock[pos]->update_state(false);
    state temp = temp_outblock->State;
    if (temp == AVAILABLE || temp == SHARABLE || temp == NATIVE){
      event_status block_status = temp_outblock->Available->query_status();
      if(block_status == COMPLETE || block_status == CHECKED || block_status == RECORDED){
#ifdef ENABLE_TRANSFER_HOPS
        double current_link_cost = link_cost_hop_2D[dev_id_in_idx][pos];
#else
        double current_link_cost = link_cost_2D[dev_id_in_idx][pos];
#endif
        if (block_status == RECORDED) current_link_cost+=current_link_cost*FETCH_UNAVAILABLE_PENALTY;
        if (current_link_cost < link_cost_2D_min) link_cost_2D_min = current_link_cost;
      }
    }
  }
  return link_cost_2D_min;
}
