#include "mandelbrot.h"

Float screenX;
Float screenY;
Float width;
Float height;
Float pstride;      //size of a pixel
Float targetX;
Float targetY;
Uint32* pixbuf;
int* iterbuf;
Uint32* colortable;
int filecount;
int numThreads;
int maxiter;
int prec;

void initPositionVars()
{
    const long double startw = 4;
    const long double starth = 4 * winh / winw;
    screenX = floatLoad(1, -startw / 2);
    screenY = floatLoad(1, -starth / 2);
    width = floatLoad(1, startw);
    height = floatLoad(1, starth);
    pstride = floatLoad(1, startw / winw);  //pixels are always exactly square, no separate x and y
}

void increasePrecision()
{
    //reallocate space for the persistent Float variables
    //INCR_PREC macro copies significant words into reallocated space and updates mantissa size
    INCR_PREC(screenX);
    INCR_PREC(screenY);
    INCR_PREC(width);
    INCR_PREC(height);
    INCR_PREC(pstride);
    prec++;
}

void writeImage()
{
    char name[32];
    sprintf(name, "output/mandel%i.png", filecount++);
    for(int i = 0; i < winw * winh; i++)
    {
        Uint32 temp = pixbuf[i]; 
        pixbuf[i] = 0xFF000000 | (temp & 0xFF << 8) | (temp & 0xFF00 >> 8) | (temp & 0xFF0000 >> 0);
    }
    lodepng_encode32_file(name, (unsigned char*) pixbuf, winw, winh);
}

void zoom()
{
    /*
    //get the highest (non-convergent) iteration count
    //in the middle 1/9 of window area
    int bestX = 0;
    int bestY = 0;
    int bestIter = 0;
    int xStart = winw * (1 - zoomRange) / 2;
    int yStart = winh * (1 - zoomRange) / 2;
    int xRange = winw * zoomRange;
    int yRange = winh * zoomRange;
    for(int i = xStart; i < xStart + xRange; i++)
    {
        for(int j = yStart; j < yStart + yRange; j++)
        {
            if(iterbuf[i + j * winw] > bestIter)
            {
                bestX = i;
                bestY = j;
                bestIter = iterbuf[i + j * winw];
            }
        }
    }
    printf("Deepest pixel has %i iters.\n", bestIter);
    //now zoom to (bestX, bestY)
    //zoom in by factor of 1.5
    //get the decrease in width and height
    real dw = width * (1 - 1 / zoomFactor);
    real dh = height * (1 - 1 / zoomFactor);
    screenX += dw * ((real) bestX / winw);
    screenY += dh * ((real) bestY / winh);
    width -= dw;
    height -= dh;
    */
}

void zoomToTarget()
{
    //should zoom towards exact center of the screen
    //choose the deepest pixel in a 4x4 area in the center of the view
    MAKE_STACK_FLOAT(sizeFactor);
    MAKE_STACK_FLOAT(temp);
    storeFloatVal(&sizeFactor, 1.0 / zoomFactor);
    fcopy(&temp, &width);
    fmul(&width, &temp, &sizeFactor);
    fcopy(&temp, &height);
    fmul(&height, &temp, &sizeFactor);
    fcopy(&temp, &pstride);
    fmul(&pstride, &temp, &sizeFactor);
    //width / 2 --> scratch1
    fcopy(&temp, &width);
    temp.expo--;
    fsub(&screenX, &targetX, &temp);
    fcopy(&temp, &height);
    temp.expo--;
    fsub(&screenY, &targetY, &temp);
}

void initColorTable()
{
    for(int i = 0; i < numColors; i++)
    {
        int t = (i * 5) % 360;
        int r = 0;
        int g = 0;
        int b = 0;
        float slope = 255.0 / 120;
        if(t <= 120)
            r = 255 - slope * t;
        if(t >= 240)
            r = 255 - slope * (360 - t);
        if(t <= 240)
            g = 255 - slope * abs((t - 120) % 360);
        if(t >= 120)
            b = 255 - slope * abs((t - 240) % 360);
        colortable[i] = 0xFF000000 | r << 0 | g << 8 | b << 16;
    }
}

