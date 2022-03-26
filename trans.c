/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, line ,tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
	if(M == 32)
		for (i = 0; i < N; i += 8)
			for(j = 0; j < N; j += 8)
				for(line = 0; line < 8; line++)
				{
					tmp1 = A[i + line][j];
					tmp2 = A[i + line][j + 1];
					tmp3 = A[i + line][j + 2];
					tmp4 = A[i + line][j + 3];
					tmp5 = A[i + line][j + 4];
					tmp6 = A[i + line][j + 5];
					tmp7 = A[i + line][j + 6];
					tmp8 = A[i + line][j + 7];
					B[j][i + line] = tmp1;
					B[j + 1][i + line] = tmp2;
					B[j + 2][i + line] = tmp3;
					B[j + 3][i + line] = tmp4;
					B[j + 4][i + line] = tmp5;
					B[j + 5][i + line] = tmp6;
					B[j + 6][i + line] = tmp7;
					B[j + 7][i + line] = tmp8;
				}
	else if(M == 64)
	{
		for (i = 0; i < N; i += 4)
			for(j = 0; j < N; j += 4)
				if(i != j)
					for(line = 0; line < 4; line++)
					{
						tmp1 = A[i + line][j];
						tmp2 = A[i + line][j + 1];
						tmp3 = A[i + line][j + 2];
						tmp4 = A[i + line][j + 3];
						B[j][i + line] = tmp1;
						B[j + 1][i + line] = tmp2;
						B[j + 2][i + line] = tmp3;
						B[j + 3][i + line] = tmp4;
					}
		for (i = 0; i < N; i += 4)
		{
			for(line = 0; line < 4; line++)
			{
				tmp1 = A[i + line][i];
				tmp2 = A[i + line][i + 1];
				tmp3 = A[i + line][i + 2];
				tmp4 = A[i + line][i + 3];
				B[i][i + line] = tmp1;
				B[i + 1][i + line] = tmp2;
				B[i + 2][i + line] = tmp3;
				B[i + 3][i + line] = tmp4;				
			}
		}
	}
	else
	{
		for (i = 0; i < 56; i += 8) // copy the up-left 56x64 matrix
			for(j = 0; j < 64; j += 8)
				for(line = 0; line < 8; line++)
				{
					tmp1 = A[j + line][i];
					tmp2 = A[j + line][i + 1];
					tmp3 = A[j + line][i + 2];
					tmp4 = A[j + line][i + 3];
					tmp5 = A[j + line][i + 4];
					tmp6 = A[j + line][i + 5];
					tmp7 = A[j + line][i + 6];
					tmp8 = A[j + line][i + 7];
					B[i][j + line] = tmp1;
					B[i + 1][j + line] = tmp2;
					B[i + 2][j + line] = tmp3;
					B[i + 3][j + line] = tmp4;
					B[i + 4][j + line] = tmp5;
					B[i + 5][j + line] = tmp6;
					B[i + 6][j + line] = tmp7;
					B[i + 7][j + line] = tmp8;
				}
		for (line = 0, i = 0; i < 64; i += 8) // copy the right 5x64 matrix
			for(j = 56; j < 61; j ++)
			{
				tmp1 = A[i][j + line];
				tmp2 = A[i + 1][j + line];
				tmp3 = A[i + 2][j + line];
				tmp4 = A[i + 3][j + line];
				tmp5 = A[i + 4][j + line];
				tmp6 = A[i + 5][j + line];
				tmp7 = A[i + 6][j + line];
				tmp8 = A[i + 7][j + line];
				B[j + line][i] = tmp1;
				B[j + line][i + 1] = tmp2;
				B[j + line][i + 2] = tmp3;
				B[j + line][i + 3] = tmp4;
				B[j + line][i + 4] = tmp5;
				B[j + line][i + 5] = tmp6;
				B[j + line][i + 6] = tmp7;
				B[j + line][i + 7] = tmp8;
			}
		for (i = 0; i < 56; i += 8) // copy the right 3x56 matrix
			for(j = 64; j < 67; j ++)
			{
				tmp1 = A[j + line][i];
				tmp2 = A[j + line][i + 1];
				tmp3 = A[j + line][i + 2];
				tmp4 = A[j + line][i + 3];
				tmp5 = A[j + line][i + 4];
				tmp6 = A[j + line][i + 5];
				tmp7 = A[j + line][i + 6];
				tmp8 = A[j + line][i + 7];
				B[i][j + line] = tmp1;
				B[i + 1][j + line] = tmp2;
				B[i + 2][j + line] = tmp3;
				B[i + 3][j + line] = tmp4;
				B[i + 4][j + line] = tmp5;
				B[i + 5][j + line] = tmp6;
				B[i + 6][j + line] = tmp7;
				B[i + 7][j + line] = tmp8;
			}
		for(i = 56; i < 61; i++) // copy the last 3x5 region
			for(j = 64; j < 67; j++)
				B[i][j] = A[j][i];
	}
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, tmp;

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; j++) {
			tmp = A[i][j];
			B[j][i] = tmp;
		}
	}    
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
	/* Register your solution function */
	registerTransFunction(transpose_submit, transpose_submit_desc); 

	/* Register any additional transpose functions */
	registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
	int i, j;

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; ++j) {
			if (A[i][j] != B[j][i]) {
				return 0;
			}
		}
	}
	return 1;
}
