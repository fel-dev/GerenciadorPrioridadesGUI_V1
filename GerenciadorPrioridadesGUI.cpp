#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK

#include <windows.h>
#include <tchar.h>
#include <string>
#include <tlhelp32.h>
#include <commctrl.h>
#include <sal.h> // Include SAL annotations header

#pragma comment(lib, "comctl32.lib")

// IDs para os controles da interface
#define ID_TEXTBOX  1001
#define ID_LOWBTN   1002
#define ID_HIGHBTN  1003
#define TOOLTIP_TEXT L"Digite um processo por linha (ex: chrome.exe)\nNão use vírgulas"

// Função para alterar a prioridade de um processo pelo nome
void setPriority(const wchar_t* processName, DWORD priorityClass) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    // Percorre todos os processos do sistema
    if (Process32FirstW(snapshot, &pe)) {
        do {
            // Compara o nome do processo
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pe.th32ProcessID);
                if (hProc) {
                    SetPriorityClass(hProc, priorityClass);
                    CloseHandle(hProc);
                }
            }
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
}

// Declaração do procedimento da janela principal
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    // Nome da classe da janela
    const wchar_t CLASS_NAME[] = L"PrioridadeJanela";

    // Estrutura de configuração da janela
    WNDCLASS wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    InitCommonControls(); // Necessário para usar ListView

    RegisterClass(&wc);

    // Cria a janela principal
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Ajustador de Prioridade",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Loop principal de mensagens do Windows
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// Função que trata as mensagens da janela principal
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Variáveis para armazenar os handles dos controles
    static HWND hTextBox, hBtnBaixa, hBtnAlta, hListView, hTooltip;

    switch (msg) {
    case WM_CREATE: {
        // Cria o campo de texto (multilinha)
        hTextBox = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
            20, 20, 340, 90,
            hwnd, (HMENU)ID_TEXTBOX, NULL, NULL);

        // Cria o tooltip corretamente associado ao hTextBox
        hTooltip = CreateWindowExW(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd, NULL, NULL, NULL);
        TOOLINFOW ti = { 0 };
        ti.cbSize = sizeof(TOOLINFOW);
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
        ti.hwnd = hTextBox; // Associar ao controle de texto
        ti.uId = (UINT_PTR)hTextBox; // ID é o HWND do controle
        ti.lpszText = (LPWSTR)TOOLTIP_TEXT;
        GetClientRect(hTextBox, &ti.rect);
        SendMessageW(hTooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);

        //SetFocus(hTextBox); // Removido para permitir que o tooltip apareça ao passar o mouse

        // Cria o botão de prioridade baixa
        hBtnBaixa = CreateWindowW(L"BUTTON", L"Prioridade Baixa",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 130, 150, 30,
            hwnd, (HMENU)ID_LOWBTN, NULL, NULL);

        // Cria o botão de prioridade alta
        hBtnAlta = CreateWindowW(L"BUTTON", L"Prioridade Alta",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            200, 130, 150, 30,
            hwnd, (HMENU)ID_HIGHBTN, NULL, NULL);

        // Cria a ListView para mostrar os resultados
        hListView = CreateWindowW(WC_LISTVIEW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
            20, 180, 340, 160,  // Posição e tamanho da tabela
            hwnd, NULL, NULL, NULL);

        // Adiciona as colunas na ListView
        {
            LVCOLUMNW col = { 0 };
            col.mask = LVCF_TEXT | LVCF_WIDTH;

            wchar_t col1[] = L"Processo";
            col.pszText = col1;
            col.cx = 120;
            ListView_InsertColumn(hListView, 0, &col);

            wchar_t col2[] = L"Prioridade";
            col.pszText = col2;
            col.cx = 100;
            ListView_InsertColumn(hListView, 1, &col);

            wchar_t col3[] = L"Status";
            col.pszText = col3;
            col.cx = 100;
            ListView_InsertColumn(hListView, 2, &col);
        }
        break;
    }

    case WM_COMMAND:
        // Se algum dos botões for pressionado
        if (LOWORD(wParam) == ID_LOWBTN || LOWORD(wParam) == ID_HIGHBTN) {
            wchar_t buffer[2048];
            GetWindowTextW(hTextBox, buffer, 2048);

            // Define a prioridade conforme o botão
            DWORD prioridade = (LOWORD(wParam) == ID_LOWBTN) ?
                BELOW_NORMAL_PRIORITY_CLASS : HIGH_PRIORITY_CLASS;

            // Divide o texto em linhas
            wchar_t* linha = wcstok(buffer, L"\n");

            while (linha) {
                // Remove espaços à esquerda
                while (*linha == L' ') ++linha;
                if (*linha) {
                    bool sucesso = false;
                    // Procura o processo pelo nome e tenta alterar a prioridade
                    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                    if (snapshot != INVALID_HANDLE_VALUE) {
                        PROCESSENTRY32W pe;
                        pe.dwSize = sizeof(pe);
                        if (Process32FirstW(snapshot, &pe)) {
                            do {
                                if (_wcsicmp(pe.szExeFile, linha) == 0) {
                                    HANDLE hProc = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pe.th32ProcessID);
                                    if (hProc) {
                                        if (SetPriorityClass(hProc, prioridade)) {
                                            sucesso = true;
                                        }
                                        CloseHandle(hProc);
                                    }
                                }
                            } while (Process32NextW(snapshot, &pe));
                        }
                        CloseHandle(snapshot);
                    }

                    // Adiciona o resultado na ListView
                    LVITEMW item = { 0 };
                    item.mask = LVIF_TEXT;
                    item.iItem = ListView_GetItemCount(hListView);

                    item.pszText = linha;
                    ListView_InsertItem(hListView, &item);

                    ListView_SetItemText(hListView, item.iItem, 1,
                        (prioridade == HIGH_PRIORITY_CLASS) ? (LPWSTR)L"Alta" : (LPWSTR)L"Baixa");

                    ListView_SetItemText(hListView, item.iItem, 2,
                        sucesso ? (LPWSTR)L"✅ Sucesso" : (LPWSTR)L"❌ Falhou");
                }
                linha = wcstok(nullptr, L"\n");
            }

            MessageBoxW(hwnd, L"Prioridades aplicadas!", L"Concluído", MB_OK);
        }
        break;

    case WM_DESTROY:
        // Encerra o programa
        PostQuitMessage(0);
        break;
    }
    // Chama o procedimento padrão para mensagens não tratadas
    return DefWindowProc(hwnd, msg, wParam, lParam);
}