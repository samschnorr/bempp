// Copyright (C) 2011-2012 by the BEM++ Authors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "config_ahmed.hpp"
#include "config_trilinos.hpp"
#ifdef WITH_AHMED

#include "discrete_aca_linear_operator.hpp"
#include "ahmed_aux.hpp"
#include "../common/not_implemented_error.hpp"
#include "../common/chunk_statistics.hpp"
#include "../fiber/explicit_instantiation.hpp"

#ifdef WITH_TRILINOS
#include <Thyra_DetachedSpmdVectorView.hpp>
#include <Thyra_SpmdVectorSpaceDefaultBase.hpp>
#endif

#include <tbb/blocked_range.h>
#include <tbb/concurrent_queue.h>
#include <tbb/parallel_reduce.h>
#include <tbb/task_scheduler_init.h>

#include <iostream>
#include <fstream>

bool multaHvec_omp(double d, blcluster* bl, mblock<double>** A, double* x,
           double* y);
bool multaHvec_omp(float d, blcluster* bl, mblock<float>** A, float* x,
           float* y);
bool multaHvec_omp(dcomp d, blcluster* bl, mblock<dcomp>** A, dcomp* x,
           dcomp* y);
bool multaHvec_omp(scomp d, blcluster* bl, mblock<scomp>** A, scomp* x,
           scomp* y);

namespace Bempp
{

namespace
{

template <typename ValueType>
class MblockMultiplicationLoopBody
{
    typedef mblock<typename AhmedTypeTraits<ValueType>::Type> AhmedMblock;
public:
    typedef tbb::concurrent_queue<size_t> LeafClusterIndexQueue;

    MblockMultiplicationLoopBody(
            ValueType multiplier,
            arma::Col<ValueType>& x,
            arma::Col<ValueType>& y,
            AhmedLeafClusterArray& leafClusters,
            boost::shared_array<AhmedMblock*> blocks,
            LeafClusterIndexQueue& leafClusterIndexQueue,
            std::vector<ChunkStatistics>& stats) :
        m_multiplier(multiplier), m_x(x), m_local_y(y),
        m_leafClusters(leafClusters), m_blocks(blocks),
        m_leafClusterIndexQueue(leafClusterIndexQueue),
        m_stats(stats)
    {
        // m_local_y.fill(static_cast<ValueType>(0.));
    }

    MblockMultiplicationLoopBody(MblockMultiplicationLoopBody& other, tbb::split) :
        m_multiplier(other.m_multiplier),
        m_x(other.m_x), m_local_y(other.m_local_y.n_rows),
        m_leafClusters(other.m_leafClusters), m_blocks(other.m_blocks),
        m_leafClusterIndexQueue(other.m_leafClusterIndexQueue),
        m_stats(other.m_stats)
    {
        m_local_y.fill(static_cast<ValueType>(0.));
    }

    template <typename Range>
    void operator() (const Range& r) {
        for (typename Range::const_iterator i = r.begin(); i != r.end(); ++i) {
            size_t leafClusterIndex = -1;
            if (!m_leafClusterIndexQueue.try_pop(leafClusterIndex)) {
                std::cerr << "MblockMultiplicationLoopBody::operator(): "
                             "Warning: try_pop failed; this shouldn't happen!"
                          << std::endl;
                continue;
            }
            m_stats[leafClusterIndex].valid = true;
            m_stats[leafClusterIndex].chunkStart = r.begin();
            m_stats[leafClusterIndex].chunkSize = r.size();
            m_stats[leafClusterIndex].startTime = tbb::tick_count::now();

            blcluster* cluster = m_leafClusters[leafClusterIndex];
            m_blocks[cluster->getidx()]->multa_vec(ahmedCast(m_multiplier),
                                                   ahmedCast(&m_x(cluster->getb2())),
                                                   ahmedCast(&m_local_y(cluster->getb1())));
            m_stats[leafClusterIndex].endTime = tbb::tick_count::now();
        }
    }

