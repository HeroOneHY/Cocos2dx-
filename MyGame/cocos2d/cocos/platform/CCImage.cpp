/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/


#include "platform/CCImage.h"

#include <string>
#include <ctype.h>

#include "base/CCData.h"
#include "base/ccConfig.h" // CC_USE_JPEG, CC_USE_TIFF, CC_USE_WEBP

extern "C"
{
    // To resolve link error when building 32bits with Xcode 6.
    // More information please refer to the discussion in https://github.com/cocos2d/cocos2d-x/pull/6986
#if defined (__unix) || (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#ifndef __ENABLE_COMPATIBILITY_WITH_UNIX_2003__
#define __ENABLE_COMPATIBILITY_WITH_UNIX_2003__
#include <stdio.h>
#include <dirent.h>
    FILE *fopen$UNIX2003( const char *filename, const char *mode )
    {
        return fopen(filename, mode);
    }
    size_t fwrite$UNIX2003( const void *a, size_t b, size_t c, FILE *d )
    {
        return fwrite(a, b, c, d);
    }
    int fputs$UNIX2003(const char *res1, FILE *res2){
        return fputs(res1,res2);
    }
    char *strerror$UNIX2003( int errnum )
    {
        return strerror(errnum);
    }
    DIR * opendir$INODE64$UNIX2003( char * dirName )
    {
        return opendir( dirName );
    }
    DIR * opendir$INODE64( char * dirName )
    {
        return opendir( dirName );
    }

    int closedir$UNIX2003(DIR * dir)
    {
        return closedir(dir);
    }

    struct dirent * readdir$INODE64( DIR * dir )
    {
        return readdir( dir );
    }
#endif
#endif

#if CC_USE_PNG
#include "png.h"
#endif //CC_USE_PNG

    
#if CC_USE_JPEG
#include "jpeglib.h"
#include <setjmp.h>
#endif // CC_USE_JPEG
}



#include "base/ccMacros.h"
#include "platform/CCCommon.h"
#include "platform/CCStdC.h"
#include "platform/CCFileUtils.h"
#include "base/CCConfiguration.h"
#include "base/ccUtils.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "platform/android/CCFileUtils-android.h"
#endif

#define CC_GL_ATC_RGB_AMD                                          0x8C92
#define CC_GL_ATC_RGBA_EXPLICIT_ALPHA_AMD                          0x8C93
#define CC_GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD                      0x87EE

NS_CC_BEGIN

//////////////////////////////////////////////////////////////////////////
//struct and data for pvr structure

namespace
{
    static const int PVR_TEXTURE_FLAG_TYPE_MASK = 0xff;
    
    static bool _PVRHaveAlphaPremultiplied = false;
    
    // Values taken from PVRTexture.h from http://www.imgtec.com
    enum class PVR2TextureFlag
    {
        Mipmap         = (1<<8),        // has mip map levels
        Twiddle        = (1<<9),        // is twiddled
        Bumpmap        = (1<<10),       // has normals encoded for a bump map
        Tiling         = (1<<11),       // is bordered for tiled pvr
        Cubemap        = (1<<12),       // is a cubemap/skybox
        FalseMipCol    = (1<<13),       // are there false colored MIP levels
        Volume         = (1<<14),       // is this a volume texture
        Alpha          = (1<<15),       // v2.1 is there transparency info in the texture
        VerticalFlip   = (1<<16),       // v2.1 is the texture vertically flipped
    };
    
    enum class PVR3TextureFlag
    {
        PremultipliedAlpha  = (1<<1)    // has premultiplied alpha
    };
    
    static const char gPVRTexIdentifier[5] = "PVR!";
    
    // v2
    enum class PVR2TexturePixelFormat : unsigned char
    {
        RGBA4444 = 0x10,
        RGBA5551,
        RGBA8888,
        RGB565,
        RGB555,          // unsupported
        RGB888,
        I8,
        AI88,
        PVRTC2BPP_RGBA,
        PVRTC4BPP_RGBA,
        BGRA8888,
        A8,
    };
        
    // v3
    enum class PVR3TexturePixelFormat : uint64_t
    {
        PVRTC2BPP_RGB  = 0ULL,
        PVRTC2BPP_RGBA = 1ULL,
        PVRTC4BPP_RGB  = 2ULL,
        PVRTC4BPP_RGBA = 3ULL,
        PVRTC2_2BPP_RGBA = 4ULL,
        PVRTC2_4BPP_RGBA  = 5ULL,
        ETC1 = 6ULL,
        DXT1 = 7ULL,
        DXT2 = 8ULL,
        DXT3 = 9ULL,
        DXT4 = 10ULL,
        DXT5 = 11ULL,
        BC1 = 7ULL,
        BC2 = 9ULL,
        BC3 = 11ULL,
        BC4 = 12ULL,
        BC5 = 13ULL,
        BC6 = 14ULL,
        BC7 = 15ULL,
        UYVY = 16ULL,
        YUY2 = 17ULL,
        BW1bpp = 18ULL,
        R9G9B9E5 = 19ULL,
        RGBG8888 = 20ULL,
        GRGB8888 = 21ULL,
        ETC2_RGB = 22ULL,
        ETC2_RGBA = 23ULL,
        ETC2_RGBA1 = 24ULL,
        EAC_R11_Unsigned = 25ULL,
        EAC_R11_Signed = 26ULL,
        EAC_RG11_Unsigned = 27ULL,
        EAC_RG11_Signed = 28ULL,
            
