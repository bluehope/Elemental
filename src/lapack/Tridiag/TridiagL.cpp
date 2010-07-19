/*
   Copyright (C) 2009-2010 Jack Poulson <jack.poulson@gmail.com>

   This file is part of Elemental.

   Elemental is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Elemental is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Elemental.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "elemental/blas_internal.hpp"
#include "elemental/lapack_internal.hpp"
using namespace std;
using namespace elemental;

template<typename R>
void
elemental::lapack::internal::TridiagL
( DistMatrix<R,MC,MR>& A )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::TridiagL");
    if( A.Height() != A.Width() )
        throw logic_error( "A must be square." );
#endif
    const Grid& g = A.GetGrid();

    // Matrix views 
    DistMatrix<R,MC,MR> 
        ATL(g), ATR(g),  A00(g), A01(g), A02(g), 
        ABL(g), ABR(g),  A10(g), A11(g), A12(g),
                         A20(g), A21(g), A22(g),
        A11Expanded(g);

    // Temporary distributions
    DistMatrix<R,Star,Star> A11_Star_Star(g);
    DistMatrix<R,MD,  Star> e1(g);
    DistMatrix<R,MC,  MR  > W11(g),  WPan(g),
                            W21(g);

    PartitionDownDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    while( ATL.Height() < A.Height() )
    {
        RepartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, /**/ A01, A02,
         /*************/ /******************/
               /**/       A10, /**/ A11, A12,
          ABL, /**/ ABR,  A20, /**/ A21, A22 );

        if( A22.Height() > 0 )
        {
            A11Expanded.View( ABR, 0, 0, A11.Height()+1, A11.Width()+1 );
            WPan.AlignWith( A11 );
            WPan.ResizeTo( ABR.Height(), A11.Width() );
            PartitionDown
            ( WPan, W11,
                    W21, A11.Height() );
            e1.AlignWithDiag( ABR, -1 );
            e1.ResizeTo( WPan.Width(), 1 );
            //----------------------------------------------------------------//
            lapack::internal::PanelTridiagL( ABR, WPan, e1 );
            blas::Syr2k( Lower, Normal, (R)-1, A21, W21, (R)1, A22 );
            A11Expanded.SetDiagonal( e1, -1 );
            //----------------------------------------------------------------//
            WPan.FreeAlignments();
            e1.FreeAlignments();
        }
        else
        {
            A11_Star_Star = A11;
            lapack::Tridiag( Lower, A11_Star_Star.LocalMatrix() );
            A11 = A11_Star_Star;
        }

        SlidePartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, A01, /**/ A02,
               /**/       A10, A11, /**/ A12,
         /*************/ /******************/
          ABL, /**/ ABR,  A20, A21, /**/ A22 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

#ifndef WITHOUT_COMPLEX
template<typename R>
void
elemental::lapack::internal::TridiagL
( DistMatrix<complex<R>,MC,MR  >& A,
  DistMatrix<complex<R>,MD,Star>& t )
{
#ifndef RELEASE
    PushCallStack("lapack::internal::TridiagL");
    if( A.GetGrid() != t.GetGrid() )
        throw logic_error( "A and t must be distributed over the same grid." );
#endif
    const Grid& g = A.GetGrid();
#ifndef RELEASE
    if( A.Height() != A.Width() )
        throw logic_error( "A must be square." );
    if( t.Viewing() || t.ConstrainedColAlignment() )
        throw logic_error( "t must not be a view or constrained." );
#endif
    typedef complex<R> C;

    t.AlignWithDiag( A, -1 );
    t.ResizeTo( A.Height()-1, 1 );

    // Matrix views 
    DistMatrix<C,MC,MR> 
        ATL(g), ATR(g),  A00(g), A01(g), A02(g), 
        ABL(g), ABR(g),  A10(g), A11(g), A12(g),
                         A20(g), A21(g), A22(g),
        A11Expanded(g);
    DistMatrix<C,MD,Star> tT(g),  t0(g), 
                          tB(g),  t1(g),
                                  t2(g);

    // Temporary distributions
    DistMatrix<C,Star,Star> A11_Star_Star(g);
    DistMatrix<R,MD,  Star> e1(g);
    DistMatrix<C,Star,Star> t1_Star_Star(g);
    DistMatrix<C,MC,  MR  > W11(g),  WPan(g),
                            W21(g);

    PartitionDownDiagonal
    ( A, ATL, ATR,
         ABL, ABR, 0 );
    PartitionDown
    ( t, tT,
         tB, 0 );
    while( ATL.Height() < A.Height() )
    {
        RepartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, /**/ A01, A02,
         /*************/ /******************/
               /**/       A10, /**/ A11, A12,
          ABL, /**/ ABR,  A20, /**/ A21, A22 );

        RepartitionDown
        ( tT,  t0,
         /**/ /**/
               t1,
          tB,  t2 );
            
        if( A22.Height() > 0 )
        {
            A11Expanded.View( ABR, 0, 0, A11.Height()+1, A11.Width()+1 );
            WPan.AlignWith( A11 );
            WPan.ResizeTo( ABR.Height(), A11.Width() );
            PartitionDown
            ( WPan, W11,
                    W21, A11.Height() );
            e1.AlignWithDiag( ABR, -1 );
            e1.ResizeTo( WPan.Width(), 1 );
            //----------------------------------------------------------------//
            lapack::internal::PanelTridiagL( ABR, WPan, e1, t1 );
            blas::Her2k( Lower, Normal, (C)-1, A21, W21, (C)1, A22 );
            A11Expanded.SetDiagonal( e1, -1 );
            //----------------------------------------------------------------//
            WPan.FreeAlignments();
            e1.FreeAlignments();
        }
        else
        {
            A11_Star_Star = A11;
            t1_Star_Star.ResizeTo( t1.Height(), 1 );

            lapack::Tridiag
            ( Lower, A11_Star_Star.LocalMatrix(), t1_Star_Star.LocalMatrix() );

            A11 = A11_Star_Star;
            t1 = t1_Star_Star;
        }

        SlidePartitionDownDiagonal
        ( ATL, /**/ ATR,  A00, A01, /**/ A02,
               /**/       A10, A11, /**/ A12,
         /*************/ /******************/
          ABL, /**/ ABR,  A20, A21, /**/ A22 );

        SlidePartitionDown
        ( tT,  t0,
               t1,
         /**/ /**/
          tB,  t2 );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}
#endif // WITHOUT_COMPLEX

template void elemental::lapack::internal::TridiagL
( DistMatrix<float,MC,MR>& A );

template void elemental::lapack::internal::TridiagL
( DistMatrix<double,MC,MR>& A );

#ifndef WITHOUT_COMPLEX
template void elemental::lapack::internal::TridiagL
( DistMatrix<scomplex,MC,MR  >& A, 
  DistMatrix<scomplex,MD,Star>& t );

template void elemental::lapack::internal::TridiagL
( DistMatrix<dcomplex,MC,MR  >& A, 
  DistMatrix<dcomplex,MD,Star>& t );
#endif

