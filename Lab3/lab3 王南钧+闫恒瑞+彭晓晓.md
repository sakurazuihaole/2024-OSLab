## 练习一：理解基于FIFO的页面替换算法（思考题）

在 `vmm.c do_pgfault` 中调用`swap_in`函数.

该函数位于`swap.c`文件中的`swap_in`。

+ `swap_in`：用于换入页面。首先调用`pmm.c`中的`alloc_page`，申请一块连续的内存空间，然后调用`get_pte`找到或者构建对应的页表项，最后调用`swapfs_read`将数据从磁盘写入内存。

+ `alloc_page`：用于申请页面。通过调用`pmm_manager->alloc_pages`申请一块连继续的内存空间，在这个过程中，如果申请页面失败，那么说明需要换出页面，则调用`swap_out`换出页面，之后再次进行申请。

+ `assert(result!=NULL)`：判断获得的页面是否为`NULL`，只有页面不为`NULL`才能继续。

+ `wap_out`：用于换出页面。函数内部是一个循环循环调用`sm->swap_out_victim`，对应于`swap_fifo`中的`_fifo_swap_out_victim`。然后调用`get_pte`获取对应的页表项，将该页面写入磁盘，如果写入成功，释放该页面；如果写入失败，调用`_fifo_map_swappable`更新FIFO队列。最后刷新TLB

