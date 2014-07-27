#include <property.h>

using namespace std;

//------------------------------------------------------------------------------
// A test of the machinery

struct TestFunctor
{
  bool operator()(int) const { return true; }
};

DECLARE_TEST(Functor, Property)
{
  TestFunctor f;
  testpp::Property p(f);
  return p.check();
}

//------------------------------------------------------------------------------
// Another machinery test

struct FuncTraitsStruct
{
  void operator()(int);
};

DECLARE_TEST(FunctorTraits, Property)
{
  typedef testpp::function_traits<FuncTraitsStruct> traits;

  return std::is_same<int, traits::argType>::value;
}

//------------------------------------------------------------------------------
// A simple test of string reversal invariant

DECLARE_PROPERTY(StringReverse, Property, const string& s)
{
  string r(s);
  reverse(r.begin(), r.end());
  reverse(r.begin(), r.end());
  return s == r;
}

//------------------------------------------------------------------------------
// This is supposed to fail to demonstrate how shrinking finds the minimal
// failure

DECLARE_PROPERTY(FailTriggersShrink, Property, const string& s)
{
  return s.find('A') == s.npos;
}

//------------------------------------------------------------------------------
// A user-defined type test

struct MyType
{
  MyType() : m_val(1337) {}
  int m_val;
};

ostream& operator<<(ostream& s, const MyType& m)
{
  return s << m.m_val;
}

DECLARE_PROPERTY(MyType, Property, const MyType& m)
{
  return m.m_val == 1337;
}

//------------------------------------------------------------------------------
DECLARE_PROPERTY(ConstInt, Property, const int)
{
  return true;
}

//------------------------------------------------------------------------------
DECLARE_PROPERTY(ConstChar, Property, const char)
{
  return true;
}
