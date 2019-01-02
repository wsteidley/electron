// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_global_shortcut.h"

#include <string>
#include <vector>

#include "atom/browser/api/atom_api_system_preferences.h"
#include "atom/common/native_mate_converters/accelerator_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

using extensions::GlobalShortcutListener;

namespace atom {

namespace api {

GlobalShortcut::GlobalShortcut(v8::Isolate* isolate) {
  Init(isolate);
}

GlobalShortcut::~GlobalShortcut() {
  UnregisterAll();
}

void GlobalShortcut::OnKeyPressed(const ui::Accelerator& accelerator) {
  if (accelerator_callback_map_.find(accelerator) ==
      accelerator_callback_map_.end()) {
    // This should never occur, because if it does, GlobalGlobalShortcutListener
    // notifes us with wrong accelerator.
    NOTREACHED();
    return;
  }
  accelerator_callback_map_[accelerator].Run();
}

bool GlobalShortcut::RegisterAll(
    const std::vector<ui::Accelerator>& accelerators,
    const base::Closure& callback) {
  std::vector<ui::Accelerator> registered;
  std::vector<std::string> mediaKeys = {"Media Play/Pause", "Media Next Track",
                                        "Media Previous Track"};

  for (auto& accelerator : accelerators) {
#if defined(OS_MACOSX)
    std::string shortcutText = base::UTF16ToUTF8(accelerator.GetShortcutText());
    if (std::find(mediaKeys.begin(), mediaKeys.end(), shortcutText) !=
        mediaKeys.end()) {
      SystemPreferences* sys = nullptr;
      bool trusted = sys->IsTrustedAccessibilityClient(false);
      if (!trusted)
        return false;
    }
    GlobalShortcutListener* listener = GlobalShortcutListener::GetInstance();
    if (!listener->RegisterAccelerator(accelerator, this)) {
      // unregister all shortcuts if any failed
      UnregisterSome(registered);
      return false;
    }
#endif
    registered.push_back(accelerator);
    accelerator_callback_map_[accelerator] = callback;
  }
  return true;
}

bool GlobalShortcut::Register(const ui::Accelerator& accelerator,
                              const base::Closure& callback) {
#if defined(OS_MACOSX)
  std::vector<std::string> mediaKeys = {"Media Play/Pause", "Media Next Track",
                                        "Media Previous Track"};
  std::string shortcutText = base::UTF16ToUTF8(accelerator.GetShortcutText());

  if (std::find(mediaKeys.begin(), mediaKeys.end(), shortcutText) !=
      mediaKeys.end()) {
    SystemPreferences* sys = nullptr;
    bool trusted = sys->IsTrustedAccessibilityClient(false);
    if (!trusted)
      return false;
  }
#endif

  if (!GlobalShortcutListener::GetInstance()->RegisterAccelerator(accelerator,
                                                                  this)) {
    return false;
  }

  accelerator_callback_map_[accelerator] = callback;
  return true;
}

void GlobalShortcut::Unregister(const ui::Accelerator& accelerator) {
  if (!ContainsKey(accelerator_callback_map_, accelerator))
    return;

  accelerator_callback_map_.erase(accelerator);
  GlobalShortcutListener::GetInstance()->UnregisterAccelerator(accelerator,
                                                               this);
}

void GlobalShortcut::UnregisterSome(
    const std::vector<ui::Accelerator>& accelerators) {
  for (auto& accelerator : accelerators) {
    Unregister(accelerator);
  }
}

bool GlobalShortcut::IsRegistered(const ui::Accelerator& accelerator) {
  return ContainsKey(accelerator_callback_map_, accelerator);
}

void GlobalShortcut::UnregisterAll() {
  accelerator_callback_map_.clear();
  GlobalShortcutListener::GetInstance()->UnregisterAccelerators(this);
}

// static
mate::Handle<GlobalShortcut> GlobalShortcut::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new GlobalShortcut(isolate));
}

// static
void GlobalShortcut::BuildPrototype(v8::Isolate* isolate,
                                    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "GlobalShortcut"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("registerAll", &GlobalShortcut::RegisterAll)
      .SetMethod("register", &GlobalShortcut::Register)
      .SetMethod("isRegistered", &GlobalShortcut::IsRegistered)
      .SetMethod("unregister", &GlobalShortcut::Unregister)
      .SetMethod("unregisterAll", &GlobalShortcut::UnregisterAll);
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("globalShortcut", atom::api::GlobalShortcut::Create(isolate));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_global_shortcut, Initialize)
