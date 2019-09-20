#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib") 

void main(int argc, char* argv[])
{
    char serviceName[] = "drvhello";
    char driverBinary[MAX_PATH] = "";
    SC_HANDLE hSc;
    SC_HANDLE hService;

    // Чтобы запустить драйвер, потребуется полный путь к нему
    // Предполагаем, что он лежит в той же папке, что и экзешник
    strcpy(driverBinary, argv[0]);                  // argv[0] - здесь будет имя экзешника с полным путем
    PathRemoveFileSpec(driverBinary);               // Выкидываем имя экзешника, остается только путь к папке
    strcat(driverBinary, "\\drvhello.sys"); // Добавляем имя драйвера.
                                    // Бэкслэш в строках Си надо удваивать, из-за его специального значения.

    hSc = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS); // Открываем SCM (Service Control Management)
                                                            // Это такая штука, которая позволяет запускать драйверы
                                                            // из пользовательского режима
    CreateService(hSc, serviceName, serviceName,
            SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
            driverBinary, NULL, NULL, NULL, NULL, NULL);                            // Загрузка в 3 этапа - создаем службу
    hService = OpenService(hSc, serviceName, SERVICE_ALL_ACCESS);   // Открываем ее
    StartService(hService, 0, NULL);        // И запускаем. Вообще-то было бы неплохо
                                            // еще и закрыть ее... Как нибудь потом.
}
