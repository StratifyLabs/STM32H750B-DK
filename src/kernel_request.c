
#include <sapi/sys/requests.h>

#include <opus_api.h>
#define OPUS_API_IGNORE_FLOAT 1
#define OPUS_API_IGNORE_PCM_SOFT_CLIP 1
#define OPUS_API_IGNORE_REPACKETIZER 0
#define OPUS_API_IGNORE_MULTISTREAM 0
#define OPUS_API_IGNORE_ENCODER 0
#define OPUS_API_IGNORE_DECODER 0
#include <opus_api_declaration.h>

#include <arm_dsp_api.h>

//Delta sizes for Q31
//#define Q7 1
#define Q15 1
//#define Q31 1
//#define F32 1
#define ARM_DSP_API_IGNORE_MATRIX 0 //3KB
#define ARM_DSP_API_IGNORE_CONTROLLER 0 //<1KB
#define ARM_DSP_API_IGNORE_TRANSFORM 0 //266KB - COMPLEX: 1KB - DCT: 131KB - FFT: 135KB
#define ARM_DSP_API_IGNORE_FILTER 0 //17KB for Q31
#define ARM_DSP_API_IGNORE_FASTMATH 0 //1KB
#define ARM_DSP_API_IGNORE_COMPLEX 0 //2KB
#define ARM_DSP_API_IGNORE_DCT 1 //131KB
#define ARM_DSP_API_IGNORE_FFT 0 //135KB

//ALL in sizes: KB = 1000 bytes
//None: 339KB
//Q7: 348KB
//Q15: 508KB
//Q31: 634KB
//F32: 676KB
//All 4 types: 1127KB

#define ARM_DSP_API_IGNORE_CONVERSIONS 0 //1KB

#include <arm_dsp_api_declaration.h>

int kernel_request(int request, void * arg){

    return -1;
}
