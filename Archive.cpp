//
//  Archive.cpp
//
//
//
//

#include "Archive.hpp"

namespace ECE141 {



  //STUDENT put archive class code here...
  Archive::Archive(const Archive& aCopy) {
    *this = aCopy;
  }



    Archive& Archive::operator=(const Archive& aCopy) {
        processors = aCopy.processors;
        observers = aCopy.observers;
        free_block_index = aCopy.free_block_index;
        archiveFullPath = aCopy.archiveFullPath;
        openSteams(archiveFullPath);

        return *this;
   }



  ArchiveStatus<std::shared_ptr<Archive>> Archive::createArchive(const std::string &anArchiveName) {

      std::string file_path = anArchiveName + ".arc";

      std::filesystem::path pathObj(file_path);
      std::filesystem::path dirPath = pathObj.parent_path();
      if (!std::filesystem::exists(dirPath)) {
          std::filesystem::create_directories(dirPath);
      }
//      Archive theArchive();
      Archive theArchiveObj(file_path, AccessMode::AsNew);
//              std::make_shared<Archive>(file_path, AccessMode::AsNew);
//
      if(!theArchiveObj.write_stream.good()) {
          return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileOpenError);
      }

      return ArchiveStatus(std::make_shared<Archive>(theArchiveObj));


  }

