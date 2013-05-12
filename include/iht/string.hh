#ifndef __IHT_STRING_HH__
#define __IHT_STRING_HH__

#include <inttypes.h>
#include <string.h>

namespace iht {
  const uint32_t GOLDEN_RATIO_PRIME=0x9e370001;

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
    
    uint32_t hash() const {
      uint32_t h = GOLDEN_RATIO_PRIME;
      for(const char* c=beg_; c != end_; c++)
        h = (h*33) + *c;
      return h;
    }

    bool operator==(const String & s) const {
      if(size() != s.size()) {
        return false;
      }
      return strncmp(data(), s.data(), size()) == 0;
    }

  private:
    const char * beg_;
    const char * end_;
  };
}

#endif
