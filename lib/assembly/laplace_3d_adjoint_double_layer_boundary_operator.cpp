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

#include "laplace_3d_adjoint_double_layer_boundary_operator.hpp"

#include "blas_quadrature_helper.hpp"
#include "context.hpp"
#include "general_elementary_singular_integral_operator_imp.hpp"
#include "laplace_3d_synthetic_boundary_operator_builder.hpp"

#include "../fiber/explicit_instantiation.hpp"

#include "../fiber/typical_test_scalar_kernel_trial_integral.hpp"
#include "../fiber/laplace_3d_adjoint_double_layer_potential_kernel_functor.hpp"
#include "../fiber/scalar_function_value_functor.hpp"
#include "../fiber/simple_test_scalar_kernel_trial_integrand_functor.hpp"

namespace Bempp
{

template <typename BasisFunctionType, typename ResultType>
BoundaryOperator<BasisFunctionType, ResultType>
laplace3dAdjointDoubleLayerBoundaryOperator(
       const shared_ptr<const Context<BasisFunctionType, ResultType> >& context,
       const shared_ptr<const Space<BasisFunctionType> >& domain,
       const shared_ptr<const Space<BasisFunctionType> >& range,
       const shared_ptr<const Space<BasisFunctionType> >& dualToRange,
       const std::string& label,
       int symmetry)
{
    const AssemblyOptions& assemblyOptions = context->assemblyOptions();
    if (assemblyOptions.assemblyMode() == AssemblyOptions::ACA &&
        assemblyOptions.acaOptions().mode == AcaOptions::LOCAL_ASSEMBLY)
        return laplace3dSyntheticBoundaryOperator(
            &laplace3dAdjointDoubleLayerBoundaryOperator<BasisFunctionType, ResultType>,
            context, domain, range, dualToRange, label, symmetry,
            NO_SYMMETRY);

    typedef typename ScalarTraits<BasisFunctionType>::RealType KernelType;
    typedef typename ScalarTraits<BasisFunctionType>::RealType CoordinateType;

    typedef Fiber::Laplace3dAdjointDoubleLayerPotentialKernelFunctor<KernelType>
    KernelFunctor;
    typedef Fiber::ScalarFunctionValueFunctor<CoordinateType>
    TransformationFunctor;
    typedef Fiber::SimpleTestScalarKernelTrialIntegrandFunctorExt<
    BasisFunctionType, KernelType, ResultType, 1> IntegrandFunctor;

    shared_ptr<Fiber::TestKernelTrialIntegral<
            BasisFunctionType, KernelType, ResultType> > integral;
    if (shouldUseBlasInQuadrature(assemblyOptions, *domain, *dualToRange))
        integral.reset(new Fiber::TypicalTestScalarKernelTrialIntegral<
                       BasisFunctionType, KernelType, ResultType>());
    else
        integral.reset(new Fiber::DefaultTestKernelTrialIntegral<IntegrandFunctor>(
                           IntegrandFunctor()));

    typedef GeneralElementarySingularIntegralOperator<
            BasisFunctionType, KernelType, ResultType> Op;
    shared_ptr<Op> newOp(new Op(
                             domain, range, dualToRange, label, symmetry,
                             KernelFunctor(),
                             TransformationFunctor(),
                             TransformationFunctor(),
                             integral));
    return BoundaryOperator<BasisFunctionType, ResultType>(context, newOp);
}

#define INSTANTIATE_NONMEMBER_CONSTRUCTOR(BASIS, RESULT) \
   template BoundaryOperator<BASIS, RESULT> \
   laplace3dAdjointDoubleLayerBoundaryOperator( \
       const shared_ptr<const Context<BASIS, RESULT> >&, \
       const shared_ptr<const Space<BASIS> >&, \
       const shared_ptr<const Space<BASIS> >&, \
       const shared_ptr<const Space<BASIS> >&, \
       const std::string&, int)
FIBER_ITERATE_OVER_BASIS_AND_RESULT_TYPES(INSTANTIATE_NONMEMBER_CONSTRUCTOR);

} // namespace Bempp
