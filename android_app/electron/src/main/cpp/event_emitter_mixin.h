#ifndef ANDROID_APP_EVENT_EMITTER_MIXIN_H
#define ANDROID_APP_EVENT_EMITTER_MIXIN_H

#include "event_emitter.h"
#include "javascript_environment.h"
#include "event_emitter_caller.h"

template<typename T>
class EventEmitterMixin {
public:
    template<typename... Args>
    bool Emit(const char *name, Args &&... args) {
        v8::Isolate *isolate = JavascriptEnvironment::GetIsolate();
        v8::Locker locker(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Object> wrapper;
//        if (!static_cast<T *>(this)->GetWrapper(isolate).ToLocal(&wrapper))
//            return false;
//        gin_helper::EmitEvent(isolate, wrapper, name, std::forward<Args>(args)...);
        return true;
    }

protected:
    EventEmitterMixin() = default;

    v8::Local<v8::ObjectTemplate> GetObjectTemplate(v8::Isolate *isolate) {
        // TODO Add cache for constructor like gin::PerIsolateData
        v8::Local<v8::FunctionTemplate> constructor = v8::FunctionTemplate::New(isolate);
        constructor->SetClassName(v8::String::NewFromUtf8(isolate, static_cast<T *>(this)->GetTypeName()));
        constructor->Inherit(internal::GetEventEmitterTemplate(isolate));

        return v8::ObjectTemplate::New(isolate, constructor);
    }
};

#endif //ANDROID_APP_EVENT_EMITTER_MIXIN_H
