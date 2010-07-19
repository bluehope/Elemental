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
#ifndef ELEMENTAL_DIST_MATRIX_STAR_MC_HPP
#define ELEMENTAL_DIST_MATRIX_STAR_MC_HPP 1

namespace elemental {

// Partial specialization to A[* ,MC]
//
// The columns of these distributed matrices will be replicated on all 
// processes (*), and the rows will be distributed like "Matrix Columns" 
// (MC). Thus the rows will be distributed among columns of the process
// grid.

template<typename T>
class DistMatrixBase<T,Star,MC> : public AbstractDistMatrix<T>
{
protected:
    typedef AbstractDistMatrix<T> ADM;

    DistMatrixBase
    ( int height,
      int width,
      bool constrainedRowAlignment,
      int rowAlignment,
      int rowShift,
      const Grid& g );

    ~DistMatrixBase();

public:
    //------------------------------------------------------------------------//
    // Fulfillments of abstract virtual func's from AbstractDistMatrixBase    //
    //------------------------------------------------------------------------//

    //
    // Non-collective routines
    //

    // (empty)

    //
    // Collective routines
    //

    virtual T Get( int i, int j ) const;
    virtual void Set( int i, int j, T alpha );

    virtual void MakeTrapezoidal
    ( Side side, Shape shape, int offset = 0 );

    virtual void Print( const std::string& s ) const;
    virtual void ResizeTo( int height, int width );
    virtual void SetToIdentity();
    virtual void SetToRandom();

    //------------------------------------------------------------------------//
    // Routines specific to [* ,MC] distribution                              //
    //------------------------------------------------------------------------//

    //
    // Non-collective routines
    //

    // (empty)

    //
    // Collective routines
    //

    // Aligns all of our DistMatrix's distributions that match a distribution
    // of the argument DistMatrix.
    void AlignWith( const DistMatrixBase<T,MR,  MC  >& A );
    void AlignWith( const DistMatrixBase<T,Star,MC  >& A );
    void AlignWith( const DistMatrixBase<T,MC,  MR  >& A );
    void AlignWith( const DistMatrixBase<T,MC,  Star>& A );
    void AlignWith( const DistMatrixBase<T,VC,  Star>& A );
    void AlignWith( const DistMatrixBase<T,Star,VC  >& A ); 
    void AlignWith( const DistMatrixBase<T,Star,MD  >& A ) {}
    void AlignWith( const DistMatrixBase<T,Star,MR  >& A ) {}
    void AlignWith( const DistMatrixBase<T,Star,VR  >& A ) {}
    void AlignWith( const DistMatrixBase<T,Star,Star>& A ) {}
    void AlignWith( const DistMatrixBase<T,MD,  Star>& A ) {}
    void AlignWith( const DistMatrixBase<T,MR,  Star>& A ) {}
    void AlignWith( const DistMatrixBase<T,VR,  Star>& A ) {}

    // Aligns our column distribution (i.e., Star) with the matching 
    // distribution of the argument. These are all no-ops and exist solely for
    // templating over distribution parameters.
    void AlignColsWith( const DistMatrixBase<T,Star,MC  >& A ) {}
    void AlignColsWith( const DistMatrixBase<T,Star,MD  >& A ) {}
    void AlignColsWith( const DistMatrixBase<T,Star,MR  >& A ) {}
    void AlignColsWith( const DistMatrixBase<T,Star,VC  >& A ) {}
    void AlignColsWith( const DistMatrixBase<T,Star,VR  >& A ) {}
    void AlignColsWith( const DistMatrixBase<T,Star,Star>& A ) {}
    void AlignColsWith( const DistMatrixBase<T,MC,  Star>& A ) {}
    void AlignColsWith( const DistMatrixBase<T,MD,  Star>& A ) {}
    void AlignColsWith( const DistMatrixBase<T,MR,  Star>& A ) {}
    void AlignColsWith( const DistMatrixBase<T,VC,  Star>& A ) {}
    void AlignColsWith( const DistMatrixBase<T,VR,  Star>& A ) {}

