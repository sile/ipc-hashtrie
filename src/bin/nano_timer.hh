#ifndef IMQUE_NANO_TIMER_HH
#define IMQUE_NANO_TIMER_HH

#include <sys/time.h>

class NanoTimer {
public:
  NanoTimer() {
    gettimeofday(&t, NULL);
  }
    
  long elapsed() const {
    timeval now;
    gettimeofday(&now, NULL);
    return ns(now) - ns(t);
  }

  double elapsed_sec() const {
    return elapsed() / 1000.0 / 1000.0 / 1000.0;
  }
    
private:
  long ns(const timeval& ts) const {
    return static_cast<long>(static_cast<long long>(ts.tv_sec)*1000*1000*1000 + ts.tv_usec*1000);
  }
    
private:
  timeval t;
};

#endif
