#include "libnode/include/node/node.h"
#include "libnode/include/node/uv.h"
#include <jni.h>
#include <string>

#include "log.h"
#include "helper.h"

#include <android/log.h>

#include "electron_api_app.h"

using node::ArrayBufferAllocator;
using node::Environment;
using node::IsolateData;
using node::MultiIsolatePlatform;
using namespace v8;

const char *LOADER =
        "const electron = globalThis.electron;"
        "console.log(electron);"
        "delete globalThis.electron;" // Environment를 현재는 접근할 수 없어서 이 방식을 선택한다.
        "const publicRequire = require('module').createRequire(process.cwd() + '/');"
        "electron.app = electron.initApp(publicRequire('events').EventEmitter.prototype);"
        "const electronRequire = function(modulePath) {"
        "  if (modulePath === 'electron') { " // intercept electron module require
        "    return electron;"
        "  } else {"
        "    return publicRequire;"
        "  }"
        "};"
        "console.log(electron);"
        "globalThis.require = electronRequire;"
        "const mainCode = require('fs').readFileSync(process.argv[1]);"
        "require('vm').runInThisContext(mainCode);"
        "electron.app.emit('ready');"
        "console.log('after ready');";

struct AndroidContext {
    JNIEnv *env;
    jobject obj;

    void StartRendererProcess(const char *propertiesJson) const {
        jstring jstr = env->NewStringUTF(propertiesJson);

        jclass cls = env->GetObjectClass(obj);
        jmethodID mid = env->GetMethodID(cls, "startRendererProcess", "(Ljava/lang/String;)V");
        env->CallVoidMethod(obj, mid, jstr);
    }
};

struct AndroidContext *androidEnv;

void Electron_BrowserWindowClass(const v8::FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    Local<Object> properties = args[0]->ToObject(isolate);
    androidEnv->StartRendererProcess(stringify(isolate, properties).c_str());
}

void Electron_BrowserWindowClass_loadURL(const FunctionCallbackInfo<Value> &args) {
    Isolate *isolate = args.GetIsolate();
    String::Utf8Value i(isolate, args[0]->ToString(isolate));
    log(ANDROID_LOG_INFO, std::string(*i).c_str());
}

void SetElectronModule(Isolate *isolate, Local<Object> module) {
    Local<Context> context = isolate->GetCurrentContext();

    RegisterApp(isolate, module);

    v8::Local<v8::FunctionTemplate> BrowserWindowClass = v8::FunctionTemplate::New(isolate, &Electron_BrowserWindowClass);
    NODE_SET_PROTOTYPE_METHOD(BrowserWindowClass, "loadURL", &Electron_BrowserWindowClass_loadURL);

    v8::Local<v8::Function> fn = BrowserWindowClass->GetFunction(context).ToLocalChecked();
    v8::Local<v8::String> fn_name = v8::String::NewFromUtf8(isolate, "BrowserWindow", v8::NewStringType::kInternalized).ToLocalChecked();
    fn->SetName(fn_name);
    module->Set(context, fn_name, fn).Check();
}

