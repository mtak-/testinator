// Copyright (c) 2015 Tyler Kopf
// This code is distributed under the MIT license. See LICENSE for details.

#include <test.h>
#include <arbitrary.h>
using namespace std;

namespace test_namespace {
  struct test_struct
  {
    test_struct(const std::vector<int>& data2)
      : data(data2)
    {}
    
    bool operator==(const test_struct& rhs) const
    { return !(*this < rhs) && !(rhs < *this); }
    
    bool operator<(const test_struct& rhs) const
    {
      return lexicographical_compare(std::begin(data), std::end(data),
                                     std::begin(rhs.data), std::end(rhs.data));
    }
    
    std::vector<int> data;
  };
}

class nested_test_struct
{
public:
  nested_test_struct(const test_namespace::test_struct& data2)
    : data(data2)
  {}
    
  bool operator==(const nested_test_struct& rhs) const
  { return !(*this < rhs) && !(rhs < *this); }
  
  bool operator<(const nested_test_struct& rhs) const
  { return data < rhs.data; }

  test_namespace::test_struct data;
};

namespace std {
  template<>
  struct hash<nested_test_struct>
  {
    size_t operator()(const nested_test_struct& n) const
    {
      auto result = hash<size_t>{}(n.data.data.size());
      for (auto& elem : n.data.data)
        result += hash<int>{}(elem);
      return result;
    }
  };
}

TESTINATOR_REGISTER_CONSTRUCTOR(test_namespace::test_struct,
                                const std::vector<int>&);

TESTINATOR_REGISTER_CONSTRUCTOR(nested_test_struct,
                                const test_namespace::test_struct&);

//------------------------------------------------------------------------------
DEF_TEST(test_namespace__test_struct, Arbitrary)
{
  testinator::ArbitraryBuilder<test_namespace::test_struct> a;
  auto tg = a.generate(1,0);
  vector<decltype(tg)> vtg = a.shrink(tg);
  
  test_namespace::test_struct& t = tg;
  vector<test_namespace::test_struct> vt = proxy_to_t(vtg);
  
  return vtg.size() == 2
    && vt.size() == 2
    && vt[0].data.size() == t.data.size()/2
    && vt[1].data.size() == t.data.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(nested_test_struct, Arbitrary)
{
  testinator::ArbitraryBuilder<nested_test_struct> a;
  auto ng = a.generate(1,0);
  vector<decltype(ng)> vng = a.shrink(ng);
  
  nested_test_struct& n = ng;
  vector<nested_test_struct> vn = proxy_to_t(vng);
  
  return vng.size() == 2
    && vn.size() == 2
    && vn[0].data.data.size() == n.data.data.size()/2
    && vn[1].data.data.size() == n.data.data.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(pair_constructible, ArbitraryConstructible)
{
  testinator::ArbitraryBuilder<pair<nested_test_struct, test_namespace::test_struct>> a;
  auto pg = a.generate(0,0);
  auto vpg = a.shrink(pg);
  
  pair<nested_test_struct, test_namespace::test_struct> p = proxy_to_t(pg);
  vector<pair<nested_test_struct, test_namespace::test_struct>> vp = proxy_to_t(vpg);
  
  return true;
}

//------------------------------------------------------------------------------
DEF_TEST(tuple_constructible, ArbitraryConstructible)
{
  testinator::ArbitraryBuilder<tuple<nested_test_struct, test_namespace::test_struct>> a;
  auto tg = a.generate(0,0);
  auto vtg = a.shrink(tg);
  
  tuple<nested_test_struct, test_namespace::test_struct> t = proxy_to_t(tg);
  vector<tuple<nested_test_struct, test_namespace::test_struct>> vt = proxy_to_t(vtg);
  
  return true;
}

//------------------------------------------------------------------------------
DEF_TEST(deque_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<deque<nested_test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  deque<nested_test_struct> d = proxy_to_t(dg);
  vector<deque<nested_test_struct>> vd = proxy_to_t(vdg);
  
  return vdg.size() == 2
    && vd.size() == 2
    && vd[0].size() == d.size()/2
    && vd[1].size() == d.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(list_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<list<nested_test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  list<nested_test_struct> d = proxy_to_t(dg);
  vector<list<nested_test_struct>> vd = proxy_to_t(vdg);
  
  return vdg.size() == 2
    && vd.size() == 2
    && vd[0].size() == d.size()/2
    && vd[1].size() == d.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(forward_list_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<list<nested_test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  list<nested_test_struct> v = proxy_to_t(dg);
  vector<list<nested_test_struct>> vv = proxy_to_t(vdg);

  std::size_t vlen = 0;
  for (auto it = v.begin(); it != v.end(); ++it, ++vlen);

  std::size_t vv0len = 0;
  std::size_t vv1len = 0;
  if (vv.size() > 0)
    for (auto it = vv[0].begin(); it != vv[0].end(); ++it, ++vv0len);
  if (vv.size() > 1)
    for (auto it = vv[1].begin(); it != vv[1].end(); ++it, ++vv1len);

  return vv.size() == 2
    && vv0len == vlen/2
    && vv1len == vlen/2;
}

//------------------------------------------------------------------------------
DEF_TEST(set_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<set<nested_test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  set<nested_test_struct> v = proxy_to_t(dg);
  vector<set<nested_test_struct>> vv = proxy_to_t(vdg);
  
  return vv.size() == 2
    && vdg.size() == 2
    && vv[0].size() == v.size()/2
    && vv[1].size() == v.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(multiset_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<multiset<nested_test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  multiset<nested_test_struct> v = proxy_to_t(dg);
  vector<multiset<nested_test_struct>> vv = proxy_to_t(vdg);
  
  return vv.size() == 2
    && vdg.size() == 2
    && vv[0].size() == v.size()/2
    && vv[1].size() == v.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(unordered_set_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<unordered_set<nested_test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  unordered_set<nested_test_struct> v = proxy_to_t(dg);
  vector<unordered_set<nested_test_struct>> vv = proxy_to_t(vdg);
  
  return vv.size() == 2
    && vdg.size() == 2
    && vv[0].size() == v.size()/2
    && vv[1].size() == v.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(map_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<map<nested_test_struct, test_namespace::test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  map<nested_test_struct, test_namespace::test_struct> v = proxy_to_t(dg);
  vector<map<nested_test_struct, test_namespace::test_struct>> vv = proxy_to_t(vdg);
  
  return vv.size() == 2
    && vdg.size() == 2
    && vv[0].size() == v.size()/2
    && vv[1].size() == v.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(multimap_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<multimap<nested_test_struct, test_namespace::test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  multimap<nested_test_struct, test_namespace::test_struct> v = proxy_to_t(dg);
  vector<multimap<nested_test_struct, test_namespace::test_struct>> vv = proxy_to_t(vdg);
  
  return vv.size() == 2
    && vdg.size() == 2
    && vv[0].size() == v.size()/2
    && vv[1].size() == v.size()/2;
}

//------------------------------------------------------------------------------
DEF_TEST(unordered_map_constructible, Arbitrary)
{
  testinator::ArbitraryBuilder<unordered_map<nested_test_struct, test_namespace::test_struct>> a;
  auto dg = a.generate(1,0);
  auto vdg = a.shrink(dg);
  
  unordered_map<nested_test_struct, test_namespace::test_struct> v = proxy_to_t(dg);
  vector<unordered_map<nested_test_struct, test_namespace::test_struct>> vv = proxy_to_t(vdg);
  
  return vv.size() == 2
    && vdg.size() == 2
    && vv[0].size() == v.size()/2
    && vv[1].size() == v.size()/2;
}