        BGRA8888       = 0x0808080861726762ULL,
        RGBA8888       = 0x0808080861626772ULL,
        RGBA4444       = 0x0404040461626772ULL,
        RGBA5551       = 0x0105050561626772ULL,
        RGB565         = 0x0005060500626772ULL,
        RGB888         = 0x0008080800626772ULL,
        A8             = 0x0000000800000061ULL,
        L8             = 0x000000080000006cULL,
        LA88           = 0x000008080000616cULL,
    };
        
        
    // v2
    typedef const std::map<PVR2TexturePixelFormat, Texture2D::PixelFormat> _pixel2_formathash;
    
    static const _pixel2_formathash::value_type v2_pixel_formathash_value[] =
    {
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::BGRA8888,        Texture2D::PixelFormat::BGRA8888),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::RGBA8888,        Texture2D::PixelFormat::RGBA8888),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::RGBA4444,        Texture2D::PixelFormat::RGBA4444),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::RGBA5551,        Texture2D::PixelFormat::RGB5A1),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::RGB565,      Texture2D::PixelFormat::RGB565),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::RGB888,      Texture2D::PixelFormat::RGB888),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::A8,          Texture2D::PixelFormat::A8),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::I8,          Texture2D::PixelFormat::I8),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::AI88,            Texture2D::PixelFormat::AI88),
            
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::PVRTC2BPP_RGBA,      Texture2D::PixelFormat::PVRTC2A),
        _pixel2_formathash::value_type(PVR2TexturePixelFormat::PVRTC4BPP_RGBA,      Texture2D::PixelFormat::PVRTC4A),
    };
        
    static const int PVR2_MAX_TABLE_ELEMENTS = sizeof(v2_pixel_formathash_value) / sizeof(v2_pixel_formathash_value[0]);
    static const _pixel2_formathash v2_pixel_formathash(v2_pixel_formathash_value, v2_pixel_formathash_value + PVR2_MAX_TABLE_ELEMENTS);
        
    // v3
    typedef const std::map<PVR3TexturePixelFormat, Texture2D::PixelFormat> _pixel3_formathash;
    static _pixel3_formathash::value_type v3_pixel_formathash_value[] =
    {
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::BGRA8888,    Texture2D::PixelFormat::BGRA8888),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::RGBA8888,    Texture2D::PixelFormat::RGBA8888),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::RGBA4444,    Texture2D::PixelFormat::RGBA4444),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::RGBA5551,    Texture2D::PixelFormat::RGB5A1),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::RGB565,      Texture2D::PixelFormat::RGB565),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::RGB888,      Texture2D::PixelFormat::RGB888),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::A8,          Texture2D::PixelFormat::A8),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::L8,          Texture2D::PixelFormat::I8),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::LA88,        Texture2D::PixelFormat::AI88),
            
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::PVRTC2BPP_RGB,       Texture2D::PixelFormat::PVRTC2),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::PVRTC2BPP_RGBA,      Texture2D::PixelFormat::PVRTC2A),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::PVRTC4BPP_RGB,       Texture2D::PixelFormat::PVRTC4),
        _pixel3_formathash::value_type(PVR3TexturePixelFormat::PVRTC4BPP_RGBA,      Texture2D::PixelFormat::PVRTC4A),

        _pixel3_formathash::value_type(PVR3TexturePixelFormat::ETC1,        Texture2D::PixelFormat::ETC),
    };
        
    static const int PVR3_MAX_TABLE_ELEMENTS = sizeof(v3_pixel_formathash_value) / sizeof(v3_pixel_formathash_value[0]);
        
    static const _pixel3_formathash v3_pixel_formathash(v3_pixel_formathash_value, v3_pixel_formathash_value + PVR3_MAX_TABLE_ELEMENTS);
        
    typedef struct _PVRTexHeader
    {
        unsigned int headerLength;
        unsigned int height;
        unsigned int width;
        unsigned int numMipmaps;
        unsigned int flags;
        unsigned int dataLength;
        unsigned int bpp;
        unsigned int bitmaskRed;
        unsigned int bitmaskGreen;
        unsigned int bitmaskBlue;
        unsigned int bitmaskAlpha;
        unsigned int pvrTag;
        unsigned int numSurfs;
    } PVRv2TexHeader;
        
#ifdef _MSC_VER
#pragma pack(push,1)
#endif
    typedef struct
    {
        uint32_t version;
        uint32_t flags;
        uint64_t pixelFormat;
        uint32_t colorSpace;
        uint32_t channelType;
        uint32_t height;
        uint32_t width;
        uint32_t depth;
        uint32_t numberOfSurfaces;
        uint32_t numberOfFaces;
        uint32_t numberOfMipmaps;
        uint32_t metadataLength;
#ifdef _MSC_VER
    } PVRv3TexHeader;