jint RunNodeInstance(MultiIsolatePlatform *platform,
                     const std::vector<std::string> &args,
                     const std::vector<std::string> &exec_args) {
    int exit_code = 0;
    uv_loop_t loop;
    int ret = uv_loop_init(&loop);
    if (ret != 0) {
        log(ANDROID_LOG_ERROR, "Failed to initialize uv_loop");
        return 1;
    }

    std::shared_ptr<ArrayBufferAllocator> allocator = ArrayBufferAllocator::Create();

    Isolate *isolate = NewIsolate(allocator.get(), &loop, platform);

    if (isolate == nullptr) {
        log(ANDROID_LOG_ERROR, "Failed to initialize V8 Isolate");
        return 1;
    }

    {
        Locker locker(isolate);
        Isolate::Scope isolate_scope(isolate);

        std::unique_ptr<IsolateData, decltype(&node::FreeIsolateData)> isolate_data(
                node::CreateIsolateData(isolate, &loop, platform, allocator.get()),
                node::FreeIsolateData);

        HandleScope handle_scope(isolate);
        Local<Context> context = node::NewContext(isolate);

        if (context.IsEmpty()) {
            log(ANDROID_LOG_ERROR, "Failed to initialize V8 Context");
            return 1;
        }

        Context::Scope context_scope(context);
        std::unique_ptr<Environment, decltype(&node::FreeEnvironment)> env(
                node::CreateEnvironment(isolate_data.get(), context, args, exec_args),
                node::FreeEnvironment);

        Local<v8::Object> electronObj = v8::Object::New(isolate);

        SetElectronModule(isolate, electronObj);

        context->Global()->Set(String::NewFromUtf8(isolate, "electron"), electronObj);
        context->Global()->Set(String::NewFromUtf8(isolate, "__dirname"),
                               String::NewFromUtf8(isolate, "__android"));

        MaybeLocal<Value> loadenv_ret = node::LoadEnvironment(env.get(), LOADER);

        if (loadenv_ret.IsEmpty())  // There has been a JS exception.
            return 1;

        {
            SealHandleScope seal(isolate);
            bool more;
            do {
                uv_run(&loop, UV_RUN_DEFAULT);

                platform->DrainTasks(isolate);
                more = uv_loop_alive(&loop);
                if (more) continue;

                node::EmitBeforeExit(env.get());
                more = uv_loop_alive(&loop);
            } while (more == true);
        }

        exit_code = node::EmitExit(env.get());

        node::Stop(env.get());
    }

    bool platform_finished = false;
    platform->AddIsolateFinishedCallback(isolate, [](void *data) {
        *static_cast<bool *>(data) = true;
    }, &platform_finished);
    platform->UnregisterIsolate(isolate);
    isolate->Dispose();

    // Wait until the platform has cleaned up all relevant resources.
    while (!platform_finished)
        uv_run(&loop, UV_RUN_ONCE);
    int err = uv_loop_close(&loop);
    assert(err == 0);

    return exit_code;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_electrom_process_MainProcess_startMainModule(
        JNIEnv *env,
        jobject obj,
        jobjectArray arguments) {
    jsize argc = env->GetArrayLength(arguments);

    //Compute byte size need for all arguments in contiguous memory.
    int c_arguments_size = 0;
    for (int i = 0; i < argc; i++) {
        c_arguments_size += strlen(
                env->GetStringUTFChars((jstring) env->GetObjectArrayElement(arguments, i), 0));
        c_arguments_size++; // for '\0'
    }

    //Stores arguments in contiguous memory.
    char *args_buffer = (char *) calloc(c_arguments_size, sizeof(char));

    //argv to pass into node.
    char **argv = new char *[argc];

    //To iterate through the expected start position of each argument in args_buffer.
    char *current_args_position = args_buffer;

    //Populate the args_buffer and argv.
    for (int i = 0; i < argc; i++) {
        const char *current_argument = env->GetStringUTFChars(
                (jstring) env->GetObjectArrayElement(arguments, i), 0);

        //Copy current argument to its expected position in args_buffer
        strncpy(current_args_position, current_argument, strlen(current_argument));

        //Save current argument start position in argv
        argv[i] = current_args_position;

        //Increment to the next argument's expected position.
        current_args_position += strlen(current_args_position) + 1;
    }

    if (start_redirecting_stdout_stderr() == -1) {
        log(ANDROID_LOG_ERROR, "Couldn't start redirecting stdout and stderr to logcat.");
    }

    androidEnv = new AndroidContext{env, obj};

    argv = uv_setup_args(argc, argv);
    std::vector<std::string> args(argv, argv + argc);
    std::vector<std::string> exec_args;
    std::vector<std::string> errors;

    int exit_code = node::InitializeNodeWithArgs(&args, &exec_args, &errors);
    if (exit_code != 0) {
        log(ANDROID_LOG_ERROR, "Failed to initialize node");
        return exit_code;
    }

    std::unique_ptr<MultiIsolatePlatform> platform = MultiIsolatePlatform::Create(2);
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    int ret = RunNodeInstance(platform.get(), args, exec_args);

    V8::Dispose();
    V8::ShutdownPlatform();

    return ret;
}


extern "C" JNIEXPORT jint JNICALL
Java_com_electrom_ElectronApp_emit(
        JNIEnv *env,
        jobject obj,
        jobjectArray arguments) {

    get()->Emit("ready", 0, nullptr);
    return 0;
}