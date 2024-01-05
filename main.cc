#include <nan.h>
#include <v8.h>
#include <queue>
#include <mutex>

using namespace std;
using namespace Nan;
using namespace v8;

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;

#define set_str_res(obj, key, val) (Nan::Set(obj, Nan::New(key).ToLocalChecked(), Nan::New(val).ToLocalChecked()))
#define set_bool_res(obj, key, val) (Nan::Set(obj, Nan::New(key).ToLocalChecked(), val ? Nan::True() : Nan::False()))
#define set_res(obj, code, msg)    \
	set_bool_res(obj, "code", code); \
	set_str_res(obj, "msg", msg);

template <typename T>
class ThreadSafeQueue
{
private:
	std::queue<T> queue;
	mutable std::mutex m;

public:
	ThreadSafeQueue() {}

	void push(T value)
	{
		std::lock_guard<std::mutex> lock(m);
		queue.push(value);
	}

	bool try_pop(T &value)
	{
		std::lock_guard<std::mutex> lock(m);
		if (queue.empty())
		{
			return false;
		}
		value = queue.front();
		queue.pop();
		return true;
	}
};

struct MouseEvent
{
	POINT pt;
	DWORD mouseData;
};
struct KeyboardEvent
{
	char *evName;
	DWORD code;
};

ThreadSafeQueue<MouseEvent> mouseEventQueue;
ThreadSafeQueue<KeyboardEvent> keyboardEventQueue;
struct ExecResult
{
	bool code;
	char *msg;
	HWND parentHandle;
};

struct NanAsyncData
{
	v8::Persistent<v8::Function> mouseCb;
	v8::Persistent<v8::Function> kbCb;
};
ExecResult DisableMinimize(HWND handle)
{
	ExecResult res;
	res.parentHandle = GetParent(handle);

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
		res.msg = "\ncould not locate SHELLDLL_DefView in WorkerW\n";
		res.code = false;
		HWND hProgman = NULL;
		do
		{
			hProgman = FindWindowEx(desktop, hProgman, "Progman", NULL);
			hShellViewWin = FindWindowEx(hProgman, 0, "SHELLDLL_DefView", 0);
		} while (hShellViewWin == NULL && hProgman != NULL);
	}
	if (hShellViewWin == NULL)
	{
		res.msg = "\ncould not locate SHELLDLL_DefView in Progman\n";
		res.code = false;
	}
	else
	{
		SetWindowLongPtr(handle, -8, (LONG_PTR)hShellViewWin);
		res.msg = "ok";
		res.code = true;
	}
	return res;
}

ExecResult EnableMinimize(HWND target, HWND parent)
{
	ExecResult res;
	SetLastError(0);
	auto code = SetWindowLongPtr(target, -8, (LONG_PTR)parent);
	if (code == 0)
	{
		res.code = true;
		res.msg = "ok";
		res.parentHandle = NULL;
	}
	else
	{
		res.code = false;
		res.msg = "err";
		res.parentHandle = NULL;
	}
	return res;
}

