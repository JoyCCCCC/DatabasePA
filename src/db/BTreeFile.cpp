#include <cstring>
#include <db/BTreeFile.hpp>
#include <db/Database.hpp>
#include <db/IndexPage.hpp>
#include <db/LeafPage.hpp>
#include <iostream>
#include <stdexcept>

using namespace db;

BTreeFile::BTreeFile(const std::string &name, const TupleDesc &td, size_t key_index)
    : DbFile(name, td), key_index(key_index) {}

void BTreeFile::insertTuple(const Tuple &t) {
  std::cout << "Inserting tuple: " << std::get<int>(t.get_field(0)) << std::endl;
  // Initialize the root page number (always 0) and fetch the root page
  size_t currentPageNo = 0;
  Page &root = getDatabase().getBufferPool().getPage({name, currentPageNo});

  // If the root node is empty, it means this is the first time the tuple is inserted.
  IndexPage indexPage(root);
  if (indexPage.header->size == 0) {
    std::cout << "Root is empty, initializing root with first leaf" << std::endl;

    // create a first leaf node
    size_t newLeafPageNo = 1;  // Assign a new leaf page number (assuming it starts at 1)
    Page &leafPage = getDatabase().getBufferPool().getPage({name, newLeafPageNo});
    LeafPage leaf(leafPage, td, key_index);

    // Insert the tuple into the newly created leaf node
    leaf.insertTuple(t);

    // Update the root node information to point to the leaf node
    indexPage.header->index_children = false;  // The root node directly points to the leaf node
    indexPage.children[0] = newLeafPageNo;     // The first child node is a leaf node
    indexPage.header->size = 1;  // The root node now has one child node

    // Mark root and leaf pages as dirty
    getDatabase().getBufferPool().markDirty({name, currentPageNo});
    getDatabase().getBufferPool().markDirty({name, newLeafPageNo});

    std::cout << "Inserted first tuple into the first leaf at page " << newLeafPageNo << std::endl;
    return;
  }

  // Traverse the B-tree to find the correct leaf page for insertion
  Page *currentPage = &root;

  while (true) {
    IndexPage indexPage(*currentPage);

    // If it's an index page, traverse down to the correct child
    if (indexPage.header->index_children) {
      size_t childPageNo = indexPage.findInsertPosition(std::get<int>(t.get_field(key_index))).pos;
      currentPage = &getDatabase().getBufferPool().getPage({name, childPageNo});
      currentPageNo = childPageNo;  // Update the current page number to the child
    } else {
      break; // We've found the leaf page
    }
  }

  // We are now at the leaf page, so we insert the tuple
  LeafPage leafPage(*currentPage, td, key_index);
  bool needsSplit = leafPage.insertTuple(t);

  if (needsSplit) {
    // If the leaf page is full, we need to split it
    // Use the next sequential page number for the new leaf page
    size_t newLeafPageNo = currentPageNo + 1;
    while (!getDatabase().getBufferPool().contains({name, newLeafPageNo})) {
      newLeafPageNo++;  // Make sure we don't overwrite existing pages
    }

    // Create a new leaf page
    Page &newLeafPage = getDatabase().getBufferPool().getPage({name, newLeafPageNo});
    LeafPage newLeaf(newLeafPage, td, key_index);

    // Split the original leaf page and propagate the median key upwards
    int splitKey = leafPage.split(newLeaf);

    while(true) {
      // We now need to propagate this split key to the parent index page
      // If the current page is the root (page 0), we must create a new root
      if (currentPageNo == 0) {
        // Create a new root page with the next sequential page number
        size_t newRootPageNo = newLeafPageNo + 1;
        while (!getDatabase().getBufferPool().contains({name, newLeafPageNo})) {
          newLeafPageNo++;  // Make sure we don't overwrite existing pages
        }

        Page &newRootPage = getDatabase().getBufferPool().getPage({name, newRootPageNo});
        IndexPage newRootIndexPage(newRootPage);

        // Insert the split key into the new root
        newRootIndexPage.insert(splitKey, newLeafPageNo);
        newRootIndexPage.header->index_children = true; // Mark as an internal node

        // Mark the new root page as dirty
        getDatabase().getBufferPool().markDirty({name, newRootPageNo});
        break;
      } else {
        // Insert the split key into the parent index page
        Page &parentPage = getDatabase().getBufferPool().getPage({name, currentPageNo});
        IndexPage parentIndexPage(parentPage);

        // Insert the split key into the parent index page
        bool needsParentSplit = parentIndexPage.insert(splitKey, newLeafPageNo);

        // Mark the parent page as dirty
        getDatabase().getBufferPool().markDirty({name, currentPageNo});
        getDatabase().getBufferPool().markDirty({name, newLeafPageNo});

        // If the parent page doesn't need to split, we are done
        if (!needsParentSplit) {
          break;
        }

        // If the parent needs to split, continue the loop
        size_t newParentPageNo = currentPageNo + 1;
        while (!getDatabase().getBufferPool().contains({name, newLeafPageNo})) {
          newLeafPageNo++;  // Make sure we don't overwrite existing pages
        }
        Page &newParentPage = getDatabase().getBufferPool().getPage({name, newParentPageNo});
        IndexPage newParentIndexPage(newParentPage);

        // Split the parent page
        int parentSplitKey = parentIndexPage.split(newParentIndexPage);

        // Update currentPageNo and splitKey to reflect the new parent split
        currentPageNo = newParentPageNo;
        splitKey = parentSplitKey;
        newLeafPageNo = newParentPageNo + 1;  // Update newLeafPageNo for the next iteration
      }
    }
  }
  // Mark the current page as dirty
  getDatabase().getBufferPool().markDirty({name, currentPageNo});
}


