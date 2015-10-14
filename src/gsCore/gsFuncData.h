/** @file gsFuncData.h

    @brief
    This object is a cache for computed values from an evaluator.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): A. Bressan, A. Manzaflaris
*/

#pragma once

#include<gsCore/gsLinearAlgebra.h> 
#include<gsCore/gsBoundary.h> // need patchSide

/**
   @brief Contains information for the functions in a gsFunctionSet.
*/
struct gsFuncInfo
{
public:
    /// \brief Dimension of the (source) domain.
    /// @return For \f$f:\mathbb{R}^n\rightarrow\mathbb{R}^m\f$ returns \f$n\f$.
    int  domainDim;
    /// \brief Dimension of the target (image) space.
    /// @return For \f$f:\mathbb{R}^n\rightarrow\mathbb{R}^m\f$ returns \f$m\f$.
    int  targetDim;
public:
    // functions giving the size (the number of returned coefficient per function)
    /// Number of derivatives (<em>targetDim*domainDim</em>).
    int  derivSize () const {return domainDim*targetDim;}

    /// Number of 2nd derivatives (<em>targetDim*domainDim*(domainDim+1)/2</em>).
    int  deriv2Size() const {return targetDim*domainDim*(domainDim+1) / 2; }
    /// Size of computed divergence (<em>targetDim/domainDim</em>).
    int  divSize   () const {return targetDim/domainDim;}
public:
    gsFuncInfo()
    {}

    /// \brief Constructor.
    ///
    /// \param[in] domDim Dimension of (source) domain (see gsFuncInfo::domainDim).
    /// \param[in] tarDim Dimension of target space (see gsFuncInfo::targetDim).
    gsFuncInfo(int domDir,int tarDim)
        :domainDim(domDir),targetDim(tarDim)
    {}

    /// Equality test, returns true if both <em>domainDim</em> and <em>targetDim</em> are equal.
    bool operator== (const gsFuncInfo& other) const {return domainDim==other.domainDim && targetDim==other.targetDim;}
};

namespace gismo 
{


/*
// AB: just for discussion
union gsNeededValues
{
    unsigned flags;
    struct
    {
    // primitive values
    int active : 2; // 0 no computation, 1 one point, -1 all points
    int derivs : 4; // 0 no computation, ... k derivatives up to order k-1
                    // this works up to derivative of order 15 and that should be enough
    // derived values for all functions
    bool div  : 1;
    bool curl : 1;
    bool lap  : 1;
    bool hess : 1;
    // derived values for parametrizations
    bool gradTransform : 1;
    bool measure       : 1;
    bool normals       : 1;
    bool outerNormals  : 1;

    bool oneElement : 1; // assume all points are in the same element
    } members;

    gsNeededValues(int der, unsigned flag)
    {
        flags  = flag;
        members.derivs = der;
    }
};
*/

template <typename T> class gsFunctionSet;

/**
   @brief the gsFuncData is a cache of pre-computed function sets values.

   Which data is contained in the gsFuncData is specidied by a flag system.
   The user must set the \a flags memeber using a combination of the constants
   \a NEED_VALUE, \a NEED_DERIV, ... then the cache is filled by calling the
   \a gsFunctionSet::compute(points, gsFuncData&) where points can either be
   a gsMatrix<> containing the point coordinates or a gsMapData object.
   
   The row matrix data are public members. There are also accessor functions
   that provide a per-point view of the data in a different format: each column
   corresponds to a different function object.
   
   \tparam T numeric type
*/
template <typename T>
class gsFuncData
{
    friend class gsFunctionSet<T>;

protected:
    typedef typename gsMatrix<T>::constColumn constColumn;
    // Types for returning quick access to data in matrix format
    typedef gsAsConstMatrix<T, -1, -1>                  matrixView;
    typedef Eigen::Transpose<typename matrixView::Base> matrixTransposeView;

public:
    unsigned flags;
    int      patchId;

    gsFuncInfo         info;
    gsMatrix<unsigned> actives;

    /// Stores values and derivatives 
    std::vector<gsMatrix<T> > values;

    gsMatrix<T> curls;
    gsMatrix<T> divs;
    gsMatrix<T> laplacians;

public:
    /**
     * @brief Main constructor
     * @param flags what to compute
     * @param patch in case of multipatch structures, on which patch to compute
     */
    explicit gsFuncData(unsigned _flags = 0, int patch = 0)
    : flags(_flags), patchId(0)
    { }

public:

    /**
     * @brief addFlags
     * set the evaluator to compute additional values
     * @param newFlags
     */
    void addFlags (unsigned newFlags) 
    { flags = flags|newFlags; }


    int maxDeriv() const 
    {
        if (flags & NEED_2ND_DER)
            return 2;
        if (flags & NEED_DERIV)
            return 1;
        if (flags & NEED_VALUE)
            return 0;
        return -1;
    }

    /**
     * @brief Provides memory usage information
     * @return the number of bytes occupied by this object
     */
    unsigned bytesUsed() const
    { /*to do*/
        return 1;
    }