Uint32 getColor(int num)
{
    if(num == -1)
        return 0xFF000000;                  //in the mandelbrot set = opaque black
    return colortable[num % numColors];
}

int getConvRate(Float* real, Float* imag)
{
    //real, imag make up "c" in z = z^2 + c
    int iter = 0;
    u64* localbuf = (u64*) alloca(8 * prec * sizeof(u64));
    MAKE_FLOAT(zr, &localbuf[prec * 0]);
    storeFloatVal(&zr, 0);
    MAKE_FLOAT(zi, &localbuf[prec * 1]);
    storeFloatVal(&zi, 0);
    MAKE_FLOAT(zrsquare, &localbuf[prec * 2]);
    MAKE_FLOAT(zisquare, &localbuf[prec * 3]);
    MAKE_FLOAT(zri, &localbuf[prec * 4]);
    MAKE_FLOAT(zrtemp, &localbuf[prec * 5]);
    MAKE_FLOAT(zitemp, &localbuf[prec * 6]);
    MAKE_FLOAT(mag, &localbuf[prec * 7]);
    for(; iter < maxiter; iter++)
    {
        //printf("Doing iter %i\n", iter);
        //printf("zr = %LF\n", getFloatVal(&zr));
        //printf("zi = %LF\n", getFloatVal(&zi));
        fmul(&zrsquare, &zr, &zr);
        fmul(&zisquare, &zi, &zi);
        //printf("zrsquare = %Lf\n", getFloatVal(&zrsquare));
        //printf("zisquare = %Lf\n", getFloatVal(&zisquare));
        fadd(&mag, &zrsquare, &zisquare);
        //printf("mag = %Lf\n", getFloatVal(&mag));
        fmul(&zri, &zr, &zi);
        //printf("r*i = %Lf\n", getFloatVal(&zri));
        if(zri.expo)            //multiply by 2
            zri.expo++;
        fsub(&zrtemp, &zrsquare, &zisquare);
        //printf("zrtemp = %Lf\n", getFloatVal(&zrtemp));
        fadd(&zr, &zrtemp, real);
        fadd(&zi, &zri, imag);
        //compare mag to 4.0
        //Use fact that 4.0 is the smallest Float value with exponent 2, regardless of precision
        if((long long) mag.expo - expoBias >= 3)
            break;
    }
    return iter == maxiter ? -1 : iter;
}

int getConvRateLD(long double real, long double imag)
{
    //real, imag make up "c" in z = z^2 + c
    int iter = 0;
    long double zr = 0;
    long double zi = 0;
    for(; iter < maxiter; iter++)
    {
        //printf("Doing iter %i\n", iter);
        //printf("zr = %LF\n", getFloatVal(&zr));
        //printf("zi = %LF\n", getFloatVal(&zi));
        long double zrsquare = zr * zr;
        long double zisquare = zi * zi;
        long double zri = zr * zi;
        long double mag = zrsquare + zisquare;
        zri *= 2;
        long double zrtemp = zrsquare - zisquare;
        zr = zrtemp + real;
        zi = zri + imag;
        //compare mag to 4.0
        //Use fact that 4.0 is the smallest Float value with exponent 2, regardless of precision
        int expo;
        frexpl(mag, &expo);
        if((long long) expo >= 3)
            break;
    }
    return iter == maxiter ? -1 : iter;
}

