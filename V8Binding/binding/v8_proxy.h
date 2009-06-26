// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_PROXY_H__
#define V8_PROXY_H__

#include <v8.h>
#include "v8_index.h"
#include "V8CustomBinding.h"
#include "V8Utilities.h"
#include "Node.h"
#include "NodeFilter.h"
#include "PlatformString.h"  // for WebCore::String
#include "ScriptSourceCode.h"  // for WebCore::ScriptSourceCode
#include "SecurityOrigin.h"  // for WebCore::SecurityOrigin
#include "V8DOMMap.h"
#include "V8EventListenerList.h"
#include <wtf/Assertions.h>
#include <wtf/PassRefPtr.h> // so generated bindings don't have to
#include <wtf/Vector.h>

#if defined(ENABLE_DOM_STATS_COUNTERS) && PLATFORM(CHROMIUM)
#include "ChromiumBridge.h"
#define INC_STATS(name) ChromiumBridge::incrementStatsCounter(name)
#else
#define INC_STATS(name)
#endif

// FIXME: Remove the following hack when we replace all references to GetDOMObjectMap.
#define GetDOMObjectMap getDOMObjectMap

namespace WebCore {

class CSSStyleDeclaration;
class ClientRectList;
class DOMImplementation;
class Element;
class Event;
class EventListener;
class Frame;
class HTMLCollection;
class HTMLOptionsCollection;
class HTMLElement;
class HTMLDocument;
class MediaList;
class NamedNodeMap;
class Node;
class NodeList;
class Screen;
class String;
class StyleSheet;
class SVGElement;
class DOMWindow;
class Document;
class EventTarget;
class Event;
class EventListener;
class Navigator;
class MimeType;
class MimeTypeArray;
class Plugin;
class PluginArray;
class StyleSheetList;
class CSSValue;
class CSSRule;
class CSSRuleList;
class CSSValueList;
class NodeFilter;
class ScriptExecutionContext;

#if ENABLE(DOM_STORAGE)
class Storage;
class StorageEvent;
#endif

#if ENABLE(SVG)
class SVGElementInstance;
#endif

class V8EventListener;
class V8ObjectEventListener;


// TODO(fqian): use standard logging facilities in WebCore.
void log_info(Frame* frame, const String& msg, const String& url);


#ifndef NDEBUG

#define GlobalHandleTypeList(V) \
  V(PROXY)                      \
  V(NPOBJECT)                   \
  V(SCHEDULED_ACTION)           \
  V(EVENT_LISTENER)             \
  V(NODE_FILTER)                \
  V(SCRIPTINSTANCE)             \
  V(SCRIPTVALUE)


// Host information of persistent handles.
enum GlobalHandleType {
#define ENUM(name) name,
  GlobalHandleTypeList(ENUM)
#undef ENUM
};


class GlobalHandleInfo {
 public:
  GlobalHandleInfo(void* host, GlobalHandleType type)
      : host_(host), type_(type) { }
  void* host_;
  GlobalHandleType type_;
};

#endif  // NDEBUG

// The following Batch structs and methods are used for setting multiple
// properties on an ObjectTemplate, used from the generated bindings
// initialization (ConfigureXXXTemplate).  This greatly reduces the binary
// size by moving from code driven setup to data table driven setup.

// BatchedAttribute translates into calls to SetAccessor() on either the
// instance or the prototype ObjectTemplate, based on |on_proto|.
struct BatchedAttribute {
  const char* const name;
  v8::AccessorGetter getter;
  v8::AccessorSetter setter;
  V8ClassIndex::V8WrapperType data;
  v8::AccessControl settings;
  v8::PropertyAttribute attribute;
  bool on_proto;
};

void BatchConfigureAttributes(v8::Handle<v8::ObjectTemplate> inst,
                              v8::Handle<v8::ObjectTemplate> proto,
                              const BatchedAttribute* attrs,
                              size_t num_attrs);

// BatchedConstant translates into calls to Set() for setting up an object's
// constants.  It sets the constant on both the FunctionTemplate |desc| and the
// ObjectTemplate |proto|.  PropertyAttributes is always ReadOnly.
struct BatchedConstant {
  const char* const name;
  int value;
};

void BatchConfigureConstants(v8::Handle<v8::FunctionTemplate> desc,
                             v8::Handle<v8::ObjectTemplate> proto,
                             const BatchedConstant* consts,
                             size_t num_consts);

const int kMaxRecursionDepth = 20;

// Information about an extension that is registered for use with V8. If scheme
// is non-empty, it contains the URL scheme the extension should be used with.
// Otherwise, the extension is used with all schemes.
struct V8ExtensionInfo {
  String scheme;
  v8::Extension* extension;
};
typedef WTF::Vector<V8ExtensionInfo> V8ExtensionList;

class V8Proxy {
 public:
  // The types of javascript errors that can be thrown.
  enum ErrorType {
    RANGE_ERROR,
    REFERENCE_ERROR,
    SYNTAX_ERROR,
    TYPE_ERROR,
    GENERAL_ERROR
  };

