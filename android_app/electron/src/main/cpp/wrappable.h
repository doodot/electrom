#ifndef ANDROID_APP_WRAPPABLE_H
#define ANDROID_APP_WRAPPABLE_H

#include "object_template_builder.h"

namespace gin {

    class WrappableBase {
    protected:
        WrappableBase();

        virtual ~WrappableBase();

        virtual ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate *isolate);

        virtual const char *GetTypeName();

        v8::MaybeLocal<v8::Object> GetWrapperImpl(v8::Isolate *isolate);

    private:
        static void FirstWeakCallback(const v8::WeakCallbackInfo<WrappableBase> &data);

        static void SecondWeakCallback(const v8::WeakCallbackInfo<WrappableBase> &data);

        bool dead_ = false;
        v8::Global<v8::Object> wrapper_;
    };

    template<typename T>
    class Wrappable : public WrappableBase {
    public:
        v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate *isolate) {
            return GetWrapperImpl(isolate);
        }

    protected:
        Wrappable() = default;

        ~Wrappable() override = default;
    };

}


#endif //ANDROID_APP_WRAPPABLE_H