    /// \brief Clear the memory that this object uses
    void clear() { /*to do*/}

    /// \brief Swaps this object with \a other
    void swap(gsFuncData & other) 
    { 
        std::swap(flags  , other.flags  );
        std::swap(patchId, other.patchId);
        std::swap(info   , other.info   );
        actives   .swap(other.actives   );
        values    .swap(other.values    );
        curls     .swap(other.curls     );
        divs      .swap(other.divs      );
        laplacians.swap(other.laplacians);
    }

public:

    inline gsMatrix<unsigned>::constColumn active(index_t point = 0) const
    {
        GISMO_ASSERT(flags & NEED_ACTIVE,
                   "actives are not computed unless the NEED_ACTIVE flag is set.");
        return actives.col(point);
    }

    inline matrixView eval (index_t point) const
    {
        GISMO_ASSERT(flags & NEED_VALUE,
                   "values are not computed unless the NEED_VALUE flag is set.");
        return values[0].reshapeCol(point, info.targetDim, values[0].rows()/info.targetDim);
    }

    inline matrixView deriv (index_t point) const
    {
        GISMO_ASSERT(flags & NEED_DERIV,
                   "derivs are not computed unless the NEED_DERIV flag is set.");
        return values[1].reshapeCol(point, info.derivSize(), values[1].rows()/info.derivSize());
    }

    inline matrixView deriv2 (index_t point) const
    {
        GISMO_ASSERT(flags & NEED_DERIV2,
                   "deriv2s are not computed unless the NEED_DERIV2 flag is set.");
        return values[2].reshapeCol(point, info.deriv2Size(), values[2].rows()/info.deriv2Size());
    }

    inline matrixView curl (index_t point) const
    {
        GISMO_ASSERT(flags & NEED_CURL,
                   "curls are not computed unless the NEED_CURL flag is set.");
        return curls.reshapeCol(point, info.targetDim, curls.rows()/info.targetDim );
    }

    inline matrixView div (index_t point) const
    {
        GISMO_ASSERT(flags & NEED_DIV,
                   "divs are not computed unless the NEED_DIV flag is set.");
        return divs.reshapeCol(point, info.divSize(), divs.rows()/info.divSize() );
    }

    inline matrixView laplacian (index_t point) const
    {
        GISMO_ASSERT(flags & NEED_LAPLACIAN,
                   "laplacians are not computed unless the NEED_LAPLACIAN flag is set.");
        return laplacians.reshapeCol(point, info.targetDim, laplacians.rows()/info.targetDim );
    }


    inline matrixTransposeView jacobian (index_t point, index_t func = 0)
    {
       GISMO_ASSERT(flags & NEED_DERIV,
                  "jacobian access needs the computation of derivs: set the NEED_DERIV flag.");
       return gsAsConstMatrix<T, Dynamic, Dynamic>(&values[1].coeffRef(point,func*info.derivSize()), info.domainDim,info.targetDim).transpose();
    }
};


/**
 @brief the gsMapData is a cache of pre-computed function (map) values.

 \tparam T numeric type 
 \sa gsFuncData
 */
template <typename T>
class gsMapData : public gsFuncData<T>
{
public:
    typedef gsFuncData<T> Base;
    typedef typename Base::constColumn         constColumn;
    typedef typename Base::matrixView          matrixView;
    typedef typename Base::matrixTransposeView matrixTransposeView;

public:
    /**
     * @brief Main constructor
     * @param flags what to compute
     */
    explicit gsMapData(unsigned _flags = 0, patchSide _side=patchSide(0,0))
    : Base(_flags), side(_side)
    { }

public:
    using Base::info;
    using Base::flags;

    gsMatrix<T> points; ///< input (parametric) points

    gsMatrix<T> measures;
    gsMatrix<T> gradTransforms;
    gsMatrix<T> normals;

    patchSide   side;
public:
    inline constColumn point(const index_t point) const { return points.col(point);}

    inline constColumn measure(const index_t point) const
    {
        GISMO_ASSERT(flags & NEED_MEASURE,
                   "measures are not computed unless the NEED_MEASURE flag is set.");
        return measures.col(point);
    }

    inline matrixView gradTransform(const index_t point) const
    {
        GISMO_ASSERT(flags & NEED_GRAD_TRANSFORM,
                   "gradTransforms are not computed unless the NEED_GRAD_TRANSFORM flag is set.");
        return gradTransforms.reshapeCol(point, info.targetDim, info.domainDim);
    }

    inline constColumn normal(const index_t point) const
    {
        GISMO_ASSERT(flags & NEED_NORMAL,
                   "normals are not computed unless the NEED_NORMAL flag is set.");
        return normals.col(point);
    }

};

} // namespace gismo

namespace std
{

// Specializations of std::swap
template <typename T>
void swap(gismo::gsFuncData<T> & f1, gismo::gsFuncData<T> & f2)
{
    f1.swap(f2);
}

} // namespace std




