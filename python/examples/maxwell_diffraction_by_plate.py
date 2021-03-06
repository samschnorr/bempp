#!/usr/bin/env python

# This script calculates the field generated by a z-polarised, x-propagating
# plane wave incident on a perfectly conducting object.

# Help Python find the bempp module
import sys,os
sys.path.append("..")

from bempp.lib import *
import numpy as np

# Parameters

wavelength = 0.5
k = 2 * np.pi / wavelength

# Boundary conditions

def evalDirichletIncData(point, normal):
    field = evalIncField(point)
    result = np.cross(field, normal, axis=0)
    return result

def evalIncField(point):
    x, y, z = point
    field = np.array([np.exp(-1j * k * z), y * 0., z * 0.])
    return field

# Load mesh

grid = createGridFactory().createStructuredGrid(
    'triangular', (-2, -1), (2, 1), (60, 30))

# Create quadrature strategy

accuracyOptions = createAccuracyOptions()
# Increase by 2 the order of quadrature rule used to approximate
# integrals of regular functions on pairs on elements
accuracyOptions.doubleRegular.setRelativeQuadratureOrder(2)
# Increase by 2 the order of quadrature rule used to approximate
# integrals of regular functions on single elements
accuracyOptions.singleRegular.setRelativeQuadratureOrder(2)
quadStrategy = createNumericalQuadratureStrategy(
    "float64", "complex128", accuracyOptions)

# Create assembly context

assemblyOptions = createAssemblyOptions()
assemblyOptions.switchToAcaMode(createAcaOptions())
context = createContext(quadStrategy, assemblyOptions)

# Initialize spaces

space = createRaviartThomas0VectorSpace(context, grid)

# Construct elementary operators

slpOp = createMaxwell3dSingleLayerBoundaryOperator(
    context, space, space, space, k, "SLP")
dlpOp = createMaxwell3dDoubleLayerBoundaryOperator(
    context, space, space, space, k, "DLP")
idOp = createMaxwell3dIdentityOperator(
    context, space, space, space, "Id")

# Form the left- and right-hand-side operators

lhsOp = slpOp
rhsOp = -idOp

# Construct the grid function representing the (input) Dirichlet data

dirichletIncData = createGridFunction(
    context, space, space, evalDirichletIncData, True)
dirichletData = -dirichletIncData

# Construct the right-hand-side grid function

rhs = rhsOp * dirichletData

# Initialize the solver

invLhsOp = acaOperatorApproximateLuInverse(
    lhsOp.weakForm().asDiscreteAcaBoundaryOperator(), 1e-2)
prec = discreteOperatorToPreconditioner(invLhsOp)
solver = createDefaultIterativeSolver(lhsOp)
solver.initializeSolver(defaultGmresParameterList(1e-8), prec)
# solver = createDefaultDirectSolver(lhsOp)

# Solve the equation

solution = solver.solve(rhs)
print solution.solverMessage()

# Extract the solution in the form of a grid function

neumannData = solution.gridFunction()
neumannData.exportToVtk("cell_data", "flat-sol", "flat-sol")

# Create potential operators

slPotOp = createMaxwell3dSingleLayerPotentialOperator(context, k)
dlPotOp = createMaxwell3dDoubleLayerPotentialOperator(context, k)

# Create a grid of points

nPointsX = 201
nPointsZ = nPointsX
x, y, z = np.mgrid[-5:5:nPointsX*1j, 0:0:1j, -5:5:nPointsZ*1j]
points = np.vstack((x.ravel(), y.ravel(), z.ravel()))

# Use Green's representation formula to evaluate the total field

evaluationOptions = createEvaluationOptions()
scatteredField = -(slPotOp.evaluateAtPoints(neumannData, points,
                                            evaluationOptions))
incidentField = evalIncField(points)
field = scatteredField + incidentField

# Display the field plot

from bempp import visualization2 as vis
tvtkField = vis.tvtkStructuredGridData(points, field, (nPointsX, nPointsZ))
tvtkGrid = vis.tvtkGrid(grid)
vis.plotVectorData(tvtkGrids=tvtkGrid, tvtkStructuredGridData=tvtkField)

# from bempp import visualization as vis
# uActor = vis.scalarDataOnRegularGridActor(
#         points, fieldMagnitude, (nPointsX, nPointsZ),
#         colorRange=(0, 2))
# legendActor = vis.legendActor(uActor)
# gridActor = vis.gridActor(grid)
# vis.plotTvtkActors([uActor, gridActor, legendActor])

# Export the results into a VTK file

from tvtk.api import write_data
write_data(tvtkField, "u.vts")
