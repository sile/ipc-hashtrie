#ifndef __IHT_STRING_HH__
#define __IHT_STRING_HH__

#include <inttypes.h>
#include <string.h>
#include <string>

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

    String(const std::string & str)
      : beg_(str.data()),
        end_(str.data() + str.size())
    {
    }

    String()
      : beg_(NULL),
        end_(NULL)
    {
    }

    operator bool() const { return beg_ < end_; }

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

    static String invalid() { return String(reinterpret_cast<const char*>(1), 
                                            reinterpret_cast<const char*>(0)); }

  private:
    String(const char * beg, const char * end)
      : beg_(beg),
        end_(end)
    {
    }

  private:
    const char * beg_;
    const char * end_;
  };
}

#endif
