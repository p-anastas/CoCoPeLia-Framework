
#include <iostream>

#include "CoCoPeLiaModel.hpp"
#include "unihelpers.hpp"

double link_cost_1D[LOC_NUM][LOC_NUM];
double link_cost_2D[LOC_NUM][LOC_NUM];

#ifdef ENABLE_TRANSFER_HOPS
short link_hop_num[LOC_NUM][LOC_NUM];
short link_hop_route[LOC_NUM][LOC_NUM][MAX_ALLOWED_HOPS];
double link_cost_hop_1D[LOC_NUM][LOC_NUM];
double link_cost_hop_2D[LOC_NUM][LOC_NUM];

void InitHopMap(double link_cost [][LOC_NUM], double link_cost_out [][LOC_NUM]){
  for (int unit_idx = 0 ; unit_idx < LOC_NUM; unit_idx++)
  for (int unit_idy = 0 ; unit_idy < LOC_NUM; unit_idy++){
    for (int hops = 0 ; hops < MAX_ALLOWED_HOPS; hops++) link_hop_route[unit_idx][unit_idy][hops] = -42;
    link_hop_num[unit_idx][unit_idy] = 0;
    double min_hop_cost = link_cost[unit_idx][unit_idy];
    if(MAX_ALLOWED_HOPS >= 1){
      for (int hop_idx = 0 ; hop_idx < LOC_NUM; hop_idx++)
      if(hop_idx!= unit_idx && hop_idx!= unit_idy && unit_idx!= unit_idy){
        double hop_cost =  fmax(link_cost[unit_idx][hop_idx], link_cost[hop_idx][unit_idy]);
        hop_cost+= HOP_PENALTY*hop_cost;
        if (hop_cost < min_hop_cost){
         min_hop_cost = hop_cost;
         link_hop_route[unit_idx][unit_idy][0] = hop_idx;
         link_hop_num[unit_idx][unit_idy] = 1;
        }
      }
    }
    if(MAX_ALLOWED_HOPS >= 2){
      for (int hop_idx = 0 ; hop_idx < LOC_NUM; hop_idx++)
      for (int hop_idy = 0 ; hop_idy < LOC_NUM; hop_idy++)
      if(hop_idx!= unit_idx && hop_idx!= unit_idy && unit_idx!= unit_idy &&
      hop_idy!= unit_idx && hop_idy!= unit_idy && hop_idy!= hop_idx){
        double hop_cost =  fmax(link_cost[unit_idx][hop_idx], fmax(link_cost[hop_idx][hop_idy], link_cost[hop_idy][unit_idy]));
        hop_cost+= 2*HOP_PENALTY*hop_cost;
        if (hop_cost < min_hop_cost){
          min_hop_cost = hop_cost;
          link_hop_route[unit_idx][unit_idy][0] = hop_idy;
          link_hop_route[unit_idx][unit_idy][1] = hop_idx;
          link_hop_num[unit_idx][unit_idy] = 2;
        }
      }
    }
    if (link_hop_num[unit_idx][unit_idy]){
    link_cost_out[unit_idx][unit_idy] = min_hop_cost;
#ifdef DSTEST
      lprintf(1, "InitHopMap: %2d->%2d transfer sequence -> %s -> ", unit_idy, unit_idx,
        printlist(link_hop_route[unit_idx][unit_idy], link_hop_num[unit_idx][unit_idy]));
      lprintf(0, "Cost No-hop = %lf, Hop-adjusted = %lf (%3lf times faster)\n", link_cost[unit_idx][unit_idy],
        link_cost_out[unit_idx][unit_idy], link_cost[unit_idx][unit_idy]/link_cost_out[unit_idx][unit_idy]);
#endif
    }
    else link_cost_out[unit_idx][unit_idy] = link_cost[unit_idx][unit_idy];
  }
}
#endif

