#include <db/BTreeFile.hpp>
#include <db/Database.hpp>
#include <gtest/gtest.h>

TEST(BTreeTest, Empty) {
  const char *name = "test.db";
  std::remove(name);
  db::TupleDesc td({db::type_t::INT, db::type_t::CHAR, db::type_t::DOUBLE}, {"id", "name", "price"});
  db::getDatabase().add(std::make_unique<db::BTreeFile>(name, td, 0));
  auto &file = db::getDatabase().get(name);
  EXPECT_EQ(file.begin(), file.end());
}

TEST(BTreeTest, Sorted) {
  const char *name = "test.db";
  std::remove(name);
  db::TupleDesc td({db::type_t::INT, db::type_t::CHAR, db::type_t::DOUBLE}, {"id", "name", "price"});
  db::getDatabase().add(std::make_unique<db::BTreeFile>(name, td, 0));
  auto &file = db::getDatabase().get(name);
  for (int i = 0; i < 54; i++) {
    db::Tuple t{{i, "apple", 1.0}};
    file.insertTuple(t);
  }
  int i = 0;
  for (const auto &t : file) {
    //EXPECT_EQ(std::get<int>(t.get_field(0)), i);
    std::cout<<std::get<int>(t.get_field(0))<<" "<<std::get<std::string>(t.get_field(1))<<" "<<std::get<double>(t.get_field(2))<<std::endl;
    //EXPECT_EQ(std::get<std::string>(t.get_field(1)), "apple");
    //EXPECT_EQ(std::get<double>(t.get_field(2)), 1.0);
    i++;
    if(i==54)
      break;
  }
  EXPECT_EQ(i, 1000000);
}

TEST(BTreeTest, Random) {
  const char *name = "test.db";
  std::remove(name);
  db::TupleDesc td({db::type_t::INT, db::type_t::CHAR, db::type_t::DOUBLE}, {"id", "name", "price"});
  db::getDatabase().add(std::make_unique<db::BTreeFile>(name, td, 0));
  auto &file = db::getDatabase().get(name);
  for (int i = 0; i < 54; i++) {
    int k = i % 2 ? 1000000 - i : i;
    db::Tuple t{{k, "apple", 1.0}};
    file.insertTuple(t);
    break;
  }
  int i = 0;
  for (const auto &t : file) {
    EXPECT_EQ(std::get<int>(t.get_field(0)), i);
    EXPECT_EQ(std::get<std::string>(t.get_field(1)), "apple");
    EXPECT_EQ(std::get<double>(t.get_field(2)), 1.0);
    i++;
    break;
  }
  EXPECT_EQ(i, 1000000);
}
