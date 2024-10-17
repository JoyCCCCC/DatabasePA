#include <db/IndexPage.hpp>
#include <stdexcept>
#include <iostream>
using namespace db;

IndexPage::IndexPage(Page &page) {
  // TODO pa2: implement
  header = reinterpret_cast<IndexPageHeader*>(page.data());


  keys = reinterpret_cast<int*>(page.data() + sizeof(IndexPageHeader));
  capacity = (DEFAULT_PAGE_SIZE - sizeof(IndexPageHeader)-sizeof(size_t)) /
             (sizeof(int) + sizeof(size_t)); //in fact, I don;t know whether it's right..
  children = reinterpret_cast<size_t*>(keys + capacity);
//  std::cout<<reinterpret_cast<uintptr_t>(children) -
//                   reinterpret_cast<uintptr_t>(keys) <<std::endl;
//

}
findIndexRet IndexPage::findInsertPosition(int key) const{
  size_t left = 0, right = header->size;
  while (left < right) {
    size_t mid = left + (right - left) / 2;
    auto mid_key = keys[mid];
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
bool IndexPage::insert(int key, size_t child) {
  // TODO pa2: implement
  //std::cout<<"Index insert:";
  //size_t pos = std::lower_bound(keys, keys + header->size, key) - keys;
  //std::cout<<pos<<std::endl;
  auto r=findInsertPosition(key);
  size_t pos=r.pos;
  if (r.update==true) {
    children[pos + 1] = child;
    return false;
  }

  std::memmove(keys + pos + 1, keys + pos,
               (header->size - pos) * sizeof(int));
  std::memmove(children + pos + 1, children + pos ,
               (header->size - pos) * sizeof(size_t));
  //std::cout<<"after memmove"<<std::endl;
  keys[pos] = key;
  children[pos] = child;

  header->size+=1;

  if(header->size == capacity){
    return true;
  }
  else{
    return false;
  }
}

int IndexPage::split(IndexPage &new_page) {
  // TODO pa2: implement
  size_t half = header->size / 2;

  std::memcpy(new_page.keys, keys + half + 1,
              (header->size - half - 1) * sizeof(int));
  std::memcpy(new_page.children, children + half + 1,
              (header->size - half) * sizeof(size_t));

  new_page.header->size = header->size - half - 1;
  header->size = half;
  return keys[half];
}
