//
//  Archive.cpp
//
//
//
//

#include "Archive.hpp"

namespace ECE141 {

    std::ofstream Archive::write_stream;
    std::ifstream Archive::read_stream;

  //STUDENT put archive class code here...
  Archive::Archive(const Archive& aCopy) {
    *this = aCopy;
  }

  bool Archive::canOpenFile = true;

    Archive& Archive::operator=(const Archive& aCopy) {
        processors = aCopy.processors;
        observers = aCopy.observers;
        free_block_index = aCopy.free_block_index;


        return *this;
   }

  ArchiveStatus<std::shared_ptr<Archive>> Archive::createArchive(const std::string &anArchiveName) {

      std::string file_path = anArchiveName + ".arc";

      std::filesystem::path pathObj(file_path);
      std::filesystem::path dirPath = pathObj.parent_path();
      if (!std::filesystem::exists(dirPath)) {
          std::filesystem::create_directories(dirPath);
      }
      Archive theArchive(file_path, AccessMode::AsNew);
      std::shared_ptr<Archive> theArchivePtr =  std::make_shared<Archive>(theArchive);

      if(!canOpenFile) {
          return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileOpenError);
      }

      return ArchiveStatus(theArchivePtr);


  }

    Archive::Archive(const std::string &aFullPath, AccessMode aMode){
      if(aMode == AccessMode::AsNew) {
//          write_stream.open(aFullPath, std::ios::binary | std::ios::trunc);
          write_stream = std::ofstream (aFullPath, std::ios::trunc);
      }
      else {
          write_stream.open(aFullPath, std::ios::binary);
          read_stream.open(aFullPath, std::ios::binary);
      }
      if(!write_stream.is_open()) {
          canOpenFile = false;
      }
      else {
          canOpenFile = true;
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
        if(!canOpenFile) {return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileOpenError);}
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
            size_t index_to_return = free_block_index.front();
            free_block_index.pop();
            return index_to_return;
        }
        else {
            read_stream.seekg(0, std::ios::end);
            return read_stream.tellg() / KBlockSize;
        }
    }

    ArchiveStatus<bool> Archive::add(const std::string &aFilename) {
//      std::ifstream readFile(aFilename, std::ios::binary);
//      if(!readFile) {return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);}
//
//      uint32_t fileName = hashString(aFilename.c_str());
//      bool addingFristBlock = true;
//      size_t byte_left_ToRead = readFile.tellg();
//      size_t next_block_index = getNextFreeBlock();
//      while(byte_left_ToRead > 0) {
//          Block newBlock;
//          size_t readingBytes = byte_left_ToRead;
//          size_t this_block_insert_index = next_block_index;
//          if(byte_left_ToRead > data_size) {
//              next_block_index = getNextFreeBlock();;
//              newBlock.meta.next_block = next_block_index;
//              readingBytes = data_size;
//          }
//          if(addingFristBlock) {
//              newBlock.meta.isFirstBlock == true;
//              addingFristBlock = false;
//          }
//          if(readFile.read(newBlock.data, readingBytes)) {
//              byte_left_ToRead -= readingBytes;
//          }
//          else {
//              return ArchiveStatus<bool>(ArchiveErrors::fileReadError);
//          }
//
//
//          write_stream.write(reinterpret_cast<const char*>(&newBlock), sizeof(newBlock));
//
//
//      }


      return ArchiveStatus(true);
    }

    ArchiveStatus<bool> Archive::extract(const std::string &aFilename, const std::string &aFullPath) {

        return ArchiveStatus(true);
    }

    ArchiveStatus<bool> Archive::remove(const std::string &aFilename) {

        return ArchiveStatus(true);
    }

    ArchiveStatus<size_t> Archive::list(std::ostream &aStream) {


        return ArchiveStatus((size_t) 0);
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
