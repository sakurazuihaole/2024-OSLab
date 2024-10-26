# 1.练习1

该代码实现了一个简单的内存管理系统，使用**首次适配（First-Fit Allocation, FFMA）算法**进行内存分配。内存块以页的形式被管理，使用双向链表记录空闲内存块。下面是各个函数的作用：

1. **`default_init`**
   
   - 初始化内存分配器的空闲内存块列表 `free_list`，并将空闲页的计数 `nr_free` 设置为 0。
   - 作用是初始化内存分配管理系统的内部数据结构。

2. **`default_init_memmap`**
   
   - 初始化指定的内存页块，将这些页的属性标记为“空闲”。
   - 通过遍历每个页，设置其属性标记，确保这些页是可分配的，并将它们加入到 `free_list` 链表中，保持地址有序。
   - 该函数还会更新系统的空闲内存页数 `nr_free`。

3. **`default_alloc_pages`**
   
   - 分配指定数量的连续内存页。
   - 从 `free_list` 中搜索第一个满足需求的空闲内存块，如果找到一个大小合适的块，则将该块的前 `n` 页分配出去，剩余的页重新插入空闲列表中。如果没有足够大的块，则返回 `NULL`。
   - 更新 `nr_free` 以反映当前空闲页的数量。

4. **`default_free_pages`**
   
   - 释放指定数量的连续内存页，并将它们重新加入空闲内存块链表中。
   - 该函数还会尝试将相邻的空闲内存块合并成更大的块，以优化内存的使用。
   - 最后更新 `nr_free`，表示系统中空闲的内存页数量。

5. **`default_nr_free_pages`**
   
   - 返回当前系统中剩余的空闲内存页数。

6. **`basic_check`**
   
   - 该函数用于基本的内存分配/释放功能测试，确保内存管理系统的各项功能如预期工作。
   - 包括分配、释放内存页，检查内存块的引用计数，确保没有内存泄漏或错误的内存管理操作。

7. **`default_check`**
   
   - 用于进一步测试首次适配算法的正确性，确保内存块的分配和释放能够按照预期执行。
   - 通过一系列的内存分配、释放操作，验证分配器在复杂情况下的行为是否正确。

8. **`pmm_manager` 结构体**
   
   - 这是一个接口定义，包含内存管理器的各种操作函数，如初始化、分配、释放内存页等。`default_pmm_manager` 通过这个结构体与系统其他部分进行交互。

### 核心思想

- 代码实现的是一种**首次适配内存分配算法**，它维护一个双向链表来管理空闲内存块，分配时扫描列表，找到第一个满足请求的块。如果一个块大于请求，则将剩余部分再次放回空闲列表中。

### 内存管理系统改进策略

在分析首次适配内存分配算法的基础上，针对现有实现可以提出以下改进策略，以提升系统的效率、性能和健壮性。

#### 1. 减少碎片化

- **问题**: 首次适配算法容易导致内存碎片化，尤其是频繁分配和释放较小块时，内存中会出现许多无法使用的小碎片。
- **改进策略**:
  - 实现**最佳适配算法（Best-Fit Allocation）**或**快速适配算法（Quick-Fit Allocation）**，通过精确匹配所需内存块大小或将内存分为固定大小的块来减少碎片化。
  - 实现**内存块合并策略**：除了释放时合并相邻块外，可以定期执行全局合并操作，减少碎片化的影响。

#### 2. 空闲列表优化

- **问题**: 每次分配内存时需要遍历整个空闲列表，随着内存块数量增加，性能会下降。
- **改进策略**:
  - 使用**分区空闲列表**：将空闲内存块按大小分区，维护多个链表，以便快速找到合适的块，减少遍历的开销。
  - ### **索引优化**：为空闲列表添加**跳表（Skip List）**或**平衡二叉树**结构，以更快速地搜索空闲块。

#### 3. 合并相邻块的优化

- **问题**: 合并相邻内存块时，线性遍历列表性能较差。
- **改进策略**:
  - 增加一个**双向链表索引表**，或使用**红黑树**、**AVL树**等复杂数据结构，将查找相邻块的时间复杂度降为 `O(log n)`。

#### 4. 内存页多状态管理

- **问题**: 内存页的状态通过简单标记管理，扩展性较差。
- **改进策略**:
  - 使用更复杂的**状态位图（bitmap）**或引入**多级状态标志位**，为不同类型的内存块提供更精细的管理。

# 2.练习2

### 核心思想

