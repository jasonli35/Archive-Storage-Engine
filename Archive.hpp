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
namespace ECE141 {

    enum class ActionType {added, extracted, removed, listed, dumped, compacted};
    enum class AccessMode {AsNew, AsExisting}; //you can change values (but not names) of this enum

    struct ArchiveObserver {
        void operator()(ActionType anAction, const std::string &aName, bool status);
    };

    class IDataProcessor {
    public:
        virtual std::vector<uint8_t> process(const std::vector<uint8_t>& input) = 0;
        virtual std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) = 0;
        virtual ~IDataProcessor(){};
    };

    /** This is new child class of data processor, use it to compress the if add asks for it*/
    class Compression : public IDataProcessor {
    public:
        std::vector<uint8_t> process(const std::vector<uint8_t>& input) override {
            // write the compress process here

            return std::vector<uint8_t >(0);
        }

        std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) override {
            // write the compress process here
            return input;
        }
        ~Compression() override = default;
    };



    enum class ArchiveErrors {
        noError=0,
        fileNotFound=1, fileExists, fileOpenError, fileReadError, fileWriteError, fileCloseError,
        fileSeekError, fileTellError, fileError, badFilename, badPath, badData, badBlock, badArchive,
        badAction, badMode, badProcessor, badBlockType, badBlockCount, badBlockIndex, badBlockData,
        badBlockHash, badBlockNumber, badBlockLength, badBlockDataLength, badBlockTypeLength, fileCreateError
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
    const size_t KBlockSize{1024};
    const size_t fileNameMaxSize{32};


    struct __attribute__ ((__packed__)) FileMeta {
        size_t endOfFile_index = 0;
    };

    const std::streampos fileHeader_size = KBlockSize;



    struct __attribute__ ((__packed__)) BlockHeader {
        signed long long next_block = -1; //this is varaible is first for a reason
        bool occupied = false;
        signed long long previous_block_index = -1;
        size_t byte_stored = 0;
        size_t fileName_size = 0;
    };
    const size_t BlockHeaderSize = sizeof(BlockHeader);

    const size_t data_size = KBlockSize - BlockHeaderSize;

    struct __attribute__ ((__packed__)) Block {
        BlockHeader meta;
        char data[data_size];
    };

    class Archive {
    protected:
        std::string archiveFullPath;

        std::size_t endOfFilePos = 0;


        std::vector<std::shared_ptr<IDataProcessor>> processors;
        std::vector<std::shared_ptr<ArchiveObserver>> observers;
//        std::ofstream write_stream;
//        std::ifstream read_stream;
        std::fstream archiveStream;


        static std::string process_archive_name(const std::string& aName);
        bool update_disk_header(BlockHeader& aHeader, size_t index);
        Archive(const std::string &aFullPath, AccessMode aMode);  //protected on purpose
        using ArchiveCallBack = std::function<bool(Block &, size_t)>;
        bool getBlock(Block &aBlock, signed long long aPos);
        Archive& each(const ArchiveCallBack& aCallBack);
        std::string getFileName(const std::string& filePath);

        void openSteams(const std::string &aFullPath);

        std::queue<size_t> free_block_index;

        void notify_all_observers(ActionType anAction, const std::string &aName, bool status);

        size_t getNextFreeBlock();

        ArchiveStatus<size_t> update_parent_index(signed long long parent_block_index, signed long long children_index);

        size_t getEOFindex();

        bool check_stream_status (const std::fstream& file_stream_to_check);

        std::string getFileName(const Block& aBlock);

        ArchiveStatus<bool> writeBlockToFile(Block& theBlock, size_t index);


    public:

        ~Archive();  //
        friend class std::shared_ptr<Archive>;
        Archive(const Archive& aCopy);
        Archive& operator=(const Archive& aCopy);
        friend class std::shared_ptr<ECE141::Archive>;

        static size_t getFileSizeInByte(std::ifstream& readFileStream);

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
        ArchiveStatus<std::string> getFullPath() const; //get archive path (including .arc extension)





        //STUDENT: add anything else you want here, (e.g. blocks?)...

    };



}

#endif /* Archive_hpp */
