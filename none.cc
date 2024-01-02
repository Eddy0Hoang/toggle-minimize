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
  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  Nan::Set(obj, Nan::New("done").ToLocalChecked(), Nan::False());
  Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("only work on Windows").ToLocalChecked());
  info.GetReturnValue().Set(obj);
}

NAN_METHOD(EnableMinimize)
{

  v8::Local<v8::Object> obj = Nan::New<v8::Object>();
  Nan::Set(obj, Nan::New("done").ToLocalChecked(), Nan::False());
  Nan::Set(obj, Nan::New("msg").ToLocalChecked(), Nan::New("only work on Windows").ToLocalChecked());
  info.GetReturnValue().Set(obj);
}

NAN_MODULE_INIT(Initialize)
{
  NAN_EXPORT(target, DisableMinimize);
  NAN_EXPORT(target, EnableMinimize);
}

NODE_MODULE(addon, Initialize);