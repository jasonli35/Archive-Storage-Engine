//
//  Archive.cpp
//
//
//
//

#include "Archive.hpp"

namespace ECE141 {

  //STUDENT put archive class code here...
  ArchiveStatus<std::shared_ptr<Archive>> Archive::createArchive(const std::string &anArchiveName) {
      std::filesystem::path pathObj(anArchiveName);
      std::filesystem::path dirPath = pathObj.parent_path();
      if (!std::filesystem::exists(dirPath)) {
          std::filesystem::create_directories(dirPath);
      }

      std::string file_path = anArchiveName + ".arc";
      std::ofstream fStream(file_path);
      if(!fStream.is_open()) {
          return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileOpenError);
      }

      fStream.close();
      if(fStream.is_open()) {return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileCloseError);}
      std::shared_ptr<Archive> theArchivePtr =  std::make_shared<Archive>(Archive(file_path, AccessMode::AsNew));
//      Archive::existing_archive[file_path] = theArchivePtr;

      return ArchiveStatus(theArchivePtr);
  }

    Archive::Archive(const std::string &aFullPath, AccessMode aMode): fullPath(aFullPath), theMode(aMode) {}

    Archive::~Archive() {}

    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName) {
        std::fstream file(anArchiveName);
        if (!file) {
            return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileNotFound);
        }
//        std::shared_ptr<Archive> theArchivePtr = Archive::existing_archive.at(anArchiveName);
//        theArchivePtr->theMode = AccessMode::AsExisting;
//        return ArchiveStatus(theArchivePtr);
        return ArchiveStatus(std::make_shared<Archive>(Archive("", AccessMode::AsNew)));
    }


    uint32_t Archive::hashString(const char *str) {
      const int gMultiplier = 37;
      uint32_t h{0};
      unsigned char *p;
      for(p = (unsigned char*) str; *p != '\0'; p++)
          h = gMultiplier * h + *p;
      return h;
  }

    ArchiveStatus<bool> Archive::add(const std::string &aFilename) {
//      std::ifstream readFile(aFilename);
//      if(!readFile) {return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);}
//
//      uint32_t fileName = hashString(aFilename.c_str());
//      bool addingFristBlock = true;
//      size_t byteToRead = readFile.tellg();
//      while(byteToRead > 0) {
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
