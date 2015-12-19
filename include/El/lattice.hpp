/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_LATTICE_HPP
#define EL_LATTICE_HPP

namespace El {

// Lenstra-Lenstra-Lovasz (LLL) lattice reduction
// ==============================================
// A reduced basis, say D, is an LLL(delta) reduction of an m x n matrix B if
//
//    B U = D = Q R,
//
// where U is unimodular (integer-valued with absolute determinant of 1)
// and Q R is a floating-point QR factorization of D that satisfies the three
//  properties:
//
//   1. R has non-negative diagonal
//
//   2. R is size-reduced:
//
//        | R(i,j) / R(i,i) | < phi(F),  for all i < j, and
//
//      where phi(F) is 1/2 for a real field F, and sqrt(2)/2 for a complex
//      field F, and
//
//   3. R is (delta-)Lovasz reduced:
//
//        delta R(i,i)^2 <= R(i+1,i+1)^2 + |R(i,i+1)|^2,  for all i.
//
// Please see
//
//   Henri Cohen, "A course in computational algebraic number theory"
// 
// for more information on the "MLLL" variant of LLL used by Elemental to 
// handle linearly dependent vectors (the algorithm was originally suggested by
// Mike Pohst).
//

struct LLLInfo
{
    Int nullity; 
    Int numSwaps;
};

template<typename Real>
struct LLLCtrl
{
    Real delta=Real(3)/Real(4);

    // A 'weak' LLL reduction only ensures that | R(i,i+1) / R(i,i) | is
    // bounded above by one-half (for complex data, by sqrt(2)/2)
    bool weak=false;

    // Preprocessing with a "rank-obscuring" column-pivoted QR factorization
    // (in the manner suggested by Wubben et al.) tends to greatly decrease
    // the number of swaps within LLL
    bool presort=true;
    bool smallestFirst=true;

    // If the size-reduced column has a squared two-norm that is less than or
    // equal to `reorthogTol` times the square of its original two-norm, then
    // it is reorthogonalized
    Real reorthogTol=0;

    // If a size-reduced column has a two-norm less than or equal to 'zeroTol',
    // then it is interpreted as a zero vector (and forced to zero)
    Real zeroTol=limits::Epsilon<Real>();

    bool progress=false;
    bool time=false;
};

// TODO: Maintain B in BigInt form

template<typename F>
LLLInfo LLL
( Matrix<F>& B,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// TODO: Also return Q?
template<typename F>
LLLInfo LLL
( Matrix<F>& B,
  Matrix<F>& R,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// TODO: Also return Q?
template<typename F>
LLLInfo LLL
( Matrix<F>& B,
  Matrix<F>& U,
  Matrix<F>& UInv,
  Matrix<F>& R,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

template<typename F>
Base<F> LLLDelta
( const Matrix<F>& R,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// Overwrite B, fill M with its (quasi-reduced) image of B, and fill K with the
// LLL-reduced basis for the kernel of B.
//
// This is essentially Algorithm 2.7.1 from Cohen's
// "A course in computational algebraic number theory". The main difference
// is that we avoid solving the normal equations and call a least squares
// solver.
// 
template<typename F>
void LatticeImageAndKernel
( Matrix<F>& B,
  Matrix<F>& M,
  Matrix<F>& K,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

// Overwrite B and fill K with the LLL-reduced basis for the kernel of B.
// This will eventually mirror Algorithm 2.7.2 from Cohen's
// "A course in computational algebraic number theory".
template<typename F>
void LatticeKernel
( Matrix<F>& B,
  Matrix<F>& K,
  const LLLCtrl<Base<F>>& ctrl=LLLCtrl<Base<F>>() );

} // namespace El

#endif // ifndef EL_LATTICE_HPP
