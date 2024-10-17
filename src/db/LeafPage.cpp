#include <db/LeafPage.hpp>
#include <stdexcept>
#include <iostream>
using namespace db;

LeafPage::LeafPage(Page &page, const TupleDesc &td, size_t key_index) : td(td), key_index(key_index) {
  // TODO pa2: implement
  capacity =  (DEFAULT_PAGE_SIZE - sizeof(LeafPageHeader)) / td.length();
  header = reinterpret_cast<db::LeafPageHeader*>(page.data());
  header->size=0;
  //size of header:8
  data = page.data() + DEFAULT_PAGE_SIZE - td.length() * capacity;
  std::cout<<"constructed:"<<capacity<<std::endl;
}

findRet LeafPage::findInsertPosition(const db::Tuple& t) const {
  auto key = t.get_field(key_index);

  size_t left = 0, right = header->size;
  while (left < right) {
    size_t mid = left + (right - left) / 2;
    db::Tuple mid_tuple = td.deserialize(data + mid * td.length());
    auto mid_key = mid_tuple.get_field(key_index);
    if(mid_key==key){
      return {true,mid};
    }
    if (mid_key < key) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return {false,left};
}

bool LeafPage::insertTuple(const Tuple &t) {
  // TODO pa2: implement

  //uto id=t.get_field(0);
  findRet r=findInsertPosition(t);
  size_t insert_pos = r.pos;
  std::cout<<"insert_pos "<<insert_pos<<std::endl;
  if(r.update){
    td.serialize(data + insert_pos * td.length(), t);
  }
  else{
    std::memmove(data + (insert_pos + 1) * td.length(),
               data + insert_pos * td.length(),
               (header->size - insert_pos) * td.length());
    td.serialize(data + insert_pos * td.length(), t);
    header->size+=1;
  }
  std::cout<<"function insert end"<<std::endl;
  if(header->size<capacity)
    return false;
  else
    return true;

}

int LeafPage::split(LeafPage &new_page) {
  // TODO pa2: implement
  size_t half = header->size / 2;
  std::memcpy(new_page.data, data + half * td.length(),
              (header->size - half) * td.length());

  // 更新新页面的元组数量
  new_page.header->size = header->size - half;

  // 更新当前页面的元组数量
  header->size = half;

  // 设置新页面的 next_leaf 指针
  new_page.header->next_leaf = header->next_leaf;
  header->next_leaf = reinterpret_cast<size_t>(&new_page); // 设置新的叶子指针

  // 返回拆分后的第一个键（新页面的第一个元组的键）
  db::Tuple first_tuple = new_page.td.deserialize(new_page.data);
  int ret=std::get<int>(first_tuple.get_field(key_index));
  return ret;
}

Tuple LeafPage::getTuple(size_t slot) const {
  // TODO pa2: implement
  return td.deserialize(data + slot * td.length());
}
