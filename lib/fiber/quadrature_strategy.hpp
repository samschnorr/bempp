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

#ifndef fiber_integration_manager_factory_hpp
#define fiber_integration_manager_factory_hpp

#include "../common/common.hpp"

#include "scalar_traits.hpp"
#include "shared_ptr.hpp"

#include "../common/armadillo_fwd.hpp"
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <memory>

namespace Fiber
{
class ParallelisationOptions;
class OpenClHandler;

template <typename ValueType> class Basis;
template <typename CoordinateType> class CollectionOfBasisTransformations;
template <typename ValueType> class Function;
template <typename ValueType> class CollectionOfKernels;
template <typename BasisFunctionType, typename KernelType, typename ResultType>
class TestKernelTrialIntegral;
template <typename BasisFunctionType, typename KernelType, typename ResultType>
class KernelTrialIntegral;
template <typename CoordinateType> class RawGridGeometry;

template <typename ResultType> class LocalAssemblerForOperators;
template <typename ResultType> class LocalAssemblerForGridFunctions;
template <typename ResultType> class EvaluatorForIntegralOperators;

template <typename BasisFunctionType, typename ResultType,
          typename GeometryFactory>
class QuadratureStrategyBase
{
public:
    typedef typename ScalarTraits<ResultType>::RealType CoordinateType;

    virtual ~QuadratureStrategyBase() {}

    /** \brief Allocate a Galerkin-mode local assembler for an integral operator
        with real kernel. */
    std::auto_ptr<LocalAssemblerForOperators<ResultType> >
    makeAssemblerForIntegralOperators(
            const shared_ptr<const GeometryFactory>& testGeometryFactory,
            const shared_ptr<const GeometryFactory>& trialGeometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& testRawGeometry,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& trialRawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const CollectionOfKernels<CoordinateType> >& kernels,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const TestKernelTrialIntegral<BasisFunctionType, CoordinateType, ResultType> >& integral,
            const shared_ptr<const OpenClHandler>& openClHandler,
            const ParallelisationOptions& parallelisationOptions,
            bool cacheSingularIntegrals) const {
        return this->makeAssemblerForIntegralOperatorsImplRealKernel(
                    testGeometryFactory, trialGeometryFactory,
                    testRawGeometry, trialRawGeometry,
                    testBases, trialBases,
                    testTransformations, kernels, trialTransformations, integral,
                    openClHandler,
                    parallelisationOptions, cacheSingularIntegrals);
    }

    /** \brief Allocate a Galerkin-mode local assembler for the identity operator. */
    virtual std::auto_ptr<LocalAssemblerForOperators<ResultType> >
    makeAssemblerForIdentityOperators(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const OpenClHandler>& openClHandler) const = 0;

    /** \brief Allocate a local assembler for calculations of the projections
      of functions from a given space on a Fiber::Function. */
    std::auto_ptr<LocalAssemblerForGridFunctions<ResultType> >
    makeAssemblerForGridFunctions(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const Function<ResultType> >& function,
            const shared_ptr<const OpenClHandler>& openClHandler) const {
        return this->makeAssemblerForGridFunctionsImplRealUserFunction(
                    geometryFactory, rawGeometry, testBases,
                    testTransformations, function, openClHandler);
    }

    /** \brief Allocate an evaluator for an integral operator with real kernel
      applied to a grid function. */
    std::auto_ptr<EvaluatorForIntegralOperators<ResultType> >
    makeEvaluatorForIntegralOperators(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfKernels<CoordinateType> >& kernels,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const KernelTrialIntegral<BasisFunctionType, CoordinateType, ResultType> >& integral,
            const shared_ptr<const std::vector<std::vector<ResultType> > >& argumentLocalCoefficients,
            const shared_ptr<const OpenClHandler>& openClHandler) const {
        return this->makeEvaluatorForIntegralOperatorsImplRealKernel(
                    geometryFactory, rawGeometry, trialBases,
                    kernels, trialTransformations, integral, argumentLocalCoefficients,
                    openClHandler);
    }

private:
    virtual std::auto_ptr<LocalAssemblerForOperators<ResultType> >
    makeAssemblerForIntegralOperatorsImplRealKernel(
            const shared_ptr<const GeometryFactory>& testGeometryFactory,
            const shared_ptr<const GeometryFactory>& trialGeometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& testRawGeometry,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& trialRawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const CollectionOfKernels<CoordinateType> >& kernel,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const TestKernelTrialIntegral<BasisFunctionType, CoordinateType, ResultType> >& integral,
            const shared_ptr<const OpenClHandler>& openClHandler,
            const ParallelisationOptions& parallelisationOptions,
            bool cacheSingularIntegrals) const = 0;

    virtual std::auto_ptr<LocalAssemblerForGridFunctions<ResultType> >
    makeAssemblerForGridFunctionsImplRealUserFunction(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const Function<CoordinateType> >& function,
            const shared_ptr<const OpenClHandler>& openClHandler) const = 0;

