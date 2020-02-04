/** @file gsNormL2.h

    @brief Computes the Semi H1 norm, needs for the parallel computing.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s): P. Weinmüller
*/


#pragma once

namespace gismo
{


template <class T>
class gsVisitorSeminormH1
{
    typedef std::multimap<index_t, std::map<index_t, std::map<index_t, gsMultiPatch<real_t>>>> typedef_g1;

public:

    gsVisitorSeminormH1(const typedef_g1 & g1):
        m_G1Basis_mp(g1)
    {
        f2param = false;
        g1basis = false;
        g1basis_mp = true;

    }

    void initialize(const gsBasis<T> & basis,
                    gsQuadRule<T> & rule,
                    unsigned      & evFlags) // replace with geoEval ?
    {
        // Setup Quadrature
        const unsigned d = basis.dim();
        gsVector<index_t> numQuadNodes( d );
        for (unsigned i = 0; i < d; ++i)
            numQuadNodes[i] = basis.degree(i) + 1;

        // Setup Quadrature
        rule = gsGaussRule<T>(numQuadNodes);// harmless slicing occurs here

        // Set Geometry evaluation flags
        evFlags = NEED_MEASURE|NEED_VALUE|NEED_GRAD_TRANSFORM;
    }

    // Evaluate on element.
    void evaluate(gsGeometryEvaluator<T> & geoEval,
                  const gsFunction<T>    & _func1,
                  const gsFunction<T>    & _func2,
                  gsMatrix<T>            & quNodes)
    {
        // Evaluate first function
        _func1.deriv_into(quNodes, f1ders);

        if (g1basis)
        {
            index_t n = m_G1Basis.at(geoEval.id()).nPatches();

            for (index_t i = 0; i < n; i++)
            {
                f1ders += m_G1Basis.at(geoEval.id()).patch(i).deriv(quNodes);
            }
        }
        if (g1basis_mp)
        {
            std::multimap<index_t, std::map<index_t, std::map<index_t, gsMultiPatch<real_t>>>>::iterator i_face;
            for (i_face=m_G1Basis_mp.equal_range(geoEval.id()).first; i_face!=m_G1Basis_mp.equal_range(geoEval.id()).second; ++i_face)
            {
                std::map<index_t, std::map<index_t, gsMultiPatch<real_t>>>::iterator i_side;
                for (i_side = i_face->second.begin(); i_side != i_face->second.end(); ++i_side)
                {
                    for (std::map<index_t, gsMultiPatch<real_t>>::iterator i_mp = i_side->second.begin();
                        i_mp != i_side->second.end(); ++i_mp)
                    {
                        gsMultiPatch<real_t> mp_side = i_mp->second;
                        for (unsigned j = 0; j < mp_side.nPatches(); j++)
                        {
                            f1ders += mp_side.patch(j).deriv(quNodes);
                        }
                    }
                }
            }
        }

        // Evaluate second function
        geoEval.evaluateAt(quNodes);
        _func2.deriv_into( f2param ? quNodes : geoEval.values() , f2ders);
    }


    // assemble on element
    inline T compute(gsDomainIterator<T>    & geo,
                     gsGeometryEvaluator<T> & geoEval,
                     gsVector<T> const      & quWeights,
                     T & accumulated)
    {
        T sum(0.0);
        for (index_t k = 0; k < quWeights.rows(); ++k) // loop over quadrature nodes
        {
            // Transform the gradients
            geoEval.transformGradients(k, f1ders, f1pders);

            // Transform the gradients, if func2 is defined on the parameter space (f2param = true)
            if(f2param)
                geoEval.transformGradients(k, f2ders, f2pders);

            // old
            //if ( f2param )
            //f2ders.col(k)=geoEval.gradTransforms().block(0, k*d,d,d) * f2ders.col(k);// to do: generalize

            const T weight = quWeights[k] *  geoEval.measure(k);

            if(!f2param) // standard case: func2 defined on physical space
            {
                // for each k: put the gradients into the columns (as in f1pders)
                gsMatrix<T> f2dersk = f2ders.col(k);
                f2dersk.resize(2, 1); // pardim(), targetDim() // TODO

                sum += weight * (f1pders - f2dersk).squaredNorm();
            }
            else // case: func2 defined on parameter space
                sum += weight * (f1pders - f2pders).squaredNorm();
        }
        accumulated += sum;
        return sum;
    }


protected:

    gsMatrix<T> f1ders, f2ders;
    gsMatrix<T> f1pders, f2pders; // f2pders only needed if f2param = true

    bool f2param;
    bool g1basis;
    bool g1basis_mp;

protected:

    std::vector< gsMultiPatch<>> m_G1Basis;
    typedef_g1 m_G1Basis_mp;
};






}







