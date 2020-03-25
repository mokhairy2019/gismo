/** @file gsNormL2.h

    @brief Computes the Semi H2 norm, needs for the parallel computing.

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
class gsVisitorSeminormH2
{
public:

    gsVisitorSeminormH2(const std::vector< gsMultiPatch<>> & g1):
        m_G1Basis(g1)
    {
        f2param = false;
        g1basis = true;

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
        evFlags = NEED_MEASURE|NEED_VALUE|NEED_GRAD_TRANSFORM|NEED_2ND_DER;
    }

    // Evaluate on element.
    void evaluate(gsGeometryEvaluator<T> & geoEval,
                  const gsFunction<T>    & _func1,
                  const gsFunction<T>    & _func2,
                  gsMatrix<T>            & quNodes)
    {
        // Evaluate first function
        _func1.deriv_into(quNodes, f1ders);
        _func1.deriv2_into(quNodes, f1ders2);

        if (g1basis)
        {
            index_t n = m_G1Basis.at(geoEval.id()).nPatches();

            for (index_t i = 0; i < n; i++)
            {
                f1ders += m_G1Basis.at(geoEval.id()).patch(i).deriv(quNodes);
                f1ders2 += m_G1Basis.at(geoEval.id()).patch(i).deriv2(quNodes);
            }
        }

        // get the gradients to columns
        f1ders.resize(quNodes.rows(), quNodes.cols() );
        //f1ders2.transposeInPlace();

        // Evaluate second function (defined of physical domain)
        geoEval.evaluateAt(quNodes);
        _func2.deriv2_into(geoEval.values(), f2ders2);
    }

    // assemble on element
    inline T compute(gsDomainIterator<T>    & ,
                     gsGeometryEvaluator<T> & geoEval,
                     gsVector<T> const      & quWeights,
                     T & accumulated)
    {
        T sum(0.0);
        for (index_t k = 0; k < quWeights.rows(); ++k) // loop over quadrature nodes
        {
            // Transform the gradients
            geoEval.transformDeriv2Hgrad(k, f1ders, f1ders2, f1pders2);
            f1pders2.transposeInPlace();
            //if ( f2Param )
            //f2ders.col(k)=geoEval.gradTransforms().block(0, k*d,d,d) * f2ders.col(k);// to do: generalize
            short_t parDim = geoEval.parDim();
            int rest = f1pders2.rows()-parDim;
            const T weight = quWeights[k] *  geoEval.measure(k);
            sum += weight * ((f1pders2.topRows(parDim) - f2ders2.col(k).topRows(parDim)).squaredNorm() +
                2*(f1pders2.bottomRows(rest) - f2ders2.col(k).bottomRows(rest)).squaredNorm());
        }
        accumulated += sum;
        return sum;
    }

private:


    gsMatrix<T> f1ders, f1ders2, f2ders2;
    gsMatrix<T> f1pders2;

    bool f2param;
    bool g1basis;

protected:

    std::vector< gsMultiPatch<>> m_G1Basis;
};

}






