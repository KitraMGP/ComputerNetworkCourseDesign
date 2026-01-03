#include <gtk/gtk.h>
#include <stdio.h>

// GLib 字符串和容器使用示例
void demonstrate_glib_features() {
    // === GString: 动态字符串 ===
    GString *str = g_string_new("Hello");
    g_string_append(str, " ");
    g_string_append(str, "World");
    g_string_append_printf(str, " %d", 2024);

    printf("GString: %s\n", str->str);
    g_string_free(str, TRUE); // TRUE=释放内容

    // === GList: 双向链表 ===
    GList *list = NULL;
    list = g_list_append(list, "Item 1");
    list = g_list_append(list, "Item 2");
    list = g_list_append(list, "Item 3");

    printf("\nGList:\n");
    for (GList *l = list; l != NULL; l = l->next) {
        printf("  - %s\n", (char*)l->data);
    }

    g_list_free(list);

    // === GPtrArray: 动态指针数组 ===
    GPtrArray *array = g_ptr_array_new();
    g_ptr_array_add(array, "First");
    g_ptr_array_add(array, "Second");
    g_ptr_array_add(array, "Third");

    printf("\nGPtrArray:\n");
    for (gsize i = 0; i < array->len; i++) {
        printf("  [%zu] %s\n", i, (char*)array->pdata[i]);
    }

    g_ptr_array_free(array, TRUE);

    // === GHashTable: 哈希表 ===
    GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(table, "name", "GTK4");
    g_hash_table_insert(table, "version", "4.0");

    printf("\nGHashTable:\n");
    printf("  name: %s\n", (char*)g_hash_table_lookup(table, "name"));
    printf("  version: %s\n", (char*)g_hash_table_lookup(table, "version"));

    g_hash_table_destroy(table);
}

// 带有 GLib 的 GTK 应用示例
static void on_button_clicked(GtkButton *button, gpointer user_data) {
    GString *message = g_string_new("点击次数: ");
    static int count = 0;

    g_string_append_printf(message, "%d", ++count);
    gtk_label_set_text(GTK_LABEL(user_data), message->str);

    g_string_free(message, TRUE);
}

// 重置按钮的回调函数
static void on_reset_clicked(GtkButton *button, gpointer user_data) {
    GtkLabel *label = GTK_LABEL(user_data);
    gtk_label_set_text(label, "点击下面的按钮");
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "GLib + GTK4 示例");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    // 使用 GString 构建界面
    GString *title = g_string_new("欢迎使用 ");
    g_string_append(title, "GLib 和 GTK4");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_window_set_child(GTK_WINDOW(window), box);

    GtkWidget *title_label = gtk_label_new(title->str);
    gtk_widget_add_css_class(title_label, "title-1");
    gtk_box_append(GTK_BOX(box), title_label);

    g_string_free(title, TRUE);

    GtkWidget *counter_label = gtk_label_new("点击下面的按钮");
    gtk_box_append(GTK_BOX(box), counter_label);

    // 使用 GPtrArray 存储多个按钮
    GPtrArray *buttons = g_ptr_array_new();

    GtkWidget *button = gtk_button_new_with_label("点击我");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), counter_label);
    g_ptr_array_add(buttons, button);
    gtk_box_append(GTK_BOX(box), button);

    button = gtk_button_new_with_label("重置");
    g_signal_connect(button, "clicked", G_CALLBACK(on_reset_clicked), counter_label);
    g_ptr_array_add(buttons, button);
    gtk_box_append(GTK_BOX(box), button);

    gtk_window_present(GTK_WINDOW(window));

    // 清理
    g_ptr_array_free(buttons, TRUE);
}

int main(int argc, char **argv) {
    // 先演示 GLib 功能
    demonstrate_glib_features();

    // 然后运行 GTK 应用
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.glib.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