#pragma pack(pop)
#else
    } __attribute__((packed)) PVRv3TexHeader;
#endif
}
//pvr structure end

//////////////////////////////////////////////////////////////////////////

//struct and data for s3tc(dds) struct
namespace
{
    struct DDColorKey
    {
        uint32_t colorSpaceLowValue;
        uint32_t colorSpaceHighValue;
    };
    
    struct DDSCaps
    {
        uint32_t caps;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
    };
    
    struct DDPixelFormat
    {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t RGBBitCount;
        uint32_t RBitMask;
        uint32_t GBitMask;
        uint32_t BBitMask;
        uint32_t ABitMask;
    };
    
    
    struct DDSURFACEDESC2
    {
        uint32_t size;
        uint32_t flags;
        uint32_t height;
        uint32_t width;
        
        union
        {
            uint32_t pitch;
            uint32_t linearSize;
        } DUMMYUNIONNAMEN1;
        
        union
        {
            uint32_t backBufferCount;
            uint32_t depth;
        } DUMMYUNIONNAMEN5;
        
        union
        {
            uint32_t mipMapCount;
            uint32_t refreshRate;
            uint32_t srcVBHandle;
        } DUMMYUNIONNAMEN2;
        
        uint32_t alphaBitDepth;
        uint32_t reserved;
        uint32_t surface;
        
        union
        {
            DDColorKey ddckCKDestOverlay;
            uint32_t emptyFaceColor;
        } DUMMYUNIONNAMEN3;
        
        DDColorKey ddckCKDestBlt;
        DDColorKey ddckCKSrcOverlay;
        DDColorKey ddckCKSrcBlt;
        
        union
        {
            DDPixelFormat ddpfPixelFormat;
            uint32_t FVF;
        } DUMMYUNIONNAMEN4;
        
        DDSCaps ddsCaps;
        uint32_t textureStage;
    } ;
    
#pragma pack(push,1)
    
    struct S3TCTexHeader
    {
        char fileCode[4];
        DDSURFACEDESC2 ddsd;
    };
    
#pragma pack(pop)

}
//s3tc struct end

//////////////////////////////////////////////////////////////////////////

//struct and data for atitc(ktx) struct
namespace
{
    struct ATITCTexHeader
    {
        //HEADER
        char identifier[12];
        uint32_t endianness;
        uint32_t glType;
        uint32_t glTypeSize;
        uint32_t glFormat;
        uint32_t glInternalFormat;
        uint32_t glBaseInternalFormat;
        uint32_t pixelWidth;
        uint32_t pixelHeight;
        uint32_t pixelDepth;
        uint32_t numberOfArrayElements;
        uint32_t numberOfFaces;
        uint32_t numberOfMipmapLevels;
        uint32_t bytesOfKeyValueData;
    };
}
//atitc struct end

//////////////////////////////////////////////////////////////////////////

namespace
{
    typedef struct 
    {
        const unsigned char * data;
        ssize_t size;
        int offset;
    }tImageSource;
 
#if CC_USE_PNG
    static void pngReadCallback(png_structp png_ptr, png_bytep data, png_size_t length)
    {
        tImageSource* isource = (tImageSource*)png_get_io_ptr(png_ptr);
        
        if((int)(isource->offset + length) <= isource->size)
        {
            memcpy(data, isource->data+isource->offset, length);
            isource->offset += length;
        }
        else
        {
            png_error(png_ptr, "pngReaderCallback failed");
        }
    }
#endif //CC_USE_PNG
}

Texture2D::PixelFormat getDevicePixelFormat(Texture2D::PixelFormat format)
{
    switch (format) {
        case Texture2D::PixelFormat::PVRTC4:
        case Texture2D::PixelFormat::PVRTC4A:
        case Texture2D::PixelFormat::PVRTC2:
        case Texture2D::PixelFormat::PVRTC2A:
            if(Configuration::getInstance()->supportsPVRTC())
                return format;
            else
                return Texture2D::PixelFormat::RGBA8888;
        case Texture2D::PixelFormat::ETC:
            if(Configuration::getInstance()->supportsETC())
                return format;
            else
                return Texture2D::PixelFormat::RGB888;
        default:
            return format;
    }
}

//////////////////////////////////////////////////////////////////////////
// Implement Image
//////////////////////////////////////////////////////////////////////////
bool Image::PNG_PREMULTIPLIED_ALPHA_ENABLED = true;

Image::Image()
: _data(nullptr)
, _dataLen(0)
, _width(0)
, _height(0)
, _unpack(false)
, _fileType(Format::UNKNOWN)
, _renderFormat(Texture2D::PixelFormat::NONE)
, _numberOfMipmaps(0)
, _hasPremultipliedAlpha(false)
{

}

Image::~Image()
{
    if(_unpack)
    {
        for (int i = 0; i < _numberOfMipmaps; ++i)
            CC_SAFE_DELETE_ARRAY(_mipmaps[i].address);
    }
    else
        CC_SAFE_FREE(_data);
}