void* workerFunc(void* indexAsInt)
{
    int numPixels = winw / numThreads;
    MAKE_STACK_FLOAT(index);
    MAKE_STACK_FLOAT(lowx);
    MAKE_STACK_FLOAT(xiter);    //keep track of real, imag parts corresponding to pixel position
    MAKE_STACK_FLOAT(yiter);
    MAKE_STACK_FLOAT(addtemp);  //need a temporary destination for iter += pstride
    MAKE_STACK_FLOAT(fpixelStart);
    storeFloatVal(&index, *((int*) indexAsInt));
    storeFloatVal(&fpixelStart, numPixels);
    fmul(&lowx, &pstride, &fpixelStart);
    fadd(&xiter, &screenX, &lowx);
    for(int i = 0; i < numPixels; i++)
    {
        fcopy(&yiter, &screenY);
        for(int j = 0; j < winh; j++)
        {
            int convRate = getConvRate(&xiter, &yiter);
            //int convRate = getConvRateLD(getFloatVal(&xiter), getFloatVal(&yiter)); 
            pixbuf[j * winw + i] = getColor(convRate);
            iterbuf[j * winw + i] = convRate;
            fcopy(&addtemp, &yiter);
            fadd(&yiter, &addtemp, &pstride);
        }
        fcopy(&addtemp, &xiter);
        fadd(&xiter, &addtemp, &pstride);
    }
    return NULL;
}

void drawBuf()
{
    if(numThreads == 1)
    {
        //directly run serial code in main thread
        MAKE_STACK_FLOAT(xiter);    //keep track of real, imag parts corresponding to pixel position
        MAKE_STACK_FLOAT(yiter);
        MAKE_STACK_FLOAT(addtemp);  //need a temporary destination for iter += pstride
        fcopy(&xiter, &screenX);
        for(int i = 0; i < winw; i++)
        {
            fcopy(&yiter, &screenY);
            for(int j = 0; j < winh; j++)
            {
                int convRate = getConvRate(&xiter, &yiter);
                //int convRate = getConvRateLD(getFloatVal(&xiter), getFloatVal(&yiter)); 
                pixbuf[j * winw + i] = getColor(convRate);
                iterbuf[j * winw + i] = convRate;
                fcopy(&addtemp, &yiter);
                fadd(&yiter, &addtemp, &pstride);
            }
            fcopy(&addtemp, &xiter);
            fadd(&xiter, &addtemp, &pstride);
        }
        return;
    }
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t) * numThreads);
    int* indices = (int*) malloc(sizeof(int) * numThreads);
    for(int i = 0; i < numThreads; i++)
    {
        indices[i] = i;
        if(pthread_create(&threads[i], NULL, workerFunc, indices + i))
        {
            puts("Fatal error: Failed to create thread.");
            exit(EXIT_FAILURE);
        }
    }
    for(int i = 0; i < numThreads; i++)
        pthread_join(threads[i], NULL);
    free(indices);
    free(threads);
}

void recomputeMaxIter()
{
    int numPixels = winw * winh;
    unsigned long long total = 0;
    int numColored = 0;
    for(int i = 0; i < numPixels; i++)
    {
        if(iterbuf[i] != -1)
        total += iterbuf[i];
        numColored++;
    }
    double avg = (double) total / numColored;
    maxiter = avg + 2000;
}

