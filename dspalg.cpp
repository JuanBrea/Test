/*---------------------------------------------------------------------------
                Copyright (C) 2013 Philips Medical Systems
                All Rights Reserved.  Proprietary and Confidential.

                File:           dspalg.cpp

                Description:    Implements the signal frequency spectrum analysis
---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "pwmutil.h"
#include "dspalg.h"

#define PI (3.14159265)
#define SCAL_15BIT	32768
#define MAX_S16 	32767

static i32 *wr=NULL, *wi=NULL, *xr=NULL, *xi=NULL;

/****************************************************************************
* Function:     fft
* Description:  Compute a complex signal FFT
* Parameters:	xr - real signal input
* 				xi - image signal input
* 				fft_len - FFT length
* Returns:
****************************************************************************/
static void fft(i32 *xr, i32 *xi, i16 fft_len)
{
	i16 angle_idx, angle_step, btfly_gap, bfly_num;
	i32 tmp_r, tmp_i, UR = 1, UI = 0;	// 0 degree cos and sin pair
	i16 half_len = fft_len/2;
	i16 i = 0, j = half_len, k = 0, LE = 0, LE2 = 0, ip=0;
	i16 stages = (((i16)(log(fft_len)/log(2))+1)>>1)<<1;

	/* Bit reversal sorting */
	for (i=1; i<fft_len-1; i++)
	{
		if (i < j)	// swap
		{
			tmp_r = xr[j];
			tmp_i = xi[j];
			xr[j] = xr[i];
			xi[j] = xi[i];
			xr[i] = tmp_r;
			xi[i] = tmp_i;
		}

		k = half_len;

		while (k <= j)
		{
			j = j-k;
			k >>= 1;
		}

		j = j+k;
	}

	/* For each synthesis stage */
	angle_step  = half_len;
	btfly_gap  = 1;

	for(k=0; k<stages; k++)
	{
		bfly_num  = btfly_gap;
		btfly_gap *= 2;

		angle_idx = 0;	// cos & sin index

		/* For each sub DFT */
		for(j=0; j<bfly_num; j++)
		{
			/* Cos and Sin pairs */
			UR = wr[angle_idx];
			UI = wi[angle_idx];

			/* For each butterfly */
			for(i=j; i<fft_len; i+=btfly_gap)
			{
				/* Butterfly Calculation */
				ip   = i + bfly_num;
				tmp_r = (xr[ip]*UR - xi[ip]*UI)/SCAL_15BIT;
				tmp_i = (xi[ip]*UR + xr[ip]*UI)/SCAL_15BIT;

				/* Cut in half to avoid 16-bit overflow */
				xr[ip] = (xr[i] - tmp_r)/2;
				xi[ip] = (xi[i] - tmp_i)/2;

				xr[i]  = (xr[i] + tmp_r)/2;
				xi[i]  = (xi[i] + tmp_i)/2;
			}

			/* cos/sin index hopper over */
			angle_idx += angle_step;
		}

		/* Angle gap between cos and sin pair */
		angle_step /= 2;
	}

	return;
}

/****************************************************************************
* Function:     RealFFT
* Description:  Compute a real signal FFT using two separate half length FFTs
* Parameters:	in_buf - real signal input
* 				fft_len - real signal length for FFT
* Returns:
****************************************************************************/
void RealFFT(i16 *in_buf, i16 fft_len)
{
	i16 half_len = fft_len/2, quad_len = fft_len/4;
	i32 UR = 0, UI = 0;
	i16 ir=0, k=0, kr=0;
	i16 tmp_r = 0, tmp_i = 0;

	/* Separate even and odd points and clear the rest of buffers */
	for (int i=0; i<half_len ; i++)
	{
		xr[i] = in_buf[2*i];
		xi[i] = in_buf[2*i+1];
	}
	memset(xr+half_len, 0, sizeof(i32)*half_len);
	memset(xi+half_len, 0, sizeof(i32)*half_len);

	/* Calculate half fft_len point 32-bit FFT */
	fft(xr, xi, half_len);

	/* Even/odd frequency domain decomposition
	 * Construct entire spectrum including image part */
	for (int i = 0; i<quad_len; i++)
	{
		/* Symmetric around half length point */
		k = i+half_len;
		ir = half_len-i;
		kr = ir+half_len;

		/* Calculate Symmetric part first before replacing the 1st half
		 * Symmetric for real part, reverse symmetric for image part around half point */
		xr[k] = (xi[i] + xi[ir])/2;
		xi[k] = -(xr[i] - xr[ir])/2;
		xr[kr] = xr[k];
		xi[kr] = -xi[k];

		xr[i] = (xr[i] + xr[ir])/2;
		xi[i] = (xi[i] - xi[ir])/2;
		xr[ir] = xr[i];
		xi[ir] = -xi[i];
	}

	/* Symmetric points construction */
	xr[quad_len*3] = xi[quad_len];
	xi[quad_len*3] = 0;
	xr[half_len] = xi[0];
	xi[half_len] = 0;
	xi[quad_len] = 0;
	xi[0] = 0;

	/* Complete the last FFT synthesis stage */
	for (int i=0, k=half_len; i<half_len; i++, k++)
	{
		UR = wr[i];	UI = wi[i];

		tmp_r = (xr[k]*UR - xi[k]*UI)/SCAL_15BIT;
		tmp_i = (xr[k]*UI + xi[k]*UR)/SCAL_15BIT;

		xr[k] = xr[i]-tmp_r;
		xi[k] = xi[i]-tmp_i;

		xr[i] = xr[i]+tmp_r;
		xi[i] = xi[i]+tmp_i;

		/* Only need to check out range for 1st half */
		if (xr[i] > MAX_S16)
			xr[i] = MAX_S16;
		else if (xr[i] < -SCAL_15BIT)
			xr[i] = -SCAL_15BIT;
		if (xi[i] > MAX_S16)
			xi[i] = MAX_S16;
		else if (xi[i] < -SCAL_15BIT)
			xi[i] = -SCAL_15BIT;

		/* Pass real and image parts together */
		in_buf[i] = xr[i];
		in_buf[i+half_len] = xi[i];
	}

	return;
}