bool Image::initWithImageFile(const std::string& path)
{
    bool ret = false;
    _filePath = FileUtils::getInstance()->fullPathForFilename(path);

    Data data = FileUtils::getInstance()->getDataFromFile(_filePath);

    if (!data.isNull())
    {
        ret = initWithImageData(data.getBytes(), data.getSize());
    }

    return ret;
}

bool Image::initWithImageFileThreadSafe(const std::string& fullpath)
{
    bool ret = false;
    _filePath = fullpath;

    Data data = FileUtils::getInstance()->getDataFromFile(fullpath);

    if (!data.isNull())
    {
        ret = initWithImageData(data.getBytes(), data.getSize());
    }

    return ret;
}

bool Image::initWithImageData(const unsigned char * data, ssize_t dataLen)
{
    bool ret = false;
    
    do
    {
        CC_BREAK_IF(! data || dataLen <= 0);
        
        unsigned char* unpackedData = nullptr;
        ssize_t unpackedLen = 0;
        
        //detect and unzip the compress file
    

            unpackedData = const_cast<unsigned char*>(data);
            unpackedLen = dataLen;
        

        _fileType = detectFormat(unpackedData, unpackedLen);

        switch (_fileType)
        {
        case Format::PNG:
            ret = initWithPngData(unpackedData, unpackedLen);
            break;
        case Format::JPG:
            ret = initWithJpgData(unpackedData, unpackedLen);
            break;
        default:
            {
              
                break;
            }
        }
        
        if(unpackedData != data)
        {
            free(unpackedData);
        }
    } while (0);
    
    return ret;
}

