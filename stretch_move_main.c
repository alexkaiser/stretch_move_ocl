/*
Copyright (c) 2013, Alex Kaiser
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer. Redistributions
 in binary form must reproduce the above copyright notice, this list
 of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "stretch_move_sampler.h"
#include "cl-helper.h"
#include "stretch_move_util.h"
#include "constants.h"
 

//    Stretch Move MCMC sampler in OpenCL
//    Alex Kaiser, Courant Institute, 2012
//    Email: user: adkaiser  domain: gmail

 
void example_simple();
void example_with_data();


int main(int argc, char **argv){

    // Simplest example of sampling possible
    example_simple();

    // Example of sampling a Gaussian where mean and covariance are passed to the function
    example_with_data();

    return 1;
}


void example_simple(){
    // Simple example of running the sampler.

    // User set parameters
    cl_int chain_length = 100000;                     // Allocate to store this much chain, sampler runs this many steps at once
    cl_int dimension = 10;                            // Dimension of the state vector
    cl_int walkers_per_group = 1024;                  // Total number of walkers is twice this
    size_t work_group_size = 128;                     // Work group size. Use 1 for CPU, larger number for GPU
    cl_int pdf_number = 0;                            // Use Gaussian debug problem
    cl_int data_length = 0;                           // No data for this example
    cl_float *data_temp = NULL;                       // Need to pass a NULL pointer for the data
    data_struct data_st;                              // Don't use this either, leave it uninitialized
    const char *plat_name = CHOOSE_INTERACTIVELY;     // Choose the device interactively at runtime
    const char *dev_name  = CHOOSE_INTERACTIVELY;     // Choose the platform interactively at runtime


    // initialize the sampler
    sampler *samp = initialize_sampler(chain_length, dimension, walkers_per_group, work_group_size,
                                        pdf_number, data_length, data_temp, data_st, plat_name, dev_name);

    // run burn-in for 10000 steps
    run_burn_in(samp, 10000);

    // run the sampler
    run_sampler(samp);

    // --------------------------------------------------------------------------
    // The array samp->samples_host now contains samples ready for use.
    //
    // Array is in component major order.
    // To access the i-th component of sample j use
    //     samp->samples_host[i + j*samp->N]
    //
    // Dimension is (samp->N x samp->total_samples)
    // --------------------------------------------------------------------------

    // free resources
    free_sampler(samp);
}




void example_with_data(){
    // Example of running the sampler.
    // Similar to above, but uses data
    // Also runs simulated annealing to accelerate convergence

    // User set parameters
    cl_int chain_length = 100000;                     // allocate to store this much chain, sampler runs this many steps at once
    cl_int dimension = 10 ;                           // dimension of the state vector
    cl_int walkers_per_group = 1024;                  // total number of walkers is twice this
    size_t work_group_size = 128;                     // Work group size. Use 1 for CPU, larger number for GPU
    cl_int pdf_number = 1;                            // Use pdf 1 for this problem
    const char *plat_name = CHOOSE_INTERACTIVELY;     // Choose the device interactively at runtime
    const char *dev_name  = CHOOSE_INTERACTIVELY;     // Choose the platform interactively at runtime
    data_struct data_st;                              // Don't use this, leave it uninitialized


    // Generate the mean and inverse covariance matrix
    // Pack mean first, then invrse covariance
    cl_int data_length = dimension + dimension*dimension;
    cl_float *data = (cl_float *) malloc(data_length * sizeof(cl_float)) ;

    for(int i=0; i < dimension; i++)
        data[i] = (cl_float) i;

    for(int i=dimension; i<data_length; i++){
        data[i] = 0.0f;
    }

    for(int i=0; i < (dimension-1); i++){
        data[ i   +     i*dimension + dimension ] =  2.0f;
        data[ i+1 +     i*dimension + dimension ] = -1.0f;
        data[ i   + (i+1)*dimension + dimension ] = -1.0f;
    }
    data[ (dimension-1) + (dimension-1)*dimension + dimension ] = 2.0f;

    // initialize the sampler
    sampler *samp = initialize_sampler(chain_length, dimension, walkers_per_group, work_group_size,
                                        pdf_number, data_length, data, data_st, plat_name, dev_name);


    // run burn-in
    int burn_length = 20000;
    run_burn_in(samp, burn_length);


    // run the sampler
    run_sampler(samp);

    // --------------------------------------------------------------------------
    // The array samp->samples_host now contains samples ready for use.
    //
    // Array is in component major order.
    // To access the i-th component of sample j use
    //     samp->samples_host[i + j*samp->N]
    //
    // Dimension is (samp->N x samp->total_samples)
    // --------------------------------------------------------------------------

    // print summary of the run including basic some statistics
    print_run_summary(samp);

    // run acor to estimate autocorrelation time
    run_acor(samp);

    // output some histograms to Matlab, don't output gnuplot
    char matlab_hist = 1, gnuplot_hist = 0;
    output_histograms(samp, matlab_hist, gnuplot_hist);

    // free resources
    free_sampler(samp);
}

