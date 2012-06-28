/*
 * Copyright (C) 2008, 2009, 2010 Richard Membarth <richard.membarth@cs.fau.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Additional permission under GNU GPL version 3 section 7
 *
 * If you modify this Program, or any covered work, by linking or combining it
 * with NVIDIA CUDA Software Development Kit (or a modified version of that
 * library), containing parts covered by the terms of a royalty-free,
 * non-exclusive license, the licensors of this Program grant you additional
 * permission to convey the resulting work.
 */

#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimp_main.hpp"
#include "gimp_gui.hpp"
#include "defines_cpu.hpp"

#define MAX(a, b) (((a)<(b))?(b):(a))
#define MIN(a, b) (((a)<(b))?(a):(b))
#define THRESH 50000.0

#ifdef PRINT_TIMES
#include <cutil.h>
#endif

void average_blur(int width, int height, guchar *image);

void compute_xgrad(int width, int height, char *x_component, guchar *image);
void compute_ygrad(int width, int height, char *y_component, guchar *image);

void gradient_magnitude(int width, int height, char *x_component, char *y_component, guchar *image);

void gradient_matrix(int width, int height, char *x_component, char *y_component, double *xx_grad, double *xy_grad, double *yy_grad);

void harris_corner(int width, int height, double *harris, double *xx_grad, double *xy_grad, double *yy_grad);

void nonmax_suppression(int width, int height, double *harris);

void threshold(int width, int height, guchar *image, double *harris);

// static variables
static float progress = 0.0f;
static float complexity = 0.0f;
static float num_col = 0;

void update_progress(float factor) {
    num_col += factor;
    while (num_col >= complexity) {
        progress += 0.01f;
        num_col -= complexity;
        gimp_progress_update(progress);
    }
}


////////////////////////////////////////////////////////////////////////////////
//! Run the multiresolution filter on the CPU
////////////////////////////////////////////////////////////////////////////////
void run_cpu(guchar *image, int width, int height, int channels) 
{
	guchar *img = (guchar *)malloc(width * height * sizeof(guchar));

	memcpy(img, image, (size_t)(width * height * sizeof(guchar)));

	average_blur(width, height, img);

	char *x_component = (char *)malloc(width * height * sizeof(char));
 	char *y_component = (char *)malloc(width * height * sizeof(char));

	double *xx_grad = (double *)malloc(width * height * sizeof(double));
	double *xy_grad = (double *)malloc(width * height * sizeof(double));
	double *yy_grad = (double *)malloc(width * height * sizeof(double));

	double *harris = (double *)malloc(width * height * sizeof(double));

	memcpy(x_component, img, (size_t)(width * height * sizeof(char)));
	memcpy(y_component, img, (size_t)(width * height * sizeof(char)));

	compute_xgrad(width, height, x_component, img);
	compute_ygrad(width, height, y_component, img);

	//gradient_magnitude(width, height, x_component, y_component, image);	
	
	gradient_matrix(width, height, x_component, y_component, xx_grad, xy_grad, yy_grad);

	harris_corner(width, height, harris, xx_grad, xy_grad, yy_grad);

	nonmax_suppression(width, height, harris);

	threshold(width, height, image, harris);

/*	int i, j;

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			printf("%f ", harris[i * width + j]);
		}
		printf("\n");
	}*/
}

void average_blur(int width, int height, guchar *image)
{
	int x, y,sum;
	int i, j;

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			sum = 0;
			int count = 0;
			for (y = MAX(0, i-1); y < MIN(height-1, i+1); y++) 
			{
				for (x = MAX(0,j-1); x < MIN(width - 1, j+1); x++) 
				{

					sum += image[y * width + x]; // top left
					count++;
				}
			}
			
			image[i * width + j] = sum / count;  // Average the values for the blur 
			
		}
	}
}