bool Image::isPng(const unsigned char * data, ssize_t dataLen)
{
    if (dataLen <= 8)
    {
        return false;
    }

    static const unsigned char PNG_SIGNATURE[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

    return memcmp(PNG_SIGNATURE, data, sizeof(PNG_SIGNATURE)) == 0;
}




bool Image::isJpg(const unsigned char * data, ssize_t dataLen)
{
    if (dataLen <= 4)
    {
        return false;
    }

    static const unsigned char JPG_SOI[] = {0xFF, 0xD8};

    return memcmp(data, JPG_SOI, 2) == 0;
}






Image::Format Image::detectFormat(const unsigned char * data, ssize_t dataLen)
{
    if (isPng(data, dataLen))
    {
        return Format::PNG;
    }
    else if (isJpg(data, dataLen))
    {
        return Format::JPG;
    }
    else
    {
        return Format::UNKNOWN;
    }
}

int Image::getBitPerPixel()
{
    return Texture2D::getPixelFormatInfoMap().at(_renderFormat).bpp;
}

bool Image::hasAlpha()
{
    return Texture2D::getPixelFormatInfoMap().at(_renderFormat).alpha;
}

bool Image::isCompressed()
{
    return Texture2D::getPixelFormatInfoMap().at(_renderFormat).compressed;
}

namespace
{
/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * We override the "error_exit" method so that control is returned to the
 * library's caller when a fatal error occurs, rather than calling exit()
 * as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */
#if CC_USE_JPEG
    struct MyErrorMgr
    {
        struct jpeg_error_mgr pub;  /* "public" fields */
        jmp_buf setjmp_buffer;  /* for return to caller */
    };
    
    typedef struct MyErrorMgr * MyErrorPtr;
    
    /*
     * Here's the routine that will replace the standard error_exit method:
     */
    
    METHODDEF(void)
    myErrorExit(j_common_ptr cinfo)
    {
        /* cinfo->err really points to a MyErrorMgr struct, so coerce pointer */
        MyErrorPtr myerr = (MyErrorPtr) cinfo->err;
        
        /* Always display the message. */
        /* We could postpone this until after returning, if we chose. */
        /* internal message function can't show error message in some platforms, so we rewrite it here.
         * edit it if has version conflict.
         */
        //(*cinfo->err->output_message) (cinfo);
        char buffer[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message) (cinfo, buffer);
        CCLOG("jpeg error: %s", buffer);
        
        /* Return control to the setjmp point */
        longjmp(myerr->setjmp_buffer, 1);
    }
#endif // CC_USE_JPEG
}

#if CC_USE_WIC
bool Image::decodeWithWIC(const unsigned char *data, ssize_t dataLen)
{
    bool bRet = false;
    WICImageLoader img;

    if (img.decodeImageData(data, dataLen))
    {
        _width = img.getWidth();
        _height = img.getHeight();

        WICPixelFormatGUID format = img.getPixelFormat();

        if (memcmp(&format, &GUID_WICPixelFormat8bppGray, sizeof(WICPixelFormatGUID)) == 0)
        {
            _renderFormat = Texture2D::PixelFormat::I8;
        }

        if (memcmp(&format, &GUID_WICPixelFormat8bppAlpha, sizeof(WICPixelFormatGUID)) == 0)
        {
            _renderFormat = Texture2D::PixelFormat::AI88;
        }

        if (memcmp(&format, &GUID_WICPixelFormat24bppRGB, sizeof(WICPixelFormatGUID)) == 0)
        {
            _renderFormat = Texture2D::PixelFormat::RGB888;
        }

        if (memcmp(&format, &GUID_WICPixelFormat32bppRGBA, sizeof(WICPixelFormatGUID)) == 0)
        {
            _renderFormat = Texture2D::PixelFormat::RGBA8888;
        }

        if (memcmp(&format, &GUID_WICPixelFormat32bppBGRA, sizeof(WICPixelFormatGUID)) == 0)
        {
            _renderFormat = Texture2D::PixelFormat::BGRA8888;
        }

        _dataLen = img.getImageDataSize();

        CCASSERT(_dataLen > 0, "Image: Decompressed data length is invalid");

        _data = new (std::nothrow) unsigned char[_dataLen];
        bRet = (img.getImageData(_data, _dataLen) > 0);

        if (_renderFormat == Texture2D::PixelFormat::RGBA8888) {
            premultipliedAlpha();
        }
    }

    return bRet;
}

bool Image::encodeWithWIC(const std::string& filePath, bool isToRGB, GUID containerFormat)
{
    // Save formats supported by WIC
    WICPixelFormatGUID targetFormat = isToRGB ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA;
    unsigned char *pSaveData = nullptr;
    int saveLen = _dataLen;
    int bpp = 4;

    if (targetFormat == GUID_WICPixelFormat24bppBGR && _renderFormat == Texture2D::PixelFormat::RGBA8888)
    {
        bpp = 3;
        saveLen = _width * _height * bpp;
        pSaveData = new (std::nothrow) unsigned char[saveLen];
        int indL = 0, indR = 0;

        while (indL < saveLen && indR < _dataLen)
        {
            memcpy(&pSaveData[indL], &_data[indR], 3);
            indL += 3;
            indR += 4;
        }
    }
    else
    {
        pSaveData = new (std::nothrow) unsigned char[saveLen];
        memcpy(pSaveData, _data, saveLen);
    }

    for (int ind = 2; ind < saveLen; ind += bpp) {
        std::swap(pSaveData[ind - 2], pSaveData[ind]);
    }

    WICImageLoader img;
    bool bRet = img.encodeImageData(filePath, pSaveData, saveLen, targetFormat, _width, _height, containerFormat);

    delete[] pSaveData;
    return bRet;
}


#endif //CC_USE_WIC

bool Image::initWithJpgData(const unsigned char * data, ssize_t dataLen)
{
#if CC_USE_WIC
    return decodeWithWIC(data, dataLen);
#elif CC_USE_JPEG
    /* these are standard libjpeg structures for reading(decompression) */
    struct jpeg_decompress_struct cinfo;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct MyErrorMgr jerr;
    /* libjpeg data structure for storing one row, that is, scanline of an image */
    JSAMPROW row_pointer[1] = {0};
    unsigned long location = 0;

    bool ret = false;
    do 
    {
        /* We set up the normal JPEG error routines, then override error_exit. */
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = myErrorExit;
        /* Establish the setjmp return context for MyErrorExit to use. */
        if (setjmp(jerr.setjmp_buffer))
        {
            /* If we get here, the JPEG code has signaled an error.
             * We need to clean up the JPEG object, close the input file, and return.
             */
            jpeg_destroy_decompress(&cinfo);
            break;
        }

        /* setup decompression process and source, then read JPEG header */
        jpeg_create_decompress( &cinfo );

#ifndef CC_TARGET_QT5
        jpeg_mem_src(&cinfo, const_cast<unsigned char*>(data), dataLen);
#endif /* CC_TARGET_QT5 */

        /* reading the image header which contains image information */
#if (JPEG_LIB_VERSION >= 90)
        // libjpeg 0.9 adds stricter types.
        jpeg_read_header(&cinfo, TRUE);
#else
        jpeg_read_header(&cinfo, TRUE);
#endif

        // we only support RGB or grayscale
        if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
        {
            _renderFormat = Texture2D::PixelFormat::I8;
        }else
        {
            cinfo.out_color_space = JCS_RGB;
            _renderFormat = Texture2D::PixelFormat::RGB888;
        }

        /* Start decompression jpeg here */
        jpeg_start_decompress( &cinfo );

        /* init image info */
        _width  = cinfo.output_width;
        _height = cinfo.output_height;

        _dataLen = cinfo.output_width*cinfo.output_height*cinfo.output_components;
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        CC_BREAK_IF(! _data);

        /* now actually read the jpeg into the raw buffer */
        /* read one scan line at a time */
        while (cinfo.output_scanline < cinfo.output_height)
        {
            row_pointer[0] = _data + location;
            location += cinfo.output_width*cinfo.output_components;
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }

    /* When read image file with broken data, jpeg_finish_decompress() may cause error.
     * Besides, jpeg_destroy_decompress() shall deallocate and release all memory associated
     * with the decompression object.
     * So it doesn't need to call jpeg_finish_decompress().
     */
    //jpeg_finish_decompress( &cinfo );
        jpeg_destroy_decompress( &cinfo );
        /* wrap up decompression, destroy objects, free pointers and close open files */        
        ret = true;
    } while (0);

    return ret;
#else
    CCLOG("jpeg is not enabled, please enable it in ccConfig.h");
    return false;
#endif // CC_USE_JPEG
}

bool Image::initWithPngData(const unsigned char * data, ssize_t dataLen)
{
#if CC_USE_WIC
    return decodeWithWIC(data, dataLen);
#elif CC_USE_PNG
    // length of bytes to check if it is a valid png file
#define PNGSIGSIZE  8
    bool ret = false;
    png_byte        header[PNGSIGSIZE]   = {0}; 
    png_structp     png_ptr     =   0;
    png_infop       info_ptr    = 0;

    do 
    {
        // png header len is 8 bytes
        CC_BREAK_IF(dataLen < PNGSIGSIZE);

        // check the data is png or not
        memcpy(header, data, PNGSIGSIZE);
        CC_BREAK_IF(png_sig_cmp(header, 0, PNGSIGSIZE));

        // init png_struct
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        CC_BREAK_IF(! png_ptr);

        // init png_info
        info_ptr = png_create_info_struct(png_ptr);
        CC_BREAK_IF(!info_ptr);

#if (CC_TARGET_PLATFORM != CC_PLATFORM_BADA && CC_TARGET_PLATFORM != CC_PLATFORM_NACL && CC_TARGET_PLATFORM != CC_PLATFORM_TIZEN)
        CC_BREAK_IF(setjmp(png_jmpbuf(png_ptr)));
#endif

        // set the read call back function
        tImageSource imageSource;
        imageSource.data    = (unsigned char*)data;
        imageSource.size    = dataLen;
        imageSource.offset  = 0;
        png_set_read_fn(png_ptr, &imageSource, pngReadCallback);

        // read png header info

        // read png file info
        png_read_info(png_ptr, info_ptr);

        _width = png_get_image_width(png_ptr, info_ptr);
        _height = png_get_image_height(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);

        //CCLOG("color type %u", color_type);

        // force palette images to be expanded to 24-bit RGB
        // it may include alpha channel
        if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            png_set_palette_to_rgb(png_ptr);
        }
        // low-bit-depth grayscale images are to be expanded to 8 bits
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        {
            bit_depth = 8;
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        // expand any tRNS chunk data into a full alpha channel
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(png_ptr);
        }  
        // reduce images with 16-bit samples to 8 bits
        if (bit_depth == 16)
        {
            png_set_strip_16(png_ptr);            
        } 

        // Expanded earlier for grayscale, now take care of palette and rgb
        if (bit_depth < 8)
        {
            png_set_packing(png_ptr);
        }
        // update info
        png_read_update_info(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);

        switch (color_type)
        {
        case PNG_COLOR_TYPE_GRAY:
            _renderFormat = Texture2D::PixelFormat::I8;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            _renderFormat = Texture2D::PixelFormat::AI88;
            break;
        case PNG_COLOR_TYPE_RGB:
            _renderFormat = Texture2D::PixelFormat::RGB888;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            _renderFormat = Texture2D::PixelFormat::RGBA8888;
            break;
        default:
            break;
        }

        // read png data
        png_size_t rowbytes;
        png_bytep* row_pointers = (png_bytep*)malloc( sizeof(png_bytep) * _height );

        rowbytes = png_get_rowbytes(png_ptr, info_ptr);

        _dataLen = rowbytes * _height;
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        if (!_data)
        {
            if (row_pointers != nullptr)
            {
                free(row_pointers);
            }
            break;
        }

        for (unsigned short i = 0; i < _height; ++i)
        {
            row_pointers[i] = _data + i*rowbytes;
        }
        png_read_image(png_ptr, row_pointers);

        png_read_end(png_ptr, nullptr);

        // premultiplied alpha for RGBA8888
        if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        {
            if (PNG_PREMULTIPLIED_ALPHA_ENABLED)
            {
                premultipliedAlpha();
            }
            else
            {
#if CC_ENABLE_PREMULTIPLIED_ALPHA != 0
                _hasPremultipliedAlpha = true;
#endif
            }
        }

        if (row_pointers != nullptr)
        {
            free(row_pointers);
        }

        ret = true;
    } while (0);

    if (png_ptr)
    {
        png_destroy_read_struct(&png_ptr, (info_ptr) ? &info_ptr : 0, 0);
    }
    return ret;
#else
    CCLOG("png is not enabled, please enable it in ccConfig.h");
    return false;
#endif //CC_USE_PNG
}