在系统需要分配内存时，同`first-fit`一样按序查找，不同的是，需要更新符合要求的最小内存块。遇到第一块比所需内存大的块是会先分配给`page`，之后仍继续进行查找，如果找到符合要求并且比当前分配的内存块更小的空闲块时，需要释放先前的内存块，进行`page`的更新，之后同`first-fit`算法类似，按照顺序插入链表，并与相邻连续的空闲块进行合并。

### 实现步骤

```
static void
best_fit_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));

        /*LAB2 EXERCISE 2: 2113277*/ 
        // 清空当前页框的标志和属性信息，并将页框的引用计数设置为0
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;
    if (list_empty(&free_list)) {
        list_add(&free_list, &(base->page_link));
    } else {
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {
            struct Page* page = le2page(le, page_link);
             /*LAB2 EXERCISE 2: 2113277*/ 
            // 编写代码
            // 1、当base < page时，找到第一个大于base的页，将base插入到它前面，并退出循环
            // 2、当list_next(le) == &free_list时，若已经到达链表结尾，将base插入到链表尾部
            if(base < page) 
            {
                list_add_before(le, &(base->page_link));
                break;
            }
            else if (list_next(le) == &free_list)
            {
                list_add(le, &(base->page_link));
            }
        }
    }
}
```

以上代码主要初始化内存管理系统的空闲页链表，并且根据`base`与`page`的比较将链表插入到正确的位置，与`first-fit`算法类似。

```
static struct Page *
best_fit_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }
    struct Page *page = NULL;
    list_entry_t *le = &free_list;
    size_t min_size = nr_free + 1;
     /*LAB2 EXERCISE 2: 2113277*/ 
    // 下面的代码是first-fit的部分代码，请修改下面的代码改为best-fit
    // 遍历空闲链表，查找满足需求的空闲页框
    // 如果找到满足需求的页面，记录该页面以及当前找到的最小连续空闲页框数量
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n && p->property < min_size) {
            page = p;
            min_size = p->property;
        }
    }

    ...
    return page;
}
```

以上代码也是**`best-fit`算法实现的核心代码**，在分配内存块时通过遍历找到满足所需大小的**最小的**空闲块，根据`min_size`进行可分配的空闲块数量的更新。

```
static void
best_fit_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }
    /*LAB2 EXERCISE 2: 2113277*/ 
    // 编写代码
    // 具体来说就是设置当前页块的属性为释放的页块数、并将当前页块标记为已分配状态、最后增加nr_free的值
    base->property = n;
    SetPageProperty(base);
    nr_free += n;
    
    ...
    
    list_entry_t* le = list_prev(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        /*LAB2 EXERCISE 2: 2113277*/ 
         // 编写代码
        // 1、判断前面的空闲页块是否与当前页块是连续的，如果是连续的，则将当前页块合并到前面的空闲页块中
        // 2、首先更新前一个空闲页块的大小，加上当前页块的大小
        // 3、清除当前页块的属性标记，表示不再是空闲页块
        // 4、从链表中删除当前页块
        // 5、将指针指向前一个空闲页块，以便继续检查合并后的连续空闲页块
        if (p + p->property == base)//连续
        {
            p->property += base->property;
            ClearPageProperty(base);
            list_del(&(base->page_link));
            base = p;
        }
    }

    ...
}
```

以上代码实现了内存块的释放，将对应的内存块的属性及标记位进行更改，同时更新可用空闲块的总数量`nr_free`，并且根据块的情况判断是否需要与前面或后面的块进行合并，减少碎片化。

#### Q：你的 Best-Fit 算法是否有进一步的改进空间？

与`first-fit`算法类似，可以通过减少碎片化、优化空闲列表、对相邻块的合并进行优化、内存页的多状态等进行改进。同时，`first-fit`算法的分配块函数中`min_size`的引入注定了该算法需要遍历所有空闲链表，时间花费较长，或许可以与`first-fit`算法交替使用。

# Challange1:伙伴系统实现思路

在这个伙伴系统的实现中，内存被分割为多个大小为2的幂次的内存块，每个内存块的大小从最小的1页到最大的2^(MAX_ORDER - 1)页。系统通过维护多个链表，每个链表对应一个不同大小的空闲内存块。当需要分配或释放内存时，系统会根据内存块的大小调整相应的链表。

## 具体实现思路

1. **初始化 (`buddy_system_init`)**
   
   - 初始化伙伴系统时，系统会创建多个链表，分别对应不同大小的内存块。每个链表的大小为2的幂次（即从1页、2页、4页……到2^(MAX_ORDER-1)页）。
   - `free_area_t`结构体包含了链表和空闲内存块的数量。

