#include <nan.h>
#include <v8.h>

using namespace std;
using namespace Nan;
using namespace v8;

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

NAN_METHOD(DisableMinimize)
{
    if (info.Length() >= 2)
    {
        return Nan::ThrowError("toggle-minimize: Invalid number of arguments. Should be 1");
    }

    v8::Local<v8::Object> bufferObj;
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();

    if (info[0]->IsArrayBufferView() && info[0]->IsObject() && info[0]->IsTypedArray() && info[0]->IsUint8Array())
    {
        bufferObj = info[0].As<v8::Object>();
    }
    else
    {
        Nan::ThrowTypeError("\n\nArgument must be a HWND handle!\nPlease use \"yourBrowserWindow.getNativeWindowHandle();\"\nhttps://electronjs.org/docs/api/browser-window#wingetnativewindowhandle\n");
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
        Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("\n\nArgument must be a HWND handle!\nPlease use \"yourBrowserWindow.getNativeWindowHandle();\"\nhttps://electronjs.org/docs/api/browser-window#wingetnativewindowhandle\n").ToLocalChecked());
        info.GetReturnValue().Set(obj);
        return;
    }
    unsigned char *bufferData = (unsigned char *)node::Buffer::Data(bufferObj);
    unsigned long handle = *reinterpret_cast<unsigned long *>(bufferData);
    HWND hwnd = (HWND)handle;
    auto originParentHwnd = GetParent(hwnd);

    HWND desktop = GetDesktopWindow();
    HWND hWorkerW = NULL;
    HWND hShellViewWin = NULL;
    do
    {
        hWorkerW = FindWindowEx(desktop, hWorkerW, "WorkerW", NULL);
        hShellViewWin = FindWindowEx(hWorkerW, 0, "SHELLDLL_DefView", 0);
    } while (hShellViewWin == NULL && hWorkerW != NULL);

    if (hShellViewWin == NULL)
    {
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
        Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("\ncould not locate SHELLDLL_DefView\n").ToLocalChecked());
    }
    else {
        SetWindowLongPtr(hwnd, -8, (LONG_PTR)hShellViewWin);
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::True());
        Nan::Set(obj, Nan::New("handle").ToLocalChecked(), Nan::New<v8::Number>(reinterpret_cast<uintptr_t>(originParentHwnd)));
    }

    info.GetReturnValue().Set(obj);
}

NAN_METHOD(EnableMinimize)
{
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    if (info.Length() > 2)
    {
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
        Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("toggle-minimize: Invalid number of arguments. Should be 2").ToLocalChecked());
        info.GetReturnValue().Set(obj);
        return;
    }
    v8::Local<v8::Object> target;

    if (info[0]->IsArrayBufferView() && info[0]->IsObject() && info[0]->IsTypedArray() && info[0]->IsUint8Array())
    {
        target = info[0].As<v8::Object>();
    }
    else {
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
        Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("Argument must be a HWND handle!\nPlease use \"BrowserWindow.getNativeWindowHandle();\"\nhttps://electronjs.org/docs/api/browser-window#wingetnativewindowhandle\n").ToLocalChecked());
        info.GetReturnValue().Set(obj);
        return;
    }

    if (!info[1]->IsNumber()) {
        Nan::ThrowTypeError("\n\nparent should be window handle of type Integer\n");
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
        Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("parent should be window handle of type Integer").ToLocalChecked());
        info.GetReturnValue().Set(obj);
        return;
    }
    unsigned char* bufferData = (unsigned char*)node::Buffer::Data(target);
    unsigned long handle = *reinterpret_cast<unsigned long*>(bufferData);
    HWND targetHwnd = (HWND)handle;

    uintptr_t hwndInt = static_cast<uintptr_t>(info[1]->NumberValue(Nan::GetCurrentContext()).FromJust());
    HWND parentHwnd = reinterpret_cast<HWND>(hwndInt);
    SetLastError(0);
    auto res = SetWindowLongPtr(targetHwnd, -8, (LONG_PTR)parentHwnd);
    if (res == 0) {
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
        Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("failed to set window ptr").ToLocalChecked());
    }
    else {
        Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::True());
    }
    info.GetReturnValue().Set(obj);
}

NAN_MODULE_INIT(Initialize)
{
    NAN_EXPORT(target, DisableMinimize);
    NAN_EXPORT(target, EnableMinimize);
}

NODE_MODULE(addon, Initialize);