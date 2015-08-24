/*  YUView - YUV player with advanced analytics toolset
*   Copyright (C) 2015  Institut für Nachrichtentechnik
*                       RWTH Aachen University, GERMANY
*
*   YUView is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   YUView is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with YUView.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "yuvfile.h"
#include <QFileInfo>
#include <QDir>
#include "math.h"
#include <cfloat>
#include <assert.h>
#if _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

// OVERALL CANDIDATE MODES, sieved more in each step
typedef struct {
 int width;
 int height;
 YUVCPixelFormatType pixelFormat;

 // flags set while checking
 bool   interesting;
 float mseY;
} candMode_t;

candMode_t candidateModes[] = {
    {176,144,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {352,240,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {352,288,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {480,480,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {480,576,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {704,480,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {720,480,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {704,576,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {720,576,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1024,768,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1280,720,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1280,960,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1920,1072,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1920,1080,YUVC_420YpCbCr8PlanarPixelFormat, false, 0.0 },
    {176,144,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {352,240,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {352,288,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {480,480,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {480,576,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {704,480,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {720,480,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {720,486,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {704,576,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {720,576,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1024,768,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1280,720,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1280,960,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1920,1072,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {1920,1080,YUVC_422YpCbCr8PlanarPixelFormat, false, 0.0 },
    {-1,-1, YUVC_UnknownPixelFormat, false, 0.0 }
};

std::map<YUVCPixelFormatType,PixelFormat> YUVFile::g_pixelFormatList = std::map<YUVCPixelFormatType,PixelFormat>();

YUVFile::YUVFile(const QString &fname, QObject *parent) : YUVSource(parent)
{
    p_srcFile = NULL;

    // open file for reading
    p_srcFile = new QFile(fname);
    p_srcFile->open(QIODevice::ReadOnly);

    // get some more information from file
    QFileInfo fileInfo(p_srcFile->fileName());
    p_path = fileInfo.path();
    p_createdtime = fileInfo.created().toString("yyyy-MM-dd hh:mm:ss");
    p_modifiedtime = fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss");
}

YUVFile::~YUVFile()
{
    delete p_srcFile;
}

void YUVFile::extractFormat(int* width, int* height, int* numFrames, double* frameRate)
{
    // preset return values
    int width1 = -1;
    int height1 = -1;
    int numFrames1 = -1;
    int width2 = -1;
    int height2 = -1;
    int numFrames2 = -1;
    int bitDepth = -1;

    double frameRate1 = -1, frameRate2 = -1;
    YUVCPixelFormatType cFormat1 = YUVC_UnknownPixelFormat, cFormat2 = YUVC_UnknownPixelFormat;

    // try to get information
    formatFromFilename(p_srcFile->fileName(), &width1, &height1, &frameRate1, &numFrames1,&bitDepth, &cFormat1);
    formatFromCorrelation(&width2, &height2, &cFormat2, &numFrames2);

    // set return values
    *width = MAX( width1, width2 );
    *height = MAX( height1, height2 );
    p_srcPixelFormat = MAX( cFormat1, cFormat2 );
    *numFrames = MAX( numFrames1, numFrames2 );
    if (frameRate1 > 0 || frameRate2 > 0)
        *frameRate = MAX( frameRate1, frameRate2 );

    if(p_srcPixelFormat == YUVC_UnknownPixelFormat)
    {
        if (bitDepth==8)
        {
        // assume 8 bit 4:2:0
        p_srcPixelFormat = YUVC_420YpCbCr8PlanarPixelFormat;
        }
        else if (bitDepth==10)
        {
        // assume 10 bit 4:2:0
        p_srcPixelFormat = YUVC_420YpCbCr10LEPlanarPixelFormat;
        }
        else
        {
            //TODO
        }
    }
}

qint64 YUVFile::getNumberFrames(int width, int height)
{
    qint64 fileSize = getFileSize();

    if (width > 0 && height > 0) {
      qint64 bpf = bytesPerFrame(width, height, p_srcPixelFormat);
      return (bpf == 0) ? -1 : fileSize / bpf;
    }
    else
        return -1;
}

qint64 YUVFile::readFrame( QByteArray *targetBuffer, unsigned int frameIdx, int width, int height )
{
    if(p_srcFile == NULL)
        return 0;

    qint64 bpf = bytesPerFrame(width, height, p_srcPixelFormat);
    qint64 startPos = frameIdx * bpf;

    // check if our buffer is big enough
    if( targetBuffer->size() != bpf )
        targetBuffer->resize(bpf);

    // read bytes from file
    readBytes( targetBuffer->data(), startPos, bpf);

    return bpf;
}

void YUVFile::readBytes( char *targetBuffer, qint64 startPos, qint64 length )
{
    if(p_srcFile == NULL)
        return;

    p_srcFile->seek(startPos);
    p_srcFile->read(targetBuffer, length);
    //targetBuffer->setRawData( (const char*)p_srcFile->map(startPos, length, QFile::NoOptions), length );
}

float computeMSE( unsigned char *ptr, unsigned char *ptr2, int numPixels )
{
    float mse=0.0;
    float diff;

    if( numPixels > 0 )
    {
        for(int i=0; i<numPixels; i++)
        {
            diff = (float)ptr[i] - (float)ptr2[i];
            mse += diff*diff;
        }

        /* normalize on correlated pixels */
        mse /= (float)(numPixels);
    }

    return mse;
}