2. **内存映射初始化 (`buddy_system_init_memmap`)**
   
   - 初始化物理内存时，伙伴系统从较大的内存块开始分配，将内存块放入对应的链表中。
   - 如果当前的内存块不能完全放入高阶的链表，会逐步将较大的块拆分为较小的块，直到所有内存块都能够放入合适的链表。
   - 每个内存块通过`property`属性标记其大小，并加入到链表中。

```C++
tatic void//从能存放最大内存块的链表开始，将要存放的物理内存放到链表中，若不能继续放入到这个链表中，则进行一步步降低能存放内存块大小的链表，
buddy_system_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    size_t curr_size = n;
    uint32_t order = MAX_ORDER - 1;
    uint32_t order_size = 1 << order;
    p = base;
    while (curr_size != 0) {
        p->property = order_size;
        SetPageProperty(p);
        nr_free(order) += 1;
        list_add_before(&(free_list(order)), &(p->page_link));
        curr_size -= order_size;
        while(order > 0 && curr_size < order_size) {
            order_size >>= 1;
            order -= 1;
        }
        p += order_size;
    }
}
```

3. **内存块拆分 (`split_page`)**
   - 当在分配内存时没有足够大小的内存块，系统会从较高阶的链表中取出一个较大的内存块，并将其拆分为两个较小的块（大小为`order-1`的块）。
   - 拆分后的两个较小的内存块会被加入到低阶的链表中，并更新空闲块数量。

```C++
//取出高一级的空闲链表中的一个块，将其分为两个较小的快，大小是order-1，加入到较低一级的链表中，注意nr_free数量的变化
static void split_page(int order) {
    if(list_empty(&(free_list(order)))) {
        split_page(order + 1);
    }
    list_entry_t* le = list_next(&(free_list(order)));
    struct Page *page = le2page(le, page_link);
    list_del(&(page->page_link));
    nr_free(order) -= 1;
    uint32_t n = 1 << (order - 1);
    struct Page *p = page + n;
    page->property = n;
    p->property = n;
    SetPageProperty(p);
    list_add(&(free_list(order-1)),&(page->page_link));
    list_add(&(page->page_link),&(p->page_link));
    nr_free(order-1) += 2;
    return;
}
```

4. **内存分配 (`buddy_system_alloc_pages`)**
   
   - 当需要分配内存时，系统首先找到适合分配的内存块大小（通过寻找大于等于所需大小的最小阶数的内存块）。
   - 如果没有适合大小的内存块，系统会通过`split_page`拆分较大的内存块。
   - 分配时，系统会从链表中取出一个合适大小的内存块并标记其为已分配。

5. **内存块释放 (`buddy_system_free_pages`)**
   
   - 释放内存时，系统会将内存块重新标记为空闲，并尝试将其与相邻的内存块合并。
   - 通过遍历相邻的内存块，判断它们是否可以合并成更大的内存块，最终可能将内存块合并回更高阶的链表中。

```C++
buddy_system_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    assert(IS_POWER_OF_2(n));
    assert(n < (1 << (MAX_ORDER - 1)));
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));//确保页面没有被保留且没有属性标志
        p->flags = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);

    uint32_t order = 0;
    size_t temp = n;
    while (temp != 1) {//找到能将此内存块放入的链表序号，根据幂次方的大小对序号进行加法运算，直到确定序号
        temp >>= 1;
        order++;
    }
    add_page(order,base);
    merge_page(order,base);
}

static size_t
buddy_system_nr_free_pages(void) {//计算空闲页面的数量，空闲块*块大小（与链表序号有关）
    size_t num = 0;
    for(int i = 0; i < MAX_ORDER; i++) {
        num += nr_free(i) << i;
    }
    return num;
}
```

6. **内存合并 (`merge_page`)**
   - 在释放内存块后，系统会检查相邻的内存块是否可以合并。
   - 如果相邻的内存块地址是连续的且大小相同，系统会将它们合并成一个更大的块，并将其放入高阶的链表中，减少内存碎片。

```C++
static void merge_page(uint32_t order, struct Page* base) {
    if (order == MAX_ORDER - 1) {//没有更大的内存块了，升不了级了
        return;
    }

    list_entry_t* le = list_prev(&(base->page_link));
    if (le != &(free_list(order))) {
        struct Page *p = le2page(le, page_link);
        if (p + p->property == base) {//若是连续内存
            p->property += base->property;
            ClearPageProperty(base);
            list_del(&(base->page_link));
            base = p;
            if(order != MAX_ORDER - 1) {
                list_del(&(base->page_link));
                add_page(order+1,base);
            }
        }
    }

    le = list_next(&(base->page_link));
    if (le != &(free_list(order))) {
        struct Page *p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
            if(order != MAX_ORDER - 1) {
                list_del(&(base->page_link));
                add_page(order+1,base);
            }
        }
    }
    merge_page(order+1,base);
    return;
}
```