  explicit V8Proxy(Frame* frame)
      : m_frame(frame), m_inlineCode(false),
        m_timerCallback(false), m_recursion(0) { }

  ~V8Proxy();

  Frame* frame() { return m_frame; }

  // Clear page-specific data, but keep the global object identify.
  void clearForNavigation();

  // Clear page-specific data before shutting down the proxy object.
  void clearForClose();

  // Update document object of the frame.
  void updateDocument();

  // Update the security origin of a document
  // (e.g., after setting docoument.domain).
  void updateSecurityOrigin();

  // Destroy the global object.
  void DestroyGlobal();

  // TODO(mpcomplete): Need comment.  User Gesture related.
  bool inlineCode() const { return m_inlineCode; }
  void setInlineCode(bool value) { m_inlineCode = value; }

  bool timerCallback() const { return m_timerCallback; }
  void setTimerCallback(bool value) { m_timerCallback = value; }

  // Has the context for this proxy been initialized?
  bool ContextInitialized();

  // Disconnects the proxy from its owner frame,
  // and clears all timeouts on the DOM window.
  void disconnectFrame();

  bool isEnabled();

  // Find/Create/Remove event listener wrappers.
  PassRefPtr<V8EventListener> FindV8EventListener(v8::Local<v8::Value> listener,
                                       bool html);
  PassRefPtr<V8EventListener> FindOrCreateV8EventListener(v8::Local<v8::Value> listener,
                                               bool html);

  PassRefPtr<V8EventListener> FindObjectEventListener(v8::Local<v8::Value> listener,
                                        bool html);
  PassRefPtr<V8EventListener> FindOrCreateObjectEventListener(v8::Local<v8::Value> listener,
                                                bool html);

  void RemoveV8EventListener(V8EventListener* listener);
  void RemoveObjectEventListener(V8ObjectEventListener* listener);

  // Protect/Unprotect JS wrappers of a DOM object.
  static void GCProtect(void* dom_object);
  static void GCUnprotect(void* dom_object);

#if ENABLE(SVG)
  static void SetSVGContext(void* object, SVGElement* context);
  static SVGElement* GetSVGContext(void* object);
#endif

  void setEventHandlerLineno(int lineno) { m_handlerLineno = lineno; }
  void finishedWithEvent(Event* event) { }

  // Evaluate JavaScript in a new context. The script gets its own global scope
  // and its own prototypes for intrinsic JavaScript objects (String, Array,
  // and so-on). It shares the wrappers for all DOM nodes and DOM constructors.
  void evaluateInNewContext(const Vector<ScriptSourceCode>& sources);

  // Evaluate a script file in the current execution environment.
  // The caller must hold an execution context.
  // If cannot evalute the script, it returns an error.
  v8::Local<v8::Value> evaluate(const ScriptSourceCode& source,
                                Node* node);

  // Run an already compiled script.
  v8::Local<v8::Value> RunScript(v8::Handle<v8::Script> script,
                                 bool inline_code);

