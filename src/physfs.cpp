#include <streambuf>
#include <string>
#include <stdexcept>
#include "physfs.hpp"

using std::streambuf;
using std::ios_base;

namespace PhysFS {

class fbuf : public streambuf
{
public:
    fbuf(const fbuf& other) = delete;
    fbuf& operator=(const fbuf& other) = delete;

private:
    int_type underflow() override
    {
        if (PHYSFS_eof(file)) 
        {
            return traits_type::eof();
        }

        const auto bytesRead = PHYSFS_readBytes(file, buffer, bufferSize);

        if (bytesRead < 1) {
            return traits_type::eof();
        }
        setg(buffer, buffer, buffer + bytesRead);
        return (unsigned char)*gptr();
    }

    pos_type seekoff(off_type pos, ios_base::seekdir dir, ios_base::openmode mode) override
    {
        switch (dir)
        {
        case std::ios_base::beg:
            PHYSFS_seek(file, pos);
            break;
        case std::ios_base::cur:
            // subtract characters currently in buffer from seek position
            PHYSFS_seek(file, (PHYSFS_tell(file) + pos) - (egptr() - gptr()));
            break;
        case std::ios_base::end:
            PHYSFS_seek(file, PHYSFS_fileLength(file) + pos);
            break;
        default:
            break;
        }

        if (mode & std::ios_base::in) 
        {
            setg(egptr(), egptr(), egptr());
        }

        if (mode & std::ios_base::out) 
        {
            setp(buffer, buffer);
        }
        return PHYSFS_tell(file);
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode mode) override
    {
        PHYSFS_seek(file, pos);
        if (mode & std::ios_base::in) 
        {
            setg(egptr(), egptr(), egptr());
        }

        if (mode & std::ios_base::out) 
        {
            setp(buffer, buffer);
        }

        return PHYSFS_tell(file);
    }

    int_type overflow(int_type c = traits_type::eof()) override
    {
        // Write buffer
        if (flush() < 0)
        {
            return traits_type::eof();
        }

        setp(buffer, buffer + bufferSize); // reset the put pointers

        if (c != traits_type::eof()) 
        {
            *pptr() = (char)c; // Add the overflow character to the put buffer
            pbump(1); // increment the put pointer by one
        }

        return c;
    }

    int sync() override
    {
        const auto result = flush();
        setp(buffer, buffer + bufferSize); // reset the put pointers

        return result;
    }

    int flush() const
    {
        const auto len = pptr() - pbase();
        if (len == 0)
        {
            return 0;  // nothing to do
        }

        if (PHYSFS_writeBytes(file, pbase(), len) < len)
        {
            return -1;
        }

        return 0;
    }

    char* buffer;
    size_t const bufferSize;

protected:
    PHYSFS_File* const file;

public:
    explicit fbuf(PHYSFS_File* file, std::size_t bufferSize = 2048) : bufferSize(bufferSize), file(file)
    {
        buffer = new char[bufferSize];
        char* end = buffer + bufferSize;
        setg(end, end, end);
        setp(buffer, end);
    }