    virtual std::auto_ptr<EvaluatorForIntegralOperators<ResultType> >
    makeEvaluatorForIntegralOperatorsImplRealKernel(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfKernels<CoordinateType> >& kernel,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const KernelTrialIntegral<BasisFunctionType, CoordinateType, ResultType> >& integral,
            const shared_ptr<const std::vector<std::vector<ResultType> > >& argumentLocalCoefficients,
            const shared_ptr<const OpenClHandler>& openClHandler) const = 0;
};

// complex ResultType
template <typename BasisFunctionType, typename ResultType,
          typename GeometryFactory, typename Enable = void>
class QuadratureStrategy :
        public QuadratureStrategyBase<BasisFunctionType, ResultType, GeometryFactory>
{
    typedef QuadratureStrategyBase<BasisFunctionType, ResultType, GeometryFactory> Base;
public:
    typedef typename Base::CoordinateType CoordinateType;

    using Base::makeAssemblerForIntegralOperators;
    using Base::makeAssemblerForGridFunctions;
    using Base::makeEvaluatorForIntegralOperators;

    /** \brief Allocate a Galerkin-mode local assembler for an integral operator
        with complex kernel. */
    std::auto_ptr<LocalAssemblerForOperators<ResultType> >
    makeAssemblerForIntegralOperators(
            const shared_ptr<const GeometryFactory>& testGeometryFactory,
            const shared_ptr<const GeometryFactory>& trialGeometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& testRawGeometry,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& trialRawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const CollectionOfKernels<ResultType> >& kernels,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const TestKernelTrialIntegral<BasisFunctionType, ResultType, ResultType> >& integral,
            const shared_ptr<const OpenClHandler>& openClHandler,
            const ParallelisationOptions& parallelisationOptions,
            bool cacheSingularIntegrals) const {
        return this->makeAssemblerForIntegralOperatorsImplComplexKernel(
                    testGeometryFactory, trialGeometryFactory,
                    testRawGeometry, trialRawGeometry,
                    testBases, trialBases,
                    testTransformations, kernels, trialTransformations, integral,
                    openClHandler,
                    parallelisationOptions, cacheSingularIntegrals);
    }

    /** \brief Allocate a local assembler for calculations of the projections
      of complex-valued functions from a given space on a Fiber::Function. */
    std::auto_ptr<LocalAssemblerForGridFunctions<ResultType> >
    makeAssemblerForGridFunctions(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const Function<ResultType> >& function,
            const shared_ptr<const OpenClHandler>& openClHandler) const {
        return this->makeAssemblerForGridFunctionsImplComplexUserFunction(
                    geometryFactory, rawGeometry, testBases,
                    testTransformations, function, openClHandler);
    }

    /** \brief Allocate an evaluator for an integral operator with
      complex-valued kernel applied to a grid function. */
    std::auto_ptr<EvaluatorForIntegralOperators<ResultType> >
    makeEvaluatorForIntegralOperators(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfKernels<ResultType> >& kernel,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const KernelTrialIntegral<BasisFunctionType, ResultType, ResultType> >& integral,
            const shared_ptr<const std::vector<std::vector<ResultType> > >& argumentLocalCoefficients,
            const shared_ptr<const OpenClHandler>& openClHandler) const {
        return this->makeEvaluatorForIntegralOperatorsImplComplexKernel(
                    geometryFactory, rawGeometry, trialBases,
                    kernel, trialTransformations, integral, argumentLocalCoefficients,
                    openClHandler);
    }

private:
    virtual std::auto_ptr<LocalAssemblerForOperators<ResultType> >
    makeAssemblerForIntegralOperatorsImplComplexKernel(
            const shared_ptr<const GeometryFactory>& testGeometryFactory,
            const shared_ptr<const GeometryFactory>& trialGeometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& testRawGeometry,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& trialRawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const CollectionOfKernels<ResultType> >& kernels,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const TestKernelTrialIntegral<BasisFunctionType, ResultType, ResultType> >& integral,
            const shared_ptr<const OpenClHandler>& openClHandler,
            const ParallelisationOptions& parallelisationOptions,
            bool cacheSingularIntegrals) const = 0;

    virtual std::auto_ptr<LocalAssemblerForGridFunctions<ResultType> >
    makeAssemblerForGridFunctionsImplComplexUserFunction(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& testBases,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& testTransformations,
            const shared_ptr<const Function<ResultType> >& function,
            const shared_ptr<const OpenClHandler>& openClHandler) const = 0;

    virtual std::auto_ptr<EvaluatorForIntegralOperators<ResultType> >
    makeEvaluatorForIntegralOperatorsImplComplexKernel(
            const shared_ptr<const GeometryFactory>& geometryFactory,
            const shared_ptr<const RawGridGeometry<CoordinateType> >& rawGeometry,
            const shared_ptr<const std::vector<const Basis<BasisFunctionType>*> >& trialBases,
            const shared_ptr<const CollectionOfKernels<ResultType> >& kernels,
            const shared_ptr<const CollectionOfBasisTransformations<CoordinateType> >& trialTransformations,
            const shared_ptr<const KernelTrialIntegral<BasisFunctionType, ResultType, ResultType> >& integral,
            const shared_ptr<const std::vector<std::vector<ResultType> > >& argumentLocalCoefficients,
            const shared_ptr<const OpenClHandler>& openClHandler) const = 0;
};

// real ResultType
template <typename BasisFunctionType, typename ResultType, typename GeometryFactory>
class QuadratureStrategy<BasisFunctionType, ResultType, GeometryFactory,
    typename boost::enable_if<boost::is_same<ResultType, typename ScalarTraits<ResultType>::RealType> >::type > :
        public QuadratureStrategyBase<BasisFunctionType, ResultType, GeometryFactory>
{
    typedef QuadratureStrategyBase<BasisFunctionType, ResultType, GeometryFactory> Base;
public:
    typedef typename Base::CoordinateType CoordinateType;
};

} // namespace Fiber

#endif