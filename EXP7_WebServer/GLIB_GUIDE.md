# GLib 数据结构快速参考

GLib已经包含在GTK4中，无需额外安装！

## 1. GString - 动态字符串

```c
// 创建
GString *str = g_string_new("Hello");
GString *str = g_string_sized_new(128);  // 预分配大小

// 追加
g_string_append(str, " World");
g_string_append_printf(str, " %d", 2024);
g_string_append_c(str, '!');

// 插入和删除
g_string_prepend(str, ">>> ");
g_string_insert(str, 5, "INSERT");
g_string_erase(str, 0, 3);  // 删除前3个字符

// 访问
printf("%s\n", str->str);   // str->len 是长度

// 释放
g_string_free(str, TRUE);   // TRUE=释放内容, FALSE=只释放结构体
```

## 2. GList / GSList - 链表

```c
// GList = 双向链表, GSList = 单向链表
GList *list = NULL;

// 添加元素
list = g_list_append(list, "data1");
list = g_list_prepend(list, "data0");  // O(1)
list = g_list_insert(list, "data2", 2);

// 遍历
for (GList *l = list; l != NULL; l = l->next) {
    printf("%s\n", (char*)l->data);
}

// 查找和长度
GList *found = g_list_find(list, "data1");
guint len = g_list_length(list);

// 删除
list = g_list_remove(list, "data1");

// 释放 (只释放链表结构，不释放数据)
g_list_free(list);
```

## 3. GPtrArray - 动态指针数组

```c
GPtrArray *array = g_ptr_array_new();

// 添加
g_ptr_array_add(array, "item1");
g_ptr_array_add(array, "item2");

// 索引访问 (O(1))
for (gsize i = 0; i < array->len; i++) {
    printf("%s\n", (char*)array->pdata[i]);
}

// 删除
g_ptr_array_remove_index(array, 0);

// 释放
g_ptr_array_free(array, TRUE);  // TRUE=同时释放数据
```

## 4. GArray - 类型安全的动态数组

```c
GArray *array = g_array_new(FALSE, FALSE, sizeof(int));

// 添加
int val = 42;
g_array_append_val(array, val);

// 访问
for (gsize i = 0; i < array->len; i++) {
    printf("%d\n", g_array_index(array, int, i));
}

// 设置大小
g_array_set_size(array, 100);

// 释放
g_array_free(array, TRUE);
```

## 5. GHashTable - 哈希表

```c
GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);

// 插入
g_hash_table_insert(table, "key", "value");
g_hash_table_insert(table, g_strdup("key2"), g_strdup("value2"));

// 查找
gpointer value = g_hash_table_lookup(table, "key");

// 遍历
GHashTableIter iter;
gpointer key, value;
g_hash_table_iter_init(&iter, table);
while (g_hash_table_iter_next(&iter, &key, &value)) {
    printf("%s: %s\n", (char*)key, (char*)value);
}

// 删除
g_hash_table_remove(table, "key");

// 释放
g_hash_table_destroy(table);
```

## 6. GQueue - 队列

```c
GQueue *queue = g_queue_new();

// 队列操作
g_queue_push_tail(queue, "item1");
g_queue_push_head(queue, "item0");

gpointer item = g_queue_pop_head(queue);
gpointer peek = g_queue_peek_head(queue);

// 栈操作
g_queue_push_head(queue, "top");
item = g_queue_pop_head(queue);

// 释放
g_queue_free(queue);
```

## 常用工具函数

```c
// 内存管理 (类似 malloc/free)
g_malloc(size)
g_malloc0(size)      // 分配并清零
g_realloc(ptr, size)
g_free(ptr)

// 字符串处理
g_strdup("string")           // 复制字符串
g_strndup("string", len)     // 复制前n个字符
g_strconcat(s1, s2, ..., NULL)  // 连接字符串
g_strsplit("a,b,c", ",", 0)  // 分割字符串，返回 gchar**

// 比较
g_strcmp0(s1, s2)            // 安全的字符串比较 (处理NULL)

// 实用宏
MAX(a, b)  / MIN(a, b)
CLAMP(x, low, high)  // 限制范围
```

## 内存管理提示

```c
// 使用 g_autoptr 自动释放 (C23 配合 GCC/Clang)
g_autoptr(GString) str = g_string_new("auto");
// 函数结束时自动释放

// 或使用 cleanup 属性
void cleanup_string(GString **s) {
    if (*s) g_string_free(*s, TRUE);
}

void example(void) {
    GString *str __attribute__((cleanup(cleanup_string))) = g_string_new("auto");
    // 自动清理
}
```

## 更多资源

- 官方文档: https://docs.gtk.org/glib/
- 哈希和相等函数: `g_str_hash`, `g_int_hash`, `g_direct_hash`
- 销毁函数: `g_hash_table_new_full()` 可指定析构函数
