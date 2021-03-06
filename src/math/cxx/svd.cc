/**
 * @file math/cxx/svd.cc
 * @date Sat Mar 19 22:14:10 2011 +0100
 * @author Laurent El Shafey <Laurent.El-Shafey@idiap.ch>
 *
 * Copyright (C) 2011-2013 Idiap Research Institute, Martigny, Switzerland
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bob/math/svd.h>
#include <bob/math/Exception.h>
#include <bob/core/assert.h>
#include <bob/core/check.h>
#include <bob/core/array_copy.h>
#include <boost/shared_array.hpp>

// Declaration of the external LAPACK function (Divide and conquer SVD)
extern "C" void dgesdd_( const char *jobz, const int *M, const int *N, 
  double *A, const int *lda, double *S, double *U, const int* ldu, double *VT,
  const int *ldvt, double *work, const int *lwork, int *iwork, int *info);

void bob::math::svd(const blitz::Array<double,2>& A, blitz::Array<double,2>& U,
  blitz::Array<double,1>& sigma, blitz::Array<double,2>& Vt)
{
  // Size variables
  const int M = A.extent(0);
  const int N = A.extent(1);
  const int nb_singular = std::min(M,N);

  // Checks zero base
  bob::core::array::assertZeroBase(A);
  bob::core::array::assertZeroBase(U);
  bob::core::array::assertZeroBase(sigma);
  bob::core::array::assertZeroBase(Vt);
  // Checks and resizes if required
  bob::core::array::assertSameDimensionLength(U.extent(0), M);
  bob::core::array::assertSameDimensionLength(U.extent(1), M);
  bob::core::array::assertSameDimensionLength(sigma.extent(0), nb_singular);
  bob::core::array::assertSameDimensionLength(Vt.extent(0), N);
  bob::core::array::assertSameDimensionLength(Vt.extent(1), N);

  bob::math::svd_(A, U, sigma, Vt);
}

void bob::math::svd_(const blitz::Array<double,2>& A, blitz::Array<double,2>& U,
  blitz::Array<double,1>& sigma, blitz::Array<double,2>& Vt)
{
  // Size variables
  const int M = A.extent(0);
  const int N = A.extent(1);
  const int nb_singular = std::min(M,N);

  // Prepares to call LAPACK function:
  // We will decompose A^T rather than A to reduce the required number of copy
  // We recall that FORTRAN/LAPACK is column-major order whereas blitz arrays 
  // are row-major order by default.
  // If A = U.S.V^T, then A^T = V.S.U^T

  // Initialises LAPACK variables
  const char jobz = 'A'; // Get All left singular vectors
  int info = 0;  
  const int lda = N;
  const int ldu = N;
  const int ldvt = M;
  // Integer (workspace) array, dimension (8*min(M,N))
  const int l_iwork = 8*std::min(M,N);
  boost::shared_array<int> iwork(new int[l_iwork]);
  // Initialises LAPACK arrays
  blitz::Array<double,2> A_blitz_lapack(bob::core::array::ccopy(A));
  double* A_lapack = A_blitz_lapack.data();
  // Tries to use U, Vt and S directly to limit the number of copy()
  // S_lapack = S
  blitz::Array<double,1> S_blitz_lapack;
  const bool sigma_direct_use = bob::core::array::isCZeroBaseContiguous(sigma);
  if (!sigma_direct_use) S_blitz_lapack.resize(nb_singular);
  else                   S_blitz_lapack.reference(sigma);
  double *S_lapack = S_blitz_lapack.data();
  // U_lapack = V^T
  blitz::Array<double,2> U_blitz_lapack;
  const bool U_direct_use = bob::core::array::isCZeroBaseContiguous(Vt);
  if (!U_direct_use) U_blitz_lapack.resize(N,N);
  else               U_blitz_lapack.reference(Vt);
  double *U_lapack = U_blitz_lapack.data();
  // V^T_lapack = U
  blitz::Array<double,2> VT_blitz_lapack;
  const bool VT_direct_use = bob::core::array::isCZeroBaseContiguous(U);
  if (!VT_direct_use) VT_blitz_lapack.resize(M,M);
  else                VT_blitz_lapack.reference(U);
  double *VT_lapack = VT_blitz_lapack.data();

  // Calls the LAPACK function:
  // We use dgesdd which is faster than its predecessor dgesvd, when
  // computing the singular vectors.
  //   (cf. http://www.netlib.org/lapack/lug/node71.html)
  // Please note that matlab is relying on dgesvd.

  // A/ Queries the optimal size of the working array
  const int lwork_query = -1;
  double work_query;
  dgesdd_( &jobz, &N, &M, A_lapack, &lda, S_lapack, U_lapack, &ldu, 
    VT_lapack, &ldvt, &work_query, &lwork_query, iwork.get(), &info );
  // B/ Computes
  const int lwork = static_cast<int>(work_query);
  boost::shared_array<double> work(new double[lwork]);
  dgesdd_( &jobz, &N, &M, A_lapack, &lda, S_lapack, U_lapack, &ldu, 
    VT_lapack, &ldvt, work.get(), &lwork, iwork.get(), &info );
 
  // Check info variable
  if (info != 0)
    throw bob::math::LapackError("The LAPACK dgesdd function returned a non-zero\
       value.");

  // Copy singular vectors back to U, V and sigma if required
  if (!U_direct_use)  Vt = U_blitz_lapack;
  if (!VT_direct_use) U = VT_blitz_lapack;
  if (!sigma_direct_use) sigma = S_blitz_lapack;
}


void bob::math::svd(const blitz::Array<double,2>& A, blitz::Array<double,2>& U,
  blitz::Array<double,1>& sigma)
{
  // Size variables
  const int M = A.extent(0);
  const int N = A.extent(1);
  const int nb_singular = std::min(M,N);

  // Checks zero base
  bob::core::array::assertZeroBase(A);
  bob::core::array::assertZeroBase(U);
  bob::core::array::assertZeroBase(sigma);
  // Checks and resizes if required
  bob::core::array::assertSameDimensionLength(U.extent(0), M);
  bob::core::array::assertSameDimensionLength(U.extent(1), nb_singular);
  bob::core::array::assertSameDimensionLength(sigma.extent(0), nb_singular);
 
  bob::math::svd_(A, U, sigma);
}

void bob::math::svd_(const blitz::Array<double,2>& A, blitz::Array<double,2>& U,
  blitz::Array<double,1>& sigma)
{
  // Size variables
  const int M = A.extent(0);
  const int N = A.extent(1);
  const int nb_singular = std::min(M,N);

  // Prepares to call LAPACK function

  // Initialises LAPACK variables
  const char jobz = 'S'; // Get first min(M,N) columns of U
  int info = 0;  
  const int lda = M;
  const int ldu = M;
  const int ldvt = std::min(M,N);

  // Integer (workspace) array, dimension (8*min(M,N))
  const int l_iwork = 8*std::min(M,N);
  boost::shared_array<int> iwork(new int[l_iwork]);
  // Initialises LAPACK arrays
  blitz::Array<double,2> A_blitz_lapack(bob::core::array::ccopy(const_cast<blitz::Array<double,2>&>(A).transpose(1,0)));
  double* A_lapack = A_blitz_lapack.data();
  // Tries to use U and S directly to limit the number of copy()
  // S_lapack = S
  blitz::Array<double,1> S_blitz_lapack;
  const bool sigma_direct_use = bob::core::array::isCZeroBaseContiguous(sigma);
  if (!sigma_direct_use) S_blitz_lapack.resize(nb_singular);
  else                   S_blitz_lapack.reference(sigma);
  double *S_lapack = S_blitz_lapack.data();
  // U_lapack = U^T
  blitz::Array<double,2> U_blitz_lapack;
  blitz::Array<double,2> Ut = U.transpose(1,0);
  const bool U_direct_use = bob::core::array::isCZeroBaseContiguous(Ut);
  if (!U_direct_use) U_blitz_lapack.resize(nb_singular,M);
  else               U_blitz_lapack.reference(Ut);
  double *U_lapack = U_blitz_lapack.data();
  boost::shared_array<double> VT_lapack(new double[nb_singular*N]);

  // Calls the LAPACK function:
  // We use dgesdd which is faster than its predecessor dgesvd, when
  // computing the singular vectors.
  //   (cf. http://www.netlib.org/lapack/lug/node71.html)
  // Please note that matlab is relying on dgesvd.

  // A/ Queries the optimal size of the working array
  const int lwork_query = -1;
  double work_query;
  dgesdd_( &jobz, &M, &N, A_lapack, &lda, S_lapack, U_lapack, &ldu, 
    VT_lapack.get(), &ldvt, &work_query, &lwork_query, iwork.get(), &info );
  // B/ Computes
  const int lwork = static_cast<int>(work_query);
  boost::shared_array<double> work(new double[lwork]);
  dgesdd_( &jobz, &M, &N, A_lapack, &lda, S_lapack, U_lapack, &ldu, 
    VT_lapack.get(), &ldvt, work.get(), &lwork, iwork.get(), &info );
 
  // Check info variable
  if (info != 0)
    throw bob::math::LapackError("The LAPACK dgesdd function returned a non-zero value.");
  
  // Copy singular vectors back to U, V and sigma if required
  if (!U_direct_use) Ut = U_blitz_lapack;
  if (!sigma_direct_use) sigma = S_blitz_lapack;
}


void bob::math::svd(const blitz::Array<double,2>& A, blitz::Array<double,1>& sigma)
{
  // Size variables
  const int M = A.extent(0);
  const int N = A.extent(1);
  const int nb_singular = std::min(M,N);

  // Checks zero base
  bob::core::array::assertZeroBase(A);
  bob::core::array::assertZeroBase(sigma);
  // Checks and resizes if required
  bob::core::array::assertSameDimensionLength(sigma.extent(0), nb_singular);
 
  bob::math::svd_(A, sigma);
}

void bob::math::svd_(const blitz::Array<double,2>& A, blitz::Array<double,1>& sigma)
{
  // Size variables
  const int M = A.extent(0);
  const int N = A.extent(1);
  const int nb_singular = std::min(M,N);

  // Prepares to call LAPACK function

  // Initialises LAPACK variables
  const char jobz = 'N'; // Get first min(M,N) columns of U
  int info = 0;  
  const int lda = M;
  const int ldu = M;
  const int ldvt = std::min(M,N);

  // Integer (workspace) array, dimension (8*min(M,N))
  const int l_iwork = 8*std::min(M,N);
  boost::shared_array<int> iwork(new int[l_iwork]);
  // Initialises LAPACK arrays
  blitz::Array<double,2> A_blitz_lapack(
    bob::core::array::ccopy(const_cast<blitz::Array<double,2>&>(A).transpose(1,0)));
  double* A_lapack = A_blitz_lapack.data();
  // Tries to use S directly to limit the number of copy()
  // S_lapack = S
  blitz::Array<double,1> S_blitz_lapack;
  const bool sigma_direct_use = bob::core::array::isCZeroBaseContiguous(sigma);
  if (!sigma_direct_use) S_blitz_lapack.resize(nb_singular);
  else                   S_blitz_lapack.reference(sigma);
  double *S_lapack = S_blitz_lapack.data();
  double *U_lapack = 0;
  double *VT_lapack = 0;

  // Calls the LAPACK function:
  // We use dgesdd which is faster than its predecessor dgesvd, when
  // computing the singular vectors.
  //   (cf. http://www.netlib.org/lapack/lug/node71.html)
  // Please note that matlab is relying on dgesvd.

  // A/ Queries the optimal size of the working array
  const int lwork_query = -1;
  double work_query;
  dgesdd_( &jobz, &M, &N, A_lapack, &lda, S_lapack, U_lapack, &ldu, 
    VT_lapack, &ldvt, &work_query, &lwork_query, iwork.get(), &info );
  // B/ Computes
  const int lwork = static_cast<int>(work_query);
  boost::shared_array<double> work(new double[lwork]);
  dgesdd_( &jobz, &M, &N, A_lapack, &lda, S_lapack, U_lapack, &ldu, 
    VT_lapack, &ldvt, work.get(), &lwork, iwork.get(), &info );
 
  // Check info variable
  if (info != 0)
    throw bob::math::LapackError("The LAPACK dgesdd function returned a non-zero value.");

  // Copy singular vectors back to U, V and sigma if required
  if (!sigma_direct_use) sigma = S_blitz_lapack;
}