    ~fbuf() override
    {
        flush();
        delete[] buffer;
    }
};

base_fstream::base_fstream(PHYSFS_File* file) : file(file)
{
    if (file == nullptr) 
    {
        throw std::invalid_argument("attempted to construct fstream with NULL ptr");
    }
}

base_fstream::~base_fstream()
{
    PHYSFS_close(file);
    file = nullptr;
}

PhysFS::size_t base_fstream::length()
{
    return PHYSFS_fileLength(file);
}

PHYSFS_File* openWithMode(char const* filename, mode openMode)
{
    PHYSFS_File* file = nullptr;
    switch (openMode)
    {
    case WRITE:
        file = PHYSFS_openWrite(filename);
        break;
    case APPEND:
        file = PHYSFS_openAppend(filename);
        break;
    case READ:
        file = PHYSFS_openRead(filename);
    }

    if (file == nullptr)
    {
        throw std::invalid_argument("file not found: " + std::string(filename));
    }
    return file;
}

void base_fstream::close()
{
    if (file)
    {
        PHYSFS_close(file);
        file = nullptr;
    }
}

ifstream::ifstream(const string& filename)
    : base_fstream(openWithMode(filename.c_str(), READ)), std::istream(new fbuf(file)) {}

ifstream::~ifstream()
{
    delete rdbuf();
}

void ifstream::open(std::string const& filename, mode)
{
    delete rdbuf();
    close();
    file = openWithMode(filename.c_str(), READ);
    if (file)
    {
        set_rdbuf(new fbuf(file));
    }
}

void ifstream::close()
{
    base_fstream::close();

    delete rdbuf();
    set_rdbuf(nullptr);
}

ofstream::ofstream(const string& filename, mode writeMode)
    : base_fstream(openWithMode(filename.c_str(), writeMode)), std::ostream(new fbuf(file)) {}

ofstream::~ofstream()
{
    delete rdbuf();
}

void ofstream::open(std::string const& filename, mode openMode)
{
    close();
    file = openWithMode(filename.c_str(), openMode);
    if (file)
    {
        set_rdbuf(new fbuf(file));
    }
}

void ofstream::close()
{
    base_fstream::close();

    delete rdbuf();
    set_rdbuf(nullptr);
}

fstream::fstream(const string& filename, mode openMode)
    : base_fstream(openWithMode(filename.c_str(), openMode)), std::iostream(new fbuf(file)) {}

fstream::~fstream()
{
    delete rdbuf();
}

void fstream::open(std::string const& filename, mode openMode)
{
    close();
    file = openWithMode(filename.c_str(), openMode);
    if (file)
    {
        set_rdbuf(new fbuf(file));
    }
}

void fstream::close()
{
    base_fstream::close();

    delete rdbuf();
    set_rdbuf(nullptr);
}

Version getLinkedVersion() {
    Version version;
    PHYSFS_getLinkedVersion(&version);
    return version;
}

void init(const char* argv0) {
    PHYSFS_init(argv0);
}

void deinit() {
    PHYSFS_deinit();
}

ArchiveInfoList supportedArchiveTypes() {
    ArchiveInfoList list;
    for (const ArchiveInfo** archiveType = PHYSFS_supportedArchiveTypes(); *archiveType != NULL; archiveType++) {
        list.push_back(**archiveType);
    }
    return list;
}

string getDirSeparator() {
    return PHYSFS_getDirSeparator();
}

void permitSymbolicLinks(bool allow) {
    PHYSFS_permitSymbolicLinks(allow);
}

StringList getCdRomDirs() {
    StringList dirs;
    char** dirBegin = PHYSFS_getCdRomDirs();
    for (char** dir = dirBegin; *dir != NULL; dir++) {
        dirs.push_back(*dir);
    }
    PHYSFS_freeList(dirBegin);
    return dirs;
}

void getCdRomDirs(StringCallback callback, void* extra) {
    PHYSFS_getCdRomDirsCallback(callback, extra);
}

string getBaseDir() {
    return PHYSFS_getBaseDir();
}

string getPrefDir(const string& org, const string& app) {
    return PHYSFS_getPrefDir(org.c_str(), app.c_str());
}

string getWriteDir() {
    return PHYSFS_getWriteDir();
}

void setWriteDir(const string& newDir) {
    PHYSFS_setWriteDir(newDir.c_str());
}

void unmount(const string& oldDir) {
    PHYSFS_unmount(oldDir.c_str());
}

StringList getSearchPath() {
    StringList pathList;
    char** pathBegin = PHYSFS_getSearchPath();
    for (char** path = pathBegin; *path != NULL; path++) {
        pathList.push_back(*path);
    }
    PHYSFS_freeList(pathBegin);
    return pathList;
}

void getSearchPath(StringCallback callback, void* extra) {
    PHYSFS_getSearchPathCallback(callback, extra);
}

void setSaneConfig(const string& orgName, const string& appName,
    const string& archiveExt, bool includeCdRoms, bool archivesFirst) {
    PHYSFS_setSaneConfig(orgName.c_str(), appName.c_str(), archiveExt.c_str(), includeCdRoms, archivesFirst);
}

int mkdir(const string& dirName) {
    return PHYSFS_mkdir(dirName.c_str());
}

int deleteFile(const string& filename) {
    return PHYSFS_delete(filename.c_str());
}

string getRealDir(const string& filename) {
    return PHYSFS_getRealDir(filename.c_str());
}

StringList enumerateFiles(const string& directory) {
    StringList files;
    char** listBegin = PHYSFS_enumerateFiles(directory.c_str());
    for (char** file = listBegin; *file != NULL; file++) {
        files.push_back(*file);
    }
    PHYSFS_freeList(listBegin);
    return files;
}

int enumerateFiles(string const& directory, EnumFilesCallback callback, void* extra) {
    return PHYSFS_enumerate(directory.c_str(), callback, extra);
}

bool exists(const string& filename) {
    return PHYSFS_exists(filename.c_str());
}

Stat getStat(const string& filename)
{
    Stat stat;
    PHYSFS_stat(filename.c_str(), &stat);

    return stat;
}

bool isDirectory(const string& filename) {
    return (getStat(filename).filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY);
}

bool isSymbolicLink(const string& filename) {
    return (getStat(filename).filetype == PHYSFS_FileType::PHYSFS_FILETYPE_SYMLINK);
}

sint64 getLastModTime(const string& filename) {
    return (getStat(filename).modtime);
}

bool isInit() {
    return PHYSFS_isInit();
}

bool symbolicLinksPermitted() {
    return PHYSFS_symbolicLinksPermitted();
}

void setAllocator(const Allocator* allocator) {
    PHYSFS_setAllocator(allocator);
}

void mount(const string& newDir, const string& mountPoint, bool appendToPath) {
    PHYSFS_mount(newDir.c_str(), mountPoint.c_str(), appendToPath);
}

string getMountPoint(const string& dir) {
    return PHYSFS_getMountPoint(dir.c_str());
}

sint16 Util::swapSLE16(sint16 value) {
    return PHYSFS_swapSLE16(value);
}

uint16 Util::swapULE16(uint16 value) {
    return PHYSFS_swapULE16(value);
}

sint32 Util::swapSLE32(sint32 value) {
    return PHYSFS_swapSLE32(value);
}

uint32 Util::swapULE32(uint32 value) {
    return PHYSFS_swapULE32(value);
}

sint64 Util::swapSLE64(sint64 value) {
    return PHYSFS_swapSLE64(value);
}

uint64 Util::swapULE64(uint64 value) {
    return PHYSFS_swapULE64(value);
}

sint16 Util::swapSBE16(sint16 value) {
    return PHYSFS_swapSBE16(value);
}

uint16 Util::swapUBE16(uint16 value) {
    return PHYSFS_swapUBE16(value);
}

sint32 Util::swapSBE32(sint32 value) {
    return PHYSFS_swapSBE32(value);
}

uint32 Util::swapUBE32(uint32 value) {
    return PHYSFS_swapUBE32(value);
}

sint64 Util::swapSBE64(sint64 value) {
    return PHYSFS_swapSBE64(value);
}

uint64 Util::swapUBE64(uint64 value) {
    return PHYSFS_swapUBE64(value);
}

string Util::utf8FromUcs4(const uint32* src) {
    string value;
    std::size_t length = strlen((char*)src);
    char* buffer = new char[length]; // will be smaller than len
    PHYSFS_utf8FromUcs4(src, buffer, length);
    value.append(buffer);
    return value;
}

string Util::utf8ToUcs4(const char* src) {
    string value;
    std::size_t length = strlen(src) * 4;
    char* buffer = new char[length]; // will be smaller than len
    PHYSFS_utf8ToUcs4(src, (uint32*)buffer, length);
    value.append(buffer);
    return value;
}

string Util::utf8FromUcs2(const uint16* src) {
    string value;
    std::size_t length = strlen((char*)src);
    char* buffer = new char[length]; // will be smaller than len
    PHYSFS_utf8FromUcs2(src, buffer, length);
    value.append(buffer);
    return value;
}

string Util::utf8ToUcs2(const char* src) {
    string value;
    std::size_t length = strlen(src) * 2;
    char* buffer = new char[length]; // will be smaller than len
    PHYSFS_utf8ToUcs2(src, (uint16*)buffer, length);
    value.append(buffer);
    return value;
}

string Util::utf8FromLatin1(const char* src) {
    string value;
    std::size_t length = strlen((char*)src) * 2;
    char* buffer = new char[length]; // will be smaller than len
    PHYSFS_utf8FromLatin1(src, buffer, length);
    value.append(buffer);
    return value;
}

}
