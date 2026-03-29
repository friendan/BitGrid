#include <Application.h>
#include <Window.h>
#include <Label.h>
#include <Button.h>
#include <TextBox.h>
#include <CheckBox.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 创建应用程序实例
    ezui::Application app;
    
    // 创建主窗口
    ezui::Window window(800, 600);
    window.SetText(L"BitGrid");
    
    // 创建标题标签
    ezui::Label titleLabel(&window);
    titleLabel.SetText(L"Welcome to BitGrid");
    titleLabel.SetLocation(ezui::Point(300, 50));
    titleLabel.SetSize(ezui::Size(200, 30));
    
    // 创建按钮
    ezui::Button button(&window);
    button.SetText(L"Click Me");
    button.SetLocation(ezui::Point(300, 150));
    button.SetSize(ezui::Size(200, 50));
    
    // 创建文本框
    ezui::TextBox textBox(&window);
    textBox.SetLocation(ezui::Point(300, 250));
    textBox.SetSize(ezui::Size(200, 30));
    
    // 创建复选框
    ezui::CheckBox checkBox(&window);
    checkBox.SetText(L"Enable Feature");
    checkBox.SetLocation(ezui::Point(300, 300));
    checkBox.SetSize(ezui::Size(150, 20));
    
    // 显示窗口
    window.Show();
    
    // 运行应用程序
    return app.Exec();
}