void compute_xgrad(int width, int height, char *x_component, guchar *image)
{
	int x, y, sum;
	
	for(y = 1; y < height - 1; y++)
	{
		for(x = 1; x < width - 1; x++)
		{
			sum = -1 * image[(y - 1) * width + (x - 1)] +
				   image[(y - 1) * width + (x + 1)] + 
				   (-2 * image[y * width + (x - 1)]) + 
				   (2 * image[y * width + (x + 1)]) + 
				   (1 * image[(y + 1) * width + (x - 1)]) + 
				   (-1 * image[(y + 1) * width + (x + 1)]);

			x_component[y * width + x] = sum;	
		}
	}
}

void compute_ygrad(int width, int height, char *y_component, guchar *image)
{
	int x, y, sum;
	int upper_row, middle_row, lower_row;

	for (y = 1; y < height - 1; y++) 
	{
		for (x = 1; x < width - 1; x++) 
		{
			sum =	-1 * image[(y - 1) * width + (x - 1)] + 
				 	-2 * image[(y - 1) * width + x] + 
					-1 * image[(y - 1) * width + (x + 1)] + 
					image[(y + 1) * width + (x - 1)] +  
					2 * image[(y + 1) * width + x] + 
					image[(y + 1) * width + (x + 1)]; 

			y_component[y * width + x] = sum;  
		}
	}
}


void gradient_magnitude(int width, int height, char *x_component, char *y_component, guchar *image)
{
	int x, y;
	
	for (y = 1; y < height - 1; y++) 
	{
		for (x = 1; x < width - 1; x++) 
		{
			image[y * width + x] = sqrt(x_component[y * width + x] * x_component[y * width + x] +
								   y_component[y * width + x] * y_component[y * width + x]);  
		}
	}

}


void gradient_matrix(int width, int height, char *x_component, char *y_component, double *xx_grad, double *xy_grad, double *yy_grad)
{
	int x, y;
	int i, j;

	double xx_sum, xy_sum, yy_sum;

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			xx_sum = 0;
			xy_sum = 0;
			yy_sum = 0;

			for (y = MAX(0, i-1); y < MIN(height-1, i+1); y++) 
			{
				for (x = MAX(0,j-1); x < MIN(width - 1, j+1); x++) 
				{
					xx_sum += x_component[y * width + x] * x_component[y * width + x];
					xy_sum += x_component[y * width + x] * y_component[y * width + x];
					yy_sum += y_component[y * width + x] * y_component[y * width + x];
				}
			}

			xx_grad[i * width + j] = xx_sum;
			xy_grad[i * width + j] = xy_sum;
			yy_grad[i * width + j] = yy_sum;
		}
	}
}

void harris_corner(int width, int height, double *harris, double *xx_grad, double *xy_grad, double *yy_grad)
{
	int x, y;
	int i, j;

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			double det = xx_grad[i * width + j] * yy_grad[i * width + j] - (xy_grad[i * width + j] * xy_grad[i * width + j]);
			double trace = xx_grad[i * width + j] + yy_grad[i * width + j];
	
			harris[i * width + j] = det - 0.06 * (trace * trace);
		}
	}
}


void nonmax_suppression(int width, int height, double *harris)
{
	int i, j, x, y;

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			double curr = harris[i * width + j];

			if(harris[i * width + j] < 0.0)
			{
				harris[i * width + j] = 0.0;
			}

			for (y = MAX(0, i-2); y < MIN(height-1, i+2); y++) 
			{
				for (x = MAX(0,j-2); x < MIN(width - 1, j+2); x++) 
				{
					if(curr > harris[y * width + x])
					{
						harris[y * width + x] = 0;
					}
					else
					{
						curr = harris[y * width + x];
					}
				}
			}
		}
	}
}


void threshold(int width, int height, guchar *image, double *harris)
{
	int i, j;

	for(i = 0; i < height; i++)
	{
		for(j = 0; j < width; j++)
		{
			if(harris[i * width + j] > THRESH)
			{
				image[i * width + j] = 0.0;
			}
		}
	}
}