void Archive::openSteams(const std::string &aFullPath) {
      write_stream.open(aFullPath, std::ios::binary);
      if(!write_stream.is_open()) {
          std::cerr << "write stream is not opening" << std::endl;
      }
      read_stream.open(aFullPath, std::ios::binary);
    if(!read_stream.is_open()) {
        std::cerr << "read stream is not opening" << std::endl;
    }
  }

    Archive::Archive(const std::string &aFullPath, AccessMode aMode): archiveFullPath(aFullPath){
      if(aMode == AccessMode::AsNew) {
//          write_stream.open(aFullPath, std::ios::binary | std::ios::trunc);
          write_stream = std::ofstream (aFullPath, std::ios::trunc);

      }
      else {
          openSteams(aFullPath);
          if(!read_stream.is_open()) {
              std::cerr << "read_stream is not opening" << std::endl;
          }

      }

      if(aMode == AccessMode::AsNew) {
          write_stream.close();
      }

   }

    Archive::~Archive() {
        if(write_stream.is_open()) {
            write_stream.close();
        }
        if(read_stream.is_open()) {
            read_stream.close();
        }

    }



    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName) {
        Archive anArchive(anArchiveName, AccessMode::AsExisting);
        if(!anArchive.write_stream.good() | !anArchive.read_stream.good()) {return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileOpenError);}
        if(!anArchive.write_stream.is_open()) {return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::badPath);}
        return ArchiveStatus(std::make_shared<Archive>(anArchive));
    }


    uint32_t Archive::hashString(const char *str) {
      const int gMultiplier = 37;
      uint32_t h{0};
      unsigned char *p;
      for(p = (unsigned char*) str; *p != '\0'; p++)
          h = gMultiplier * h + *p;
      return h;
  }

  size_t Archive::getNextFreeBlock() {

        if(!free_block_index.empty()) {
            size_t index = free_block_index.front();
            free_block_index.pop();
            return index;
        }
        else {
            read_stream.seekg(0, std::ios::end);
            std::streampos tellGPos = read_stream.tellg();

            return tellGPos / KBlockSize;
        }
    }



    size_t Archive::getFileSizeInByte(std::ifstream& readFileStream) {
        std::streampos initial_position = readFileStream.tellg();
        readFileStream.seekg (0, std::ios::end);
        size_t sizeOfFile = readFileStream.tellg();
        readFileStream.seekg(initial_position);
       return sizeOfFile;
  }

    ArchiveStatus<bool> Archive::add(const std::string &aFilename) {
        if(aFilename.size() >= fileNameMaxSize) {
            std::cerr << "file name is too long. It must be less than 32 character" <<std::endl;
            return ArchiveStatus<bool>(ArchiveErrors::badFilename);
        }
      std::ifstream readFile(aFilename, std::ios::binary);
      if(!readFile.good()) {
          return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);
      }

      uint32_t fileName = hashString(aFilename.c_str());
      bool addingFristBlock = true;
      size_t byte_left_ToRead = Archive::getFileSizeInByte(readFile);

      size_t current_block_index = getNextFreeBlock();
      size_t next_block_index;
      const char* fName_c_string = aFilename.c_str();
      uint32_t fname_hash = hashString(fName_c_string);
      while(byte_left_ToRead > 0) {
          Block newBlock;

          std::strcpy(newBlock.fileName, fName_c_string);

          size_t readingBytes = byte_left_ToRead;

          BlockHeader &newBlockHeader = newBlock.meta;
          newBlockHeader.occupied = true;
          newBlockHeader.hash = fname_hash;
          newBlockHeader.previous_block_index = current_block_index;
          if(addingFristBlock) {
              addingFristBlock = false;
              newBlockHeader.previous_block_index = -1; //indicate first block of the file that copy from
          }
          current_block_index = next_block_index; // set it to the current inserting block index
          if(byte_left_ToRead > data_size) {
              next_block_index = getNextFreeBlock();;
              newBlockHeader.next_block = next_block_index;
              readingBytes = data_size;
          }

          if(readFile.read(newBlock.data, readingBytes)) {
              byte_left_ToRead -= readingBytes;
          }
          else {
              return ArchiveStatus<bool>(ArchiveErrors::fileReadError);
          }

          write_stream.seekp(current_block_index * KBlockSize);
          if(!write_stream.good()) {
              return ArchiveStatus<bool>(ArchiveErrors::fileSeekError);
          }
          write_stream.write(reinterpret_cast<const char*>(&newBlock), sizeof(newBlock));
          write_stream.flush();
          if(!write_stream.good()) {
              return ArchiveStatus<bool>(ArchiveErrors::fileWriteError);
          }


      }


      return ArchiveStatus(true);
    }

    ArchiveStatus<bool> Archive::extract(const std::string &aFilename, const std::string &aFullPath) {

        return ArchiveStatus(true);
    }

    ArchiveStatus<bool> Archive::remove(const std::string &aFilename) {

        return ArchiveStatus(true);
    }

    bool Archive::getBlock(Block &aBlock, int aPos) {

        read_stream.read(reinterpret_cast<char*>(&aBlock), KBlockSize);
        if(read_stream.good()) {
            return true;
        }

        return false;
    }



    Archive& Archive::each(ArchiveCallBack aCallBack) {
        int thePos{0};
        bool theResult{true};
        Block theBlock;
        read_stream.clear();
        read_stream.seekg(0, std::ios::beg);
        if(!read_stream.good()) {
            std::cerr << "bad seek" << std::endl;
        }

        while(theResult) {
            theResult = getBlock(theBlock, thePos);
            if(theResult) {
                theResult = aCallBack(theBlock, thePos++);
            }
        }
        return *this;
    }

    ArchiveStatus<size_t> Archive::list(std::ostream &aStream) {
        std::unordered_set<uint32_t> existingFile;
        std::cout << "(line 244) is reading stream open: " << read_stream.is_open() <<std::endl;
        each([&] (Block &aBlock, size_t aPos) {
            uint32_t fname_hash = aBlock.meta.hash;
            if(existingFile.find(fname_hash) == existingFile.end()) {
                existingFile.insert(fname_hash);
                aStream << aBlock.fileName << '\n';
            }
            return true;
        });

        return ArchiveStatus(existingFile.size());
    }
    ArchiveStatus<size_t> Archive::debugDump(std::ostream &aStream) {

        return ArchiveStatus((size_t) 0);
    }

    ArchiveStatus<size_t> Archive::compact() {

        return ArchiveStatus((size_t) 0);
    }

    ArchiveStatus<std::string> Archive::getFullPath() const {

        return ArchiveStatus((std::string)"");
    }

    Archive&  Archive::addObserver(std::shared_ptr<ArchiveObserver> anObserver) {

        return *this;
    }


    void ArchiveObserver::operator()(ActionType anAction, const std::string &aName, bool status) {

   }




}