    // Aligns our row distribution (i.e., MC) with the matching distribution
    // of the argument. We recognize that a VC distribution can be a subset 
    // of an MC distribution.
    void AlignRowsWith( const DistMatrixBase<T,MR,  MC  >& A );
    void AlignRowsWith( const DistMatrixBase<T,Star,MC  >& A );
    void AlignRowsWith( const DistMatrixBase<T,MC,  MR  >& A );
    void AlignRowsWith( const DistMatrixBase<T,MC,  Star>& A );
    void AlignRowsWith( const DistMatrixBase<T,VC,  Star>& A );
    void AlignRowsWith( const DistMatrixBase<T,Star,VC  >& A );

    // (Immutable) view of a distributed matrix
    void View( DistMatrixBase<T,Star,MC>& A );
    void LockedView( const DistMatrixBase<T,Star,MC>& A );

    // (Immutable) view of a portion of a distributed matrix
    void View
    ( DistMatrixBase<T,Star,MC>& A,
      int i, int j, int height, int width );

    void LockedView
    ( const DistMatrixBase<T,Star,MC>& A,
      int i, int j, int height, int width );

    // (Immutable) view of two horizontally contiguous partitions of a
    // distributed matrix
    void View1x2
    ( DistMatrixBase<T,Star,MC>& AL, DistMatrixBase<T,Star,MC>& AR );

    void LockedView1x2
    ( const DistMatrixBase<T,Star,MC>& AL, 
      const DistMatrixBase<T,Star,MC>& AR );

    // (Immutable) view of two vertically contiguous partitions of a
    // distributed matrix
    void View2x1
    ( DistMatrixBase<T,Star,MC>& AT,
      DistMatrixBase<T,Star,MC>& AB );

    void LockedView2x1
    ( const DistMatrixBase<T,Star,MC>& AT,
      const DistMatrixBase<T,Star,MC>& AB );

    // (Immutable) view of a contiguous 2x2 set of partitions of a
    // distributed matrix
    void View2x2
    ( DistMatrixBase<T,Star,MC>& ATL, DistMatrixBase<T,Star,MC>& ATR,
      DistMatrixBase<T,Star,MC>& ABL, DistMatrixBase<T,Star,MC>& ABR );

    void LockedView2x2
    ( const DistMatrixBase<T,Star,MC>& ATL, 
      const DistMatrixBase<T,Star,MC>& ATR,
      const DistMatrixBase<T,Star,MC>& ABL, 
      const DistMatrixBase<T,Star,MC>& ABR );

    // AllReduce over process row
    void SumOverRow();

    // Routines needed to implement algorithms that avoid using
    // inefficient unpackings of partial matrix distributions
    void ConjugateTransposeFrom( const DistMatrixBase<T,VC,Star>& A );
    void TransposeFrom( const DistMatrixBase<T,VC,Star>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,MC,MR>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,MC,Star>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,Star,MR>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,MD,Star>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,Star,MD>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,MR,MC>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,MR,Star>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,Star,MC>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,VC,Star>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,Star,VC>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,VR,Star>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,Star,VR>& A );

    const DistMatrixBase<T,Star,MC>&
    operator=( const DistMatrixBase<T,Star,Star>& A );
};

template<typename R>
class DistMatrix<R,Star,MC> : public DistMatrixBase<R,Star,MC>
{
protected:
    typedef DistMatrixBase<R,Star,MC> DMB;

public:
    DistMatrix
    ( const Grid& g );

    DistMatrix
    ( int height, int width, const Grid& g );

    DistMatrix
    ( bool constrainedRowAlignment, int rowAlignment, const Grid& g );

    DistMatrix
    ( int height, int width,
      bool constrainedRowAlignment, int rowAlignment, const Grid& g );

    DistMatrix
    ( const DistMatrix<R,Star,MC>& A );

    ~DistMatrix();
    
    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,MC,MR>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,MC,Star>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,Star,MR>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,MD,Star>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,Star,MD>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,MR,MC>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,MR,Star>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,Star,MC>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,VC,Star>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,Star,VC>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,VR,Star>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,Star,VR>& A );

    const DistMatrix<R,Star,MC>&
    operator=( const DistMatrix<R,Star,Star>& A );