Nan::Callback *nodeMouseMoveCb = NULL;
HHOOK hMouseHook = NULL;
HHOOK hKbHook = NULL;
uv_async_t async;
bool async_inited = false;
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		if (wParam == WM_MOUSEMOVE)
		{
			auto pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
			if (pMouseStruct != NULL)
			{
				MouseEvent e;
				e.pt = pMouseStruct->pt;
				e.mouseData = pMouseStruct->mouseData;
				mouseEventQueue.push(e);
			}

			uv_async_send(&async);
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		if (wParam == WM_KEYDOWN || wParam == WM_KEYUP)
		{
			KeyboardEvent e;
			switch (wParam)
			{
			case WM_KEYDOWN:
				e.evName = "keydown";
				break;
			case WM_KEYUP:
				e.evName = "keyup";
				break;
			default:
				break;
			}

			PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
			if (p != NULL)
			{
				e.code = p->vkCode;
				keyboardEventQueue.push(e);
			}

			uv_async_send(&async);
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void AsyncCb(uv_async_t *handle)
{
	Isolate *isolate = Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	MouseEvent e;
	while (mouseEventQueue.try_pop(e))
	{
		v8::Local<v8::Object> eventObj = v8::Object::New(isolate);
		Nan::Set(eventObj, Nan::New("x").ToLocalChecked(), Nan::New(e.pt.x));
		Nan::Set(eventObj, Nan::New("y").ToLocalChecked(), Nan::New(e.pt.y));

		NanAsyncData *data = static_cast<NanAsyncData *>(handle->data);
		auto cb = Nan::New(data->mouseCb);
		Local<Value> argv[1] = {eventObj};
		auto context = isolate->GetCurrentContext();
		cb->Call(context, context->Global(), 1, argv);
	}

	KeyboardEvent e2;
	while (keyboardEventQueue.try_pop(e2))
	{
		v8::Local<v8::Object> eventObj = v8::Object::New(isolate);
		set_str_res(eventObj, "evName", e2.evName);
		Nan::Set(eventObj, Nan::New("code").ToLocalChecked(), Nan::New(UINT32(e2.code)));

		NanAsyncData *data = static_cast<NanAsyncData *>(handle->data);
		auto cb = Nan::New(data->kbCb);
		Local<Value> argv[1] = {eventObj};
		auto context = isolate->GetCurrentContext();
		cb->Call(context, context->Global(), 1, argv);
	}
}

NAN_METHOD(disableMinimize)
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
	auto res = DisableMinimize(hwnd);
	Nan::Set(obj, Nan::New("code").ToLocalChecked(), res.code ? Nan::True() : Nan::False());
	if (res.code)
	{
		Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New(res.parentHandle == NULL ? "could not locate parent window handle" : "ok").ToLocalChecked());
		Nan::Set(obj, Nan::New("handle").ToLocalChecked(), Nan::New<v8::Number>(reinterpret_cast<uintptr_t>(res.parentHandle)));
	}
	else
	{
		Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New(res.msg).ToLocalChecked());
	}
	info.GetReturnValue().Set(obj);
}

NAN_METHOD(enableMinimize)
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
	else
	{
		Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
		Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("Argument must be a HWND handle!\nPlease use \"BrowserWindow.getNativeWindowHandle();\"\nhttps://electronjs.org/docs/api/browser-window#wingetnativewindowhandle\n").ToLocalChecked());
		info.GetReturnValue().Set(obj);
		return;
	}

	if (!info[1]->IsNumber())
	{
		Nan::ThrowTypeError("\n\nparent should be window handle of type Integer\n");
		Nan::Set(obj, Nan::New("code").ToLocalChecked(), Nan::False());
		Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("parent should be window handle of type Integer").ToLocalChecked());
		info.GetReturnValue().Set(obj);
		return;
	}
	unsigned char *bufferData = (unsigned char *)node::Buffer::Data(target);
	unsigned long handle = *reinterpret_cast<unsigned long *>(bufferData);
	HWND targetHwnd = (HWND)handle;

	HWND parentHwnd;

	if (info[0]->Int32Value(Nan::GetCurrentContext()).FromJust() == 0)
	{
		parentHwnd = GetDesktopWindow();
	}
	else
	{
		uintptr_t hwndInt = static_cast<uintptr_t>(info[1]->NumberValue(Nan::GetCurrentContext()).FromJust());
		parentHwnd = reinterpret_cast<HWND>(hwndInt);
	}
	auto res = EnableMinimize(targetHwnd, parentHwnd);
	Nan::Set(obj, Nan::New("code").ToLocalChecked(), res.code ? Nan::True() : Nan::False());
	Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New(res.msg).ToLocalChecked());
	info.GetReturnValue().Set(obj);
}

