//
//  Chunker.hpp
//  Chunker
//
//  Created by rick gessner on 3/6/24.
//

#ifndef Chunker_h
#define Chunker_h


#include "IDataProcessor.hpp"

namespace ECE141 {

    const size_t KBlockSize{1024};



    struct __attribute__ ((__packed__)) BlockHeader {
        signed long long next_block = -1; //this is varaible is first for a reason
        bool occupied = false;
        signed long long previous_block_index = -1;
        size_t byte_stored = 0;
        size_t fileName_size = 0;
        size_t fname_hash;
        bool isCompressed = false;
        IDataProcessor* theProcessor;
    };
    const size_t BlockHeaderSize = sizeof(BlockHeader);

    const size_t data_size = KBlockSize - BlockHeaderSize;

    struct __attribute__ ((__packed__)) Block {
        BlockHeader meta;
        char data[data_size];
    };


  using BlockVisitor = std::function<bool(Block &aBlock, size_t aPos, size_t delta_size)>;
  
  struct Chunker {
    Chunker(std::istream &anInput, size_t anOffset) : input(anInput), offset(anOffset) {
        kChunkSize = data_size - anOffset;
    }
    
    size_t getSize() const {
      input.seekg(0, std::ios::end); //end of the stream
      return input.tellg();
    }

      size_t chunkCount() const { //how many chunks for given stream?
          auto theSize=getSize();
          return (theSize/kChunkSize)+(theSize % kChunkSize ? 1: 0);
      }

      bool each(BlockVisitor aCallback) {
          size_t theLen{getSize()};
          input.seekg(0, std::ios::beg); //point to start of input...

          Block   theBlock;
          size_t  theIndex{0};
          while(theLen) {
              size_t theDelta=std::min(kChunkSize, theLen);
              theLen-=theDelta;
              std::memset(theBlock.data,0,kChunkSize);
              input.read(theBlock.data + offset, theDelta);
              aCallback(theBlock, theIndex++, theDelta);
          }
          return true;
      }

  protected:
    std::istream &input;
    size_t offset;
    size_t kChunkSize;

  };

}

#endif /* Chunker_h */
