#ifdef __APPLE__

#include <pthread.h>

template <class T, class P = std::pair<T*, void(*)(T*)>>
void* start_routine(void* ptr)
{
   P* p = reinterpret_cast<P*>(ptr);
   p->second(p->first); // Call member function pointer
   delete p;
   return NULL;
}

class NativeThread {

   pthread_t thread;

public:
  template<class T, class P = std::pair<T*, void(*)(T*)>>
  explicit NativeThread(void (*fun)(T*), T* obj) {
    pthread_attr_t attr_storage, *attr = &attr_storage;
    pthread_attr_init(attr);
    pthread_attr_setstacksize(attr, 4194304);
    pthread_create(&thread, attr, start_routine<T>, new P(obj, fun));
  }
  void join() { pthread_join(thread, NULL); }
};

#else

#include <thread>
typedef std::thread NativeThread;

#endif