/****************************************************************************
* Function:     setupFFT
* Description:  Setup FFT environment
* Parameters:	fft_len - FFT length
* Returns:
****************************************************************************/
int setupFFT(u16 fft_len)
{
	u16 half_len = fft_len/2;

	/* Buffer to hold real and image half length FFTs */
	xr = new i32[fft_len];
	xi = new i32[fft_len];

	wr = new i32[half_len];
	wi = new i32[half_len];

	if (!xr || !xi || !wr || !wi)
		return -1;

	/* Sin a Cos pairs */
	double omega = (double)PI/half_len;
	for(int i=0; i<half_len; i++)
	{
		wr[i] = (i32)SCAL_15BIT*cos(omega*i);
		wi[i] = -(i32)SCAL_15BIT*sin(omega*i);
	}

	return 0;
}

/****************************************************************************
* Function:     endFFT
* Description:  Clean up FFT environment
* Parameters:
* Returns:
****************************************************************************/
void endFFT()
{
	delete []wr;
	delete []wi;
	delete []xr;
	delete []xi;

	return;
}

/****************************************************************************
* Function:     getPowerSpect
* Description:  Calculate the Auto Power Spectrum
* Parameters: spect - signal frequency spectrum
*  			  spect_len - spectrum length
*			  gain - gain factor
*			  no_dc - DC factor deduction flag
* Returns:
****************************************************************************/
void getPowerSpect(i16 *spect, i16 spect_len, i16 gain, bool no_dc)
{
    i32 pwr_spect = 0;
    i16 *pspect = new i16[spect_len];

    pspect[0] = 0;	// don't need base frequency
    for (int i=1; i<spect_len; i++)
    {
       /* Compute the log(squared magnitude) */
       long re = spect[i];
       long im = spect[i+spect_len];

       pwr_spect = sqrt(re*re + im*im);
       pwr_spect >>= gain;
       if (pwr_spect > 32767)
       {
    	   printf("======= Spectrum Power overflow: spect len = %d,  re[%d] = %d, im[%d] = %d\n",
    			   spect_len, i, re, i+spect_len, im);
    	   pwr_spect = 32767;  /* Watch for possible overflow */
       }

       *(pspect+i) = pwr_spect;
    }

    if (no_dc) // 1st frequency is the DC factor to be removed
    	*pspect = 0;

    memcpy(spect, pspect, sizeof(i16)*spect_len);

    delete []pspect;
    return;
}

/****************************************************************************
* Function:     calBandPower
* Description:  Calculate the Power Spectrum Banded Power
* Parameters:   PSpect - signal frequency power spectrum
*				PSpectLen - power spectrum length
*				pwr_band_num - power spectrum band number
*				scale - scale factor to keep the result in range
* Returns:
****************************************************************************/
void calBandPower(u16 *PSpect, i16 PSpectLen, i16 pwr_band_num, i16 scale)
{
	i16 sp_lines = PSpectLen/pwr_band_num;
	i32 *SpectSum = new i32[pwr_band_num];

	memset(SpectSum, 0x0, sizeof(i32)*pwr_band_num);

	/* Get sum of all spectrum lines within a band */
	for (int i=0; i<pwr_band_num; i++)
	{
		for (int k=0; k<sp_lines; k++)
			SpectSum[i] += *(PSpect+i*sp_lines+k);

		SpectSum[i] >>= scale;
		if (SpectSum[i] > 0xFFFF)
		{
			printf("======= Error: Band Power[%d] overflow  = %d\n", i, SpectSum[i]);
			*(PSpect+i) = 0xFFFF;
		}
		else
			*(PSpect+i) = SpectSum[i];
	}

	delete []SpectSum;
	return;
}
