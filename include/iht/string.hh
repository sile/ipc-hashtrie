#ifndef __IHT_STRING_HH__
#define __IHT_STRING_HH__

#include <inttypes.h>
#include <string.h>

namespace iht {
  class String {
  public:
    String(const char * data, uint32_t size)
      : beg_(data),
        end_(data + size)
    {
    }
    
    String(const char * str)
      : beg_(str),
        end_(str + strlen(str))
    {
    }

    String()
      : beg_(NULL),
        end_(NULL)
    {
    }
    
    operator bool() const { return beg_ != NULL && beg_ < end_; }
    
    uint32_t size() const { return end_ - beg_; }
    
    const char * data() const { return beg_; }
    
  private:
    const char * beg_;
    const char * end_;
  };
}

#endif
