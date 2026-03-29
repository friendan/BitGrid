#include <Windows.h>
#include <Application.h>
#include <Window.h>
#include <VLayout.h>
#include <Label.h>
#include <Button.h>
#include <TextBox.h>
#include <CheckBox.h>

using namespace ezui;

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    Application app;//消息循环对象
    app.EnableHighDpi();//启用高DPI适配
    
    Window window(800, 600);//创建窗口
    window.SetText(L"BitGrid");//设置窗口标题
    
    VLayout mainLayout;//窗口中的主布局
    mainLayout.Style.BackColor = Color::White;//主布局背景颜色
    
    // 创建标题标签
    Label titleLabel;
    titleLabel.SetParent(&mainLayout);
    titleLabel.SetText(L"Welcome to BitGrid");
    titleLabel.SetFixedSize(Size(200, 30));
    titleLabel.Style.FontSize = 18;
    
    // 创建按钮
    Button button;
    button.SetParent(&mainLayout);
    button.SetText(L"Click Me");
    button.SetFixedSize(Size(200, 50));
    button.EventHandler = [](Control* sd, const EventArgs& arg)->void {
        if (arg.EventType == Event::OnMouseDown) {
            MessageBoxW(NULL, L"Hello, BitGrid!", L"Message", MB_OK);
        }
    };
    
    // 创建文本框
    TextBox textBox;
    textBox.SetParent(&mainLayout);
    textBox.SetFixedSize(Size(200, 30));
    textBox.Style.Border.Color = Color::Gray;
    textBox.Style.Border = 1;
    
    // 创建复选框
    CheckBox checkBox;
    checkBox.SetParent(&mainLayout);
    checkBox.SetText(L"Enable Feature");
    checkBox.SetFixedSize(Size(150, 20));
    
    // 给窗口设置布局
    window.SetLayout(&mainLayout);
    
    // 显示窗口
    window.Show();
    
    // 进行消息循环
    return app.Exec();
}
