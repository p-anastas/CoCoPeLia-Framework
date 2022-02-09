///
/// \author Anastasiadis Petros (panastas@cslab.ece.ntua.gr)
///
/// \brief Some CUDA function calls with added error-checking
///
#ifndef COCOPELIA_MODEL_H
#define COCOPELIA_MODEL_H

#include "CoCoPeLiaCoModel.hpp"
#include "backend_wrappers.hpp"

// TODO: To avoid mallocs, define a set vec size of 4 (No BLAS has that many data arguments anyway)
typedef struct V_struct{
	// Routine specific
	short numT;
	short dtype_sz;
	short in[4]; // Possibly modified from scalar_nz
	short out[4]; // Possibly modified from scalar_nz

	// Problem specific
	size_t *Dim1[4];
	size_t *Dim2[4];
	short loc[4];
	short out_loc[4];

}* Vdata_p;

enum ModelType{
	WERKHOVEN = 0,
	WERKHOVEN_DATALOC = 1,
	WERKHOVEN_LOOKUP_EXEC_TILES = 2,
	COCOPELIA_BASELINE = 3,
	COCOPELIA_DATALOC = 4,
	COCOPELIA_BIDIRECTIONAL = 5,
	COCOPELIA_REUSE = 6,
	COCOPELIA_PIPELINE_EMULATE = 7
};

typedef struct tunableParams{
	size_t T;
	double pred_t;
}* tunableParams_p;

typedef struct flagParams{
	char TransA;
	char TransB;
	//TODO: Add all flags used in BLAS, only applicable initialized/used for each routine.
}* flagParams_p;

typedef struct CoCo_model{
	Vdata_p V;
	//TODO: Legacy h2d/d2h links for Werkhoven/old models
	CoModel_p h2d;
	CoModel_p d2h;
	CoModel_p link[DEV_NUM];
	char* func;
	double Ker_pot;
	size_t D1, D2, D3;
	flagParams_p flags;
	void* GPUexec_model_ptr;
	// FIXME: Add cpu_exec prediction

}* CoCoModel_p;

short* CoCoPeLiaDeviceSelectBest(short used_devs, short avail_devs, short* avail_dev_ids,
	CoCoModel_p* avail_dev_model_list);

tunableParams_p CoCoPeLiaModelMultidevOptimizeTile(short used_devs, short* used_dev_ids,
	CoCoModel_p* dev_model_list);

///  Mode-Generalized prediction wrapper
double CoCoPeLiaModelPredict(CoCoModel_p model, size_t T, ModelType mode);

double WerkhovenModelPredictWrapper(CoCo_model* model, size_t T, short t_exec_method);
double CoCopeLiaPredictBaseline(CoCoModel_p model, size_t T);
double CoCopeLiaPredictDataLoc(CoCoModel_p model, size_t T);
double CoCopeLiaPredictBidirectional(CoCoModel_p model, size_t T);
double CoCopeLiaPredictReuse(CoCoModel_p model, size_t T);
double CoCopeLiaPipelineEmulate(CoCoModel_p model, size_t T);

CoCoModel_p CoCoPeLiaTileModelInit(short dev_id, char* func_name, void* func_data);
///  Predicts Best tile size for 3-way overlaped execution time for BLAS3 2-dim blocking.
tunableParams_p CoCoPeLiaModelOptimizeTile(CoCoModel_p model, ModelType mode);

/// Choose the best way to approach h2d/d2h overlap
short CoCoModel_choose_transfer_mode3(CoCoModel_p model, size_t T);

///  Predicts Best tile size for 3-way overlaped execution time for BLAS1 1-dim blocking.
size_t CoCoModel_optimize1(CoCoModel_p model);


CoCoModel_p CoCoModel_gemm_init(CoCoModel_p base_model, short dev_id, char* func, gemm_backend_in_p func_data);
CoCoModel_p CoCoModel_axpy_init(size_t N, short x_loc, short y_loc, short dev_id, char* func);

const char* printModel(ModelType mode);

tunableParams_p tunableParamsInit();

const char* printTunableParams(tunableParams_p params);

/// Each device gets 1/num_devices Subkernels without acounting for their size or location
void CoCoDistributeSubkernelsNaive(int* Subkernel_dev_id_list,
  int* Subkernels_per_dev, short num_devices, int MGridSz, int NGridSz, int KGridSz);

/// A classic round-robin distribution without acounting for their size or location
void CoCoDistributeSubkernelsRoundRobin(int* Subkernel_dev_id_list,
  int* Subkernels_per_dev, short num_devices, int MGridSz, int NGridSz, int KGridSz);

/// Each device gets 1/num_devices Subkernels without acounting for their size or location, but the last dimension is reversed in odd devices
void CoCoDistributeSubkernelsRevLast(int* Subkernel_dev_id_list,
	  int* Subkernels_per_dev, short num_devices, int MGridSz, int NGridSz, int KGridSz);

#endif