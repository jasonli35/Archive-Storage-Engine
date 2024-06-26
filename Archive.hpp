//
//  Archive.hpp
//
//
//
//

#ifndef Archive_hpp
#define Archive_hpp

#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <queue>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <unordered_set>
#include <cstring>
#include <zlib.h>
#include "Chunker.hpp"

#include <sstream>

namespace ECE141 {

    enum class ActionType {added, extracted, removed, listed, dumped, compacted};
    enum class AccessMode {AsNew, AsExisting}; //you can change values (but not names) of this enum

    struct ArchiveObserver {
        void operator()(ActionType anAction, const std::string &aName, bool status);
    };



    /** This is new child class of data processor, use it to compress the if add asks for it*/
    class Compression : public IDataProcessor {
    public:
        std::vector<uint8_t> process(const std::vector<uint8_t>& input) override;

        std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) override;
        ~Compression() override = default;

    };



    enum class ArchiveErrors {
        noError=0,
        fileNotFound=1, fileExists, fileOpenError, fileReadError, fileWriteError, fileCloseError,
        fileSeekError, fileTellError, fileError, badFilename, badPath, badData, badBlock, badArchive,
        badAction, badMode, badProcessor, badBlockType, badBlockCount, badBlockIndex, badBlockData,
        badBlockHash, badBlockNumber, badBlockLength, badBlockDataLength, badBlockTypeLength, fileCreateError,
        invalidArgument
    };

    template<typename T>
    class ArchiveStatus {
    public:
        // Constructor for success case
        explicit ArchiveStatus(const T value)
                : value(value), error(ArchiveErrors::noError) {}

        // Constructor for error case
        explicit ArchiveStatus(ArchiveErrors anError)
                : value(std::nullopt), error(anError) {
            if (anError == ArchiveErrors::noError) {
                throw std::logic_error("Cannot use noError with error constructor");
            }
        }

        // Deleted copy constructor and copy assignment to make ArchiveStatus move-only
        ArchiveStatus(const ArchiveStatus&) = delete;
        ArchiveStatus& operator=(const ArchiveStatus&) = delete;

        // Default move constructor and move assignment
        ArchiveStatus(ArchiveStatus&&) noexcept = default;
        ArchiveStatus& operator=(ArchiveStatus&&) noexcept = default;

        T getValue() const {
            if (!isOK()){
                throw std::runtime_error("Operation failed with error");
            }
            return *value;
        }

        bool isOK() const { return error == ArchiveErrors::noError && value.has_value(); }
        ArchiveErrors getError() const { return error; }

    private:
        std::optional<T> value;
        ArchiveErrors error;
    };


    //--------------------------------------------------------------------------------
    //You'll need to define your own classes for Blocks, and other useful types...
    //--------------------------------------------------------------------------------


    class Archive {
    protected:
        std::string archiveFullPath;
        std::vector<std::shared_ptr<IDataProcessor>> processors;
        std::vector<std::shared_ptr<ArchiveObserver>> observers;
//        std::ofstream write_stream;
//        std::ifstream read_stream;
        std::fstream archiveStream;

        void findFirstBlock(size_t& index, size_t& fileName_size, const std::string &aFilename);

        void handleReverseProcess(const Block& theBlock, std::vector<char>& contentVector, size_t& byteToWrite);

        static std::string process_archive_name(const std::string& aName);
        bool update_disk_header(BlockHeader& aHeader, size_t index);
        Archive(const std::string &aFullPath, AccessMode aMode);  //protected on purpose
        using ArchiveCallBack = std::function<bool(Block &, size_t)>;
        bool getBlock(Block &aBlock, signed long long aPos);
        Archive& each(const ArchiveCallBack& aCallBack);
        std::string getFileName(const std::string& filePath);

        void openSteams(const std::string &aFullPath);


        void notify_all_observers(ActionType anAction, const std::string &aName, bool status);



        size_t getNextFreeBlock();

        ArchiveStatus<size_t> update_parent_index(signed long long parent_block_index, signed long long children_index);

        size_t getEOFindex();

        bool check_stream_status (const std::fstream& file_stream_to_check);

        std::string getFileName(const Block& aBlock);

        ArchiveStatus<bool> writeBlockToFile(Block& theBlock, size_t index);

        std::iostream& vectorToFstreamAdpoter(const std::vector<uint8_t>& theVec);
        std::vector<uint8_t> fstreamAdpoterToVec(std::istream& aFstream);

        void preprocess(std::iostream& aStream, const std::string& fname, size_t& uncompressed_fname_size,IDataProcessor* aProcessor, uint32_t& fname_hash, std::string& short_file_name);
        std::vector<uint8_t> stringToVectorUInt8(const std::string& str);


    public:

        ~Archive();  //
        friend class std::shared_ptr<Archive>;
        Archive(const Archive& aCopy);
        Archive& operator=(const Archive& aCopy);
        friend class std::shared_ptr<ECE141::Archive>;
        using IntVector = std::vector<signed long long>;

        ArchiveStatus<bool> getFreeBlocks(size_t aCount, IntVector &aList);

        size_t countBlocks();

        static size_t getFileSizeInByte(std::fstream& readFileStream);

        static    ArchiveStatus<std::shared_ptr<Archive>> createArchive(const std::string &anArchiveName);
        static    ArchiveStatus<std::shared_ptr<Archive>> openArchive(const std::string &anArchiveName);

        Archive&  addObserver(std::shared_ptr<ArchiveObserver> anObserver);

        static uint32_t hashString(const char *str);

        ArchiveStatus<bool>      add(const std::string &aFilename, IDataProcessor* aProcessor=nullptr);
        ArchiveStatus<bool>      extract(const std::string &aFilename, const std::string &aFullPath);
        ArchiveStatus<bool>      remove(const std::string &aFilename);

        ArchiveStatus<size_t>    list(std::ostream &aStream);
        ArchiveStatus<size_t>    debugDump(std::ostream &aStream);

        ArchiveStatus<size_t>    compact();

        //STUDENT: add anything else you want here, (e.g. blocks?)...

    };


}

#endif /* Archive_hpp */
