#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment (linker,"-merge:.rdata=.text")
const CHAR szMessage[]="Hello, world!";

void CenterText(HDC hDC,int x,int y,LPCTSTR szMessage,int point)
{
    HFONT hFont=CreateFont(
            point * GetDeviceCaps(hDC, LOGPIXELSY) / 60,
            0, 370, 0, FW_BOLD, TRUE, FALSE, FALSE,
            RUSSIAN_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            PROOF_QUALITY, VARIABLE_PITCH, "Times New Roman");      // Создаем новый шрифт
    HGDIOBJ hOld = SelectObject(hDC, hFont);        // Сохраняем старый шрифт
    SetTextAlign(hDC, TA_CENTER | TA_BASELINE);     // Текст выровняем по центру
    SetBkMode(hDC, TRANSPARENT);    // Прозрачность, чтоб текст не оказался в квадратной рамке с цветом фона
    SetTextColor(hDC, RGB(0x40,0xAA,0));    // Цвет текста
    TextOut(hDC, x, y, szMessage, strlen(szMessage));       // Рисуем текст на поверхности
    SelectObject(hDC, hOld);        // Возвращаем старый шрифт
    DeleteObject(hFont);            // Удаляем наш кастомный шрифт
}

void WinMainCRTStartup()
{
    // HDC - это такая поверхность, по которой планируется рисовать
    // В данном случае мы будем рисовать по поверхности всех имеющихся
    // мониторов. А в общем случае она может быть создана и для принтера
    // и для других экзотических устройств
    HDC hDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    while(1)        // Повторять будем бесконечно
    {
        // Выводим текст
        CenterText(hDC, GetSystemMetrics(SM_CXSCREEN) / 2,
                GetSystemMetrics(SM_CYSCREEN) / 2, szMessage, 72);
        Sleep(100);     // Ждем 1/10 секунды, чтоб не забивать процессор
    }
    // Сюда выполнение не дойдет, задачу нужно завершить через список процессов
    ReleaseDC(NULL, hDC);
    ExitProcess(0);
}
