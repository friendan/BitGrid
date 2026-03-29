#include <EzUI/EzUI.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 创建应用程序实例
    EzUI::Application app;
    
    // 创建主窗口
    auto window = EzUI::Window::Create(L"BitGrid", 800, 600);
    window->SetBackgroundColor(EzUI::Color::White);
    
    // 创建标题标签
    auto titleLabel = EzUI::Label::Create(L"Welcome to BitGrid");
    titleLabel->SetPosition(300, 50);
    titleLabel->SetSize(200, 30);
    titleLabel->SetFontSize(18);
    window->AddChild(titleLabel);
    
    // 创建按钮
    auto button = EzUI::Button::Create(L"Click Me");
    button->SetPosition(300, 150);
    button->SetSize(200, 50);
    button->OnClick([](EzUI::Control* sender) {
        EzUI::MessageBox::Show(L"Hello, BitGrid!");
    });
    window->AddChild(button);
    
    // 创建文本框
    auto textBox = EzUI::TextBox::Create();
    textBox->SetPosition(300, 250);
    textBox->SetSize(200, 30);
    window->AddChild(textBox);
    
    // 创建复选框
    auto checkBox = EzUI::CheckBox::Create(L"Enable Feature");
    checkBox->SetPosition(300, 300);
    checkBox->SetSize(150, 20);
    window->AddChild(checkBox);
    
    // 显示窗口
    window->Show();
    
    // 运行应用程序
    return app.Run();
}