    //------------------------------------------------------------------------//
    // Fulfillments of abstract virtual func's from AbstractDistMatrixBase    //
    //------------------------------------------------------------------------//

    //
    // Non-collective routines
    //

    // (empty)

    //
    // Collective routines
    //

    virtual void SetToRandomHPD();
};

#ifndef WITHOUT_COMPLEX
template<typename R>
class DistMatrix<std::complex<R>,Star,MC> 
: public DistMatrixBase<std::complex<R>,Star,MC>
{
protected:
    typedef DistMatrixBase<std::complex<R>,Star,MC> DMB;

public:

    DistMatrix
    ( const Grid& g );

    DistMatrix
    ( int height, int width, const Grid& g );

    DistMatrix
    ( bool constrainedRowAlignment, int rowAlignment, const Grid& g );

    DistMatrix
    ( int height, int width,
      bool constrainedRowAlignment, int rowAlignment, const Grid& g );

    DistMatrix
    ( const DistMatrix<std::complex<R>,Star,MC>& A );

    ~DistMatrix();
    
    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,MC,MR>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,MC,Star>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,Star,MR>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,MD,Star>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,Star,MD>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,MR,MC>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,MR,Star>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,Star,MC>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,VC,Star>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,Star,VC>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,VR,Star>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,Star,VR>& A );

    const DistMatrix<std::complex<R>,Star,MC>&
    operator=( const DistMatrix<std::complex<R>,Star,Star>& A );

    //------------------------------------------------------------------------//
    // Fulfillments of abstract virtual func's from AbstractDistMatrixBase    //
    //------------------------------------------------------------------------//

    //
    // Non-collective routines
    //

    // (empty)

    //
    // Collective routines
    //

    virtual void SetToRandomHPD();

    //------------------------------------------------------------------------//
    // Fulfillments of abstract virtual func's from AbstractDistMatrix        //
    //------------------------------------------------------------------------//

    //
    // Non-collective routines
    //

    // (empty)

    //
    // Collective routines
    //

    virtual R GetReal( int i, int j ) const;
    virtual R GetImag( int i, int j ) const;
    virtual void SetReal( int i, int j, R u );
    virtual void SetImag( int i, int j, R u );
};
#endif

//----------------------------------------------------------------------------//
// Implementation begins here                                                 //
//----------------------------------------------------------------------------//

//
// DistMatrixBase[* ,MC]
//

template<typename T>
inline
DistMatrixBase<T,Star,MC>::DistMatrixBase
( int height,
  int width,
  bool constrainedRowAlignment,
  int rowAlignment,
  int rowShift,
  const Grid& g )
: ADM(height,width,false,constrainedRowAlignment,0,rowAlignment,0,rowShift,g)
{ }

template<typename T>
inline
DistMatrixBase<T,Star,MC>::~DistMatrixBase()
{ }

//
// Real DistMatrix[* ,MC]
//

template<typename R>
inline
DistMatrix<R,Star,MC>::DistMatrix
( const Grid& g )
: DMB(0,0,false,0,g.MCRank(),g)
{ }