    void join(const MblockMultiplicationLoopBody& other) {
        m_local_y += other.m_local_y;
    }

private:
    ValueType m_multiplier;
    arma::Col<ValueType>& m_x;
public:
    arma::Col<ValueType> m_local_y;
private:
    AhmedLeafClusterArray& m_leafClusters;
    boost::shared_array<AhmedMblock*> m_blocks;
    LeafClusterIndexQueue& m_leafClusterIndexQueue;
    std::vector<ChunkStatistics>& m_stats;
};

} // namespace

template <typename ValueType>
DiscreteAcaLinearOperator<ValueType>::
DiscreteAcaLinearOperator(
        unsigned int rowCount, unsigned int columnCount,
        int maximumRank,
        std::auto_ptr<AhmedBemBlcluster> blockCluster,
        boost::shared_array<AhmedMblock*> blocks,
        const IndexPermutation& domainPermutation,
        const IndexPermutation& rangePermutation) :
#ifdef WITH_TRILINOS
    m_domainSpace(Thyra::defaultSpmdVectorSpace<ValueType>(columnCount)),
    m_rangeSpace(Thyra::defaultSpmdVectorSpace<ValueType>(rowCount)),
#else
    m_rowCount(rowCount), m_columnCount(columnCount),
#endif
    m_maximumRank(maximumRank),
    m_blockCluster(blockCluster), m_blocks(blocks),
    m_domainPermutation(domainPermutation),
    m_rangePermutation(rangePermutation)
{
}

template <typename ValueType>
void
DiscreteAcaLinearOperator<ValueType>::
dump() const
{
    std::cout << asMatrix() << std::endl;
}

template <typename ValueType>
arma::Mat<ValueType>
DiscreteAcaLinearOperator<ValueType>::
asMatrix() const
{
    const unsigned int nRows = rowCount();
    const unsigned int nCols = columnCount();

    arma::Mat<ValueType> permutedOutput(nRows, nCols );
    permutedOutput.fill(0.);
    arma::Col<ValueType> unit(nCols );
    unit.fill(0.);

    for (unsigned int col = 0; col < nCols ; ++col)
    {
        if (col > 0)
            unit(col - 1) = 0.;
        unit(col) = 1.;
        multaHvec(1., m_blockCluster.get(), m_blocks.get(),
                  ahmedCast(unit.memptr()),
                  ahmedCast(permutedOutput.colptr(col)));
    }

    arma::Mat<ValueType> output(nRows, nCols );
    for (unsigned int col = 0; col < nCols ; ++col)
        for (unsigned int row = 0; row < nRows; ++row)
            output(row, col) =
                    permutedOutput(m_rangePermutation.permuted(row),
                                   m_domainPermutation.permuted(col));

    return output;
}

template <typename ValueType>
unsigned int
DiscreteAcaLinearOperator<ValueType>::
rowCount() const
{
#ifdef WITH_TRILINOS
    return m_rangeSpace->dim();
#else
    return m_rowCount;
#endif
}

template <typename ValueType>
unsigned int
DiscreteAcaLinearOperator<ValueType>::
columnCount() const
{
#ifdef WITH_TRILINOS
    return m_domainSpace->dim();
#else
    return m_columnCount;
#endif
}

template <typename ValueType>
void
DiscreteAcaLinearOperator<ValueType>::
addBlock(const std::vector<int>& rows,
         const std::vector<int>& cols,
         const ValueType alpha,
         arma::Mat<ValueType>& block) const
{
    throw std::runtime_error("DiscreteAcaLinearOperator::"
                             "addBlock(): not implemented yet");
}

template <typename ValueType>
const DiscreteAcaLinearOperator<ValueType>&
DiscreteAcaLinearOperator<ValueType>::castToAca(
        const DiscreteLinearOperator<ValueType>& discreteOperator)
{
    return dynamic_cast<const DiscreteAcaLinearOperator<ValueType>&>(discreteOperator);
}

#ifdef WITH_TRILINOS
template <typename ValueType>
Teuchos::RCP<const Thyra::VectorSpaceBase<ValueType> >
DiscreteAcaLinearOperator<ValueType>::
domain() const
{
    return m_domainSpace;
}

template <typename ValueType>
Teuchos::RCP<const Thyra::VectorSpaceBase<ValueType> >
DiscreteAcaLinearOperator<ValueType>::
range() const
{
    return m_rangeSpace;
}

template <typename ValueType>
bool
DiscreteAcaLinearOperator<ValueType>::
opSupportedImpl(Thyra::EOpTransp M_trans) const
{
    // TODO: implement remaining variants (transpose & conjugate transpose)
    return (M_trans == Thyra::NOTRANS);
}

template <typename ValueType>
void
DiscreteAcaLinearOperator<ValueType>::
applyImpl(const Thyra::EOpTransp M_trans,
          const Thyra::MultiVectorBase<ValueType>& X_in,
          const Teuchos::Ptr<Thyra::MultiVectorBase<ValueType> >& Y_inout,
          const ValueType alpha,
          const ValueType beta) const
{
    typedef Thyra::Ordinal Ordinal;

    // Note: the name is VERY misleading: these asserts don't disappear in
    // release runs, and in case of failure throw exceptions rather than
    // abort.
    TEUCHOS_ASSERT(X_in.range()->isCompatible(*this->domain()));
    TEUCHOS_ASSERT(Y_inout->range()->isCompatible(*this->range()));

    const Ordinal colCount = X_in.domain()->dim();

    // Loop over the input columns

    for (Ordinal col = 0; col < colCount; ++col) {
        // Get access the the elements of column col
        Thyra::ConstDetachedSpmdVectorView<ValueType> xVec(X_in.col(col));
        Thyra::DetachedSpmdVectorView<ValueType> yVec(Y_inout->col(col));
        const Teuchos::ArrayRCP<const ValueType> xArray(xVec.sv().values());
        const Teuchos::ArrayRCP<ValueType> yArray(yVec.sv().values());

        // Wrap the Trilinos array in an Armadillo vector. const_cast is used
        // because it's more natural to have a const arma::Col<ValueType> array
        // than an arma::Col<const ValueType> one.
        const arma::Col<ValueType> xCol(
                    const_cast<ValueType*>(xArray.get()), xArray.size(),
                    false /* copy_aux_mem */);
        arma::Col<ValueType> yCol(yArray.get(), yArray.size(), false);

        applyBuiltInImpl(static_cast<TranspositionMode>(M_trans),
                         xCol, yCol, alpha, beta);
    }
}
#endif // WITH_TRILINOS

template <typename ValueType>
void
DiscreteAcaLinearOperator<ValueType>::
applyBuiltInImpl(const TranspositionMode trans,
                 const arma::Col<ValueType>& x_in,
                 arma::Col<ValueType>& y_inout,
                 const ValueType alpha,
                 const ValueType beta) const
{
    if (trans != NO_TRANSPOSE)
        throw std::runtime_error(
                "DiscreteAcaLinearOperator::applyBuiltInImpl(): "
                "transposition modes other than NO_TRANSPOSE are not supported");
    if (columnCount() != x_in.n_rows && rowCount() != y_inout.n_rows)
        throw std::invalid_argument(
                "DiscreteAcaLinearOperator::applyBuiltInImpl(): "
                "incorrect vector length");

    if (beta == static_cast<ValueType>(0.))
        y_inout.fill(static_cast<ValueType>(0.));
    else
        y_inout *= beta;

    arma::Col<ValueType> permutedArgument;
    m_domainPermutation.permuteVector(x_in, permutedArgument);

    // const_cast because Ahmed internally calls BLAS
    // functions, which don't respect const-correctness
    arma::Col<ValueType> permutedResult;
    m_rangePermutation.permuteVector(y_inout, permutedResult);

    // Old serial version
//    std::cout << "----------------------------\nperm Arg\n" << permutedArgument;
//    std::cout << "perm Res\n" << permutedResult;
      multaHvec(ahmedCast(alpha), m_blockCluster.get(), m_blocks.get(),
                ahmedCast(permutedArgument.memptr()),
                ahmedCast(permutedResult.memptr()));
//     std::cout << "perm Res2\n" << permutedResult;

   // multaHvec_omp(ahmedCast(alpha), m_blockCluster.get(), m_blocks.get(),
   //           ahmedCast(permutedArgument.memptr()),
   //           ahmedCast(permutedResult.memptr()));

//    AhmedLeafClusterArray leafClusters(m_blockCluster.get());
//    leafClusters.sortAccordingToClusterSize();
//    const size_t leafClusterCount = leafClusters.size();

//    // TODO: make it possible to select number of threads
//    tbb::task_scheduler_init scheduler;
//    std::vector<ChunkStatistics> chunkStats(leafClusterCount);

//    typedef MblockMultiplicationLoopBody<ValueType> Body;
//    typename Body::LeafClusterIndexQueue leafClusterIndexQueue;
//    for (size_t i = 0; i < leafClusterCount; ++i)
//        leafClusterIndexQueue.push(i);

//    // std::cout << "----------------------------\nperm Arg\n" << permutedArgument;
//    // std::cout << "perm Res\n" << permutedResult;
//    Body body(alpha, permutedArgument, permutedResult,
//              leafClusters, m_blocks,
//              leafClusterIndexQueue, chunkStats);
//    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, leafClusterCount),
//                         body);
//    permutedResult = body.m_local_y;
//    // std::cout << "perm Res2\n" << permutedResult;

    m_rangePermutation.unpermuteVector(permutedResult, y_inout);
}

template <typename ValueType>
void
DiscreteAcaLinearOperator<ValueType>::
makeAllMblocksDense()
{
    for (int i = 0; i < m_blockCluster->nleaves(); ++i)
        if (m_blocks[i]->islwr())
            m_blocks[i]->conv_lwr_to_dns();
}

FIBER_INSTANTIATE_CLASS_TEMPLATED_ON_RESULT(DiscreteAcaLinearOperator);

} // namespace Bempp

#endif // WITH_AHMED