NAN_METHOD(hookMouseMove)
{
	auto isolate = info.GetIsolate();
	v8::Local<v8::Object> obj = Nan::New<v8::Object>();
	if (hMouseHook != NULL)
	{
		set_res(obj, false, "already hooked");
		info.GetReturnValue().Set(obj);
		return;
	}
	if (info.Length() >= 1 && !info[0]->IsFunction())
	{
		set_res(obj, false, "param 0 should be type of function");
		info.GetReturnValue().Set(obj);
		return;
	}
	v8::Local<v8::Function> callbackHandle = info[0].As<v8::Function>();
	if (async.data == NULL)
	{
		NanAsyncData *asyncData = new NanAsyncData();
		asyncData->mouseCb.Reset(isolate, callbackHandle);
		async.data = asyncData;
	}
	else
	{
		auto data = static_cast<NanAsyncData *>(async.data);
		data->mouseCb.Reset(isolate, callbackHandle);
	}
	if (!async_inited)
	{
		uv_async_init(uv_default_loop(), &async, AsyncCb);
		async_inited = true;
	}
	hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
	if (hMouseHook == NULL)
	{
		set_res(obj, false, "failed to hook mouse event");
	}
	else
	{
		set_res(obj, true, "mouse event hooked");
	}
	info.GetReturnValue().Set(obj);
}

NAN_METHOD(unhookMouse)
{
	v8::Local<v8::Object> obj = Nan::New<v8::Object>();
	if (hMouseHook != NULL)
	{
		auto res = UnhookWindowsHookEx(hMouseHook);
		hMouseHook = NULL;

		auto asyncData = static_cast<NanAsyncData *>(async.data);
		asyncData->mouseCb.Reset();
		if (asyncData->kbCb.IsEmpty())
		{
			delete async.data;
		}

		if (res)
		{
			set_res(obj, true, "done");
		}
		else
		{
			set_res(obj, false, "failed to unhook");
		}
	}
	else
	{
		set_res(obj, false, "no hook in process");
	}
	info.GetReturnValue().Set(obj);
}

NAN_METHOD(hookKeyboard)
{
	auto isolate = info.GetIsolate();
	v8::Local<v8::Object> obj = Nan::New<v8::Object>();
	if (hKbHook != NULL)
	{
		set_res(obj, false, "already hooked");
		info.GetReturnValue().Set(obj);
		return;
	}
	if (info.Length() >= 1 && !info[0]->IsFunction())
	{
		set_res(obj, false, "param 0 should be type of function");
		info.GetReturnValue().Set(obj);
		return;
	}
	v8::Local<v8::Function> callbackHandle = info[0].As<v8::Function>();
	if (async.data == NULL)
	{
		NanAsyncData *asyncData = new NanAsyncData();
		asyncData->kbCb.Reset(isolate, callbackHandle);

		// 初始化和开始Libuv的异步工作 (uv_work_t)
		async.data = asyncData;
	}
	else
	{
		auto data = static_cast<NanAsyncData *>(async.data);
		data->kbCb.Reset(isolate, callbackHandle);
	}
	if (!async_inited)
	{
		uv_async_init(uv_default_loop(), &async, AsyncCb);
		async_inited = true;
	}
	hKbHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
	if (hKbHook == NULL)
	{
		set_res(obj, false, "failed to hook mouse event");
	}
	else
	{
		set_res(obj, true, "mouse event hooked");
	}
	info.GetReturnValue().Set(obj);
}

NAN_METHOD(unhookKeyboard)
{
	v8::Local<v8::Object> obj = Nan::New<v8::Object>();
	if (hKbHook != NULL)
	{
		auto res = UnhookWindowsHookEx(hKbHook);
		hKbHook = NULL;

		auto asyncData = static_cast<NanAsyncData *>(async.data);
		asyncData->kbCb.Reset();
		if (asyncData->mouseCb.IsEmpty())
		{
			delete async.data;
		}

		if (res)
		{
			set_res(obj, true, "done");
		}
		else
		{
			set_res(obj, false, "failed to unhook");
		}
	}
	else
	{
		set_res(obj, false, "no hook in process");
	}
	info.GetReturnValue().Set(obj);
}
NAN_MODULE_INIT(Initialize)
{
	NAN_EXPORT(target, disableMinimize);
	NAN_EXPORT(target, enableMinimize);
	NAN_EXPORT(target, hookMouseMove);
	NAN_EXPORT(target, unhookMouse);
	NAN_EXPORT(target, hookKeyboard);
	NAN_EXPORT(target, unhookKeyboard);
}

NODE_MODULE(addon, Initialize);