+ _fifo_swap_out_victim`：用于获得需要换出的页面。查找队尾的页面，作为需要释放的页面。

+ `_fifo_map_swappable`：将最近使用的页面添加到队头。在`swap_out`中调用是用于将队尾的页面移动到队头，防止下一次换出失败。

+ `free_page`：用于释放页面。通过调用`pmm_manager->free_pages`释放页面。

+ `assert((*ptep & PTE_V) != 0);`：用于判断获得的页表项是否合法。由于这里需要交换出去页面，所以获得的页表项必须是合法的。

+ `swapfs_write`：用于将页面写入磁盘。在这里由于需要换出页面，而页面内容如果被修改过那么就与磁盘中的不一致，所以需要将其重新写回磁盘。

+ `tlb_invalidate`：用于刷新TLB。通过调用`flush_tlb`刷新TLB。

+ `get_pte`：用于获得页表项。
  
  

## 练习二：理解基于FIFO的页面替换算法（思考题）

get_pte()函数（位于`kern/mm/pmm.c`）用于在页表中查找或创建页表项，从而实现对指定线性地址对应的物理页的访问和映射操作。这在操作系统中的分页机制下，是实现虚拟内存与物理内存之间映射关系非常重要的内容。

- get_pte()函数中有两段形式类似的代码， 结合sv32，sv39，sv48的异同，解释这两段代码为什么如此相像。
  - 两端代码的功能都是要获取虚拟地址对应的页表项，并在缺页等异常时创建新的page以及页表项。由于三种页表管理结构基本，只在页表级和地址空间大小上有所不同，所以两段代码在功能实现时只需要规定不同的地址偏移量便可满足相应需求。
- 目前get_pte()函数将页表项的查找和页表项的分配合并在一个函数里，你认为这种写法好吗？有没有必要把两个功能拆开?
  - 在大部分情况下，只有获取非法页表时才需要创建并分配新的页表，在此情况下，将两个功能合并在一个函数中能够减少代码重复和函数调用，提高程序的运行效率。

## 练习三：给未被映射的地址映射上物理页（需要编程）

补充完成do_pgfault（mm/vmm.c）函数，给未被映射的地址映射上物理页。设置访问权限 的时候需要参考页面所在 VMA 的权限，同时需要注意映射物理页时需要操作内存控制 结构所指定的页表，而不是内核的页表。

代码实现

```代码实现
swap_in(mm,addr,&page);//首先需要根据页表基地址和虚拟地址完成磁盘的读取，写入内存，
返回内存中的物理页。
page_insert(mm->pgdir,page,addr,perm);//然后完成虚拟地址和内存中物理页的映射。
swap_map_swappable(mm,addr,page,0);//最后设置该页面是可交换的。
```

- 请描述页目录项（Page Directory Entry）和页表项（Page Table Entry）中组成部分对ucore实现页替换算法的潜在用处。
  - 页目录项（PDE)和页表项(PTE)的结构相似，都包含物理页地址和一些标志位。其中存在位（P)可以用来判断该页面是否存在；读写位（R/W）来判断页面是否可读写；访问位（A）来判断该页面最近是否被访问过；脏页位（D）则表明页面内容被修改过，需要写回磁盘后才能替换等等。其中一部分标志位可以用于CLOCK算法或LRU算法。
- 如果ucore的缺页服务例程在执行过程中访问内存，出现了页访问异常，请问硬件要做哪些事情？
  -  首先保存当前缺页服务例程，然后调用exception_handler中的CAUSE_LOAD_ACCESS函数处理当前的的缺页异常，处理完之后再跳转到保存的缺页服务例程，完成操作后再返回到异常出现处继续允许程序。
  - 数据结构Page的全局变量（其实是一个数组）的每一项与页表中的页目录项和页表项有无对应关系？如果有，其对应关系是啥？
    - 有对应关系。 数据结构Page的全局变量中的每一项代表一个物理页，而页目录项和页表项则记录了这些物理页的地址和状态信息。这可以用来索引到对应的 Page 结构体，从而允许操作系统管理和跟踪物理内存的使用。以使得操作系统能够有效地管理和替换页面
      
      

## 练习四：补充完成Clock页替换算法

### 实验要求

通过之前的练习，相信大家对FIFO的页面替换算法有了更深入的了解，现在请在我们给出的框架上，填写代码，实现 `Clock`页替换算法（`mm/swap_clock.c`）。

### 代码实现

`_clock_init_mm()`函数实现如下：

```C++
static int
_clock_init_mm(struct mm_struct *mm)
{     
     /*LAB3 EXERCISE 4: 2113277*/ 
     // 初始化pra_list_head为空链表
     // 初始化当前指针curr_ptr指向pra_list_head，表示当前页面替换位置为链表头
     // 将mm的私有成员指针指向pra_list_head，用于后续的页面替换算法操作
     //cprintf(" mm->sm_priv %x in fifo_init_mm\n",mm->sm_priv);
     list_init(&pra_list_head);
     curr_ptr = &pra_list_head;
     mm->sm_priv = &pra_list_head;
     return 0;
}
```

根据练习提示，将链表`pra_list_head`初始化为空链表，定义`curr_ptr`指向链表头部用以表示当前指针，并且将`mm`的私有成员也指向`pra_list_head`。

`_clock_map_swappable()`函数实现如下：

```c
static int
_clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *entry=&(page->pra_page_link);

    assert(entry != NULL && curr_ptr != NULL);
    //record the page access situlation
    /*LAB3 EXERCISE 4: 2113277*/ 
    // link the most recent arrival page at the back of the pra_list_head qeueue.
    // 将页面page插入到页面链表pra_list_head的末尾
    // 将页面的visited标志置为1，表示该页面已被访问
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_add(head, entry);
    page->visited = 1;
    return 0;
}
```

根据练习提示，从内存管理结构`mm`中获取页面链表`pra_list_head`的头指针后，将当前访问页`page`插入到`pra_list_head`的末尾，并将其标志位置为1，可以理解为将最近访问的页面移动到页面链表`pra_list_head`的末尾，以保证页表项的新鲜度。

`_clock_swap_out_victim()`函数实现如下：

```c
static int
_clock_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
     list_entry_t *head=(list_entry_t*) mm->sm_priv;
         assert(head != NULL);
     assert(in_tick==0);
     /* Select the victim */
     //(1)  unlink the  earliest arrival page in front of pra_list_head qeueue
     //(2)  set the addr of addr of this page to ptr_page
    while (1) {
        /*LAB3 EXERCISE 4: 2113277*/ 
        // 编写代码
        // 遍历页面链表pra_list_head，查找最早未被访问的页面
        // 获取当前页面对应的Page结构指针
        // 如果当前页面未被访问，则将该页面从页面链表中删除，并将该页面指针赋值给ptr_page作为换出页面
        // 如果当前页面已被访问，则将visited标志置为0，表示该页面已被重新访问
        if(curr_ptr == head){
            curr_ptr = list_prev(curr_ptr);
            continue;
        }
        struct Page* curr_page = le2page(curr_ptr,pra_page_link);
        if(curr_page->visited == 0){
            cprintf("curr_ptr %p\n", curr_ptr);
            curr_ptr = list_prev(curr_ptr);
            list_del(list_next(curr_ptr));
            *ptr_page = curr_page;
            return 0;
        }
        curr_page->visited = 0;
        curr_ptr = list_prev(curr_ptr);
    }
    return 0;
}
```

根据练习提示，通过遍历页面链表`pra_list_head`，查找最早未被访问的页面`page`。如果该页面未被访问，则将其从页面链表中删除，以便换入需要的页表项；如果该页面被访问过，则将标志位置为0，继续遍历直至找到符合条件的需要换出的页。

### 问题：**比较Clock页替换算法和FIFO算法的不同。**

`FIFO`算法：算法的核心思想是淘汰最先进入内存的页，通过维护一个队列数据结构实现。局限性在于，`FIFO` 算法只在应用程序按**线性顺序访问地址空间**时效果才好，否则效率并不高。`FIFO` 算法也存在一种异常现象（`Belady` 现象），即在增加放置页的物理页帧的情况下，反而使页访问异常次数增多。

`Clock`页替换算法：时钟算法需要在页表项中设置了一位访问位来表示此页表项**对应的页当前是否被访问过**。该算法近似地体现了 `LRU` 的思想，易于实现，开销少，需要硬件支持来设置访问位。时钟页替换算法在本质上与 `FIFO` 算法是类似的，不同之处是在时钟页替换算法中跳过了访问位为 1 的页。

综上所述，`Clock`页替换算法考虑了页表项表示的页是否被访问过，而`FIFO`算法并没有考虑这点。

### 思考：关于硬件部分PTE中access位的检查

在实验中只考虑了对于内存中定义的`visited`位进行检查，实际上硬件中的`PTE`中`access`位才真正标识了当前页是否被访问过，如果想要实现对`access`位的检查，可以考虑：

+ 增加一个检查`access`位的函数`check_pte_access()`
+ 增加一个改变`access`位的函数`change_pte_access()`
+ 在实现`clock()`算法时，可以先对`access`位进行检查后，再进行`visited`位的处理，最后在`visited`改变之后，同时运行`change_pte_access()`以改变标识位，实现代码和硬件的统一性。

 

## 练习五：阅读代码和实现手册，理解页表映射方式相关知识

#### 提问：如果我们采用”一个大页“ 的页表映射方式，相比分级页表，有什么好处、优势，有什么坏处、风险？

如果我们采用”一个大页“ 的页表映射方式，相比分级页表，其优势在于：表现方式更加直观和简单，查找页表项的速度也更快，不存在时延，如果需要分配较大连续内存时，契合度更高；其劣势在于：如果只有一个大页，并且不需要很多内存时，其中有许多部分会被浪费，灵活性也不如分级页表，还有潜在的兼容性问题。