namespace
{
    bool testFormatForPvr2TCSupport(PVR2TexturePixelFormat /*format*/)
    {
        return true;
    }
    
    bool testFormatForPvr3TCSupport(PVR3TexturePixelFormat format)
    {
        switch (format) {
            case PVR3TexturePixelFormat::DXT1:
            case PVR3TexturePixelFormat::DXT3:
            case PVR3TexturePixelFormat::DXT5:
                return Configuration::getInstance()->supportsS3TC();
                
            case PVR3TexturePixelFormat::BGRA8888:
                return Configuration::getInstance()->supportsBGRA8888();
                
            case PVR3TexturePixelFormat::PVRTC2BPP_RGB:
            case PVR3TexturePixelFormat::PVRTC2BPP_RGBA:
            case PVR3TexturePixelFormat::PVRTC4BPP_RGB:
            case PVR3TexturePixelFormat::PVRTC4BPP_RGBA:
            case PVR3TexturePixelFormat::ETC1:
            case PVR3TexturePixelFormat::RGBA8888:
            case PVR3TexturePixelFormat::RGBA4444:
            case PVR3TexturePixelFormat::RGBA5551:
            case PVR3TexturePixelFormat::RGB565:
            case PVR3TexturePixelFormat::RGB888:
            case PVR3TexturePixelFormat::A8:
            case PVR3TexturePixelFormat::L8:
            case PVR3TexturePixelFormat::LA88:
                return true;
                
            default:
                return false;
        }
    }
}