void YUVFile::formatFromFilename(QString filePath, int* width, int* height, double* frameRate, int* numFrames, int* bitDepth, YUVCPixelFormatType* cFormat, bool isYUV)
{
    if(filePath.isEmpty())
        return;

    // preset return values first
    *width = -1;
    *height = -1;
    *frameRate = -1;
    *numFrames = -1;
    *bitDepth = -1;
    int subFormat = -1;

    // parse filename and extract width, height and framerate
    // default format is: sequenceName_widthxheight_framerate.yuv
    QRegExp rxExtendedFormat("([0-9]+)x([0-9]+)_([0-9]+)_([0-9]+)_([0-9]+)");
    QRegExp rxExtended("([0-9]+)x([0-9]+)_([0-9]+)_([0-9]+)");
    QRegExp rxDefault("([0-9]+)x([0-9]+)_([0-9]+)");

    if (rxExtendedFormat.indexIn(filePath) > -1)
    {
        QString widthString = rxExtendedFormat.cap(1);
        *width = widthString.toInt();

        QString heightString = rxExtendedFormat.cap(2);
        *height = heightString.toInt();

        QString rateString = rxExtendedFormat.cap(3);
        *frameRate = rateString.toDouble();

        QString bitDepthString = rxExtendedFormat.cap(4);
        *bitDepth = bitDepthString.toInt();

        QString subSampling = rxExtendedFormat.cap(5);
        subFormat = subSampling.toInt();

    }
    else if(rxExtended.indexIn(filePath) > -1)
    {
        QString widthString = rxExtended.cap(1);
        *width = widthString.toInt();

        QString heightString = rxExtended.cap(2);
        *height = heightString.toInt();

        QString rateString = rxExtended.cap(3);
        *frameRate = rateString.toDouble();

        QString bitDepthString = rxExtended.cap(4);
        *bitDepth = bitDepthString.toInt();
    }
    else if (rxDefault.indexIn(filePath) > -1 ) {
        QString widthString = rxDefault.cap(1);
        *width = widthString.toInt();

        QString heightString = rxDefault.cap(2);
        *height = heightString.toInt();

        QString rateString = rxDefault.cap(3);
        *frameRate = rateString.toDouble();

        *bitDepth = 8; // assume 8 bit
    }
    else
    {
        // try to find resolution indicators (e.g. 'cif', 'hd') in file name
        if( filePath.contains("_cif", Qt::CaseInsensitive) )
        {
            *width = 352;
            *height = 288;
        }
        else if( filePath.contains("_qcif", Qt::CaseInsensitive) )
        {
            *width = 176;
            *height = 144;
        }
        else if( filePath.contains("_4cif", Qt::CaseInsensitive) )
        {
            *width = 704;
            *height = 576;
        }
    }
//To-Do: find pixel format
    if(isYUV && *width > 0 && *height > 0 && *bitDepth > 0)
    {
        QFileInfo fileInfo(filePath);
        qint64 fileSize = fileInfo.size();
        if (*bitDepth==8)
        {
            *cFormat = YUVC_420YpCbCr8PlanarPixelFormat;
            int bpf = YUVFile::bytesPerFrame(*width, *height, *cFormat); // assume 4:2:0, 8bit
            if (bpf == 0) 
                return;
            *numFrames = fileSize / bpf;
        }
        else if (*bitDepth==10)
        {
            *cFormat = YUVC_420YpCbCr10LEPlanarPixelFormat;
            int bpf = YUVFile::bytesPerFrame(*width, *height, *cFormat); // assume 4:2:0, 10bit
            if(subFormat == 444)
            {
                *cFormat = YUVC_444YpCbCr10LEPlanarPixelFormat;
                bpf = YUVFile::bytesPerFrame(*width, *height, *cFormat);
            }
            if (bpf == 0)
                return;
            *numFrames = fileSize / bpf;
        }
        else
        {
            // do other stuff
        }
    }
}

