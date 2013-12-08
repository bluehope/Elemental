/*
   Copyright (c) 2009-2013, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef ELEM_LAPACK_NORM_TWOESTIMATE_HPP
#define ELEM_LAPACK_NORM_TWOESTIMATE_HPP

#include "elemental/blas-like/level2/Gemv.hpp"
#include "elemental/blas-like/level2/Hemv.hpp"
#include "elemental/blas-like/level2/Symv.hpp"
#include "elemental/lapack-like/Norm/Frobenius.hpp"
#include "elemental/matrices/Gaussian.hpp"

namespace elem {

template<typename F>
inline BASE(F)
TwoNormEstimate( const Matrix<F>& A, BASE(F) tol=1e-6, Int maxIts=1000 )
{
    DEBUG_ONLY(CallStackEntry cse("TwoNormEstimate"))
    typedef Base<F> Real;
    const Int m = A.Height();
    const Int n = A.Width();

    Matrix<F> x, y;
    Gaussian( y, n, 1 );
    
    Int numIts=0;
    Real estimate=0, lastEst;
    do
    {
        lastEst = estimate;
        Gemv( NORMAL, F(1), A, y, x );
        const Real xNorm = FrobeniusNorm( x );
        if( xNorm == Real(0) )
            Gaussian( x, m, 1 );    
        else
            Scale( Real(1)/xNorm, x );
        Gemv( ADJOINT, F(1), A, x, y );
        estimate = FrobeniusNorm( y );
    } while( ++numIts < maxIts && Abs(estimate-lastEst) > tol*Max(m,n) );

    if( Abs(estimate-lastEst) > tol*Max(m,n) )
        RuntimeError("Two-norm estimate did not converge in time");

    return estimate;
}

template<typename F>
inline BASE(F)
TwoNormEstimate( const DistMatrix<F>& A, BASE(F) tol=1e-6, Int maxIts=1000 )
{
    DEBUG_ONLY(CallStackEntry cse("TwoNormEstimate"))
    typedef Base<F> Real;
    const Grid& g = A.Grid();
    const Int m = A.Height();
    const Int n = A.Width();

    DistMatrix<F> x(g), y(g);
    Gaussian( y, n, 1 );
    
    Int numIts=0;
    Real estimate=0, lastEst;
    do
    {
        lastEst = estimate;
        Gemv( NORMAL, F(1), A, y, x );
        const Real xNorm = FrobeniusNorm( x );
        if( xNorm == Real(0) )
            Gaussian( x, m, 1 );    
        else
            Scale( Real(1)/xNorm, x );
        Gemv( ADJOINT, F(1), A, x, y );
        estimate = FrobeniusNorm( y );
    } while( ++numIts < maxIts && Abs(estimate-lastEst) > tol*Max(m,n) );

    if( Abs(estimate-lastEst) > tol*Max(m,n) )
        RuntimeError("Two-norm estimate did not converge in time");

    return estimate;
}

template<typename F>
inline BASE(F)
HermitianTwoNormEstimate
( UpperOrLower uplo, const Matrix<F>& A, BASE(F) tol=1e-6, Int maxIts=1000 )
{
    DEBUG_ONLY(CallStackEntry cse("HermitianTwoNormEstimate"))
    typedef Base<F> Real;
    const Int n = A.Height();

    Matrix<F> x, y;
    Zeros( x, n, 1 );
    Gaussian( y, n, 1 );
    
    Int numIts=0;
    Real estimate=0, lastEst;
    do
    {
        lastEst = estimate;
        Hemv( uplo, F(1), A, y, F(0), x );
        const Real xNorm = FrobeniusNorm( x );
        if( xNorm == Real(0) )
            Gaussian( x, n, 1 );    
        else
            Scale( Real(1)/xNorm, x );
        Hemv( uplo, F(1), A, x, F(0), y );
        estimate = FrobeniusNorm( y );
    } while( ++numIts < maxIts && Abs(estimate-lastEst) > tol*n );

    if( Abs(estimate-lastEst) > tol*n )
        RuntimeError("Two-norm estimate did not converge in time");

    return estimate;
}

template<typename F>
inline BASE(F)
HermitianTwoNormEstimate
( UpperOrLower uplo, const DistMatrix<F>& A, BASE(F) tol=1e-6, Int maxIts=1000 )
{
    DEBUG_ONLY(CallStackEntry cse("HermitianTwoNormEstimate"))
    typedef Base<F> Real;
    const Grid& g = A.Grid();
    const Int n = A.Height();

    DistMatrix<F> x(g), y(g);
    Zeros( x, n, 1 );
    Gaussian( y, n, 1 );
    
    Int numIts=0;
    Real estimate=0, lastEst;
    do
    {
        lastEst = estimate;
        Hemv( uplo, F(1), A, y, F(0), x );
        const Real xNorm = FrobeniusNorm( x );
        if( xNorm == Real(0) )
            Gaussian( x, n, 1 );    
        else
            Scale( Real(1)/xNorm, x );
        Hemv( uplo, F(1), A, x, F(0), y );
        estimate = FrobeniusNorm( y );
    } while( ++numIts < maxIts && Abs(estimate-lastEst) > tol*n );

    if( Abs(estimate-lastEst) > tol*n )
        RuntimeError("Two-norm estimate did not converge in time");

    return estimate;
}

template<typename F>
inline BASE(F)
SymmetricTwoNormEstimate
( UpperOrLower uplo, const Matrix<F>& A, BASE(F) tol=1e-6, Int maxIts=1000 )
{
    DEBUG_ONLY(CallStackEntry cse("SymmetricTwoNormEstimate"))
    typedef Base<F> Real;
    const Int n = A.Height();

    Matrix<F> x, y;
    Zeros( x, n, 1 );
    Gaussian( y, n, 1 );
    
    Int numIts=0;
    Real estimate=0, lastEst;
    do
    {
        lastEst = estimate;
        Symv( uplo, F(1), A, y, F(0), x );
        const Real xNorm = FrobeniusNorm( x );
        if( xNorm == Real(0) )
            Gaussian( x, n, 1 );    
        else
            Scale( Real(1)/xNorm, x );
        Conjugate( x );
        Symv( uplo, F(1), A, x, F(0), y );
        Conjugate( y );
        estimate = FrobeniusNorm( y );
    } while( ++numIts < maxIts && Abs(estimate-lastEst) > tol*n );

    if( Abs(estimate-lastEst) > tol*n )
        RuntimeError("Two-norm estimate did not converge in time");

    return estimate;
}

template<typename F>
inline BASE(F)
SymmetricTwoNormEstimate
( UpperOrLower uplo, const DistMatrix<F>& A, BASE(F) tol=1e-6, Int maxIts=1000 )
{
    DEBUG_ONLY(CallStackEntry cse("SymmetricTwoNormEstimate"))
    typedef Base<F> Real;
    const Grid& g = A.Grid();
    const Int n = A.Height();

    DistMatrix<F> x(g), y(g);
    Zeros( x, n, 1 );
    Gaussian( y, n, 1 );
    
    Int numIts=0;
    Real estimate=0, lastEst;
    do
    {
        lastEst = estimate;
        Symv( uplo, F(1), A, y, F(0), x );
        const Real xNorm = FrobeniusNorm( x );
        if( xNorm == Real(0) )
            Gaussian( x, n, 1 );    
        else
            Scale( Real(1)/xNorm, x );
        Conjugate( x );
        Symv( uplo, F(1), A, x, F(0), y );
        Conjugate( y );
        estimate = FrobeniusNorm( y );
    } while( ++numIts < maxIts && Abs(estimate-lastEst) > tol*n );

    if( Abs(estimate-lastEst) > tol*n )
        RuntimeError("Two-norm estimate did not converge in time");

    return estimate;
}

} // namespace elem

#endif // ifndef ELEM_LAPACK_NORM_TWOESTIMATE_HPP