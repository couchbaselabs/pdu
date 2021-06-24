#pragma once

#include "decoder.h"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/iostreams/stream.hpp>
#include <string_view>

using namespace boost::interprocess;
namespace io = boost::iostreams;

namespace boost::filesystem {
class path;
}

/**
 * Container for an mmapped file, exposing an std::istream.
 *
 * Designed to allow the easy substitution of a mapped file in place of
 * an ifstream.
 */
class FileMap {
public:
    FileMap(const boost::filesystem::path& fileName);

    Decoder& operator*();

    Decoder& get();

private:
    file_mapping mappedFile;
    mapped_region region;
    Decoder decoder;
};