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



    Archive::Archive(const std::string &aFullPath, AccessMode aMode): archiveFullPath(aFullPath) {
      if(aMode == AccessMode::AsNew) {
          archiveStream.open(aFullPath, std::ios::out|std::ios::in|std::ios::binary|std::ios::trunc);
          if(!archiveStream.is_open()) {
              std::cerr << "Archive file cannot be created" << std::endl;
          }

      }
      else {
          openSteams(aFullPath);
      }
   }



    Archive::~Archive() {

        if(archiveStream.is_open()) {
            archiveStream.close();
        }

    }



    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName) {
        Archive anArchive(anArchiveName, AccessMode::AsExisting);
        if(!anArchive.archiveStream.is_open()) {return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::badPath);}
        return ArchiveStatus(std::make_shared<Archive>(anArchive));
    }





    size_t Archive::getFileSizeInByte(std::fstream& readFileStream) {

        readFileStream.seekg (0, std::ios::end);
        size_t sizeOfFile = readFileStream.tellg();
        readFileStream.seekg(0, std::ios::beg);
        readFileStream.clear();
       return sizeOfFile;
  }

  std::streampos block_index_to_address(signed long long aPos) {
      return (std::streampos )aPos * KBlockSize;
  }

    std::string Archive::getFileName(const std::string& filePath) {

        size_t pos = filePath.find_last_of("/");

        if (pos == std::string::npos) return filePath;

        return filePath.substr(pos + 1);
    }

    ArchiveStatus<bool> Archive::writeBlockToFile(Block& theBlock, size_t index) {
        archiveStream.clear();
        archiveStream.seekp(block_index_to_address(index));

        if(!check_stream_status(archiveStream)) {
            return ArchiveStatus<bool>(ArchiveErrors::fileSeekError);
        }


        archiveStream.write(reinterpret_cast<char*>(&theBlock), theBlock.meta.byte_stored + BlockHeaderSize + theBlock.meta.fileName_size);

        archiveStream.flush();
        if(!check_stream_status(archiveStream)) {
            return ArchiveStatus<bool>(ArchiveErrors::fileWriteError);
        }
        return ArchiveStatus<bool>(true);
  }

//    std::vector<uint8_t>  fStreamToVectorAdpoter(std::fstream aFstream) {
//      return std::vector<uint8_t>(std::istreambuf_iterator<char>(aFstream),  std::istreambuf_iterator<char>());
//  }
//
//    std::fstream vectorToFstreamAdpoter(std::vector<uint8_t> data) {
//      std::fstream file(aFileStream, std::ios::out | std::ios::binary);
//      return
//  }

    size_t Archive::countBlocks() {
        archiveStream.clear(); //just in case
        archiveStream.seekg(0, std::ios::end);
        std::streampos thePos = archiveStream.tellg();
        return static_cast<size_t>(thePos / KBlockSize + (thePos % KBlockSize ? 1: 0));
    }

    //get N free blocks (may be 'unoccupied' or appended block #'s)
    ArchiveStatus<bool> Archive::getFreeBlocks(size_t aCount, IntVector &aList) {
        if(aCount < 1) {return ArchiveStatus<bool>(ArchiveErrors::invalidArgument);}
        aList.clear(); //erase previous contents...
        aList.push_back(-1);
        size_t end_index = aCount + 1;
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            if(!aBlock.meta.occupied) {
                aList.push_back(aPos);
            }
            return (aList.size()==end_index); //quit once we have aCount items...
        });
        size_t theBCount=countBlocks();
        while(aList.size()<end_index) {
            aList.push_back(theBCount++);
        }
        aList.push_back(-1); //makes linking block logic easier...
        return ArchiveStatus<bool>(true);  //lacks error handling
    }



    ArchiveStatus<bool> Archive::add(const std::string &aFilename, IDataProcessor* aProcessor) {
        std::string short_file_name = getFileName(aFilename);
        size_t file_name_size = short_file_name.size() + 1;
        //std::ios::out|std::ios::in|std::ios::binary|std::ios::trunc
      std::fstream readFile(aFilename, std::ios::in|std::ios::binary);
      if(!readFile.good()) {
          notify_all_observers(ActionType::added, aFilename, false);
          return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);
      }
      ECE141::Chunker theChunker(readFile, file_name_size);
      IntVector thePlaces;
      size_t payload_size = data_size - file_name_size;
      getFreeBlocks(theChunker.chunkCount(), thePlaces);
      theChunker.each([&](ECE141::Block& aBlock, size_t aPartNum, size_t writen_size) {
            BlockHeader& currentHeader = aBlock.meta;
            currentHeader.occupied=true;
            currentHeader.previous_block_index = thePlaces[aPartNum];
            currentHeader.next_block = thePlaces[aPartNum + 2];
            currentHeader.byte_stored = writen_size;
            currentHeader.fileName_size = file_name_size;
            std::strcpy(aBlock.data, short_file_name.c_str());
            ArchiveStatus<bool> status = writeBlockToFile(aBlock, thePlaces[aPartNum + 1]);
            return status.getValue();
      });



