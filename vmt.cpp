#include "vmt.hpp"

bool SVMT::Setup()
{
  vehHandle = AddVectoredExceptionHandler(1, &SVMT::VectoredHandler);
  if ( !vehHandle )
    return false;

  std::cout << "VEH Handler: " << vehHandle << "\n";

  return true;
}

void *SVMT::Malloc(size_t size)
{
  return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  //return malloc(size);
}

void SVMT::Free(void *ptr)
{
  VirtualFree(ptr, 0, MEM_RELEASE);
  // free(ptr);
}

bool SVMT::VMT::Setup(void *_object, size_t vtableOffset)
{
  if ( !_object )
    return false;

  offset = vtableOffset;
  object = reinterpret_cast<uintptr_t **>(_object);
  original = *reinterpret_cast<uintptr_t **>(reinterpret_cast<uintptr_t>(object) + offset);
  size = 0;

  if ( !original )
    return false;

  // better way to get size is to parse sections and check for IMAGE_SCN_MEM_EXECUTE
  while ( true )
  {
    MEMORY_BASIC_INFORMATION mbi{};
    if ( !VirtualQuery(reinterpret_cast<void *>(original[size]), &mbi, sizeof(MEMORY_BASIC_INFORMATION)) )
      break;

    if ( mbi.Protect != PAGE_EXECUTE_READ )
      break;

    size++;
  }

  if ( size == 0 )
    return false;

  replacement = reinterpret_cast<uintptr_t *>(Malloc(size * sizeof(uintptr_t)));
  if ( !replacement )
    return false;

  memcpy(replacement, original, size * sizeof(uintptr_t));

  infos.emplace_back(reinterpret_cast<uintptr_t>(replacement), reinterpret_cast<uintptr_t>(replacement + size), reinterpret_cast<uintptr_t>(original), false, false, 0, 0);

  return true;
}

void SVMT::VMT::Apply()
{
  if ( object && replacement )
  {
    *(uintptr_t **)(uintptr_t(object) + offset) = replacement;
    std::cout << "Original functions : " << std::hex << uintptr_t(original) << " -> " << original[0] << " " << original[1] << "\n";
    std::cout << "Replacement functions : " << std::hex << uintptr_t(replacement) << " -> " << replacement[0] << " " << replacement[1] << "\n";
  }
}

void SVMT::VMT::Restore()
{
  if ( object && original )
    *(uintptr_t **)(uintptr_t(object) + offset) = original;
}

LONG SVMT::VectoredHandler(PEXCEPTION_POINTERS exceptionInfo)
{
  PCONTEXT ctx = exceptionInfo->ContextRecord;
  PEXCEPTION_RECORD record = exceptionInfo->ExceptionRecord;
  if ( record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION )
    return EXCEPTION_CONTINUE_SEARCH;

  uintptr_t accessAddress = record->ExceptionInformation[1];

  LOG("Exception on {:#x} trying to access {:#x}", ctx->Rip, accessAddress);

  for ( auto& info : infos )
  {
    const auto inRange = [&info](uintptr_t x) { return x >= info.start && x < info.end; };
    if ( !inRange(accessAddress) )
      continue;

    uintptr_t offset = accessAddress - info.start;

    LOG("Found protected address: {:#x} (index : {})", accessAddress, offset / sizeof(uintptr_t));

    if ( info.redirect )
    {
      uintptr_t redirectAddress = info.original + offset;

      LOG("Redirecting to: {:#x}[{}] -> {:#x}", redirectAddress, offset / sizeof(uintptr_t), reinterpret_cast<uintptr_t*>(info.original)[offset / sizeof(uintptr_t)]);
      // set where the exception should have occured (not necessary)
      record->ExceptionInformation[1] = redirectAddress;
      // set data
      if ( inRange(ctx->Rcx) )
        ctx->Rcx = (ctx->Rcx - info.start) + info.original;
      if ( inRange(ctx->Rdx) )
        ctx->Rdx = (ctx->Rdx - info.start) + info.original;
      if ( inRange(ctx->R8) )
        ctx->R8 = (ctx->R8 - info.start) + info.original;
      if ( inRange(ctx->R9) )
        ctx->R9 = (ctx->R9 - info.start) + info.original;
    }
    else
    {
      info.Unprotect();
    }

    std::cout << "\n";

    return EXCEPTION_CONTINUE_EXECUTION;
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

void SVMT::Info::Protect()
{
  ULONGLONG now = GetTickCount64();
  if ( isProtected )
    return;

  LOG("Protecting {:#x} ({})", start, (end - start) / sizeof(uintptr_t));

  DWORD oldProtection;
  if ( !oldProtect )
    VirtualProtect(reinterpret_cast<uintptr_t*>(start), end - start, PAGE_NOACCESS, &oldProtect);
  else
    VirtualProtect(reinterpret_cast<uintptr_t*>(start), end - start, PAGE_NOACCESS, &oldProtection);

  lastProtectedTime = now;
  isProtected = true;
}

void SVMT::Info::Unprotect(bool force)
{
  DWORD oldProtection;
  if ( force || (isProtected && oldProtect) )
  {
    VirtualProtect(reinterpret_cast<uintptr_t*>(start), end - start, oldProtect, &oldProtection);
    LOG("Unprotecting {:#x} ({})", uintptr_t(start), (end - start) / sizeof(uintptr_t));
  }
  isProtected = false;
}