void BTreeFile::deleteTuple(const Iterator &it) {
  // Do not implement
}

Tuple BTreeFile::getTuple(const Iterator &it) const {
  // TODO pa2: implement
  // Get the leaf page based on the page number from the iterator
  Page &leaf = getDatabase().getBufferPool().getPage({name, it.page});

  // Create a LeafPage object to interact with the page
  LeafPage leafPage(leaf, td, key_index);

  // Retrieve the tuple at the current slot indicated by the iterator
  Tuple tuple = leafPage.getTuple(it.slot);

  // Return the retrieved tuple
  return tuple;
}

void BTreeFile::next(Iterator &it) const {
  std::cout << "Moving to next tuple at page: " << it.page << ", slot: " << it.slot << std::endl;
  // TODO pa2: implement
  // Get the current leaf page based on the iterator's page
  Page &leaf = getDatabase().getBufferPool().getPage({name, it.page});
  LeafPage leafPage(leaf, td, key_index);

  // Move to the next slot if possible
  if (it.slot < leafPage.header->size - 1) {
    it.slot++;
    std::cout << "Moving to next tuple at page: " << it.page << ", slot: " << it.slot << std::endl;
  } else if (leafPage.header->next_leaf != 0) {
    // If the current page is exhausted, move to the next leaf page
    it.page = leafPage.header->next_leaf;
    it.slot = 0; // Start at the first slot of the next leaf
    std::cout << "Moving to the next leaf page: " << it.page << std::endl;
  } else {
    // If there are no more pages, set the iterator to the end
    it.page = static_cast<size_t>(-1); // Invalid page number to indicate "end"
    it.slot = static_cast<size_t>(-1); // Invalid slot number
    std::cout << "End of B-tree reached" << std::endl;
  }
}

Iterator BTreeFile::begin() const {
  std::cout << "Calling begin()" << std::endl;
  // TODO pa2: implement
  // Start from the root page (always page 0)
  size_t rootPageNo = 0;
  Page &root = getDatabase().getBufferPool().getPage({name, rootPageNo});

  // Check if the root page contains any valid tuples (i.e., tree is not empty)
  IndexPage indexPage(root);
  std::cout << "Root page has " << indexPage.header->size << " children" << std::endl;

  // If there are no children or no keys in the root, the tree is empty
  if (indexPage.header->size == 0) {
    // Tree is empty, return end() iterator
    return end();
    std::cout << "Tree is empty, returning end()" << std::endl;
  }

  // Otherwise, traverse down to the leftmost leaf page
  Page *currentPage = &root;
  size_t currentPageNo = rootPageNo;

  while (indexPage.header->index_children) {
    std::cout << "Traversing to child page: " << indexPage.children[0] << std::endl;
    // Move to the leftmost child
    currentPageNo = indexPage.children[0];
    currentPage = &getDatabase().getBufferPool().getPage({name, currentPageNo});
    indexPage = IndexPage(*currentPage);
  }

  std::cout << "Found first leaf page: " << currentPageNo << std::endl;
  // Return an iterator pointing to the first tuple in the leftmost leaf page
  return Iterator(*this, currentPageNo, 0);
}

Iterator BTreeFile::end() const {
  // TODO pa2: implement
  // Return an invalid iterator, which signifies the end
  return Iterator(*this, static_cast<size_t>(-1), static_cast<size_t>(-1));
}
