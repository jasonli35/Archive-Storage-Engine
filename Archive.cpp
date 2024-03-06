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

      return ArchiveStatus(std::make_shared<Archive>(theArchiveObj));


  }

void Archive::openSteams(const std::string &aFullPath) {
      archiveStream.open(aFullPath, std::ios::in | std::ios::out | std::ios::binary);
      if(!archiveStream.is_open()) {
          std::cerr << "archiveStream is not opening" << std::endl;
      }

  }

    size_t Archive::getEOFindex() {
        FileMeta fMeta;
        archiveStream.clear();
        archiveStream.read(reinterpret_cast<char*>(&fMeta), fileHeader_size);
        return fMeta.endOfFile_index;
  }

    Archive::Archive(const std::string &aFullPath, AccessMode aMode): archiveFullPath(aFullPath) {
      if(aMode == AccessMode::AsNew) {
          std::ofstream fileStream(aFullPath, std::ios::trunc);
          if(!fileStream.is_open()) {
              std::cerr << "Archive file cannot be created" << std::endl;
          }
          else {
              endOfFilePos = 0;
              fileStream.close();
          }
      }
      else {
          openSteams(aFullPath);
          endOfFilePos = getEOFindex();
      }


   }



    Archive::~Archive() {
        archiveStream.clear();
        archiveStream.seekp(0);
        FileMeta fMeta;
        fMeta.endOfFile_index = endOfFilePos;
        archiveStream.clear();
        archiveStream.write(reinterpret_cast<char*>(&fMeta), fileHeader_size);
        if(archiveStream.is_open()) {
            archiveStream.close();
        }

    }



    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName) {
        Archive anArchive(anArchiveName, AccessMode::AsExisting);

        if(!anArchive.archiveStream.is_open()) {return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::badPath);}
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
            return endOfFilePos++;
        }
    }



    size_t Archive::getFileSizeInByte(std::ifstream& readFileStream) {

        readFileStream.seekg (0, std::ios::end);
        size_t sizeOfFile = readFileStream.tellg();
        readFileStream.seekg(0, std::ios::beg);
        readFileStream.clear();
       return sizeOfFile;
  }

  std::streampos block_index_to_address(size_t aPos) {
      return aPos * KBlockSize + fileHeader_size;
  }

    ArchiveStatus<bool> Archive::add(const std::string &aFilename) {
        if(aFilename.size() >= fileNameMaxSize) {
            std::cerr << "file name is too long. It must be less than 32 character" <<std::endl;
            notify_all_observers(ActionType::added, aFilename, false);
            return ArchiveStatus<bool>(ArchiveErrors::badFilename);
        }
      std::ifstream readFile(aFilename, std::ios::binary);
      if(!readFile.good()) {
          notify_all_observers(ActionType::added, aFilename, false);
          return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);
      }

      uint32_t fileName = hashString(aFilename.c_str());
      bool addingFristBlock = true;
      size_t byte_left_ToRead = Archive::getFileSizeInByte(readFile);


      const char* fName_c_string = aFilename.c_str();
      uint32_t fname_hash = hashString(fName_c_string);
      size_t current_block_index;
      int next_block_index = -1;

      std::ofstream anOutputStream(archiveFullPath, std::ofstream::binary);
        // have one output and dont close it
      while(byte_left_ToRead > 0 || (addingFristBlock && byte_left_ToRead == 0)) {

          if(next_block_index == -1) {
              current_block_index = getNextFreeBlock();
          }
          else {
              current_block_index = next_block_index;
          }
          Block newBlock;

          std::strcpy(newBlock.fileName, fName_c_string);

          size_t readingBytes = byte_left_ToRead;

          BlockHeader &newBlockHeader = newBlock.meta;
          newBlockHeader.occupied = true;
          newBlockHeader.hash = fname_hash;
          newBlockHeader.previous_block_index = current_block_index;
          if(addingFristBlock) {
              newBlockHeader.previous_block_index = -1; //indicate first block of the file that copy from
              addingFristBlock = false;
          }
           // set it to the current inserting block index
          if(byte_left_ToRead > data_size) {
              next_block_index = getNextFreeBlock();
              newBlockHeader.next_block = next_block_index;
              readingBytes = data_size;
          }

          readFile.read(newBlock.data, readingBytes);
          if(readFile) {
              byte_left_ToRead -= readingBytes;
              //readFile.seekg(readingBytes + readFile.tellg());
              // this is an issue becasue read already advance the file pointer
              // if you advance again it consumes too much next time corruption
          }
          else {
              notify_all_observers(ActionType::added, aFilename, false);
              return ArchiveStatus<bool>(ArchiveErrors::fileReadError);
          }

          anOutputStream.seekp(block_index_to_address(current_block_index));
          if(!anOutputStream.good()) {
              return ArchiveStatus<bool>(ArchiveErrors::fileSeekError);
          }
          anOutputStream.clear();
          anOutputStream.write(reinterpret_cast<char*>(&newBlock), KBlockSize);
          anOutputStream.flush();
          if(!anOutputStream.good()) {
              return ArchiveStatus<bool>(ArchiveErrors::fileWriteError);
          }

      }

      notify_all_observers(ActionType::added, aFilename, true);
      return ArchiveStatus(true);
    }

    ArchiveStatus<bool> Archive::extract(const std::string &aFilename, const std::string &aFullPath) {

        return ArchiveStatus(true);
    }

    ArchiveStatus<bool> Archive::remove(const std::string &aFilename) {

        return ArchiveStatus(true);
    }

    bool Archive::check_stream_status (const std::fstream& file_stream_to_check) {
      if(!file_stream_to_check.good()) {
          std::cout << "Error flags: ";
          if (file_stream_to_check.rdstate() & std::ios::eofbit)
              std::cout << "eof ";
          if (file_stream_to_check.rdstate() & std::ios::failbit)
              std::cout << "fail ";
          if (file_stream_to_check.rdstate() & std::ios::badbit)
              std::cout << "bad ";
          std::cout << std::endl;
          return false;
      }
      return true;
  }

    bool Archive::getBlock(Block &aBlock, int aPos) {
        archiveStream.seekg(block_index_to_address(aPos));
        if(!check_stream_status(archiveStream)) {
            return false;
        }
        archiveStream.clear();
        archiveStream.read(reinterpret_cast<char*>(&aBlock), KBlockSize);
        return check_stream_status(archiveStream);
    }



    Archive& Archive::each(const ArchiveCallBack& aCallBack) {
        int thePos{0};
        bool theResult{true};
        Block theBlock;
        archiveStream.clear();
        archiveStream.seekg(fileHeader_size, std::ios::beg);
        if(!archiveStream.good()) {
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
        aStream << "List of files in the archive: " << std::endl;
        aStream << "--------------------------------" << std::endl;
        each([&] (Block &aBlock, size_t aPos) {
            uint32_t fname_hash = aBlock.meta.hash;
            aStream<< "fname_hash: " << aBlock.fileName << std::endl;
            // your hash checker does not work properly you need ot fix commented out lines
            // for now I added debug at line 287 288 291
//            if(existingFile.find(fname_hash) == existingFile.end()) {
//                existingFile.insert(fname_hash);
//                aStream << aBlock.fileName << '\n';
//            }
            return true;
        });

        notify_all_observers(ActionType::listed, "", true);
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
        observers.push_back(anObserver);
        return *this;
    }

    void Archive::notify_all_observers(ActionType anAction, const std::string &aName, bool status) {
      for(const std::shared_ptr<ArchiveObserver>& anObserver: observers) {
          anObserver->operator()(anAction, aName, status);
      }
  }


    void ArchiveObserver::operator()(ActionType anAction, const std::string &aName, bool status) {

   }




}