//      size_t buffer_size = data_size - file_name_size;


//      bool addingFristBlock = true;
//      size_t byte_left_ToRead = Archive::getFileSizeInByte(readFile);

//
//      size_t current_block_index;
//      int next_block_index = -1;


//      while(byte_left_ToRead > 0 || (addingFristBlock && byte_left_ToRead == 0)) {
//
//          if(next_block_index == -1) {
//              current_block_index = getNextFreeBlock();
//          }
//          else {
//              current_block_index = next_block_index;
//          }
//          Block newBlock;
//
//          std::strcpy(newBlock.data, short_file_name.c_str());
//
//          size_t readingBytes = byte_left_ToRead;
//
//          BlockHeader &newBlockHeader = newBlock.meta;
//          newBlockHeader.fileName_size = file_name_size;
//          newBlockHeader.occupied = true;
//          newBlockHeader.previous_block_index = current_block_index;
//          if(addingFristBlock) {
//              newBlockHeader.previous_block_index = -1; //indicate first block of the file that copy from
//              addingFristBlock = false;
//          }
//           // set it to the current inserting block index
//          if(byte_left_ToRead > buffer_size) {
//              next_block_index = getNextFreeBlock();
//              newBlockHeader.next_block = next_block_index;
//              readingBytes = buffer_size;
//          }
//          newBlockHeader.byte_stored = readingBytes + file_name_size;
//          readFile.read(newBlock.data + file_name_size, readingBytes);
//          if(readFile) {
//              byte_left_ToRead -= readingBytes;
//
//          }
//          else {
//              notify_all_observers(ActionType::added, aFilename, false);
//              return ArchiveStatus<bool>(ArchiveErrors::fileReadError);
//          }
//
//          ArchiveStatus<bool> status = writeBlockToFile(newBlock, current_block_index);
//          if(!status.isOK()) {return status;}
//
//      }

      readFile.close();
      notify_all_observers(ActionType::added, aFilename, true);
      return ArchiveStatus(true);
    }



    ArchiveStatus<bool> Archive::extract(const std::string &aFilename, const std::string &aFullPath) {
      int current_index = -1;
      Block first_block;

        std::fstream write_file(aFullPath, std::ios::out|std::ios::in|std::ios::binary|std::ios::trunc);

        if(!check_stream_status(write_file)) {
            return ArchiveStatus<bool>(ArchiveErrors::fileCreateError);
        }
        each([&] (Block &aBlock, size_t aPos) {
            std::string data(aBlock.data);
            const std::string block_file_name = data.substr(0, aBlock.meta.fileName_size - 1);
            if(aBlock.meta.occupied && block_file_name.compare(aFilename) == 0 && aBlock.meta.previous_block_index == -1) {
                current_index = aPos;
                first_block = aBlock;

                return false;
            }
            else {
                return true;
            }
        });

        if(current_index == - 1) {
            return ArchiveStatus<bool>(false);
        }

        size_t file_name_size = first_block.meta.fileName_size;

        while(current_index != -1) {
            Block current_block;
            getBlock(current_block, current_index);

            write_file.write(current_block.data + file_name_size, current_block.meta.byte_stored);
            if(!write_file.good()) {
                return ArchiveStatus<bool>(ArchiveErrors::fileWriteError);
            }
            current_index = current_block.meta.next_block;
        }

        return ArchiveStatus<bool>(true);


    }

    bool Archive::update_disk_header(BlockHeader& aHeader, size_t index) {
      archiveStream.clear();
      archiveStream.seekp(block_index_to_address(index));
      if(!check_stream_status(archiveStream)) {return false;}
      archiveStream.write(reinterpret_cast<char*>(&aHeader), BlockHeaderSize);
      archiveStream.flush();
      if(!check_stream_status(archiveStream)) {return false;}
      return true;

  }

  std::string Archive::getFileName(const Block& aBlock) {
     std::string current_fName (aBlock.data, aBlock.meta.fileName_size - 1);
      return current_fName;
  }

    ArchiveStatus<bool> Archive::remove(const std::string &aFilename) {
      bool isFileExist = false;
      each([&] (Block& theBlock, size_t aPos) {

          if(getFileName(theBlock) == aFilename) {
              BlockHeader modified_header = theBlock.meta;
              modified_header.occupied = false;
              update_disk_header(modified_header, aPos);
              free_block_index.push(aPos);
              isFileExist = true;
          }
          return true;
      });
      return ArchiveStatus<bool>(isFileExist);

    }

    bool Archive::check_stream_status (const std::fstream& file_stream_to_check) {
      if(!file_stream_to_check.good()) {
          std::cerr << "Error flags: ";
          if (file_stream_to_check.rdstate() & std::ios::eofbit)
              std::cerr << "eof ";
          if (file_stream_to_check.rdstate() & std::ios::failbit)
              std::cerr << "fail ";

          if (file_stream_to_check.rdstate() & std::ios::badbit)
              std::cerr << "bad ";
          std::cerr << std::endl;
          return false;
      }
      return true;
  }

    bool Archive::getBlock(Block &aBlock, signed long long aPos) {
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
                std::string current_fname = getFileName(aBlock);
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

      size_t i = 0;

      each([&](Block& theBlock, size_t thePos) {
         aStream << ++i << ".   ";
         if(theBlock.meta.occupied) {
             aStream << "used     " << getFileName(theBlock) << std::endl;
         }
         else {
             aStream << "empty" << std::endl;
         }
         return true;
      });

        return ArchiveStatus(i);

    }
    ArchiveStatus<size_t> Archive::update_parent_index(signed long long parent_block_index, signed long long children_index) {
      archiveStream.clear();
      archiveStream.seekp(block_index_to_address(parent_block_index));
      if(!check_stream_status(archiveStream)) {return ArchiveStatus<size_t>(ArchiveErrors::fileSeekError);}
        archiveStream.write(reinterpret_cast<char*>(&children_index), sizeof (children_index));
      if(!check_stream_status(archiveStream)) {return ArchiveStatus<size_t>(ArchiveErrors::fileWriteError);}
      return ArchiveStatus<size_t>(1);
   }

    ArchiveStatus<size_t> Archive::compact() {
      size_t lastChunkIndex = countBlocks();
        ArchiveStatus<size_t> numBlockRemoved(0) ;
        each([&] (Block& theBlock, size_t thePos) {

            if(!theBlock.meta.occupied) {
                Block last_Block;
                do {
                    if(--lastChunkIndex <= thePos) {
                        return false;
                    }
                    getBlock(last_Block, lastChunkIndex);
                } while(!last_Block.meta.occupied);

                writeBlockToFile(last_Block, thePos);
                signed long long int parent_block_index = last_Block.meta.previous_block_index;
                if(parent_block_index != -1) {
                    numBlockRemoved = update_parent_index(parent_block_index, thePos);
                    if(!numBlockRemoved.isOK()) {
                        return false;
                    }
                    else {
                        numBlockRemoved = ArchiveStatus<size_t> (numBlockRemoved.getValue() + 1);
                    }
                }
            }
            return true;
        });

        return numBlockRemoved;
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

    std::vector<uint8_t> Compression::process(const std::vector<uint8_t>& input) {
      orignal_size += input.size();
      uLongf compressedSize = compressBound(orignal_size);
      std::vector<uint8_t> output(compressedSize);
      if (compress(output.data(), &compressedSize, input.data(), orignal_size) != Z_OK) {
            std::cerr << "Compression failed!" << std::endl;
      }

      return output;
    }

    std::vector<uint8_t> Compression::reverseProcess(const std::vector<uint8_t>& input) {
      std::vector<uint8_t> original(orignal_size);
      if(uncompress(original.data(), &orignal_size, input.data(), input.size()) != Z_OK) {
          std::cerr << "reverse compression failed!" << std::endl;
      }
      return input;
  }






}