void YUVFile::formatFromCorrelation(int* width, int* height, YUVCPixelFormatType* cFormat, int* numFrames)
{
    if(p_srcFile->fileName().isEmpty())
        return;

    unsigned char *ptr;
    float leastMSE, mse;
    int   bestMode;

    // step1: file size must be a multiple of w*h*(color format)
    QFileInfo fileInfo(*p_srcFile);
    qint64 fileSize = fileInfo.size();
    qint64 picSize;

    if(fileSize < 1)
        return;

    // if any candidate exceeds file size for two frames, discard
    // if any candidate does not represent a multiple of file size, discard
    int i = 0;
    bool found = false;
    while( candidateModes[i].pixelFormat != YUVC_UnknownPixelFormat )
    {
        /* one pic in bytes */
        picSize = bytesPerFrame(candidateModes[i].width, candidateModes[i].height, candidateModes[i].pixelFormat);

        if( fileSize >= (picSize*2) )    // at least 2 pics for correlation analysis
        {
            if( (fileSize % picSize) == 0 ) // important: file size must be multiple of pic size
            {
                candidateModes[i].interesting = true; // test passed
                found = true;
            }
        }

        i++;
    };

    if( !found  ) // no proper candidate mode ?
        return;

    // step2: calculate max. correlation for first two frames, use max. candidate frame size
    i=0;
    while( candidateModes[i].pixelFormat != YUVC_UnknownPixelFormat )
    {
        if( candidateModes[i].interesting )
        {
            picSize = bytesPerFrame(candidateModes[i].width, candidateModes[i].height, candidateModes[i].pixelFormat);

            QByteArray yuvBytes(picSize*2, 0);

            readBytes(yuvBytes.data(), 0, picSize*2);

            // assumptions: YUV is planar (can be changed if necessary)
            // only check mse in luminance
            ptr  = (unsigned char*) yuvBytes.data();
            candidateModes[i].mseY = computeMSE( ptr, ptr + picSize, picSize );
        }
        i++;
    };

    // step3: select best candidate
    i=0;
    leastMSE = FLT_MAX; // large error...
    bestMode = 0;
    while( candidateModes[i].pixelFormat != YUVC_UnknownPixelFormat )
    {
        if( candidateModes[i].interesting )
        {
            mse = (candidateModes[i].mseY);
            if( mse < leastMSE )
            {
                bestMode = i;
                leastMSE = mse;
            }
        }
        i++;
    };

    if( leastMSE < 400 )
    {
        *width  = candidateModes[bestMode].width;
        *height = candidateModes[bestMode].height;
        *cFormat = candidateModes[bestMode].pixelFormat;
        int bpf = bytesPerFrame(*width, *height, *cFormat);
        if (bpf != 0)
            *numFrames = fileSize / bpf;
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


QString YUVFile::getName()
{
    p_path = p_srcFile->fileName();
    QStringList components = p_srcFile->fileName().split(QDir::separator());
    return components.last();
}

qint64 YUVFile::getFileSize()
{
    QFileInfo fileInfo(p_srcFile->fileName());
    return fileInfo.size();
}

// Check if the size/format/nrBytes makes sense
QString YUVFile::getStatus(int width, int height)
{
  qint64 nrBytes = getFileSize();
  int nrBytesPerFrame = bytesPerFrame(width, height, p_srcPixelFormat);
  if (nrBytes % nrBytesPerFrame != 0)
  {
    // Division is with residual
    return QString("Error: File Size and resolution do not match.");
  }
  return QString("OK");
}

void YUVFile::getOneFrame(QByteArray* targetByteArray, unsigned int frameIdx, int width, int height )
{
    // check if we need to do chroma upsampling
    if(p_srcPixelFormat != YUVC_444YpCbCr8PlanarPixelFormat && p_srcPixelFormat != YUVC_444YpCbCr12NativePlanarPixelFormat && p_srcPixelFormat != YUVC_444YpCbCr16NativePlanarPixelFormat && p_srcPixelFormat != YUVC_24RGBPixelFormat )
    {
        // read one frame into temporary buffer
        readFrame( &p_tmpBufferYUV, frameIdx, width, height);
        //use dummy data to check
        // convert original data format into YUV444 planar format
        convert2YUV444(&p_tmpBufferYUV, width, height, targetByteArray);
    }
    else    // source and target format are identical --> no conversion necessary
    {
        // read one frame into cached frame (already in YUV444 format)
        readFrame( targetByteArray, frameIdx, width, height);
    }
}

