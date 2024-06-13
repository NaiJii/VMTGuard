#include "includes.hpp"

namespace SVMT
{
  bool Setup();

  // bindings to potentially game allocator
  void *Malloc(size_t size);
  void Free(void *ptr);

  struct Info
  {
    uintptr_t start = 0;
    uintptr_t end = 0;
    uintptr_t original = 0;
    bool isProtected = false;
    bool redirect = false;
    DWORD oldProtect = 0;
    ULONGLONG lastProtectedTime = 0;

    void Protect();
    void Unprotect(bool force = false);
  };

  struct VMT
  {
    VMT() = default;
    
    bool Setup(void* _object, size_t vtableOffset = 0x0);
    void Apply();
    void Restore();

    uintptr_t GetOriginal(size_t index) const
    {
      if ( !original || index >= size )
        return {};

      return original[index];
    }

    template <typename T>
    T GetOriginal(size_t index) const
    {
      if ( !original || index >= size )
        return {};

      return reinterpret_cast<T>(original[index]);
    }

    uintptr_t GetReplacement(size_t index) const
    {
      if ( !replacement || index >= size )
        return {};

      return replacement[index];
    }

    template <typename T>
    T GetReplacement(size_t index) const 
    {
      if ( !replacement || index >= size )
        return {};

      return reinterpret_cast<T>(replacement[index]);
    }

    template <typename T>
    uintptr_t Replace(size_t index, T hook)
    {
      if ( !replacement || index >= size )
        return {};

      replacement[index] = uintptr_t(hook);
      return original[index];
    }

    uintptr_t **object = nullptr;
    uintptr_t *original = nullptr;
    uintptr_t *replacement = nullptr;
    size_t size = 0;
    size_t offset = 0x0;
  };

  LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exceptionInfo);

  inline PVOID vehHandle;
  inline std::vector<Info> infos;
}