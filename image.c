#include "image.h"

#define GET_R(px) (((px) & 0xFF0000) >> 16)
#define GET_G(px) (((px) & 0xFF00) >> 8)
#define GET_B(px) ((px) & 0xFF)

void blockFilter(float constant, Uint32* buf, int w, int h)
{
    Uint32* blurred = (Uint32*) malloc(w * h * sizeof(Uint32));
    const Uint32 BLACK = 0xFF000000;        //opaque black
    //write the blurred pixels into the new buffer
    bool fadeIntoBlack = false;
    for(int i = 0; i < w; i++)
    {
        for(int j = 0; j < h; j++)
        {
            float rsum = 0;
            float gsum = 0;
            float bsum = 0;
            float totalWeight = 0;  //want total weight to be 1 after adding all neighbors and self
            if(i > 0)
            {
                if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
                {
                    rsum += constant * GET_R(buf[i - 1 + w * j]);
                    gsum += constant * GET_G(buf[i - 1 + w * j]);
                    bsum += constant * GET_B(buf[i - 1 + w * j]);
                    totalWeight += constant;
                }
            }
            if(i < w - 1)
            {
                if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
                {
                    rsum += constant * GET_R(buf[i + 1 + w * j]);
                    gsum += constant * GET_G(buf[i + 1 + w * j]);
                    bsum += constant * GET_B(buf[i + 1 + w * j]);
                    totalWeight += constant;
                }
            }
            if(j > 0)
            {
                if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
                {
                    rsum += constant * GET_R(buf[i + w * (j - 1)]);
                    gsum += constant * GET_G(buf[i + w * (j - 1)]);
                    bsum += constant * GET_B(buf[i + w * (j - 1)]);
                    totalWeight += constant;
                }
            }
            if(j < h - 1)
            {
                if(!fadeIntoBlack || buf[i - 1 + w * j] != BLACK)
                {
                    rsum += constant * GET_R(buf[i + w * (j + 1)]);
                    gsum += constant * GET_G(buf[i + w * (j + 1)]);
                    bsum += constant * GET_B(buf[i + w * (j + 1)]);
                    totalWeight += constant;
                }
            }
            rsum += (1.0f - totalWeight) * GET_R(buf[i + w * j]);
            gsum += (1.0f - totalWeight) * GET_G(buf[i + w * j]);
            bsum += (1.0f - totalWeight) * GET_B(buf[i + w * j]);
            //now [rgb]sum have the final colok
            //clamp to [0, 0x100)
            rsum = min(rsum, 0xFF);
            gsum = min(gsum, 0xFF);
            bsum = min(bsum, 0xFF);
            rsum = max(rsum, 0);
            gsum = max(gsum, 0);
            bsum = max(bsum, 0);
            blurred[i + j * w] = (unsigned) rsum << 24 | (unsigned) gsum << 16 | (unsigned) bsum << 8 | 0xFF;
        }
    }
    //copy blurred pixels back to original buffer
    memcpy(buf, blurred, w * h * sizeof(Uint32));
    free(blurred);
}

void reduceIters(int* iterbuf, int diffCap, int w, int h)
{
    bool changed;
    do
    {
        changed = false;
        for(int i = 1; i < w - 1; i++)
        {
            for(int j = 1; j < h - 1; j++)
            {
                int thisIter = iterbuf[i + j * w];
                if(thisIter == -1)
                    continue;
                int minNeighbor = thisIter;
                for(int ii = -1; ii <= 1; ii++)
                {
                    for(int jj = -1; jj <= 1; jj++)
                    {
                        int neighbor = iterbuf[(i + ii) + (j + jj) * w];
                        if(neighbor != -1 && neighbor < thisIter)
                            minNeighbor = neighbor;
                    }
                }
                if(thisIter - minNeighbor > diffCap)
                {
                    iterbuf[i + j * w] = minNeighbor + diffCap;
                    //changed = true;
                }
            }
        }
    }
    while(changed);
}