void getInterestingLocation(int minExpo)
{
    //make a temporary iteration count buffer
    int gilPrec = 1;
    int gpx = 32;                       //size, in pixels, of GIL iteration buffer (must be PoT)
    long double initViewport = 4;
    Float x = floatLoad(1, 0);          //real, imag of view center
    Float y = floatLoad(1, 0);
    Float gpstride = floatLoad(1, initViewport / gpx);
    int* escapeTimes = (int*) malloc(sizeof(int) * gpx * gpx);
    int zoomExpo = 3;               //log_2 of zoom factor 
    Float lowx = FloatCtor(1);
    Float lowy = FloatCtor(1);
    Float xiter = FloatCtor(1);
    Float yiter = FloatCtor(1);
    Float width = FloatCtor(1);
    Float temp = FloatCtor(1);
    Float fbestPX = FloatCtor(1);
    Float fbestPY = FloatCtor(1);
    maxiter = 200;
    while((long long) gpstride.expo - expoBias >= minExpo)
    {
        //compute viewport / 2 ("width")
        fcopy(&width, &gpstride);
        width.expo += 4;            //multiply by 16 (half of gpx)
        fsub(&lowx, &x, &width);
        fcopy(&xiter, &lowx);
        fsub(&lowy, &y, &width);
        for(int r = 0; r < gpx; r++)
        {
            fcopy(&yiter, &lowy);
            for(int i = 0; i < gpx; i++)
            {
                escapeTimes[r + i * gpx] = getConvRate(&xiter, &yiter);
                fcopy(&temp, &yiter);
                fadd(&yiter, &temp, &gpstride);
            }
            fcopy(&temp, &xiter);
            fadd(&xiter, &temp, &gpstride);
        }
        int bestPX;
        int bestPY;
        int bestIters = 0;
        u64 itersum = 0;
        for(int i = 0; i < gpx; i++)
        {
            for(int j = 0; j < gpx; j++)
            {
                itersum = escapeTimes[i + j * gpx];
                if(escapeTimes[i + j * gpx] > bestIters)
                {
                    bestPX = i;
                    bestPY = j;
                    bestIters = escapeTimes[i + j * gpx];
                }
            }
        }
        itersum /= (gpx * gpx); //compute average iter count
        maxiter = (itersum * 2 + bestIters) / 3 + 200;
        if(bestIters == 0)
        {
            puts("getInterestingLocation() stuck on window of converging points!");
            printf("Window width is %Le\n", getFloatVal(&width));
            puts("Decrease zoom factor or increase resolution.");
            break;
        }
        maxiter = bestIters + 100;
        storeFloatVal(&fbestPX, bestPX);
        storeFloatVal(&fbestPY, bestPY);
        fmul(&temp, &gpstride, &fbestPX);
        fadd(&x, &lowx, &temp);
        fmul(&temp, &gpstride, &fbestPY);
        fadd(&y, &lowy, &temp);
        //update gpstride by changing only exponent
        gpstride.expo -= zoomExpo;
        if(gilPrec < getPrec((long long) gpstride.expo - expoBias))
        {
            INCR_PREC(x);
            INCR_PREC(y);
            INCR_PREC(xiter);
            INCR_PREC(yiter);
            INCR_PREC(gpstride);
            INCR_PREC(temp);
            INCR_PREC(lowx);
            INCR_PREC(lowy);
            INCR_PREC(fbestPX);
            INCR_PREC(fbestPY);
            INCR_PREC(width);
            gilPrec++;
        }
    }
    free(escapeTimes);
    printf("Will zoom towards %.20Lf, %.20Lf\n", getFloatVal(&x), getFloatVal(&y));
    CHANGE_PREC(targetX, gilPrec);
    CHANGE_PREC(targetY, gilPrec);
    fcopy(&targetX, &x);
    fcopy(&targetY, &y);
}

int getPrec(int expo)
{
    return -expo / 80;
}

int main(int argc, const char** argv)
{
    staticPrecInit(100);
    prec = 1;
    getInterestingLocation(-200);
    maxiter = 500;
    colortable = (Uint32*) malloc(sizeof(Uint32) * numColors);
    initColorTable();
    initPositionVars();
    if(argc == 2)
        sscanf(argv[1], "%i", &numThreads);
    else
        numThreads = 4;
    printf("Running on %i thread(s).\n", numThreads);
    pixbuf = (Uint32*) malloc(sizeof(Uint32) * winw * winh);
    iterbuf = (int*) malloc(sizeof(int) * winw * winh);
    filecount = 0;
    puts("Ready to produce images.");
    for(int i = 0; i < 150; i++)
    {
        time_t start = time(NULL);
        drawBuf();
        writeImage();
        //zoom();
        zoomToTarget();
        recomputeMaxIter();
        if(maxiter > totalIter)
            maxiter = totalIter;
        int timeDiff = time(NULL) - start;
        if(getPrec(pstride.expo) > prec)
        {
            increasePrecision();
            printf("*** Increasing precision to %i bits. ***\n", 63 * prec);
        }
        printf("Generated image #%4i in %6i seconds.\n", filecount - 1, timeDiff);
    }
    free(iterbuf);
    free(pixbuf);
    free(colortable);
}
