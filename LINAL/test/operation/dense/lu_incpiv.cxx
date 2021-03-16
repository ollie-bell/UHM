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
#include "dense_test.hxx"

int main(int argc, char **argv) {


  linal::Flat_ A1, A2, B, X1, X2, p1, p2, norm;
  linal::Hier_ hA2, hp2;;

  if (argc != 2) {
    printf("Try :: lu_incpiv [thread]\n");
    return -1;
  }

  int datatype = TEST_DATATYPE;
  int nthread = atoi( (argv[1]) );

  // ---------------------------------------
  // ** Initialization  
  FLA_Init();

  // ---------------------------------------
  // ** Matrices
  A1.create   (datatype, N, N);
  A2.create   (datatype, N, N);
  hA2.create  (A2, BMN, BMN);

  B.create    (datatype, N, 1);

  X1.create   (datatype, N, 1);
  X2.create   (datatype, N, 1);

  p1.create   (LINAL_INT,  N, 1);
  p2.create   (LINAL_INT,  N, 1);
  hp2.create  (p2, BMN, BMN);

  norm.create(datatype, 1, 1);

  FLA_Random_matrix(~A1);
  FLA_Copy(~A1, ~A2);
  FLA_Random_matrix(~B);

  // ---------------------------------------
  // ** FLAME
  FLA_LU_piv(~A1,~p1);

  // ---------------------------------------
  // ** LINAL
  omp_set_num_threads( nthread );
#pragma omp parallel
  {
#pragma omp single nowait
    linal::dense::lu_incpiv( hA2, hp2 );
  }

  // ---------------------------------------
  // ** Check
  FLA_LU_piv_solve(~A1, ~p1, ~B, ~X1);
  FLA_LU_piv_solve(~A2, ~p2, ~B, ~X2);
 
  FLA_Axpy(FLA_MINUS_ONE, ~X1, ~X2);
  FLA_Norm1( ~X2, ~norm);

  float  norm_f = (float)norm(0,0);
  double norm_d = (double)norm(0,0);

  int rval;

  printf("- TEST::");
  for (int i=0;i<argc;++i)
    printf(" %s ", argv[i] );
  printf("\n");

  switch (datatype) {
  case LINAL_SINGLE_REAL:
    if (norm_f < LINAL_ERROR_TOL) {
      printf("PASS::Norm :: %E \n", norm_f);   rval = 0;
    } else {
      printf("FAIL::Norm :: %E \n", norm_f);   rval = -1;
    }
    break;
  case LINAL_DOUBLE_REAL:
    if (norm_d < LINAL_ERROR_TOL) {
      printf("PASS::Norm :: %E \n", norm_d);   rval = 0;
    } else {
      printf("FAIL::Norm :: %E \n", norm_d);   rval = -1;
    }
    break;
  }

  // ---------------------------------------
  // ** Matrices  
  A1.free();  A2.free();   hA2.free(); 
  p1.free();  p2.free();   hp2.free();
  B.free();

  X1.free();  X2.free(); 

  norm.free();

  // ---------------------------------------
  // ** Finalization
  FLA_Finalize();

  return rval;
}