  // Call the function with the given receiver and arguments.
  v8::Local<v8::Value> CallFunction(v8::Handle<v8::Function> function,
                                    v8::Handle<v8::Object> receiver,
                                    int argc,
                                    v8::Handle<v8::Value> argv[]);

  // Call the function as constructor with the given arguments.
  v8::Local<v8::Value> NewInstance(v8::Handle<v8::Function> constructor,
                                   int argc,
                                   v8::Handle<v8::Value> argv[]);

  // Returns the dom constructor function for the given node type.
  v8::Local<v8::Function> GetConstructor(V8ClassIndex::V8WrapperType type);

  // To create JS Wrapper objects, we create a cache of a 'boiler plate'
  // object, and then simply Clone that object each time we need a new one.
  // This is faster than going through the full object creation process.
  v8::Local<v8::Object> CreateWrapperFromCache(V8ClassIndex::V8WrapperType type);

  // Returns the window object of the currently executing context.
  static DOMWindow* retrieveWindow();
  // Returns the window object associated with a context.
  static DOMWindow* retrieveWindow(v8::Handle<v8::Context> context);
  // Returns V8Proxy object of the currently executing context.
  static V8Proxy* retrieve();
  // Returns V8Proxy object associated with a frame.
  static V8Proxy* retrieve(Frame* frame);
  // Returns V8Proxy object associated with a script execution context.
  static V8Proxy* retrieve(ScriptExecutionContext* context);

  // Returns the frame object of the window object associated
  // with the currently executing context.
  static Frame* retrieveFrame();
  // Returns the frame object of the window object associated with
  // a context.
  static Frame* retrieveFrame(v8::Handle<v8::Context> context);


  // The three functions below retrieve WebFrame instances relating the
  // currently executing JavaScript. Since JavaScript can make function calls
  // across frames, though, we need to be more precise.
  //
  // For example, imagine that a JS function in frame A calls a function in
  // frame B, which calls native code, which wants to know what the 'active'
  // frame is.
  //
  // The 'entered context' is the context where execution first entered the
  // script engine; the context that is at the bottom of the JS function stack.
  // RetrieveFrameForEnteredContext() would return Frame A in our example.
  // This frame is often referred to as the "dynamic global object."
  //
  // The 'current context' is the context the JS engine is currently inside of;
  // the context that is at the top of the JS function stack.
  // RetrieveFrameForCurrentContext() would return Frame B in our example.
  // This frame is often referred to as the "lexical global object."
  //
  // Finally, the 'calling context' is the context one below the current
  // context on the JS function stack.  For example, if function f calls
  // function g, then the calling context will be the context associated with
  // f.  This context is commonly used by DOM security checks because they want
  // to know who called them.
  //
  // If you are unsure which of these functions to use, ask abarth.
  //
  // NOTE: These cannot be declared as inline function, because VS complains at
  // linking time.
  static Frame* retrieveFrameForEnteredContext();
  static Frame* retrieveFrameForCurrentContext();
  static Frame* retrieveFrameForCallingContext();

  // Returns V8 Context of a frame. If none exists, creates
  // a new context.  It is potentially slow and consumes memory.
  static v8::Local<v8::Context> GetContext(Frame* frame);
  static v8::Local<v8::Context> GetCurrentContext();

  // If the current context causes out of memory, JavaScript setting
  // is disabled and it returns true.
  static bool HandleOutOfMemory();

  // Check if the active execution context can access the target frame.
  static bool CanAccessFrame(Frame* target, bool report_error);

  // Check if it is safe to access the given node from the
  // current security context.
  static bool CheckNodeSecurity(Node* node);

  static v8::Handle<v8::Value> CheckNewLegal(const v8::Arguments& args);

  // Create a V8 wrapper for a C pointer
  static v8::Handle<v8::Value> WrapCPointer(void* cptr) {
    // Represent void* as int
    int addr = reinterpret_cast<int>(cptr);
    ASSERT((addr & 0x01) == 0);  // the address must be aligned.
    return v8::Integer::New(addr >> 1);
  }