template<typename R>
inline
DistMatrix<R,Star,MC>::DistMatrix
( int height, int width, const Grid& g )
: DMB(height,width,false,0,g.MCRank(),g)
{
#ifndef RELEASE
    PushCallStack("DistMatrix[* ,MC]::DistMatrix");
#endif
    DMB::LocalMatrix().ResizeTo
    ( height, utilities::LocalLength( width, g.MCRank(), g.Height() ) );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R>
inline
DistMatrix<R,Star,MC>::DistMatrix
( bool constrainedRowAlignment, int rowAlignment, const Grid& g )
: DMB(0,0,constrainedRowAlignment,rowAlignment,
      utilities::Shift( g.MCRank(), rowAlignment, g.Height() ),g)
{ }

template<typename R>
inline
DistMatrix<R,Star,MC>::DistMatrix
( int height, int width,
  bool constrainedRowAlignment, int rowAlignment, const Grid& g )
: DMB(height,width,constrainedRowAlignment,rowAlignment,
      utilities::Shift( g.MCRank(), rowAlignment, g.Height() ),g)
{
#ifndef RELEASE
    PushCallStack("DistMatrix[* ,MC]::DistMatrix");
#endif
    DMB::LocalMatrix().ResizeTo
    ( height, utilities::LocalLength( width, DMB::RowShift(), g.Height() ) );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R>
inline
DistMatrix<R,Star,MC>::DistMatrix
( const DistMatrix<R,Star,MC>& A )
: DMB(0,0,false,0,0,A.GetGrid())
{
#ifndef RELEASE
    PushCallStack("DistMatrix[* ,MC]::DistMatrix");
#endif
    if( &A != this )
        *this = A;
    else
        throw std::logic_error
        ( "Attempted to construct a [* ,MC] with itself." );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R>
inline
DistMatrix<R,Star,MC>::~DistMatrix()
{ }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,MC,MR>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,MC,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,Star,MR>& A)
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,MD,Star>& A)
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,Star,MD>& A)
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,MR,MC>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,MR,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,Star,MC>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,VC,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,Star,VC>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,VR,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,Star,VR>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<R,Star,MC>&
DistMatrix<R,Star,MC>::operator=
( const DistMatrix<R,Star,Star>& A )
{ DMB::operator=( A ); return *this; }

//
// Complex DistMatrix[* ,MC]
//

#ifndef WITHOUT_COMPLEX
template<typename R>
inline
DistMatrix<std::complex<R>,Star,MC>::DistMatrix
( const Grid& g )
: DMB(0,0,false,0,g.MCRank(),g)
{ }

template<typename R>
inline
DistMatrix<std::complex<R>,Star,MC>::DistMatrix
( int height, int width, const Grid& g )
: DMB(height,width,false,0,g.MCRank(),g)
{
#ifndef RELEASE
    PushCallStack("DistMatrix[* ,MC]::DistMatrix");
#endif
    DMB::LocalMatrix().ResizeTo
    ( height, utilities::LocalLength( width, g.MCRank(), g.Height() ) );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R>
inline
DistMatrix<std::complex<R>,Star,MC>::DistMatrix
( bool constrainedRowAlignment, int rowAlignment, const Grid& g )
: DMB(0,0,constrainedRowAlignment,rowAlignment,
      utilities::Shift( g.MCRank(), rowAlignment, g.Height() ),g)
{ }

template<typename R>
inline
DistMatrix<std::complex<R>,Star,MC>::DistMatrix
( int height, int width,
  bool constrainedRowAlignment, int rowAlignment, const Grid& g )
: DMB(height,width,constrainedRowAlignment,rowAlignment,
      utilities::Shift( g.MCRank(), rowAlignment, g.Height() ),g)
{
#ifndef RELEASE
    PushCallStack("DistMatrix[* ,MC]::DistMatrix");
#endif
    DMB::LocalMatrix().ResizeTo
    ( height, utilities::LocalLength( width, DMB::RowShift(), g.Height() ) );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R>
inline
DistMatrix<std::complex<R>,Star,MC>::DistMatrix
( const DistMatrix<std::complex<R>,Star,MC>& A )
: DMB(0,0,false,0,0,A.GetGrid())
{
#ifndef RELEASE
    PushCallStack("DistMatrix[* ,MC]::DistMatrix");
#endif
    if( &A != this )
        *this = A;
    else
        throw std::logic_error
        ( "Attempted to construct a [* ,MC] with itself." );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename R>
inline
DistMatrix<std::complex<R>,Star,MC>::~DistMatrix()
{ }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,MC,MR>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,MC,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,Star,MR>& A)
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,MD,Star>& A)
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,Star,MD>& A)
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,MR,MC>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,MR,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,Star,MC>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,VC,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,Star,VC>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,VR,Star>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,Star,VR>& A )
{ DMB::operator=( A ); return *this; }

template<typename R>
inline const DistMatrix<std::complex<R>,Star,MC>&
DistMatrix<std::complex<R>,Star,MC>::operator=
( const DistMatrix<std::complex<R>,Star,Star>& A )
{ DMB::operator=( A ); return *this; }
#endif // WITHOUT_COMPLEX

} // elemental

#endif /* ELEMENTAL_DIST_MATRIX_STAR_MC_HPP */

