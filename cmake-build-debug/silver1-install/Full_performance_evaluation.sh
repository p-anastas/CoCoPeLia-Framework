#!/bin/bash
echo "Bash version ${BASH_VERSION}..."

PROJECT_INSTALL_DIR=/tmp/tmp.YXAEzsRmOZ/cmake-build-debug/silver1-install

BACKEND=
machine=silver1

LIBSC_DIR=$PROJECT_INSTALL_DIR/Benchmarking
LIBSC_TEST_LOG_DIR=$LIBSC_DIR/testLogs

mkdir -p "${LIBSC_TEST_LOG_DIR}/exec_logs"

alpha=1.2345
beta=1.1154

for FUNC in Dgemm Sgemm 
do
	perf_log="${LIBSC_TEST_LOG_DIR}/exec_logs/${FUNC}_perf_eval.log"
	rm $perf_log

	CoCopelia_run=$LIBSC_DIR/testing-bin/CoCoPeLia${FUNC}Runner
	cuBLASXt_run=$LIBSC_DIR/testing-bin/cuBLASXt${FUNC}Runner
	BLASX_run=$LIBSC_DIR/testing-bin/BLASx${FUNC}Runner
	BLASXEX_run=$LIBSC_DIR/testing-bin/BLASxEx${FUNC}Runner
	echo "Performing Benchmarks for ${FUNC} evaluation..."
	for TransA in N T;
	do
		for TransB in N T;
		do
			for A_loc in -1 0;
			do
				for B_loc in -1 1;
				do
					for C_loc in -1 0 1;
					do
						for Sq in {4096..35000..1024}
						do 
							echo "$CoCopelia_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
							$CoCopelia_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc  &>> $perf_log
							echo "$cuBLASXt_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
							$cuBLASXt_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc &>> $perf_log
							echo "$BLASX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
							$BLASX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc &>> $perf_log
							echo "$BLASXEX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
							$BLASXEX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $Sq $Sq $Sq $A_loc $B_loc $C_loc $C_loc &>> $perf_log
						done


					done 
				done
			done
			A_loc=-1
			B_loc=-1
			C_loc=-1
			for inbalanced in {4096..35000..1024}
			do 
				for ctr in 3 4 5 #2 3 4 5 6 7 8; # testbedI for 12000 can't do 7,8
				do 
					fat=$(($inbalanced*$ctr/2))
					double_thin=$(($inbalanced*4/$ctr/$ctr))

					echo "$CoCopelia_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$CoCopelia_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log
					echo "$cuBLASXt_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$cuBLASXt_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log
					echo "$BLASX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$BLASX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log
					echo "$BLASXEX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$BLASXEX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $fat $fat $double_thin $A_loc $B_loc $C_loc $C_loc &>> $perf_log
				done

				for ctr in 3 4 5 #2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
				do
					double_fat=$(($inbalanced*$ctr*$ctr/4))
					thin=$(($inbalanced*2/$ctr))

					echo "$CoCopelia_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$CoCopelia_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log
					echo "$cuBLASXt_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$cuBLASXt_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log
					echo "$BLASX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$BLASX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log
					echo "$BLASXEX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log"
					$BLASXEX_run -1 -1 -1 -1 $TransA $TransB $alpha $beta $thin $thin $double_fat $A_loc $B_loc $C_loc $C_loc &>> $perf_log
				done
			done
			echo "Done"
		done
	done
done





