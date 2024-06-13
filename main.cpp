#include "includes.hpp"
#include "vmt.hpp"

struct MyClass
{
  virtual void MyFunction1() { }
  virtual void MyFunction2() { }
};

static void Hook1() { }
static void Hook2() { }

MyClass a;
SVMT::VMT vmt;

int main()
{
  std::atexit([]()
    {
      std::cin.get();
    });

  if ( !SVMT::Setup() )
  {
    std::cout << "Failed to setup veh\n";
    return 1;
  }

  if ( !vmt.Setup(&a) )
  {
    std::cout << "Failed to setup vmt\n";
    return 1;
  }

  vmt.Replace(0, Hook1);
  vmt.Replace(1, Hook2);
  vmt.Apply();

  std::cout << "\n";

  uintptr_t *table = *reinterpret_cast<uintptr_t **>(&a);

  SVMT::infos[0].redirect = false;
  SVMT::infos[0].Protect();

  uintptr_t f = table[1];

  LOG("Original functions : {:#x}", f);

  std::cout << "\n";

  SVMT::infos[0].redirect = true;
  SVMT::infos[0].Protect();

  f = table[1];
  LOG("Replacement functions : {:#x}", f);

  return 0;
}