void CoCoUpdateLinkSpeed1D(CoControl_p autotuned_vals, CoCoModel_p* glob_model){
  short lvl = 2;
  #ifdef DDEBUG
    lprintf(lvl, "|-----> CoCoUpdateLinkSpeed2D(dev_num = %d, LOC_NUM = %d)\n", autotuned_vals->dev_num, LOC_NUM);
  #endif
  for (int i = 0; i < autotuned_vals->dev_num; i++){
		short dev_id_idi = idxize(autotuned_vals->dev_ids[i]);
    for(int j = 0; j < LOC_NUM; j++){
      short dev_id_idj = idxize(j);
      if(dev_id_idi == dev_id_idj) link_cost_1D[dev_id_idi][dev_id_idj] = 0;
      else link_cost_1D[dev_id_idi][dev_id_idj] = t_com_predict(glob_model[dev_id_idi]->revlink[dev_id_idj], autotuned_vals->T*sizeof(VALUE_TYPE));
    }
    for(int j = 0; j < LOC_NUM; j++){
      short dev_id_idj = idxize(j);
      if(dev_id_idi == dev_id_idj) continue;
      int flag_normalize[LOC_NUM] = {0}, normalize_num = 1;
      double normalize_sum = link_cost_1D[dev_id_idi][dev_id_idj];
      flag_normalize[j] = 1;
      for (int k = j + 1; k < LOC_NUM; k++)
        if(abs(link_cost_1D[dev_id_idi][dev_id_idj] - link_cost_1D[dev_id_idi][idxize(k)])
          /link_cost_1D[dev_id_idi][dev_id_idj] < NORMALIZE_NEAR_SPLIT_LIMIT){
          flag_normalize[k] = 1;
          normalize_sum+=link_cost_1D[dev_id_idi][idxize(k)];
          normalize_num++;
        }
      for (int k = j ; k < LOC_NUM; k++) if(flag_normalize[k]) link_cost_1D[dev_id_idi][idxize(k)] = normalize_sum/normalize_num;
    }
  }
#ifdef ENABLE_TRANSFER_HOPS
  InitHopMap(link_cost_1D, link_cost_hop_1D);
#endif

#ifdef DEBUG
  lprintf(lvl-1, "<-----| CoCoUpdateLinkSpeed1D()\n");
#endif
}

void CoCoUpdateLinkSpeed2D(CoControl_p autotuned_vals, CoCoModel_p* glob_model){
  short lvl = 2;
  #ifdef DDEBUG
    lprintf(lvl, "|-----> CoCoUpdateLinkSpeed2D(dev_num = %d, LOC_NUM = %d)\n", autotuned_vals->dev_num, LOC_NUM);
  #endif
  for (int i = 0; i < autotuned_vals->dev_num; i++){
		short dev_id_idi = idxize(autotuned_vals->dev_ids[i]);
    for(int j = 0; j < LOC_NUM; j++){
      short dev_id_idj = idxize(j);
      if(dev_id_idi == dev_id_idj) link_cost_2D[dev_id_idi][dev_id_idj] = 0;
      else link_cost_2D[dev_id_idi][dev_id_idj] = t_com_predict(glob_model[dev_id_idi]->revlink[dev_id_idj], autotuned_vals->T*autotuned_vals->T*sizeof(VALUE_TYPE));
    }
    for(int j = 0; j < LOC_NUM; j++){
      short dev_id_idj = idxize(j);
      if(dev_id_idi == dev_id_idj) continue;
      int flag_normalize[LOC_NUM] = {0}, normalize_num = 1;
      double normalize_sum = link_cost_2D[dev_id_idi][dev_id_idj];
      flag_normalize[j] = 1;
      for (int k = j + 1; k < LOC_NUM; k++)
        if(abs(link_cost_2D[dev_id_idi][dev_id_idj] - link_cost_2D[dev_id_idi][idxize(k)])
          /link_cost_2D[dev_id_idi][dev_id_idj] < NORMALIZE_NEAR_SPLIT_LIMIT){
          flag_normalize[k] = 1;
          normalize_sum+=link_cost_2D[dev_id_idi][idxize(k)];
          normalize_num++;
        }
      for (int k = j ; k < LOC_NUM; k++) if(flag_normalize[k]) link_cost_2D[dev_id_idi][idxize(k)] = normalize_sum/normalize_num;
    }
  }
#ifdef ENABLE_TRANSFER_HOPS
  InitHopMap(link_cost_2D, link_cost_hop_2D);
#endif
#ifdef DEBUG
  lprintf(lvl-1, "<-----| CoCoUpdateLinkSpeed2D()\n");
#endif
}
