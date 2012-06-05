
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

#ifndef fiber_geometrical_data_hpp
#define fiber_geometrical_data_hpp

#include <armadillo>

namespace Fiber
{

enum GeometricalDataType
{
    GLOBALS = 0x0001,
    INTEGRATION_ELEMENTS = 0x0002,
    NORMALS = 0x0004,
    JACOBIANS_TRANSPOSED = 0x0008,
    JACOBIAN_INVERSES_TRANSPOSED = 0x0010
};

template <typename CoordinateType>
struct GeometricalData
{
    arma::Mat<CoordinateType> globals;
    arma::Row<CoordinateType> integrationElements;
    arma::Cube<CoordinateType> jacobiansTransposed;
    arma::Cube<CoordinateType> jacobianInversesTransposed;
    arma::Mat<CoordinateType> normals;
};

template <typename CoordinateType>
class ConstGeometricalDataSlice
{
public:
    ConstGeometricalDataSlice(const GeometricalData<CoordinateType>& geomData,
                              int point) :
        m_geomData(geomData), m_point(point) {}

    CoordinateType globals(int dim) {
        return m_geomData.globals(dim, m_point);
    }
    CoordinateType integrationElement() {
        return m_geomData.integrationElements(m_point);
    }
    CoordinateType jacobianTransposed(int row, int col) {
        return m_geomData.jacobiansTransposed(row, col, m_point);
    }
    CoordinateType jacobianInverseTransposed(int row, int col) {
        return m_geomData.jacobianInversesTransposed(row, col, m_point);
    }
    CoordinateType normal(int dim) {
        return m_geomData.normals(dim, m_point);
    }

private:
    const GeometricalData<CoordinateType>& m_geomData;
    int m_point;
};

} // namespace Fiber

#endif // GEOMETRY_DATA_TYPES_HPP
