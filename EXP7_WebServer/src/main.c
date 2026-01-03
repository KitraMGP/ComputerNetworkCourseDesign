#include <gtk/gtk.h>

static void on_button_clicked(GtkButton *button, gpointer user_data) {
    GtkLabel *label = GTK_LABEL(user_data);

    // 使用 GString 构建动态字符串
    GString *msg = g_string_new("Hello, GTK4! 次数: ");
    static int click_count = 0;

    g_string_append_printf(msg, "%d", ++click_count);
    gtk_label_set_text(label, msg->str);

    // 释放 GString
    g_string_free(msg, TRUE);
}

static void activate(GtkApplication *app, gpointer user_data) {
    // 创建主窗口
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "GTK4 Simple Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    // 创建主容器
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_window_set_child(GTK_WINDOW(window), box);

    // 创建标题标签
    GtkWidget *title_label = gtk_label_new("欢迎使用 GTK4");
    gtk_widget_add_css_class(title_label, "title-1");
    gtk_box_append(GTK_BOX(box), title_label);

    // 创建消息标签
    GtkWidget *message_label = gtk_label_new("点击下面的按钮");
    gtk_box_append(GTK_BOX(box), message_label);

    // 创建按钮
    GtkWidget *button = gtk_button_new_with_label("点击我");
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), message_label);
    gtk_box_append(GTK_BOX(box), button);

    // 显示窗口
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
