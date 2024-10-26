#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_system_pmm.h>
#include <stdio.h>

#define MAX_ORDER 11

// 外部声明的空闲区域数组
extern free_area_t free_area[MAX_ORDER];

// 宏定义，用于方便访问
#define free_list(i) free_area[(i)].free_list
#define nr_free(i) free_area[(i)].nr_free
#define IS_POWER_OF_2(x) (!((x)&((x)-1))) // 判断是否为2的幂

// 初始化空闲页面管理
static void buddy_system_init(void) {
    for (int i = 0; i < MAX_ORDER; i++) {
        list_init(&(free_area[i].free_list)); // 初始化链表
        free_area[i].nr_free = 0; // 设置空闲计数为0
    }
}

// 初始化物理内存映射
static void buddy_system_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(PageReserved(p));
        p->flags = p->property = 0; // 重置标志和属性
        set_page_ref(p, 0);
    }
    size_t curr_size = n;
    uint32_t order = MAX_ORDER - 1;
    uint32_t order_size = 1 << order;
    p = base;
    while (curr_size != 0) {
        p->property = order_size; // 设置当前页面大小
        SetPageProperty(p);
        nr_free(order) += 1; // 增加空闲计数
        list_add_before(&(free_list(order)), &(p->page_link)); // 加入链表
        curr_size -= order_size;
        while (order > 0 && curr_size < order_size) {
            order_size >>= 1; // 减小大小
            order -= 1;
        }
        p += order_size; // 移动指针
    }
}

// 分裂页面
static void split_page(int order) {
    if (list_empty(&(free_list(order)))) {
        split_page(order + 1); // 递归到下一级
    }
    list_entry_t *le = list_next(&(free_list(order)));
    struct Page *page = le2page(le, page_link);
    list_del(&(page->page_link)); // 从链表中移除
    nr_free(order) -= 1; // 更新空闲计数
    uint32_t n = 1 << (order - 1);
    struct Page *p = page + n;
    page->property = p->property = n; // 设置新页面属性
    SetPageProperty(p);
    list_add(&(free_list(order - 1)), &(page->page_link)); // 加入低一级链表
    list_add(&(page->page_link), &(p->page_link));
    nr_free(order - 1) += 2; // 更新计数
}

// 分配页面
static struct Page *buddy_system_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > (1 << (MAX_ORDER - 1))) return NULL; // 超过最大限制
    struct Page *page = NULL;
    uint32_t order = MAX_ORDER - 1;
    while (n < (1 << order)) order--; // 找到合适的订单
    order += 1; // 转到下一级
    uint32_t flag = 0;
    for (int i = order; i < MAX_ORDER; i++) flag += nr_free(i); // 统计空闲页面
    if (flag == 0) return NULL; // 没有空闲页面
    if (list_empty(&(free_list(order)))) split_page(order + 1);
    if (list_empty(&(free_list(order)))) return NULL; // 仍然没有空闲页面
    list_entry_t *le = list_next(&(free_list(order)));
    page = le2page(le, page_link);
    list_del(&(page->page_link)); // 移除页面
    ClearPageProperty(page); // 清除属性
    return page;
}

// 添加页面到指定的链表
static void add_page(uint32_t order, struct Page *base) {
    list_entry_t *le = &(free_list(order));
    while ((le = list_next(le)) != &(free_list(order))) {
        struct Page *page = le2page(le, page_link);
        if (base < page) {
            list_add_before(le, &(base->page_link)); // 插入到合适的位置
            return;
        }
    }
    list_add(le, &(base->page_link)); // 追加到链表末尾
}

// 合并页面
static void merge_page(uint32_t order, struct Page *base) {
    if (order == MAX_ORDER - 1) return; // 达到最大订单，停止
    list_entry_t *le = list_prev(&(base->page_link));
    // 检查前一个页面
    if (le != &(free_list(order))) {
        struct Page *p = le2page(le, page_link);
        if (p + p->property == base) {
            p->property += base->property; // 合并
            ClearPageProperty(base);
            list_del(&(base->page_link));
            base = p;
            if (order != MAX_ORDER - 1) {
                list_del(&(base->page_link));
                add_page(order + 1, base);
            }
        }
    }
    // 检查后一个页面
    le = list_next(&(base->page_link));
    if (le != &(free_list(order))) {
        struct Page *p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property; // 合并
            ClearPageProperty(p);
            list_del(&(p->page_link));
            if (order != MAX_ORDER - 1) {
                list_del(&(base->page_link));
                add_page(order + 1, base);
            }
        }
    }
    merge_page(order + 1, base); // 递归合并
}

// 释放页面
static void buddy_system_free_pages(struct Page *base, size_t n) {
    assert(n > 0 && IS_POWER_OF_2(n) && n < (1 << (MAX_ORDER - 1)));
    struct Page *p = base;
    for (; p != base + n; p++) {
        assert(!PageReserved(p) && !PageProperty(p)); // 确保页面可以释放
        p->flags = 0; // 重置标志
        set_page_ref(p, 0); // 清除引用
    }
    base->property = n; // 设置属性
    SetPageProperty(base);
    
    uint32_t order = 0;
    size_t temp = n;
    while (temp != 1) {
        temp >>= 1; // 确定订单
        order++;
    }
    add_page(order, base); // 添加到链表
    merge_page(order, base); // 尝试合并
}

// 计算空闲页面数量
static size_t buddy_system_nr_free_pages(void) {
    size_t num = 0;
    for (int i = 0; i < MAX_ORDER; i++) {
        num += nr_free(i) << i; // 计算总数
    }
    return num;
}

// 基本检查
static void basic_check(void) {
    struct Page *p0, *p1, *p2;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);
    
    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    // 检查页面地址
    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);

    // 初始化空闲链表
    for (int i = 0; i < MAX_ORDER; i++) {
        list_init(&(free_list(i)));
        assert(list_empty(&(free_list(i))));
        nr_free(i) = 0; // 重置计数
    }

    assert(alloc_page() == NULL); // 无可用页面

    // 释放页面并检查
    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(buddy_system_nr_free_pages() == 3);

    // 重新分配页面
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);
    assert(alloc_page() == NULL); // 仍然没有可用页面

    // 确保再次分配的是第一个页面
    free_page(p0);
    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL); // 依旧无页面可用

    assert(buddy_system_nr_free_pages() == 0); // 应该没有空闲页面
    free_page(p);
    free_page(p1);
    free_page(p2);
}

// 检查系统状态的函数，未实现具体内容
static void buddy_system_check(void) {}

// 定义伙伴系统页面管理器
const struct pmm_manager buddy_system_pmm_manager = {
    .name = "default_pmm_manager", // 管理器名称
    .init = buddy_system_init, // 初始化函数
    .init_memmap = buddy_system_init_memmap, // 初始化内存映射
    .alloc_pages = buddy_system_alloc_pages, // 分配页面函数
    .free_pages = buddy_system_free_pages, // 释放页面函数
    .nr_free_pages = buddy_system_nr_free_pages, // 获取空闲页面数量
    .check = buddy_system_check, // 检查函数
};
