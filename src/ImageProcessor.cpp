/*
 * Copyright 2009, Yahoo!
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 *  3. Neither the name of Yahoo! nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ImageProcessor.hh"
#include "Transformations.hh"
#include "magick/api.h"
#include "bp-file/bpfile.h"
#include <sstream>
#include <assert.h>
#include <string.h>

#ifdef WIN32
#define strcasecmp _stricmp
#endif

// a map of supported types.
// written in init, and only read after that (for threading issues)
struct CaseInsensitiveCompare 
{
    bool operator()(const std::string& lhs, const std::string& rhs) const 
    {
        return(strcasecmp(lhs.c_str(), rhs.c_str()) < 0);
    }
};

typedef std::map<std::string, std::string, CaseInsensitiveCompare> ExtMap;
static ExtMap s_imgFormats;

const imageproc::Type imageproc::UNKNOWN = NULL;

void
imageproc::init()
{
    unsigned int i;
    
    RegisterStaticModules();
    InitializeMagick(NULL);

    // let's output a startup banner with available image type support
    ExceptionInfo exception;
    MagickInfo ** arr = GetMagickInfoArray( &exception );
    std::stringstream ss;
    
    ss << "GraphicsMagick engine initialized with support for: [ ";

    bool first = true;
    while (arr && *arr) {
        if (!first) ss << ", ";
        first = false;
        ss << (*arr)->name;
        char * mt = MagickToMime( (*arr)->name );
        if (mt) {
            s_imgFormats[(*arr)->name] = std::string(mt);
            ss << " (" << mt << ")";
            free(mt);
        }
        arr++;
    }
    ss << " ]";
#if 0
    g_bpCoreFunctions->log(BP_INFO, ss.str().c_str());
#endif // 0

    ss.str("");
    ss << "Supported transformations: [ ";
    first = true;
    for (i = 0; i < trans::num(); i++)
    {
        if (!first) ss << ", ";
        first = false;
        ss << trans::get(i)->name;
    }
    ss << " ]";
#if 0
    g_bpCoreFunctions->log(BP_INFO, ss.str().c_str());
#endif // 0
}

void
imageproc::shutdown()
{
    DestroyMagick();
}

#ifdef WIN32
#define strcasecmp _stricmp
#endif

imageproc::Type
imageproc::pathToType(const std::string & path)
{
    Type rval = UNKNOWN;

    if (!path.empty())
    {
        size_t pos = path.rfind('.');
        if (pos == std::string::npos) pos = -1;
        std::string ext = path.substr(pos+1, std::string::npos);
        
        ExtMap::const_iterator it = s_imgFormats.find(ext);
        if (it != s_imgFormats.end()) {
            rval = it->first.c_str();
        }
    }

    return rval;
}

std::string
imageproc::typeToExt(Type t)
{
    std::string ext;
    while (t && *t) { ext.append(1, (char) tolower(*t++)); }
    return ext;
}

static
Image * runTransformations(Image * image,
                           const bplus::List & transList,
                           int quality, std::string & oError)
{
#if 0
    g_bpCoreFunctions->log(
        BP_INFO, "%lu transformation actions specified",
        transList.size());
#endif // 0
    
    for (unsigned int i = 0; i < transList.size(); i++)
    {
        const bplus::Object * o = transList.value(i);

        std::string command;
        const bplus::Object * args = NULL;
        
        // o may either be a string transformation: i.e. "solarize"
        // or a may transform: i.e. { "crop": { .25, .75, .25, .75 } }
        // first we'll extract the command
        if (o->type() == BPTString) {
            command = (std::string)(*o);            
        } else  if (o->type() == BPTMap) {
            const bplus::Map * m = (const bplus::Map *) o;
            if (m->size() != 1) {
                std::stringstream ss;
                ss << "transform " << i << " is malformed.  An action is  "
                   << "an object with a single property which is the action "
                   << "name";
                oError = ss.str();
                break;
            }
            bplus::Map::Iterator i(*m);
            command.append(i.nextKey());
            args = m->get(command.c_str());
            assert(args != NULL);
        } else {
            std::stringstream ss;
            ss << "transform " << i << " is malformed.  An action is  "
               << "either a string or an object with a single property which "
               << "is the name of an action to perform";
            oError = ss.str();
            break;
        }

#if 0
        g_bpCoreFunctions->log(
            BP_INFO, "transform [%s] with%s args",
            command.c_str(), (args ? "" : "out"));
#endif // 0

        // does the command exist?
        const trans::Transformation * t = trans::get(command);
        if (t == NULL) {
            std::stringstream ss;
            ss << "no such transformation: " << command;
            oError = ss.str();
            break;
        }

        // are the arguments correct?
        if (t->requiresArgs && !args) {
            oError.append(command);
            oError.append(" missing required argument");
            break;
        }

        if (!t->acceptsArgs && args) {        
            oError.append(command);
            oError.append(" doesn't accept arguments");
            break;
        }

        {
            Image * newImage = t->transform(image, args, quality, oError);
            DestroyImage(image);
            image = newImage;
        }
        
        // abort if the transformation failed
        if (!image) break;
    }

    if (!oError.empty() && image) {
        DestroyImage(image);
        image = NULL;
    }

    return image;
}

static Image *
IP_ReadImageFile(const ImageInfo * image_info,
                 const std::string & path,
                 ExceptionInfo * exception)
 {
    if (path.empty()) return NULL;
    
    std::ifstream fstream;
    if (!bp::file::openReadableStream(fstream, path, std::ios_base::in | std::ios_base::binary)) {
#if 0
        g_bpCoreFunctions->log(
            BP_ERROR, "Couldn't open file for reading: %s", path.c_str());
#endif // 0
        return NULL;
    }
    // get filesize
    fstream.seekg(0, std::ios::end);
    size_t len = (size_t) fstream.tellg();
    fstream.seekg(0, std::ios::beg);
    //bplus::service::Service::log(BP_DEBUG, "file size = " + size);
    if (len <= 0) {
#if 0
        g_bpCoreFunctions->log(
            BP_ERROR, "Couldn't determine file length: %s", path.c_str());
#endif // 0
        return NULL;
    }
    void * img = malloc(len);
    if (!img) {
#if 0
        g_bpCoreFunctions->log(
            BP_ERROR, "memory allocation failed (%ld bytes) when trying to "
            "read image", len);
#endif // 0
        return NULL;
    }
#if 0
    g_bpCoreFunctions->log(
        BP_INFO, "Attempting to read %ld bytes from '%s'",
        len, path.c_str());
#endif // 0
    fstream.read((char*)img, len);
    size_t rd = (size_t)fstream.gcount();
    fstream.close();
    if (rd != len) {
#if 0
        g_bpCoreFunctions->log(
            BP_ERROR, "Partial read detected, got %ld of %ld bytes",
            rd, len);
#endif // 0
        free(img);
        return NULL;
    }

    // now convert it into a GM image 
    Image * i = BlobToImage(image_info, img, len, exception);

#if 0
    g_bpCoreFunctions->log(BP_ERROR, "read img: %p", i);
#endif // 0

    free(img);

    return i;
}


std::string
imageproc::ChangeImage(const std::string & inPath,
                       const std::string & tmpDir,
                       Type outputFormat,
                       const bplus::List & transformations,
                       int quality,
                       unsigned int & x, unsigned int & y, 
                       unsigned int & orig_x, unsigned int & orig_y, 
                       std::string & oError)
{
    ExceptionInfo exception;
    Image *images;
    ImageInfo *image_info;

    orig_x = orig_y = x = y = 0;
    
    GetExceptionInfo(&exception);
    image_info = CloneImageInfo((ImageInfo *) NULL);

    // first we read the image
    if (exception.severity != UndefinedException)
    {
#if 0
		if (exception.reason)
            g_bpCoreFunctions->log(BP_ERROR, "after: %s\n",
                                   exception.reason);
		if (exception.description)
            g_bpCoreFunctions->log(BP_ERROR, "after: %s\n",
                                   exception.description);
#endif // 0
		CatchException(&exception);
    }


	(void) strcpy(image_info->filename, inPath.c_str());
    images = IP_ReadImageFile(image_info, inPath, &exception);
    
    if (exception.severity != UndefinedException)
    {
#if 0
		if (exception.reason)
            g_bpCoreFunctions->log(BP_ERROR, "after: %s\n",
                                   exception.reason);
		if (exception.description)
            g_bpCoreFunctions->log(BP_ERROR, "after: %s\n",
                                   exception.description);
#endif // 0
		CatchException(&exception);
    }
    
    if (!images)
    {
        oError.append("couldn't read image");
        DestroyImageInfo(image_info);
        image_info = NULL;
        DestroyExceptionInfo(&exception);
        return std::string();
    }

#if 0
	g_bpCoreFunctions->log(
        BP_INFO, "Image contains %lu frames, type: %s\n",
        GetImageListLength(images),
        images->magick);
#endif // 0

    // set quality
    if (quality > 100) quality = 100;
    if (quality < 0) quality = 0;
    image_info->quality = quality;

#if 0
    g_bpCoreFunctions->log(
        BP_INFO, "Quality set to %d (0-100, worst-best)", quality);
#endif // 0

    // execute 'actions' 
    images = runTransformations(images, transformations, quality, oError);

    // was all that successful?
    if (!images)
    {
        DestroyImageInfo(image_info);
        image_info = NULL;
        DestroyExceptionInfo(&exception);
        return std::string();
    } 

    // set the output size
    orig_x = images->magick_columns;
    orig_y = images->magick_rows;
    x = images->columns;
    y = images->rows;

    // let's set the output format correctly (default to input format)
    std::string name;
    boost::filesystem::path inPath1(inPath);
    if (outputFormat == UNKNOWN) name.append(inPath1.stem().string());
    else {
        name.append("img.");
        name.append(typeToExt(outputFormat));
        (void) sprintf(images->magick, outputFormat);
#if 0
        g_bpCoreFunctions->log(BP_INFO, "Output to format: %s", outputFormat);
#endif // 0
    }
    
    // Now let's go directly from blob to file.  We bypass
    // GM to-file functions so that we can handle wide filenames
    // safely on win32 systems.  A superior solution would
    // be to use GM stream facilities (if they exist)
    
    // upon success, will hold path to output file and will be returned to
    // client
    std::string rv;
    
    {
        size_t l = 0;
        void * blob = NULL;
        blob = ImageToBlob( image_info, images, &l, &exception );

        if (exception.severity != UndefinedException)
        {
            oError.append("ImageToBlob failed.");
            CatchException(&exception);
        }
        else
        {
#if 0
            g_bpCoreFunctions->log(BP_INFO, "Creating tempdir %s",
                                   tmpDir.c_str());
#endif // 0
#if 0
            g_bpCoreFunctions->log(BP_INFO, "Writing %lu bytes to %s",
                                   l, name.c_str());
#endif // 0

            if (!boost::filesystem::is_directory(tmpDir) && !boost::filesystem::create_directory(tmpDir)) {
                oError.append("Couldn't create temp dir");
            } else {
                std::ofstream ofs;
                boost::filesystem::path outpath = bp::file::getTempPath(tmpDir, name);
                if (!bp::file::openWritableStream(ofs, outpath, std::ios_base::out | std::ios_base::binary)) {
                    throw std::string("unable to create new file");
#if 0
                    g_bpCoreFunctions->log(
                        BP_ERROR, "Couldn't open '%s' for writing!",
                        outpath.string().c_str());
#endif // 0
                    oError.append("Error saving output image");
                }
                ofs.write((const char*)blob, l);
                bool bad = ofs.bad();
                ofs.close();

                if (bad) {
#if 0
                    g_bpCoreFunctions->log(
                        BP_ERROR,
                        "Bad write (\?\?/%lu) when writing resultant "
                        "image '%s'",
                        l, outpath.string().c_str());
#endif // 0
                    oError.append("Error saving output image");
                } else {
                    // success!
                    rv = outpath.string();
                }
            }
        }
    }
    
    DestroyImage(images);
    DestroyImageInfo(image_info);
    image_info = NULL;
    DestroyExceptionInfo(&exception);

    return rv;
}