  // Take C pointer out of a v8 wrapper
  template <class C>
  static C* ExtractCPointer(v8::Handle<v8::Value> obj) {
    return static_cast<C*>(ExtractCPointerImpl(obj));
  }

  static v8::Handle<v8::Script> CompileScript(v8::Handle<v8::String> code,
                                              const String& fileName,
                                              int baseLine);

#ifndef NDEBUG
  // Checks if a v8 value can be a DOM wrapper
  static bool MaybeDOMWrapper(v8::Handle<v8::Value> value);
#endif

  // Sets contents of a DOM wrapper.
  static void SetDOMWrapper(v8::Handle<v8::Object> obj, int type, void* ptr);

  static v8::Handle<v8::Object> LookupDOMWrapper(
    V8ClassIndex::V8WrapperType type, v8::Handle<v8::Value> value);

  // A helper function extract native object pointer from a DOM wrapper
  // and cast to the specified type.
  template <class C>
  static C* DOMWrapperToNative(v8::Handle<v8::Value> object) {
    ASSERT(MaybeDOMWrapper(object));
    v8::Handle<v8::Value> ptr =
      v8::Handle<v8::Object>::Cast(object)->GetInternalField(
          V8Custom::kDOMWrapperObjectIndex);
    return ExtractCPointer<C>(ptr);
  }

  // A help function extract a node type pointer from a DOM wrapper.
  // Wrapped pointer must be cast to Node* first.
  static void* DOMWrapperToNodeHelper(v8::Handle<v8::Value> value);

  template <class C>
  static C* DOMWrapperToNode(v8::Handle<v8::Value> value) {
    return static_cast<C*>(DOMWrapperToNodeHelper(value));
  }

  template<typename T>
  static v8::Handle<v8::Value> ToV8Object(V8ClassIndex::V8WrapperType type, PassRefPtr<T> imp)
  {
    return ToV8Object(type, imp.get());
  }
  static v8::Handle<v8::Value> ToV8Object(V8ClassIndex::V8WrapperType type,
                                          void* imp);
  // Fast-path for Node objects.
  static v8::Handle<v8::Value> NodeToV8Object(Node* node);

  template <class C>
  static C* ToNativeObject(V8ClassIndex::V8WrapperType type,
                           v8::Handle<v8::Value> object) {
    return static_cast<C*>(ToNativeObjectImpl(type, object));
  }

  static V8ClassIndex::V8WrapperType GetDOMWrapperType(
      v8::Handle<v8::Object> object);

  // If the exception code is different from zero, a DOM exception is
  // schedule to be thrown.
  static void SetDOMException(int exception_code);

  // Schedule an error object to be thrown.
  static v8::Handle<v8::Value> ThrowError(ErrorType type, const char* message);

  // Create an instance of a function descriptor and set to the global object
  // as a named property. Used by v8_test_shell.
  static void BindJSObjectToWindow(Frame* frame,
                                   const char* name,
                                   int type,
                                   v8::Handle<v8::FunctionTemplate> desc,
                                   void* imp);

  static v8::Handle<v8::Value> EventToV8Object(Event* event);
  static Event* ToNativeEvent(v8::Handle<v8::Value> jsevent) {
    if (!IsDOMEventWrapper(jsevent)) return 0;
    return DOMWrapperToNative<Event>(jsevent);
  }

  static v8::Handle<v8::Value> EventTargetToV8Object(EventTarget* target);
  // Wrap and unwrap JS event listeners
  static v8::Handle<v8::Value> EventListenerToV8Object(EventListener* target);

  // DOMImplementation is a singleton and it is handled in a special
  // way.  A wrapper is generated per document and stored in an
  // internal field of the document.
  static v8::Handle<v8::Value> DOMImplementationToV8Object(
      DOMImplementation* impl);

  // Wrap JS node filter in C++
  static PassRefPtr<NodeFilter> ToNativeNodeFilter(v8::Handle<v8::Value> filter);

  static v8::Persistent<v8::FunctionTemplate> GetTemplate(
      V8ClassIndex::V8WrapperType type);

  template <int tag, typename T>
    static v8::Handle<v8::Value> ConstructDOMObject(const v8::Arguments& args);

