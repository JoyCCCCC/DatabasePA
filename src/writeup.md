# Database pa2

---

## Design Decisions

### 1. LeafPage
- constructor:
    - compute the capacity: (DEFAULT_PAGE_SIZE - sizeof(LeafPageHeader)) / td.length();
    //the leafpage stores header and tuples.
    - initialize pointers of head and data;
- insertTuple:
    - I write a helper function called findInsertPosition, it returns {bool:update, size_t:pos}.
    In that function, I use 2-divide algorithm to find the position to insert.
    If the key alreadys exists, the function returns {true, pos} in condition of (MID==KEY) during that algorithm. Otherwise, it returns {false, pos}. We then can simply update it, or move the data behind this pos, and insert it.
- split:
    - caculate new sizes for both pages, and copy half of the data to the new page.
    - use td.deserialize to get the first key and update it for new page.

### 2. IndexPage
- constructor:
    - it stores header, keys, and children. Since the num of children is size+1, the capacity is: (Page_size - Header_Size -Child_Size)/(key_size+child_size)
    - caculate capacity before initialize children* since we need it to calculate how far it is from keys*.
- insert:
    Just like what I do in LeafPage.cpp, I also write a similar helper function to find whether it is an updatition and where is the insert position.
    Then, update or copy and insert.
- split:
    It's also the same as what I do in LeafPage.cpp.
    But when copying data, notice that the number of children is 1+keys.size().

### 3. BTreeFile
- insertTuple:
    - **Description**: Inserts a tuple `t` into the B-tree, managing splits in leaf or index pages as needed. Split keys are propagated upwards to parent nodes.
    - **Logic**:
        - Begins at the root page (`page 0`), using a stack to track parent pages.
        - If the root is empty, creates the first leaf node and initializes the root's child to this new leaf.
        - If the root is populated, traverses down the tree:
        - Determine whether the child node is a leaf by checking the value of `index_children`. Finds the appropriate leaf page for insertion.
        - If the leaf page is full, splits the page, propagating the split key up to the parent node, potentially splitting parent nodes up to the root.
        - Marks affected pages as "dirty" to track changes in the buffer pool.

- getTuple:
    - **Description**: Retrieves a tuple from the B-tree at a specified iterator position.
    - **Logic**:
        - Obtains the leaf page based on the iterator’s page number.
        - Retrieves the tuple at the iterator's slot within the leaf page.

- next:
    - **Description**: Moves the iterator to the next tuple within the B-tree.
    - **Logic**:
        - Advances to the next slot in the current leaf page if possible.
        - If the current page is exhausted, moves to the next leaf page if it exists.
        - If no further pages are available, sets the iterator to an "end" state, representing the end of the B-tree.

- begin:
    - **Description**: Initializes an iterator pointing to the first tuple in the B-tree.
    - **Logic**:
        - Starts at the root page, checking if the B-tree is empty.
        - If the tree is non-empty, traverses down to the leftmost leaf page.
        - Returns an iterator positioned at the first tuple in this leaf.

- end:
    - **Description**: Returns an "end" iterator, representing the position after the last tuple in the B-tree.
    - **Logic**:
        - Sets the iterator to an invalid state(invalid page and slot), which acts as a signal for reaching the end of the B-tree.  

## Incomplete Elements
There are still some issues with `insertTuple` in `BTreeFile`. When the root splits, I set `index_children` of the new root to `true`, indicating that the next layer of the root is internal nodes. However, when inserting the next tuple, `index_children` of the new root changes back to `false`, which causes issues with subsequent insertions. I’m not sure why this is happening.  

## Conclusion
The majority of the time was spent on the `insertTuple` function of `BTreeFile` this time. Due to the complexity of the logic,
many detailed errors occurred along the way. A particular challenge was how to update the `Keys` and `Children` arrays in the structure whenever a split occurred and a new `indexPage` was created.

## Collaboration
Juling Fan: Complete IndexPage and LeafPage.
Yijia Chen: Complete BTreeFile based on IndexPage and LeafPage logic.