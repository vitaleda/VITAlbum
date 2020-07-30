#include <codecvt>
#include <cstring>
#include <locale>
#include <psp2/ime_dialog.h>
#include <vitaGL.h>

#include "keyboard.h"
#include "log.h"
#include "utils.h"

namespace Keyboard {
    static bool running = false;
    static const int SCE_COMMON_DIALOG_STATUS_CANCELLED = 3;
    static uint16_t buffer[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
    std::string text = std::string();

    int Init(const std::string &title, const std::string &initial_text) {
        if (running)
            return -1;

        // UTF8 -> UTF16
        std::u16string title_u16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(title.data());
        std::u16string initial_text_u16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(initial_text.data());

        SceImeDialogParam param;
        sceImeDialogParamInit(&param);

        param.supportedLanguages = SCE_IME_LANGUAGE_ENGLISH;
        param.languagesForced = SCE_TRUE;
        param.type = SCE_IME_TYPE_NUMBER;
        param.option = 0;
            
        param.title = (const SceWChar16 *)title_u16.c_str();
        param.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
        param.initialText = (SceWChar16 *)initial_text_u16.c_str();
        param.inputTextBuffer = buffer;
        
        int ret = 0;
        if (R_SUCCEEDED(ret = sceImeDialogInit(&param)))
            running = true;
        
        return ret;
    }

    SceCommonDialogStatus Update(void) {
        if (!running)
            return SCE_COMMON_DIALOG_STATUS_NONE;
        
        SceCommonDialogStatus status = sceImeDialogGetStatus();
        if (status == SCE_COMMON_DIALOG_STATUS_FINISHED) {
            SceImeDialogResult result;
            std::memset(&result, 0, sizeof(SceImeDialogResult));
            sceImeDialogGetResult(&result);

            if ((result.button == SCE_IME_DIALOG_BUTTON_CLOSE) || (result.button == SCE_IME_DIALOG_BUTTON_ENTER)) {
                std::u16string buffer_u16 = (char16_t *)buffer;
                text = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(buffer_u16.data());
            }
            else
                status = (SceCommonDialogStatus)SCE_COMMON_DIALOG_STATUS_CANCELLED;
            
            sceImeDialogTerm();
            running = false;
        }

        return status;
    }

    std::string GetText(const std::string &title, const std::string &initial_text) {
        if (R_FAILED(Init(title, initial_text)))
            return std::string();
        
        bool done = false;

        do {
            glClear(GL_COLOR_BUFFER_BIT);
            glClearColor(0, 0, 0, 1);
            done = true;
            
            SceCommonDialogStatus ret = Update();
            if (ret != SCE_COMMON_DIALOG_STATUS_CANCELLED)
                done = false;
            
            vglStopRenderingInit();
            vglUpdateCommonDialog();
            vglStopRenderingTerm();
            vglStartRendering();
        } while(!done);

        return text;
    }
}