7. **计算空闲页面数量 (`buddy_system_nr_free_pages`)**
   
   - 系统通过遍历各个链表，计算当前系统中空闲的内存页数量。空闲块的数量乘以其对应的块大小，即可得到空闲页数。

8. **基本检查 (`basic_check`)**
   
   - 通过基本的分配与释放内存块的流程，验证伙伴系统的正确性，包括重复分配、释放内存后重新分配等操作。

9. **伙伴系统接口 (`buddy_system_pmm_manager`)**
   
   - `buddy_system_pmm_manager`结构体封装了伙伴系统的核心功能，包括初始化、内存分配、释放、检查等操作接口，供外部调用。
     
     ## 

## Challenge3 :如何获取当前内存大小



- 可以分段检测物理内存是否被占用，或者读取系统内存映射表，如ACPI表中的内存资源信息，俩获得未被占用的物理内存。
- 向内存中写入并读取数据，如果超过内存范围，那么不管写入什么，再读取都会返回零。
- 利用BIOS/UEFI的终端功能：BIOS终端会提供一个检索内存的功能，如x86中的INT 0x 15中断调用来获取内存大小信息

## 重点内容

- 以页为单位管理物理内存
  页表存储在内存中，由若干个固定大小的页表项组成
  
  页表项的固定格式是，在sv39中，页表项的大小是占8字节（64位），第53-10位为物理页号（56为地址，最后12位是偏移量，同一位置的物理地址与虚拟地址的偏移量是一样的，所以可不用记录，所以用44位来表示物理页号），9-0位共10位描述状态信息：
  
  - RSW：2位，留给S态的应用程序
  - D：自被清零后，有虚拟地址通过此页表项进行写入
  - A：自被清零后，有虚拟地址通过此页表项进行读、写入、取值
  - G：全局，所有页表都包含这个页表项
  - U：用户态的程序通过此页表项进行映射，S Mode不一定通过U=1的页表项及逆行映射，S Mode都不允许执行U=1的页表里包含的指令
  - R、W、X：可读、可写、可执行

- 多级页表管理
  
  - 每个一级页表项控制一个虚拟页号，即控制 4KB 虚拟内存；
  - 每个二级页表项则控制 9 位虚拟页号，总计控制 4KB×2^9 =2MB 虚拟内存；
  - 每个三级页表项控制18 位虚拟页号，总计控制 2MB×2^9 =1GB 虚拟内存；
  
  39位的虚拟地址，前27位中9位代表三级页号，9位代表三级页号中的偏移量（二级页号）、9位代表二级页号中的偏移量（一级页号），一级页号中的页表项对应一个4KB的虚拟页号，后12位是虚拟页号中的偏移量。也就是说**有512（2 的 9 次方）个大大页**，每个大大页里有 **512 个大页**，每个大页里有 **512 个页**，每个页里有 4096 个字节。整个虚拟内存空间里就有 512∗512∗512∗4096 个字节，是 512GB 的地址空间。
  
  Sv39 的多级页表在逻辑上是一棵树，它的每个叶子节点（直接映射 4KB 的页的页表项）都对应内存的一页，其根节点的物理地址（最高级页表的物理页号）存放在satp寄存器中；

- **进入虚拟内存访问方式的步骤：**
  
  - 分配页表所在内存空间并初始化页表
  - 设置好基址寄存器（指向页表起始地址）
  - 刷新TLB

- 多种内存分配算法：
  
  - First Fit算法：当需要分配页面时，它会从空闲页块链表中找到第一个适合大小的空闲页块，然后进行分配；当释放页面时，它会将释放的页面添加回链表，并在必要时合并相邻的空闲页块，以最大限度地减少内存碎片；
  - Best Fit算法：在分配内存块时，按照顺序查找，遇到第一块比所需内存块大的空闲内存块时，先将该块分配给`page`，之后继续查询，如果查询到大小比分配的内存块小的空闲内存块，将`page`更新为当前的内存块。释放内存块时，按照顺序将其插入链表中，并合并与之相邻且连续的空闲内存块；
  - buddy system分配算法：是一种内存分配策略，用于管理和分配物理内存页面，使用11个链表来存储0~1024大小的内存块；
