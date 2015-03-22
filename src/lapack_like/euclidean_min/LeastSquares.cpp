/*
   Copyright (c) 2009-2015, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "El.hpp"

namespace El {

template<typename F> 
void LeastSquares
( Orientation orientation, 
  Matrix<F>& A, const Matrix<F>& B, 
                      Matrix<F>& X )
{
    DEBUG_ONLY(CallStackEntry cse("LeastSquares"))

    Matrix<F> t;
    Matrix<Base<F>> d;

    const Int m = A.Height();
    const Int n = A.Width();
    if( m >= n )
    {
        QR( A, t, d );
        qr::SolveAfter( orientation, A, t, d, B, X );
    }
    else
    {
        LQ( A, t, d );
        lq::SolveAfter( orientation, A, t, d, B, X );
    }
}

template<typename F> 
void LeastSquares
( Orientation orientation, 
  AbstractDistMatrix<F>& APre, const AbstractDistMatrix<F>& B, 
                                     AbstractDistMatrix<F>& X )
{
    DEBUG_ONLY(CallStackEntry cse("LeastSquares"))

    auto APtr = ReadProxy<F,MC,MR>( &APre );
    auto& A = *APtr;

    DistMatrix<F,MD,STAR> t(A.Grid());
    DistMatrix<Base<F>,MD,STAR> d(A.Grid());

    const Int m = A.Height();
    const Int n = A.Width();
    if( m >= n )
    {
        QR( A, t, d );
        qr::SolveAfter( orientation, A, t, d, B, X );
    }
    else
    {
        LQ( A, t, d );
        lq::SolveAfter( orientation, A, t, d, B, X );
    }
}

// The following routines solve either
//
//   Minimum length: 
//     min_X || X ||_F 
//     s.t. W X = B, or
//
//   Least squares:  
//     min_X || W X - B ||_F,
//
// where W=op(A) is either A, A^T, or A^H, via forming a Hermitian 
// quasi-semidefinite system 
//
//    | alpha*I  W | | R/alpha | = | B |,
//    |   W^H    0 | | X       |   | 0 |
//
// when height(W) >= width(W), or
//
//    | alpha*I  W^H | | X | = | 0 |,
//    |   W       0  | | Y |   | B |
//
// when height(W) < width(W).
//  
// The latter guarantees that W X = B and X in range(W^H), which shows that 
// X solves the minimum length problem. The former defines R = B - W X and 
// ensures that R is in the null-space of W^H (therefore solving the least 
// squares problem).
// 
// Note that, ideally, alpha is roughly the minimum (nonzero) singular value
// of W, which implies that the condition number of the quasi-semidefinite 
// system is roughly equal to the condition number of W (see the analysis of 
// Bjorck). If it is too expensive to estimate the minimum singular value, and 
// W is equilibrated to have a unit two-norm, a typical choice for alpha is 
// epsilon^0.25.
//
// The Hermitian quasi-semidefinite systems are solved by converting them into
// Hermitian quasi-definite form via a priori regularization, applying an 
// LDL^H factorization with static pivoting to the regularized system, and
// using the iteratively-refined solution of with the regularized factorization
// as a preconditioner for the original problem (defaulting to Flexible GMRES
// for now).
//
// This approach originated within 
//
//    Michael Saunders, 
//   "Chapter 8, Cholesky-based Methods for Sparse Least Squares:
//    The Benefits of Regularization",
//    in L. Adams and J.L. Nazareth (eds.), Linear and Nonlinear Conjugate
//    Gradient-Related Methods, SIAM, Philadelphia, 92--100 (1996).
//
// But note that SymmLQ and LSQR were used rather than flexible GMRES, and 
// iteratively refining *within* the preconditioner was not discussed.
//

// NOTE: The following routines are implemented as a special case of Tikhonov
//       regularization with either an m x 0 or 0 x n regularization matrix.

namespace ls {

template<typename F>
inline void Equilibrated
( const SparseMatrix<F>& A,  const Matrix<F>& B, 
                                   Matrix<F>& X,
  const Matrix<Base<F>>& dR, const Matrix<Base<F>>& dC,
  const LeastSquaresCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(
      CallStackEntry cse("ls::Equilibrated");
      if( A.Height() != B.Height() )
          LogicError("Heights of A and B must match");
      if( A.Width() != B.Height() )
          LogicError("Width of A and height of B must match");
    )
    typedef Base<F> Real;

    const Int m = A.Height();
    const Int n = A.Width();
    const Int k = B.Width();
    const Int numEntriesA = A.NumEntries();

    SparseMatrix<F> J, JOrig;
    Zeros( J, m+n, m+n );
    J.Reserve( 2*numEntriesA + Max(m,n) );
    if( m >= n )
    {
        // Form J = [D_r^{-2}*alpha, A; A^H, 0]
        // ====================================
        for( Int e=0; e<numEntriesA; ++e )
        {
            J.QueueUpdate( A.Row(e),   A.Col(e)+m,      A.Value(e)  );
            J.QueueUpdate( A.Col(e)+m, A.Row(e),   Conj(A.Value(e)) );
        }
        for( Int e=0; e<m; ++e )
            J.QueueUpdate( e, e, Pow(dR.Get(e,0),Real(-2))*ctrl.alpha );
    }
    else
    {
        // Form J = [D_c^{-2}*alpha, A^H; A, 0]
        // ====================================
        for( Int e=0; e<numEntriesA; ++e )
        {
            J.QueueUpdate( A.Col(e),   A.Row(e)+n, Conj(A.Value(e)) );
            J.QueueUpdate( A.Row(e)+n, A.Col(e),        A.Value(e)  );
        }
        for( Int e=0; e<n; ++e )
            J.QueueUpdate( e, e, Pow(dC.Get(e,0),Real(-2))*ctrl.alpha );
    }
    J.MakeConsistent();

    Matrix<F> D;
    Zeros( D, m+n, k );
    if( m >= n )
    {
        // Form D = [B; 0]
        // ==================
        auto DT = D( IR(0,m), IR(0,k) );
        DT = B;
    }
    else
    {
        // Form D = [0; B]
        // ===============
        auto DB = D( IR(n,m+n), IR(0,k) );
        DB = B;
    }

    // Compute the regularized quasi-semidefinite fact of J
    // ====================================================
    Matrix<Real> reg;
    reg.Resize( m+n, 1 );
    for( Int i=0; i<Max(m,n); ++i )
        reg.Set( i, 0, ctrl.qsdCtrl.regPrimal );
    for( Int i=Max(m,n); i<m+n; ++i )
        reg.Set( i, 0, -ctrl.qsdCtrl.regDual );
    JOrig = J;
    UpdateRealPartOfDiagonal( J, Real(1), reg );

    vector<Int> map, invMap;
    SymmNodeInfo info;
    Separator rootSep;
    NestedDissection( J.LockedGraph(), map, rootSep, info );
    InvertMap( map, invMap );
    SymmFront<F> JFront( J, map, info );
    LDL( info, JFront );

    // Successively solve each of the k linear systems
    // ===============================================
    // TODO: Extend the iterative refinement to handle multiple RHS
    Matrix<F> u;
    Zeros( u, m+n, 1 );
    for( Int j=0; j<k; ++j )
    {
        auto d = D( IR(0,m+n), IR(j,j+1) );
        u = d;
        reg_qsd_ldl::SolveAfter
        ( JOrig, reg, invMap, info, JFront, u, ctrl.qsdCtrl );
        d = u;
    }

    Zeros( X, n, k );
    if( m >= n )
    {
        // Extract X from [R/alpha; X]
        // ===========================
        auto DB = D( IR(m,m+n), IR(0,k) );
        X = DB;
    }
    else
    {
        // Extract X from [X; Y]
        // =====================
        auto DT = D( IR(0,n), IR(0,k) );
        X = DT;
    }
}

} // namespace ls

template<typename F>
void LeastSquares
( Orientation orientation,
  const SparseMatrix<F>& A, const Matrix<F>& B, 
                                  Matrix<F>& X,
  const LeastSquaresCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(CallStackEntry cse("LeastSquares"))
    typedef Base<F> Real;

    SparseMatrix<F> ABar;
    if( orientation == NORMAL )
        ABar = A;
    else if( orientation == TRANSPOSE )
        Transpose( A, ABar );
    else
        Adjoint( A, ABar );
    auto BBar = B;
    const Int m = ABar.Height();
    const Int n = ABar.Width();

    // Equilibrate the least squares problem
    // =====================================
    Matrix<Real> dR, dC;
    if( ctrl.equilibrate )
    {
        GeomEquil( ABar, dR, dC, ctrl.progress );
        DiagonalSolve( LEFT, NORMAL, dR, BBar );
    }
    else
    {
        Ones( dR, m, 1 );
        Ones( dC, n, 1 );
    }

    // Solve the equilibrated least squares problem
    // ============================================
    ls::Equilibrated( ABar, BBar, X, dR, dC, ctrl );

    // Unequilibrate the solution
    // ==========================
    if( ctrl.equilibrate )
        DiagonalSolve( LEFT, NORMAL, dC, X );
}

namespace ls {

template<typename F>
void Equilibrated
( const DistSparseMatrix<F>& A,    const DistMultiVec<F>& B, 
                                         DistMultiVec<F>& X,
  const DistMultiVec<Base<F>>& dR, const DistMultiVec<Base<F>>& dC,
  const LeastSquaresCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(
      CallStackEntry cse("ls::Equilibrated");
      if( A.Height() != B.Height() )
          LogicError("Heights of A and B must match");
      if( A.Width() != B.Height() )
          LogicError("Width of A and height of B must match");
    )
    typedef Base<F> Real;
    mpi::Comm comm = A.Comm();
    const int commSize = mpi::Size(comm);
    const int commRank = mpi::Rank(comm);
    Timer timer;

    const Int m = A.Height();
    const Int n = A.Width();
    const Int k = B.Width();

    // J := [D_r^{-2}*alpha,A;A^H,0] or [D_c^{-2}*alpha,A^H;A,0]
    // =========================================================
    DistSparseMatrix<F> J(comm), JOrig(comm);
    Zeros( J, m+n, m+n );
    const Int numLocalEntriesA = A.NumLocalEntries();
    {
        // Compute metadata
        // ----------------
        vector<int> sendCounts(commSize,0);
        for( Int e=0; e<numLocalEntriesA; ++e )
        {
            const Int i = A.Row(e);
            const Int j = A.Col(e);
            if( m >= n )
            {
                // Sending A
                ++sendCounts[ J.RowOwner(i) ];
                // Sending A^H
                ++sendCounts[ J.RowOwner(j+m) ];
            }
            else
            {
                // Sending A
                ++sendCounts[ J.RowOwner(i+n) ];
                // Sending A^H
                ++sendCounts[ J.RowOwner(j) ];
            }
        }
        if( m >= n )
        {
            for( Int iLoc=0; iLoc<dR.LocalHeight(); ++iLoc )
                ++sendCounts[ J.RowOwner(dR.GlobalRow(iLoc)) ];
        }
        else
        {
            for( Int iLoc=0; iLoc<dC.LocalHeight(); ++iLoc )
                ++sendCounts[ J.RowOwner(dC.GlobalRow(iLoc)) ];
        }
        vector<int> recvCounts(commSize);
        mpi::AllToAll( sendCounts.data(), 1, recvCounts.data(), 1, comm );
        vector<int> sendOffs, recvOffs;
        const int totalSend = Scan( sendCounts, sendOffs );
        const int totalRecv = Scan( recvCounts, recvOffs );
        // Pack
        // ----
        vector<ValueIntPair<F>> sendBuf(totalSend);
        auto offs = sendOffs;
        for( Int e=0; e<numLocalEntriesA; ++e )
        {
            const Int i = A.Row(e);
            const Int j = A.Col(e);
            const F value = A.Value(e);

            if( m >= n )
            {
                // Sending A
                int owner = J.RowOwner(i);
                sendBuf[offs[owner]].indices[0] = i;
                sendBuf[offs[owner]].indices[1] = j+m;
                sendBuf[offs[owner]].value = value;
                ++offs[owner];

                // Sending A^H
                owner = J.RowOwner(j+m);
                sendBuf[offs[owner]].indices[0] = j+m;
                sendBuf[offs[owner]].indices[1] = i;
                sendBuf[offs[owner]].value = Conj(value);
                ++offs[owner];
            }
            else
            {
                // Sending A
                int owner = J.RowOwner(i+n);
                sendBuf[offs[owner]].indices[0] = i+n;
                sendBuf[offs[owner]].indices[1] = j;
                sendBuf[offs[owner]].value = value;
                ++offs[owner];

                // Sending A^H
                owner = J.RowOwner(j);
                sendBuf[offs[owner]].indices[0] = j;
                sendBuf[offs[owner]].indices[1] = i+n;
                sendBuf[offs[owner]].value = Conj(value);
                ++offs[owner];
            }
        }
        if( m >= n )
        {
            for( Int iLoc=0; iLoc<dR.LocalHeight(); ++iLoc )
            {
                const Int i = dR.GlobalRow(iLoc);
                const int owner = J.RowOwner(i);
                sendBuf[offs[owner]].indices[0] = i; 
                sendBuf[offs[owner]].indices[1] = i;
                sendBuf[offs[owner]].value = 
                    Pow(dR.GetLocal(iLoc,0),Real(-2))*ctrl.alpha;
                ++offs[owner];
            }
        }
        else
        {
            for( Int iLoc=0; iLoc<dC.LocalHeight(); ++iLoc )
            {
                const Int i = dC.GlobalRow(iLoc);
                const int owner = J.RowOwner(i);
                sendBuf[offs[owner]].indices[0] = i; 
                sendBuf[offs[owner]].indices[1] = i;
                sendBuf[offs[owner]].value = 
                    Pow(dC.GetLocal(iLoc,0),Real(-2))*ctrl.alpha;
                ++offs[owner];
            }
        }

        // Exchange
        // --------
        vector<ValueIntPair<F>> recvBuf(totalRecv);
        mpi::AllToAll
        ( sendBuf.data(), sendCounts.data(), sendOffs.data(),
          recvBuf.data(), recvCounts.data(), recvOffs.data(), comm );
        // Unpack
        // ------
        J.Reserve( totalRecv );
        for( Int e=0; e<totalRecv; ++e )
            J.QueueLocalUpdate
            ( recvBuf[e].indices[0]-J.FirstLocalRow(), recvBuf[e].indices[1], 
              recvBuf[e].value );
        J.MakeConsistent();
    }

    // Set D to [B; 0] or [0; B]
    // =========================
    DistMultiVec<F> D(comm);
    Zeros( D, m+n, k );
    {
        // Compute metadata
        // ----------------
        vector<int> sendCounts(commSize,0);
        for( Int iLoc=0; iLoc<B.LocalHeight(); ++iLoc )
        {
            const Int i = B.GlobalRow(iLoc);
            if( m >= n )
                sendCounts[ D.RowOwner(i) ] += k;
            else
                sendCounts[ D.RowOwner(i+n) ] += k;
        }
        vector<int> recvCounts(commSize);
        mpi::AllToAll( sendCounts.data(), 1, recvCounts.data(), 1, comm );
        vector<int> sendOffs, recvOffs;
        const int totalSend = Scan( sendCounts, sendOffs );
        const int totalRecv = Scan( recvCounts, recvOffs );
        // Pack
        // ----
        vector<ValueIntPair<F>> sendBuf(totalSend);
        auto offs = sendOffs;
        for( Int iLoc=0; iLoc<B.LocalHeight(); ++iLoc )
        {
            const Int i = B.GlobalRow(iLoc);
            if( m >= n )
            {
                int owner = D.RowOwner(i);
                for( Int j=0; j<k; ++j )
                {
                    const F value = B.GetLocal(iLoc,j);
                    sendBuf[offs[owner]].indices[0] = i;
                    sendBuf[offs[owner]].indices[1] = j;
                    sendBuf[offs[owner]].value = value;
                    ++offs[owner];
                }
            }
            else
            {
                int owner = D.RowOwner(i+n);
                for( Int j=0; j<k; ++j )
                {
                    const F value = B.GetLocal(iLoc,j);
                    sendBuf[offs[owner]].indices[0] = i+n;
                    sendBuf[offs[owner]].indices[1] = j;
                    sendBuf[offs[owner]].value = value;
                    ++offs[owner];
                }
            }
        }
        // Exchange
        // --------
        vector<ValueIntPair<F>> recvBuf(totalRecv);
        mpi::AllToAll
        ( sendBuf.data(), sendCounts.data(), sendOffs.data(),
          recvBuf.data(), recvCounts.data(), recvOffs.data(), comm );
        // Unpack
        // ------
        for( Int e=0; e<totalRecv; ++e )
            D.UpdateLocal
            ( recvBuf[e].indices[0]-D.FirstLocalRow(), recvBuf[e].indices[1], 
              recvBuf[e].value );
    }

    // Compute the regularized quasi-semidefinite fact of J
    // ====================================================
    DistMultiVec<Real> reg(comm);
    reg.Resize( m+n, 1 );
    for( Int iLoc=0; iLoc<reg.LocalHeight(); ++iLoc )
    {
        const Int i = reg.GlobalRow(iLoc);
        if( i < Max(m,n) )
            reg.SetLocal( iLoc, 0, ctrl.qsdCtrl.regPrimal );
        else
            reg.SetLocal( iLoc, 0, -ctrl.qsdCtrl.regDual );
    }
    JOrig = J;
    UpdateRealPartOfDiagonal( J, Real(1), reg );

    DistMap map, invMap;
    DistSymmNodeInfo info;
    DistSeparator rootSep;
    if( commRank == 0 && ctrl.time )
        timer.Start();
    NestedDissection( J.LockedDistGraph(), map, rootSep, info );
    if( commRank == 0 && ctrl.time )
        cout << "  ND: " << timer.Stop() << " secs" << endl;
    InvertMap( map, invMap );
    DistSymmFront<F> JFront( J, map, rootSep, info );

    if( commRank == 0 && ctrl.time )
        timer.Start();
    LDL( info, JFront, LDL_2D );
    if( commRank == 0 && ctrl.time )
        cout << "  LDL: " << timer.Stop() << " secs" << endl;

    // Successively solve each of the k linear systems
    // ===============================================
    // TODO: Extend the iterative refinement to handle multiple right-hand sides
    DistMultiVec<F> u(comm);
    Zeros( u, m+n, 1 );
    auto& DLoc = D.Matrix();
    auto& uLoc = u.Matrix();
    const Int DLocHeight = DLoc.Height();
    if( commRank == 0 && ctrl.time )
        timer.Start();
    for( Int j=0; j<k; ++j )
    {
        auto dLoc = DLoc( IR(0,DLocHeight), IR(j,j+1) );
        Copy( dLoc, uLoc );
        reg_qsd_ldl::SolveAfter
        ( JOrig, reg, invMap, info, JFront, u, ctrl.qsdCtrl );
        Copy( uLoc, dLoc );
    }
    if( commRank == 0 && ctrl.time )
        cout << "  Solve: " << timer.Stop() << " secs" << endl;

    // Extract X from [R/alpha; X] or [X; Y] and then rescale
    // ======================================================
    Zeros( X, n, k );
    {
        // Compute metadata
        // ----------------
        vector<int> sendCounts(commSize,0);
        for( Int iLoc=0; iLoc<DLocHeight; ++iLoc )
        {
            const Int i = D.GlobalRow(iLoc);
            if( m > n )
            {
                if( i >= m )
                    sendCounts[ X.RowOwner(i-m) ] += k;
            }
            else
            {
                if( i < n )
                    sendCounts[ X.RowOwner(i) ] += k;
                else
                    break;
            }
        }
        vector<int> recvCounts(commSize);
        mpi::AllToAll( sendCounts.data(), 1, recvCounts.data(), 1, comm );
        vector<int> sendOffs, recvOffs;
        const int totalSend = Scan( sendCounts, sendOffs );
        const int totalRecv = Scan( recvCounts, recvOffs );
        // Pack
        // ----
        vector<ValueIntPair<F>> sendBuf(totalSend);
        auto offs = sendOffs;
        for( Int iLoc=0; iLoc<DLocHeight; ++iLoc )
        {
            const Int i = D.GlobalRow(iLoc);
            if( m >= n )
            {
                if( i >= m )
                {
                    int owner = X.RowOwner(i-m);
                    for( Int j=0; j<k; ++j )
                    {
                        const F value = D.GetLocal(iLoc,j);
                        sendBuf[offs[owner]].indices[0] = i-m;
                        sendBuf[offs[owner]].indices[1] = j;
                        sendBuf[offs[owner]].value = value;
                        ++offs[owner];
                    }
                }
            }
            else
            {
                if( i < n )
                {
                    int owner = X.RowOwner(i);
                    for( Int j=0; j<k; ++j )
                    {
                        const F value = D.GetLocal(iLoc,j);
                        sendBuf[offs[owner]].indices[0] = i;
                        sendBuf[offs[owner]].indices[1] = j;
                        sendBuf[offs[owner]].value = value;
                        ++offs[owner];
                    }
                }
                else
                    break;
            }
        }
        // Exchange
        // --------
        vector<ValueIntPair<F>> recvBuf(totalRecv);
        mpi::AllToAll
        ( sendBuf.data(), sendCounts.data(), sendOffs.data(),
          recvBuf.data(), recvCounts.data(), recvOffs.data(), comm );
        // Unpack
        // ------
        for( Int e=0; e<totalRecv; ++e )
            X.SetLocal
            ( recvBuf[e].indices[0]-X.FirstLocalRow(), recvBuf[e].indices[1], 
              recvBuf[e].value );
    }
}

} // namespace ls

template<typename F>
void LeastSquares
( Orientation orientation,
  const DistSparseMatrix<F>& A, const DistMultiVec<F>& B, 
                                      DistMultiVec<F>& X,
  const LeastSquaresCtrl<Base<F>>& ctrl )
{
    DEBUG_ONLY(
      CallStackEntry cse("LeastSquares");
      if( orientation == NORMAL && A.Height() != B.Height() )
          LogicError("Heights of A and B must match");
      if( orientation != NORMAL && A.Width() != B.Height() )
          LogicError("Width of A and height of B must match");
    )
    typedef Base<F> Real;
    mpi::Comm comm = A.Comm();
    const int commRank = mpi::Rank(comm);
    Timer timer;

    DistSparseMatrix<F> ABar(comm);
    if( orientation == NORMAL )
        ABar = A;
    else if( orientation == TRANSPOSE )
        Transpose( A, ABar );
    else
        Adjoint( A, ABar );
    auto BBar = B;
    const Int m = ABar.Height();
    const Int n = ABar.Width();

    // Equilibrate the problem
    // =======================
    DistMultiVec<Real> dR(comm), dC(comm);
    if( ctrl.equilibrate )
    {
        if( commRank == 0 && ctrl.time )
            timer.Start();
        GeomEquil( ABar, dR, dC, ctrl.progress );
        if( commRank == 0 && ctrl.time )
            cout << "  GeomEquil: " << timer.Stop() << " secs" << endl;
        DiagonalSolve( LEFT, NORMAL, dR, BBar );
    }
    else
    {
        Ones( dR, m, 1 );
        Ones( dC, n, 1 );
    }

    // Solve the equilibrated problem
    // ==============================
    ls::Equilibrated( ABar, BBar, X, dR, dC, ctrl );

    // Unequilibrate the solution
    // ==========================
    if( ctrl.equilibrate )
        DiagonalSolve( LEFT, NORMAL, dC, X );
}

#define PROTO(F) \
  template void LeastSquares \
  ( Orientation orientation, Matrix<F>& A, const Matrix<F>& B, \
    Matrix<F>& X ); \
  template void LeastSquares \
  ( Orientation orientation, AbstractDistMatrix<F>& A, \
    const AbstractDistMatrix<F>& B, AbstractDistMatrix<F>& X ); \
  template void LeastSquares \
  ( Orientation orientation, \
    const SparseMatrix<F>& A, const Matrix<F>& B, \
    Matrix<F>& X, const LeastSquaresCtrl<Base<F>>& ctrl ); \
  template void LeastSquares \
  ( Orientation orientation, \
    const DistSparseMatrix<F>& A, const DistMultiVec<F>& B, \
    DistMultiVec<F>& X, const LeastSquaresCtrl<Base<F>>& ctrl );

#define EL_NO_INT_PROTO
#include "El/macros/Instantiate.h"

} // namespace El