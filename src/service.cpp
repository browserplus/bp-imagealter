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

#include "bpservice/bpservice.h"
#include "ImageProcessor.hh"
#include "Transformations.hh"
#include "bp-file/bpfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

// NEEDSWORK!!!  Delete this once we move to v5 and no longer need file uri's
#include "bpurlutil.cpp"
// END NEEDSWORK!!!

#define IA_DEFAULT_QUALITY 75
#define IA_DEFAULT_QUALITY_STR "75"

class ImageAlter : public bplus::service::Service {
public:
BP_SERVICE(ImageAlter)
    ImageAlter();
    ~ImageAlter();
    static bool onServiceLoad();
    static bool onServiceUnload();
    virtual void finalConstruct();
    void transform(const bplus::service::Transaction& tran, const bplus::Map& args);
private:
    std::string m_tempDir;
};

BP_SERVICE_DESC(ImageAlter, "ImageAlter", "4.1.0",
                "Implements client side Image manipulation")
ADD_BP_METHOD(ImageAlter, transform,
              "Perform a set of transformations on an input image")
ADD_BP_METHOD_ARG(transform, "file", Path, true,
                  "The image to transform.")
ADD_BP_METHOD_ARG(transform, "format", String, false,
                  "The format of the output image.  "
                  "Default is to output in the same "
                  "format as the input image.  A string, one of: "
                  "jpg, gif, or png")
ADD_BP_METHOD_ARG(transform, "quality", Integer, false,
                  "The quality of the output image.  "
                  "From 0-100.  Lower qualities "
                  "result in faster operations and smaller file "
                  "sizes, at the cost of image quality (default: "
                  IA_DEFAULT_QUALITY_STR ")")
ADD_BP_METHOD_ARG(transform, "actions", List, false,
                  "An array of actions to perform.  Each action is either a "
                  "string (i.e. { actions: [ 'solarize' ] }) , or an object with "
                  "a single property, where the property name is the action "
                  "to perform, and the property value is the argument (i.e. "
                  "{ actions: [{rotate: 90}] }.  Supported actions include: ")
#if 0
        // add all actions
        for (unsigned int i = 0; i < trans::num(); i++) {
            const trans::Transformation* t = trans::get(i);
            ss << t->name << " -- " << t->doc << " | ";
        }
#endif // 0
END_BP_SERVICE_DESC

ImageAlter::ImageAlter() {
}

ImageAlter::~ImageAlter() {
}

bool
ImageAlter::onServiceLoad() {
    // initialize the GraphicsMagick engine.  vroom.
    imageproc::init();
    return true;
}

bool
ImageAlter::onServiceUnload() {
    // shutdown the GraphicsMagick engine.  vroom.
    imageproc::shutdown();
    return true;
}

void
ImageAlter::finalConstruct() {
    // extract the temporary directory
    m_tempDir = context("temp_dir");
    boost::filesystem::create_directory(m_tempDir);
    std::stringstream ss;
    ss << "session allocated, using temp dir: " << (m_tempDir.empty() ? "<empty>" : m_tempDir);
    log(BP_INFO, ss.str());
}

void
ImageAlter::transform(const bplus::service::Transaction& tran, const bplus::Map& args) {
    std::stringstream ss;
    // XXX: we need to get a little thready here
    // first we'll get the input file into a string
    const bplus::Path* bpPath = dynamic_cast<const bplus::Path*>(args.value("file"));
// NEEDSWORK!!!  Fix this when port to v5 since we don't need URI's anymore
#if 0
    std::string path = bpPath;
#else // 0
    std::string path = bp::urlutil::pathFromURL((std::string)(*bpPath));
#endif // 0
    if (path.empty()) {
        ss.str("");
        ss << "can't parse file:// url: " << (std::string)(*bpPath);
        log(BP_ERROR, ss.str());
        tran.error("bp.fileAccessError", "invalid file URI");
        return;
    }
    // now let's figure out the output format
    imageproc::Type t = imageproc::UNKNOWN;
    if (args.has("format")) {
        t = imageproc::pathToType(*(args.get("format")));
        if (t == imageproc::UNKNOWN) {
            log(BP_ERROR, "can't determine output format");
            tran.error("bp.invalidArguments", "can't determine output format");
            return;
        }
    }
    // extract quality argument
    int quality = IA_DEFAULT_QUALITY;
    if (args.has("quality", BPTInteger)) {
        quality = (int)(long long)*((const bplus::Integer*)(args.get("quality")));
    }
    // finally, let's pull out the list of transformation actions
    bplus::List emptyList;
    bplus::List* lPtr = &emptyList;
    if (args.has("actions")) {
        lPtr = (bplus::List *) args.get("actions");
    }
    std::string err;
    unsigned int x;
    unsigned int y;
    unsigned int orig_x;
    unsigned int orig_y;
    std::string rez = imageproc::ChangeImage(path, m_tempDir, t, *lPtr, quality, x, y, orig_x, orig_y, err);
    if (rez.empty()) {
        if (err.empty()) {
            err.append("unknown");
        }
        // error!
        ss.str("");
        ss << "couldn't transform image: " << err;
        log(BP_ERROR, ss.str());
        tran.error("bp.transformFailed", err.c_str());
    }
    else {
        // success!
        bplus::Map m;
        m.add("file", new bplus::Path(rez));
        m.add("width", new bplus::Integer(x));
        m.add("height", new bplus::Integer(y));
        m.add("orig_width", new bplus::Integer(orig_x));
        m.add("orig_height", new bplus::Integer(orig_y));
        tran.complete(m);
    }
}