  // Checks whether a DOM object has a JS wrapper.
  static bool DOMObjectHasJSWrapper(void* obj);
  // Set JS wrapper of a DOM object, the caller in charge of increase ref.
  static void SetJSWrapperForDOMObject(void* obj,
                                       v8::Persistent<v8::Object> wrapper);
  static void SetJSWrapperForActiveDOMObject(void* obj,
                                             v8::Persistent<v8::Object> wrapper);
  static void SetJSWrapperForDOMNode(Node* node,
                                     v8::Persistent<v8::Object> wrapper);

  // Process any pending JavaScript console messages.
  static void ProcessConsoleMessages();

#ifndef NDEBUG
  // For debugging and leak detection purpose
  static void RegisterGlobalHandle(GlobalHandleType type,
                                   void* host,
                                   v8::Persistent<v8::Value> handle);
  static void UnregisterGlobalHandle(void* host,
                                     v8::Persistent<v8::Value> handle);
#endif

  // Check whether a V8 value is a wrapper of type |classType|.
  static bool IsWrapperOfType(v8::Handle<v8::Value> obj,
                              V8ClassIndex::V8WrapperType classType);

  // Function for retrieving the line number and source name for the top
  // JavaScript stack frame.
  static int GetSourceLineNumber();
  static String GetSourceName();


  // Returns a local handle of the context.
  v8::Local<v8::Context> GetContext() {
    return v8::Local<v8::Context>::New(m_context);
  }

  // Registers an extension to be available on webpages with a particular scheme
  // If the scheme argument is empty, the extension is available on all pages.
  // Will only affect v8 contexts initialized after this call. Takes ownership
  // of the v8::Extension object passed.
  static void RegisterExtension(v8::Extension* extension,
                                const String& schemeRestriction);

  static void* ToSVGPODTypeImpl(V8ClassIndex::V8WrapperType type,
                                v8::Handle<v8::Value> object);

 private:
  v8::Persistent<v8::Context> createNewContext(v8::Handle<v8::Object> global);
  void InitContextIfNeeded();
  void DisconnectEventListeners();
  void SetSecurityToken();
  void ClearDocumentWrapper();
  void UpdateDocumentWrapper(v8::Handle<v8::Value> wrapper);

  // The JavaScript wrapper for the document object is cached on the global
  // object for fast access.  UpdateDocumentWrapperCache sets the wrapper
  // for the current document on the global object.  ClearDocumentWrapperCache
  // deletes the document wrapper from the global object.
  void UpdateDocumentWrapperCache();
  void ClearDocumentWrapperCache();

  // Dispose global handles of m_contexts and friends.
  void DisposeContextHandles();

  static bool CanAccessPrivate(DOMWindow* target);

  // Check whether a V8 value is a DOM Event wrapper
  static bool IsDOMEventWrapper(v8::Handle<v8::Value> obj);

  static void* ToNativeObjectImpl(V8ClassIndex::V8WrapperType type,
                                  v8::Handle<v8::Value> object);

  // Take C pointer out of a v8 wrapper
  static void* ExtractCPointerImpl(v8::Handle<v8::Value> obj) {
    ASSERT(obj->IsNumber());
    int addr = obj->Int32Value();
    return reinterpret_cast<void*>(addr << 1);
  }


  static v8::Handle<v8::Value> StyleSheetToV8Object(StyleSheet* sheet);
  static v8::Handle<v8::Value> CSSValueToV8Object(CSSValue* value);
  static v8::Handle<v8::Value> CSSRuleToV8Object(CSSRule* rule);
  // Returns the JS wrapper of a window object, initializes the environment
  // of the window frame if needed.
  static v8::Handle<v8::Value> WindowToV8Object(DOMWindow* window);

#if ENABLE(SVG)
  static v8::Handle<v8::Value> SVGElementInstanceToV8Object(
      SVGElementInstance* instance);
  static v8::Handle<v8::Value> SVGObjectWithContextToV8Object(
      V8ClassIndex::V8WrapperType type, void* object);
#endif

