/*
  Copyright © 2011, Kyungjoo Kim
  All rights reserved.
  
  This file is part of LINAL.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  3. Neither the name of the owner nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#include "gpu_perform.hxx"

int main(int argc, char **argv) {

  if (argc != 5) {
    printf("Try :: qr [thread] [blocksize] [ndof] [nitr]\n");
    return 0;
  }

  // ---------------------------------------
  // ** Initialization   
  int ndof, blocksize, nthread, nitr;
  nthread   = atoi( (argv[1]) );
  blocksize = atoi( (argv[2]) );
  ndof      = atoi( (argv[3]) );
  nitr      = atoi( (argv[4]) );

  printf("** TEST ENVIRONMENT **\n");
  printf("NDOF      = %d\n", ndof);
  printf("Blocksize = %d\n", blocksize);
  printf("N thread  = %d\n", nthread);
  printf("Iteration = %d\n", nitr);

  int b_mn[2], t_ndof;
  b_mn[0] = b_mn[1] = blocksize;
  t_ndof = (ndof/blocksize+(ndof%blocksize!=0))*blocksize;

  FLA_Init();

  // ---------------------------------------
  // ** Environment setting
  double flop;
  flop = linal::get_flop_qr( 0, ndof, ndof );

  FLASH_Queue_set_num_threads(nthread);
  FLASH_Queue_set_sorting(TRUE);
  FLASH_Queue_set_caching(TRUE);
  omp_set_num_threads(nthread);

  // ---------------------------------------
  // ** Matrices
  double t_base, t_flash_repack, t_linal_repack, t_temp, 
    t_flash_decompose, t_linal_decompose;

  linal::Flat_ A, T;
  A.create(TEST_DATATYPE, ndof, ndof);
  T.create(TEST_DATATYPE, t_ndof, t_ndof);
  FLA_Random_matrix( ~A );

  FLA_Obj hA_fla, hT_fla;
  t_base = FLA_Clock();
  FLASH_QR_UT_inc_create_hier_matrices( ~A, 1, (dim_t*)b_mn, (dim_t)blocksize/2, &hA_fla, &hT_fla );
  t_flash_repack = FLA_Clock()-t_base;

  linal::Hier_ hA_linal, hT_linal;
  t_base = FLA_Clock();
  hA_linal.create(A, blocksize, blocksize);
  hT_linal.create(T, blocksize, blocksize);
  t_linal_repack = FLA_Clock()-t_base;

  // ---------------------------------------
  // ** FLASH
  t_flash_decompose = MAX_TIME;
  for (int q=0;q<nitr;++q) {
    printf("*** FLASH::QR BEGIN ***\n");
    t_base = FLA_Clock();
    FLASH_Queue_begin();
    FLASH_QR_UT_inc(hA_fla,hT_fla);
    FLASH_Queue_end();
    t_temp = FLA_Clock()-t_base;
    printf("*** FLASH::QR END ***\n");

    t_flash_decompose = min(t_temp, t_flash_decompose);
  }
  
  // ---------------------------------------
  // ** LINAL
  t_linal_decompose = MAX_TIME;
  for (int q=0;q<nitr;++q) {
    printf("*** LINAL::QR BEGIN ***\n");

    t_base = FLA_Clock();
#pragma omp parallel 
    {
#pragma omp single nowait
      linal::dense::qr_inc(hA_linal, hT_linal);
    }
    t_temp = FLA_Clock()-t_base;
    printf("*** LINAL::QR END ***\n");

    t_linal_decompose = min(t_temp, t_linal_decompose);
  }

  printf("----------------------------------------------\n");
  printf("*** Report QR nopiv ***\n");
  printf("Ndof       = %d\n", ndof);
  printf("Blocksize  = %d\n", blocksize);
  printf("Nthread    = %d\n", nthread);
  printf("Niteration = %d\n", nitr);
  printf("----------------------------------------------\n");
  printf("Time Supermatrix   = %6.3lf [sec]\n", t_flash_decompose);
  printf("Time LINAL         = %6.3lf [sec]\n", t_linal_decompose);
  printf("----------------------------------------------\n");
  printf("FLOPS Supermatrix  = %6.3lf [Gflops]\n", 
	 flop/t_flash_decompose/1.0e9);
  printf("FLOPS LINAL        = %6.3lf [Gflops]\n", 
	 flop/t_linal_decompose/1.0e9);
  printf("----------------------------------------------\n");
  printf("With repacking cost \n");
  printf("FLOPS Supermatrix  = %6.3lf [Gflops]\n", 
	 flop/(t_flash_decompose+t_flash_repack)/1.0e9);
  printf("FLOPS LINAL        = %6.3lf [Gflops]\n", 
	 flop/(t_linal_decompose+t_linal_repack)/1.0e9);
  printf("----------------------------------------------\n");
  
  // ---------------------------------------
  // ** Matrices
  FLASH_Obj_free(&hA_fla);
  FLASH_Obj_free(&hT_fla);
  hA_linal.free();
  hT_linal.free();
  A.free();
  T.free();

  printf("*** TEST FINISHED ***\n");

  // ---------------------------------------
  // ** Finalization
  FLA_Finalize();
  return 0;
}
