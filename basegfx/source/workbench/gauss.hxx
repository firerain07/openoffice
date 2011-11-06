/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



/** This method eliminates elements below main diagonal in the given
    matrix by gaussian elimination. 

    @param matrix
    The matrix to operate on. Last column is the result vector (right
    hand side of the linear equation). After successful termination,
    the matrix is upper triangular. The matrix is expected to be in
    row major order.

    @param rows
    Number of rows in matrix

    @param cols
    Number of columns in matrix

    @param minPivot
    If the pivot element gets lesser than minPivot, this method fails,
    otherwise, elimination succeeds and true is returned.

    @return true, if elimination succeeded.
 */
template <class Matrix, typename BaseType> 
bool eliminate( 	Matrix&			matrix,
                    int				rows,
                    int				cols,
                    const BaseType& minPivot	)
{
	BaseType	temp;
	int			max, i, j, k;	/* *must* be signed, when looping like: j>=0 ! */

	/* eliminate below main diagonal */
	for(i=0; i<cols-1; ++i)
	{
		/* find best pivot */
		max = i;
		for(j=i+1; j<rows; ++j)
			if( fabs(matrix[ j*cols + i ]) > fabs(matrix[ max*cols + i ]) ) 
				max = j;

		/* check pivot value */
		if( fabs(matrix[ max*cols + i ]) < minPivot )
			return false;	/* pivot too small! */

		/* interchange rows 'max' and 'i' */
		for(k=0; k<cols; ++k)
		{
			temp = matrix[ i*cols + k ];
			matrix[ i*cols + k ] = matrix[ max*cols + k ];
			matrix[ max*cols + k ] = temp;
		}
		
		/* eliminate column */
		for(j=i+1; j<rows; ++j)
			for(k=cols-1; k>=i; --k)
				matrix[ j*cols + k ] -= matrix[ i*cols + k ] * 
					matrix[ j*cols + i ] / matrix[ i*cols + i ];
	}

	/* everything went well */
	return true;
}


/** Retrieve solution vector of linear system by substituting backwards.

	This operation _relies_ on the previous successful
    application of eliminate()!

    @param matrix
    Matrix in upper diagonal form, as e.g. generated by eliminate()

    @param rows
    Number of rows in matrix

    @param cols
    Number of columns in matrix

    @param result
    Result vector. Given matrix must have space for one column (rows entries).

    @return true, if back substitution was possible (i.e. no division
    by zero occured).
 */
template <class Matrix, class Vector, typename BaseType> 
bool substitute(	const Matrix&	matrix,
                    int				rows,
                    int				cols,
                    Vector&			result	)
{
	BaseType	temp;
	int			j,k;	/* *must* be signed, when looping like: j>=0 ! */

	/* substitute backwards */
	for(j=rows-1; j>=0; --j)
	{
		temp = 0.0;
		for(k=j+1; k<cols-1; ++k)
			temp += matrix[ j*cols + k ] * result[k];

		if( matrix[ j*cols + j ] == 0.0 )
			return false;	/* imminent division by zero! */

		result[j] = (matrix[ j*cols + cols-1 ] - temp) / matrix[ j*cols + j ];
	}

	/* everything went well */
	return true;
}


/** This method determines solution of given linear system, if any

	This is a wrapper for eliminate and substitute, given matrix must
	contain right side of equation as the last column.

    @param matrix
    The matrix to operate on. Last column is the result vector (right
    hand side of the linear equation). After successful termination,
    the matrix is upper triangular. The matrix is expected to be in
    row major order.

    @param rows
    Number of rows in matrix

    @param cols
    Number of columns in matrix

    @param minPivot
    If the pivot element gets lesser than minPivot, this method fails,
    otherwise, elimination succeeds and true is returned.

    @return true, if elimination succeeded.
 */
template <class Matrix, class Vector, typename BaseType> 
bool solve(	Matrix&		matrix,
            int			rows,
            int			cols,
            Vector&		result,
            BaseType	minPivot	)
{
	if( eliminate<Matrix,BaseType>(matrix, rows, cols, minPivot) )
		return substitute<Matrix,Vector,BaseType>(matrix, rows, cols, result);

	return false;
}