namespace
{
    static uint32_t makeFourCC(char ch0, char ch1, char ch2, char ch3)
    {
        const uint32_t fourCC = ((uint32_t)(char)(ch0) | ((uint32_t)(char)(ch1) << 8) | ((uint32_t)(char)(ch2) << 16) | ((uint32_t)(char)(ch3) << 24 ));
        return fourCC;
    }
}




bool Image::initWithRawData(const unsigned char * data, ssize_t /*dataLen*/, int width, int height, int /*bitsPerComponent*/, bool preMulti)
{
    bool ret = false;
    do 
    {
        CC_BREAK_IF(0 == width || 0 == height);

        _height   = height;
        _width    = width;
        _hasPremultipliedAlpha = preMulti;
        _renderFormat = Texture2D::PixelFormat::RGBA8888;

        // only RGBA8888 supported
        int bytesPerComponent = 4;
        _dataLen = height * width * bytesPerComponent;
        _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        CC_BREAK_IF(! _data);
        memcpy(_data, data, _dataLen);

        ret = true;
    } while (0);

    return ret;
}


#if (CC_TARGET_PLATFORM != CC_PLATFORM_IOS)
bool Image::saveToFile(const std::string& filename, bool isToRGB)
{
    //only support for Texture2D::PixelFormat::RGB888 or Texture2D::PixelFormat::RGBA8888 uncompressed data
    if (isCompressed() || (_renderFormat != Texture2D::PixelFormat::RGB888 && _renderFormat != Texture2D::PixelFormat::RGBA8888))
    {
        CCLOG("cocos2d: Image: saveToFile is only support for Texture2D::PixelFormat::RGB888 or Texture2D::PixelFormat::RGBA8888 uncompressed data for now");
        return false;
    }

    std::string fileExtension = FileUtils::getInstance()->getFileExtension(filename);

    if (fileExtension == ".png")
    {
        return saveImageToPNG(filename, isToRGB);
    }
    else if (fileExtension == ".jpg")
    {
        return saveImageToJPG(filename);
    }
    else
    {
        CCLOG("cocos2d: Image: saveToFile no support file extension(only .png or .jpg) for file: %s", filename.c_str());
        return false;
    }
}
#endif

