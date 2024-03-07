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

   std::string Archive::process_archive_name(const std::string& aName) {
       std::string last3 = aName.substr(aName.size()-4);
       std::string file_path = aName;
       if (last3 != ".arc") {
           file_path+=".arc";
       }
       return file_path;
  }


  ArchiveStatus<std::shared_ptr<Archive>> Archive::createArchive(const std::string &anArchiveName) {

      std::string file_path = process_archive_name(anArchiveName);

      auto temp = new Archive(file_path, AccessMode::AsNew);

        if (temp->archiveStream.is_open())
            return ArchiveStatus(std::shared_ptr<Archive>(temp));
        else
            return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileOpenError);


  }

void Archive::openSteams(const std::string &aFullPath) {
      archiveStream.open(process_archive_name(aFullPath), std::ios::in | std::ios::out | std::ios::binary);
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
          archiveStream.open(aFullPath, std::ios::out|std::ios::in|std::ios::binary|std::ios::trunc);
          if(!archiveStream.is_open()) {
              std::cerr << "Archive file cannot be created" << std::endl;
          }
          else {
              endOfFilePos = 0;
          }
      }
      else {
          openSteams(aFullPath);
          endOfFilePos = getEOFindex();
      }

   }



    Archive::~Archive() {

        if(archiveStream.is_open()) {
            archiveStream.clear();
            archiveStream.seekp(0);
            FileMeta fMeta;
            fMeta.endOfFile_index = endOfFilePos;
            archiveStream.clear();
            archiveStream.write(reinterpret_cast<char*>(&fMeta), fileHeader_size);
            archiveStream.flush();
            archiveStream.close();
        }

    }



    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName) {
        Archive anArchive(anArchiveName, AccessMode::AsExisting);

        if(!anArchive.archiveStream.is_open()) {return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::badPath);}
        return ArchiveStatus(std::make_shared<Archive>(anArchive));
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

    std::string Archive::getFileName(const std::string& filePath) {

        size_t pos = filePath.find_last_of("/");

        if (pos == std::string::npos) return filePath;

        return filePath.substr(pos + 1);
    }

    ArchiveStatus<bool> Archive::add(const std::string &aFilename) {
        std::string short_file_name = getFileName(aFilename);

        if(short_file_name.size() >= fileNameMaxSize) {
            std::cerr << "file name is too long. It must be less than 32 character" <<std::endl;
            notify_all_observers(ActionType::added, aFilename, false);
            return ArchiveStatus<bool>(ArchiveErrors::badFilename);
        }
      std::ifstream readFile(aFilename, std::ifstream::binary);

      if(!readFile.good()) {
          notify_all_observers(ActionType::added, aFilename, false);
          return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);
      }


      bool addingFristBlock = true;
      size_t byte_left_ToRead = Archive::getFileSizeInByte(readFile);




      size_t current_block_index;
      int next_block_index = -1;


      while(byte_left_ToRead > 0 || (addingFristBlock && byte_left_ToRead == 0)) {

          if(next_block_index == -1) {
              current_block_index = getNextFreeBlock();
          }
          else {
              current_block_index = next_block_index;
          }
          Block newBlock;

          std::strcpy(newBlock.fileName, short_file_name.c_str());

          size_t readingBytes = byte_left_ToRead;

          BlockHeader &newBlockHeader = newBlock.meta;
          newBlockHeader.occupied = true;
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
          newBlockHeader.byte_stored = readingBytes;
          readFile.read(newBlock.data, readingBytes);
          if(readFile) {
              byte_left_ToRead -= readingBytes;

          }
          else {
              notify_all_observers(ActionType::added, aFilename, false);
              return ArchiveStatus<bool>(ArchiveErrors::fileReadError);
          }

          archiveStream.clear();
          archiveStream.seekp(block_index_to_address(current_block_index));

          if(!check_stream_status(archiveStream)) {
              return ArchiveStatus<bool>(ArchiveErrors::fileSeekError);
          }

          archiveStream.write(reinterpret_cast<char*>(&newBlock), KBlockSize);

          archiveStream.flush();
          if(!check_stream_status(archiveStream)) {
              return ArchiveStatus<bool>(ArchiveErrors::fileWriteError);
          }

      }
      readFile.close();
      notify_all_observers(ActionType::added, aFilename, true);
      return ArchiveStatus(true);
    }

    ArchiveStatus<bool> Archive::extract(const std::string &aFilename, const std::string &aFullPath) {
      int current_index = -1;

      std::fstream write_file(aFullPath.c_str(), std::ostream::binary | std::ostream::app | std::ios::out);

        check_stream_status(write_file);
        each([&] (Block &aBlock, size_t aPos) {
            if(aBlock.meta.occupied && aBlock.fileName == aFilename && aBlock.meta.previous_block_index == -1) {
                current_index = aPos;
                return false;
            }
            else {
                return true;
            }
        });

        if(current_index == - 1) {
            return ArchiveStatus<bool>(false);
        }

        while(current_index != -1) {
            Block current_block;
            getBlock(current_block, current_index);
            write_file.write(current_block.data, current_block.meta.byte_stored);
            if(check_stream_status(write_file)) {
                return ArchiveStatus<bool>(ArchiveErrors::fileWriteError);
            }
            current_index = current_block.meta.next_block;
        }

        return ArchiveStatus<bool>(true);


    }

    ArchiveStatus<bool> Archive::remove(const std::string &aFilename) {
      bool isFileExist = false;
      each([&] (Block& theBlock, size_t aPos) {
          if(theBlock.fileName == aFilename) {
              theBlock.meta.occupied = false;

              free_block_index.push(aPos);
              isFileExist = true;
          }
          return true;
      });
      return ArchiveStatus<bool>(isFileExist);

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
        archiveStream.clear();
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


        while(theResult) {
            Block theBlock;
            theResult = getBlock(theBlock, thePos);
            if(theResult) {
                theResult = aCallBack(theBlock, thePos++);
            }
        }
        return *this;
    }

    ArchiveStatus<size_t> Archive::list(std::ostream &aStream) {
        std::unordered_set<std::string> existingFile;
        aStream << "###  name         size       date added" << std::endl;
        aStream << "--------------------------------" << std::endl;
        each([&] (Block &aBlock, size_t aPos) {

            if(aBlock.meta.occupied) {
                std::string current_fname = aBlock.fileName;
                if(!existingFile.count(current_fname)) {
                    existingFile.insert(current_fname);
                    aStream << current_fname << '\n';
                }
            }

            return true;
        });

        notify_all_observers(ActionType::listed, "", true);
        return ArchiveStatus(existingFile.size());
    }
    ArchiveStatus<size_t> Archive::debugDump(std::ostream &aStream) {
      aStream << "###  status   name    " << std::endl;
      aStream << "-----------------------" << std::endl;
      size_t count = 0;

      for(size_t i = 0; i < endOfFilePos; ++i) {
         aStream << count << ".   ";
         Block currentBlock;
         getBlock(currentBlock, i);
         if(currentBlock.meta.occupied) {
             aStream << "used     " << currentBlock.fileName << std::endl;
         }
         else {
             aStream << "empty" << std::endl;
         }
         ++count;
      }

        return ArchiveStatus(count);

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
