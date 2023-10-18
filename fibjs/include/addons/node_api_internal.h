#pragma once

#include "object.h"
#include "Buffer.h"
#include "node_version.h"

#ifdef __GNUC__
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define LIKELY(expr) expr
#define UNLIKELY(expr) expr
#define PRETTY_FUNCTION_NAME ""
#endif

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define ERROR_AND_ABORT(expr)                                                     \
    do {                                                                          \
        /* Make sure that this struct does not end up in inline code, but      */ \
        /* rather in a read-only data section when modifying this code.        */ \
        static const node::AssertionInfo args = {                                 \
            __FILE__ ":" STRINGIFY(__LINE__), #expr, PRETTY_FUNCTION_NAME         \
        };                                                                        \
        node::Assert(args);                                                       \
    } while (0)

#define CHECK(expr)                \
    do {                           \
        if (UNLIKELY(!(expr))) {   \
            ERROR_AND_ABORT(expr); \
        }                          \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_NULL(val) CHECK((val) == nullptr)
#define CHECK_NOT_NULL(val) CHECK((val) != nullptr)
#define CHECK_IMPLIES(a, b) CHECK(!(a) || (b))

#define DCHECK(expr)
#define DCHECK_EQ(a, b)
#define DCHECK_GE(a, b)
#define DCHECK_GT(a, b)
#define DCHECK_LE(a, b)
#define DCHECK_LT(a, b)
#define DCHECK_NE(a, b)
#define DCHECK_NULL(val)
#define DCHECK_NOT_NULL(val)
#define DCHECK_IMPLIES(a, b)

#ifndef NODE_STRINGIFY
#define NODE_STRINGIFY(n) NODE_STRINGIFY_HELPER(n)
#define NODE_STRINGIFY_HELPER(n) #n
#endif

namespace node {

typedef fibjs::Isolate Environment;

template <typename Fn>
inline int32_t OnScopeLeave(Fn&& fn)
{
    return 0;
}

struct AssertionInfo {
    const char* file_line; // filename:line
    const char* message;
    const char* function;
};

inline void Assert(const AssertionInfo& info)
{
    std::string name = "fibjs";

    fprintf(stderr,
        "%s: %s:%s%s Assertion `%s' failed.\n",
        name.c_str(),
        info.file_line,
        info.function,
        *info.function ? ":" : "",
        info.message);
    fflush(stderr);

    abort();
}

namespace errors {
    inline void TriggerUncaughtException(v8::Isolate* isolate, v8::Local<v8::Value> error,
        v8::Local<v8::Message> message, bool from_promise = false)
    {
        CHECK(!error.IsEmpty());
        v8::HandleScope scope(isolate);

        if (message.IsEmpty())
            message = v8::Exception::CreateMessage(isolate, error);

        CHECK(isolate->InContext());
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        Environment* env = Environment::GetCurrent(context);
        if (env == nullptr) {
            // This means that the exception happens before Environment is assigned
            // to the context e.g. when there is a SyntaxError in a per-context
            // script - which usually indicates that there is a bug because no JS
            // error is supposed to be thrown at this point.
            // Since we don't have access to Environment here, there is not
            // much we can do, so we just print whatever is useful and crash.
            // PrintToStderrAndFlush(
            //     FormatCaughtException(isolate, context, error, message));
            abort();
        }
    }
}

inline void OnFatalError(const char* location, const char* message)
{
    abort();
}

template <typename T, size_t kStackStorageSize = 1024>
class MaybeStackBuffer {
public:
    const T* out() const
    {
        return buf_;
    }

    T* out()
    {
        return buf_;
    }

    // operator* for compatibility with `v8::String::(Utf8)Value`
    T* operator*()
    {
        return buf_;
    }

    const T* operator*() const
    {
        return buf_;
    }

    T& operator[](size_t index)
    {
        CHECK_LT(index, length());
        return buf_[index];
    }

    const T& operator[](size_t index) const
    {
        CHECK_LT(index, length());
        return buf_[index];
    }

    size_t length() const
    {
        return length_;
    }

    // Current maximum capacity of the buffer with which SetLength() can be used
    // without first calling AllocateSufficientStorage().
    size_t capacity() const
    {
        return capacity_;
    }

    // Make sure enough space for `storage` entries is available.
    // This method can be called multiple times throughout the lifetime of the
    // buffer, but once this has been called Invalidate() cannot be used.
    // Content of the buffer in the range [0, length()) is preserved.
    void AllocateSufficientStorage(size_t storage);

    void SetLength(size_t length)
    {
        // capacity() returns how much memory is actually available.
        CHECK_LE(length, capacity());
        length_ = length;
    }

    void SetLengthAndZeroTerminate(size_t length)
    {
        // capacity() returns how much memory is actually available.
        CHECK_LE(length + 1, capacity());
        SetLength(length);

        // T() is 0 for integer types, nullptr for pointers, etc.
        buf_[length] = T();
    }

    // Make dereferencing this object return nullptr.
    // This method can be called multiple times throughout the lifetime of the
    // buffer, but once this has been called AllocateSufficientStorage() cannot
    // be used.
    void Invalidate()
    {
        CHECK(!IsAllocated());
        capacity_ = 0;
        length_ = 0;
        buf_ = nullptr;
    }

    // If the buffer is stored in the heap rather than on the stack.
    bool IsAllocated() const
    {
        return !IsInvalidated() && buf_ != buf_st_;
    }

    // If Invalidate() has been called.
    bool IsInvalidated() const
    {
        return buf_ == nullptr;
    }

    // Release ownership of the malloc'd buffer.
    // Note: This does not free the buffer.
    void Release()
    {
        CHECK(IsAllocated());
        buf_ = buf_st_;
        length_ = 0;
        capacity_ = arraysize(buf_st_);
    }

    MaybeStackBuffer()
        : length_(0)
        , capacity_(arraysize(buf_st_))
        , buf_(buf_st_)
    {
        // Default to a zero-length, null-terminated buffer.
        buf_[0] = T();
    }

    explicit MaybeStackBuffer(size_t storage)
        : MaybeStackBuffer()
    {
        AllocateSufficientStorage(storage);
    }

    ~MaybeStackBuffer()
    {
        if (IsAllocated())
            free(buf_);
    }

    inline std::basic_string<T> ToString() const { return { out(), length() }; }
    inline std::basic_string_view<T> ToStringView() const
    {
        return { out(), length() };
    }

private:
    size_t length_;
    // capacity of the malloc'ed buf_
    size_t capacity_;
    T* buf_;
    T buf_st_[kStackStorageSize];
};

class Utf8Value : public MaybeStackBuffer<char> {
public:
    explicit Utf8Value(v8::Isolate* isolate, v8::Local<v8::Value> value);

    inline std::string ToString() const { return std::string(out(), length()); }
    inline std::string_view ToStringView() const
    {
        return std::string_view(out(), length());
    }

    inline bool operator==(const char* a) const { return strcmp(out(), a) == 0; }
    inline bool operator!=(const char* a) const { return !(*this == a); }
};

namespace Buffer {

    typedef void (*FreeCallback)(char* data, void* hint);

    inline v8::MaybeLocal<v8::Object> New(v8::Isolate* isolate, char* data,
        size_t length, FreeCallback callback, void* hint)
    {
        v8::EscapableHandleScope handle_scope(isolate);
        Environment* env = Environment::GetCurrent(isolate);

        fibjs::obj_ptr<fibjs::Buffer> buf = new fibjs::Buffer(data, length);
        callback(data, hint);
        return handle_scope.Escape(buf->wrap(env));
    }

    inline v8::MaybeLocal<v8::Object> New(v8::Isolate* isolate, size_t length)
    {
        v8::EscapableHandleScope handle_scope(isolate);
        Environment* env = Environment::GetCurrent(isolate);

        fibjs::obj_ptr<fibjs::Buffer> buf = new fibjs::Buffer(NULL, length);
        return handle_scope.Escape(buf->wrap(env));
    }

    inline bool HasInstance(v8::Local<v8::Value> val)
    {
        return val->IsArrayBufferView();
    }

    inline bool HasInstance(v8::Local<v8::Object> obj)
    {
        return obj->IsArrayBufferView();
    }

    inline char* Data(v8::Local<v8::Value> val)
    {
        CHECK(val->IsArrayBufferView());
        v8::Local<v8::ArrayBufferView> ui = val.As<v8::ArrayBufferView>();
        return static_cast<char*>(ui->Buffer()->Data()) + ui->ByteOffset();
    }

    inline char* Data(v8::Local<v8::Object> obj)
    {
        return Data(obj.As<v8::Value>());
    }

    inline size_t Length(v8::Local<v8::Value> val)
    {
        CHECK(val->IsArrayBufferView());
        v8::Local<v8::ArrayBufferView> ui = val.As<v8::ArrayBufferView>();
        return ui->ByteLength();
    }

    inline size_t Length(v8::Local<v8::Object> obj)
    {
        CHECK(obj->IsArrayBufferView());
        v8::Local<v8::ArrayBufferView> ui = obj.As<v8::ArrayBufferView>();
        return ui->ByteLength();
    }

    inline v8::MaybeLocal<v8::Object> Copy(v8::Isolate* isolate, const char* data, size_t length)
    {
        v8::EscapableHandleScope handle_scope(isolate);
        Environment* env = Environment::GetCurrent(isolate);

        fibjs::obj_ptr<fibjs::Buffer> buf = new fibjs::Buffer(data, length);
        return handle_scope.Escape(buf->wrap(env));
    }
}

typedef void* AsyncCleanupHookHandle;

inline void* AddEnvironmentCleanupHook(AsyncCleanupHookHandle holder)
{
    return nullptr;
}

inline void* RemoveEnvironmentCleanupHook(AsyncCleanupHookHandle holder)
{
    return nullptr;
}

template <typename T>
inline void* AddEnvironmentCleanupHook(v8::Isolate* isolate, T fn, void* arg)
{
    return nullptr;
}

template <typename T>
inline void* RemoveEnvironmentCleanupHook(v8::Isolate* isolate, T fn, void* arg)
{
    return nullptr;
}

typedef void (*addon_register_func)(
    v8::Local<v8::Object> exports,
    v8::Local<v8::Value> module,
    void* priv);

typedef void (*addon_context_register_func)(
    v8::Local<v8::Object> exports,
    v8::Local<v8::Value> module,
    v8::Local<v8::Context> context,
    void* priv);

enum ModuleFlags {
    kLinked = 0x02
};

struct node_module {
    int nm_version;
    unsigned int nm_flags;
    void* nm_dso_handle;
    const char* nm_filename;
    node::addon_register_func nm_register_func;
    node::addon_context_register_func nm_context_register_func;
    const char* nm_modname;
    void* nm_priv;
    struct node_module* nm_link;
};

void node_module_register(void* mod);

}

void napi_module_register_by_symbol(v8::Local<v8::Object> exports, v8::Local<v8::Value> module,
    v8::Local<v8::Context> context, napi_addon_register_func init,
    int32_t module_api_version = NODE_API_DEFAULT_MODULE_API_VERSION);