bool Image::saveImageToPNG(const std::string& filePath, bool isToRGB)
{
#if CC_USE_WIC
    return encodeWithWIC(filePath, isToRGB, GUID_ContainerFormatPng);
#elif CC_USE_PNG
    bool ret = false;
    do
    {
        FILE *fp;
        png_structp png_ptr;
        png_infop info_ptr;
        png_bytep *row_pointers;

        fp = fopen(FileUtils::getInstance()->getSuitableFOpen(filePath).c_str(), "wb");
        CC_BREAK_IF(nullptr == fp);

        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

        if (nullptr == png_ptr)
        {
            fclose(fp);
            break;
        }

        info_ptr = png_create_info_struct(png_ptr);
        if (nullptr == info_ptr)
        {
            fclose(fp);
            png_destroy_write_struct(&png_ptr, nullptr);
            break;
        }
#if (CC_TARGET_PLATFORM != CC_PLATFORM_BADA && CC_TARGET_PLATFORM != CC_PLATFORM_NACL && CC_TARGET_PLATFORM != CC_PLATFORM_TIZEN)
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }
#endif
        png_init_io(png_ptr, fp);

        if (!isToRGB && hasAlpha())
        {
            png_set_IHDR(png_ptr, info_ptr, _width, _height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        } 
        else
        {
            png_set_IHDR(png_ptr, info_ptr, _width, _height, 8, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        }
        
        png_write_info(png_ptr, info_ptr);

        png_set_packing(png_ptr);

        row_pointers = (png_bytep *)malloc(_height * sizeof(png_bytep));
        if(row_pointers == nullptr)
        {
            fclose(fp);
            png_destroy_write_struct(&png_ptr, &info_ptr);
            break;
        }

        if (!hasAlpha())
        {
            for (int i = 0; i < (int)_height; i++)
            {
                row_pointers[i] = (png_bytep)_data + i * _width * 3;
            }

            png_write_image(png_ptr, row_pointers);

            free(row_pointers);
            row_pointers = nullptr;
        }
        else
        {
            if (isToRGB)
            {
                unsigned char *tempData = static_cast<unsigned char*>(malloc(_width * _height * 3 * sizeof(unsigned char)));
                if (nullptr == tempData)
                {
                    fclose(fp);
                    png_destroy_write_struct(&png_ptr, &info_ptr);
                    
                    free(row_pointers);
                    row_pointers = nullptr;
                    break;
                }

                for (int i = 0; i < _height; ++i)
                {
                    for (int j = 0; j < _width; ++j)
                    {
                        tempData[(i * _width + j) * 3] = _data[(i * _width + j) * 4];
                        tempData[(i * _width + j) * 3 + 1] = _data[(i * _width + j) * 4 + 1];
                        tempData[(i * _width + j) * 3 + 2] = _data[(i * _width + j) * 4 + 2];
                    }
                }

                for (int i = 0; i < (int)_height; i++)
                {
                    row_pointers[i] = (png_bytep)tempData + i * _width * 3;
                }

                png_write_image(png_ptr, row_pointers);

                free(row_pointers);
                row_pointers = nullptr;

                if (tempData != nullptr)
                {
                    free(tempData);
                }
            } 
            else
            {
                for (int i = 0; i < (int)_height; i++)
                {
                    row_pointers[i] = (png_bytep)_data + i * _width * 4;
                }

                png_write_image(png_ptr, row_pointers);

                free(row_pointers);
                row_pointers = nullptr;
            }
        }

        png_write_end(png_ptr, info_ptr);

        png_destroy_write_struct(&png_ptr, &info_ptr);

        fclose(fp);

        ret = true;
    } while (0);
    return ret;
#else
    CCLOG("png is not enabled, please enable it in ccConfig.h");
    return false;
#endif // CC_USE_PNG
}

bool Image::saveImageToJPG(const std::string& filePath)
{
#if CC_USE_WIC
    return encodeWithWIC(filePath, false, GUID_ContainerFormatJpeg);
#elif CC_USE_JPEG
    bool ret = false;
    do 
    {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        FILE * outfile;                 /* target file */
        JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */
        int     row_stride;          /* physical row width in image buffer */

        cinfo.err = jpeg_std_error(&jerr);
        /* Now we can initialize the JPEG compression object. */
        jpeg_create_compress(&cinfo);

        CC_BREAK_IF((outfile = fopen(FileUtils::getInstance()->getSuitableFOpen(filePath).c_str(), "wb")) == nullptr);
        
        jpeg_stdio_dest(&cinfo, outfile);

        cinfo.image_width = _width;    /* image width and height, in pixels */
        cinfo.image_height = _height;
        cinfo.input_components = 3;       /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 90, TRUE);
        
        jpeg_start_compress(&cinfo, TRUE);

        row_stride = _width * 3; /* JSAMPLEs per row in image_buffer */

        if (hasAlpha())
        {
            unsigned char *tempData = static_cast<unsigned char*>(malloc(_width * _height * 3 * sizeof(unsigned char)));
            if (nullptr == tempData)
            {
                jpeg_finish_compress(&cinfo);
                jpeg_destroy_compress(&cinfo);
                fclose(outfile);
                break;
            }

            for (int i = 0; i < _height; ++i)
            {
                for (int j = 0; j < _width; ++j)

                {
                    tempData[(i * _width + j) * 3] = _data[(i * _width + j) * 4];
                    tempData[(i * _width + j) * 3 + 1] = _data[(i * _width + j) * 4 + 1];
                    tempData[(i * _width + j) * 3 + 2] = _data[(i * _width + j) * 4 + 2];
                }
            }

            while (cinfo.next_scanline < cinfo.image_height)
            {
                row_pointer[0] = & tempData[cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }

            if (tempData != nullptr)
            {
                free(tempData);
            }
        } 
        else
        {
            while (cinfo.next_scanline < cinfo.image_height) {
                row_pointer[0] = & _data[cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }
        }

        jpeg_finish_compress(&cinfo);
        fclose(outfile);
        jpeg_destroy_compress(&cinfo);
        
        ret = true;
    } while (0);
    return ret;
#else
    CCLOG("jpeg is not enabled, please enable it in ccConfig.h");
    return false;
#endif // CC_USE_JPEG
}

void Image::premultipliedAlpha()
{
#if CC_ENABLE_PREMULTIPLIED_ALPHA == 0
        _hasPremultipliedAlpha = false;
        return;
#else
    CCASSERT(_renderFormat == Texture2D::PixelFormat::RGBA8888, "The pixel format should be RGBA8888!");
    
    unsigned int* fourBytes = (unsigned int*)_data;
    for(int i = 0; i < _width * _height; i++)
    {
        unsigned char* p = _data + i * 4;
        fourBytes[i] = CC_RGB_PREMULTIPLY_ALPHA(p[0], p[1], p[2], p[3]);
    }
    
    _hasPremultipliedAlpha = true;
#endif
}


void Image::setPVRImagesHavePremultipliedAlpha(bool haveAlphaPremultiplied)
{
    _PVRHaveAlphaPremultiplied = haveAlphaPremultiplied;
}

NS_CC_END