  // Set hidden references in a DOMWindow object of a frame.
  static void SetHiddenWindowReference(Frame* frame,
                                       const int internal_index,
                                       v8::Handle<v8::Object> jsobj);

  static V8ClassIndex::V8WrapperType GetHTMLElementType(HTMLElement* elm);

  // The first parameter, desc_type, specifies the function descriptor
  // used to create JS object. The second parameter, cptr_type, specifies
  // the type of third parameter, impl, for type casting.
  // For example, a HTML element has HTMLELEMENT desc_type, but always
  // use NODE as cptr_type. JS wrapper stores cptr_type and impl as
  // internal fields.
  static v8::Local<v8::Object> InstantiateV8Object(
      V8ClassIndex::V8WrapperType desc_type,
      V8ClassIndex::V8WrapperType cptr_type,
      void* impl);

  static const char* GetRangeExceptionName(int exception_code);
  static const char* GetEventExceptionName(int exception_code);
  static const char* GetXMLHttpRequestExceptionName(int exception_code);
  static const char* GetDOMExceptionName(int exception_code);

#if ENABLE(XPATH)
  static const char* GetXPathExceptionName(int exception_code);
#endif

#if ENABLE(SVG)
  static V8ClassIndex::V8WrapperType GetSVGElementType(SVGElement* elm);
  static const char* GetSVGExceptionName(int exception_code);
#endif

  // Create and populate the utility context.
  static void CreateUtilityContext();

  // Returns a local handle of the utility context.
  static v8::Local<v8::Context> GetUtilityContext() {
    if (m_utilityContext.IsEmpty()) {
      CreateUtilityContext();
    }
    return v8::Local<v8::Context>::New(m_utilityContext);
  }

  Frame* m_frame;

  v8::Persistent<v8::Context> m_context;
  // For each possible type of wrapper, we keep a boilerplate object.
  // The boilerplate is used to create additional wrappers of the same type.
  // We keep a single persistent handle to an array of the activated
  // boilerplates.
  v8::Persistent<v8::Array> m_wrapper_boilerplates;
  v8::Persistent<v8::Value> m_object_prototype;

  v8::Persistent<v8::Object> m_global;
  v8::Persistent<v8::Value> m_document;

  // Utility context holding JavaScript functions used internally.
  static v8::Persistent<v8::Context> m_utilityContext;

  int m_handlerLineno;

  // A list of event listeners created for this frame,
  // the list gets cleared when removing all timeouts.
  V8EventListenerList m_event_listeners;

  // A list of event listeners create for XMLHttpRequest object for this frame,
  // the list gets cleared when removing all timeouts.
  V8EventListenerList m_xhr_listeners;

  // True for <a href="javascript:foo()"> and false for <script>foo()</script>.
  // Only valid during execution.
  bool m_inlineCode;

  // True when executing from within a timer callback.  Only valid during
  // execution.
  bool m_timerCallback;

  // Track the recursion depth to be able to avoid too deep recursion. The V8
  // engine allows much more recursion than KJS does so we need to guard against
  // excessive recursion in the binding layer.
  int m_recursion;

  // List of extensions registered with the context.
  static V8ExtensionList m_extensions;
};

template <int tag, typename T>
v8::Handle<v8::Value> V8Proxy::ConstructDOMObject(const v8::Arguments& args) {
  if (!args.IsConstructCall()) {
    V8Proxy::ThrowError(V8Proxy::TYPE_ERROR,
        "DOM object constructor cannot be called as a function.");
    return v8::Undefined();
  }


  // Note: it's OK to let this RefPtr go out of scope because we also call
  // SetDOMWrapper(), which effectively holds a reference to obj.
  RefPtr<T> obj = T::create();
  V8Proxy::SetDOMWrapper(args.Holder(), tag, obj.get());
  obj->ref();
  V8Proxy::SetJSWrapperForDOMObject(
      obj.get(), v8::Persistent<v8::Object>::New(args.Holder()));
  return args.Holder();
}

}  // namespace WebCore

#endif  // V8_PROXY_H__