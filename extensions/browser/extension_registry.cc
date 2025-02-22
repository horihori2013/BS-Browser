// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_registry.h"

#include "base/strings/string_util.h"
#include "extensions/browser/extension_registry_factory.h"
#include "extensions/browser/extension_registry_observer.h"

namespace extensions {

ExtensionRegistry::ExtensionRegistry(content::BrowserContext* browser_context)
    : browser_context_(browser_context) {}
ExtensionRegistry::~ExtensionRegistry() {}

// static
ExtensionRegistry* ExtensionRegistry::Get(content::BrowserContext* context) {
  return ExtensionRegistryFactory::GetForBrowserContext(context);
}

std::unique_ptr<ExtensionSet>
ExtensionRegistry::GenerateInstalledExtensionsSet() const {
  return GenerateInstalledExtensionsSet(EVERYTHING);
}

std::unique_ptr<ExtensionSet> ExtensionRegistry::GenerateInstalledExtensionsSet(
    int include_mask) const {
  std::unique_ptr<ExtensionSet> installed_extensions(new ExtensionSet);
  if (include_mask & IncludeFlag::ENABLED)
    installed_extensions->InsertAll(enabled_extensions_);
  if (include_mask & IncludeFlag::DISABLED)
    installed_extensions->InsertAll(disabled_extensions_);
  if (include_mask & IncludeFlag::TERMINATED)
    installed_extensions->InsertAll(terminated_extensions_);
  if (include_mask & IncludeFlag::BLACKLISTED)
    installed_extensions->InsertAll(blacklisted_extensions_);
  if (include_mask & IncludeFlag::BLOCKED)
    installed_extensions->InsertAll(blocked_extensions_);
  return installed_extensions;
}

base::Version ExtensionRegistry::GetStoredVersion(const ExtensionId& id) const {
  int include_mask = ExtensionRegistry::ENABLED | ExtensionRegistry::DISABLED |
                     ExtensionRegistry::TERMINATED |
                     ExtensionRegistry::BLACKLISTED |
                     ExtensionRegistry::BLOCKED;
  const Extension* registry_extension = GetExtensionById(id, include_mask);
  return registry_extension ? registry_extension->version() : base::Version();
}

void ExtensionRegistry::AddObserver(ExtensionRegistryObserver* observer) {
  observers_.AddObserver(observer);
}

void ExtensionRegistry::RemoveObserver(ExtensionRegistryObserver* observer) {
  observers_.RemoveObserver(observer);
}

void ExtensionRegistry::TriggerOnLoaded(const Extension* extension) {
  CHECK(extension);
  DCHECK(enabled_extensions_.Contains(extension->id()));
  for (auto& observer : observers_)
    observer.OnExtensionLoaded(browser_context_, extension);
}

void ExtensionRegistry::TriggerOnReady(const Extension* extension) {
  CHECK(extension);
  DCHECK(enabled_extensions_.Contains(extension->id()));
  for (auto& observer : observers_)
    observer.OnExtensionReady(browser_context_, extension);
}

void ExtensionRegistry::TriggerOnUnloaded(const Extension* extension,
                                          UnloadedExtensionReason reason) {
  CHECK(extension);
  DCHECK(!enabled_extensions_.Contains(extension->id()));
  for (auto& observer : observers_)
    observer.OnExtensionUnloaded(browser_context_, extension, reason);
}

void ExtensionRegistry::TriggerOnWillBeInstalled(const Extension* extension,
                                                 bool is_update,
                                                 const std::string& old_name) {
  CHECK(extension);
  DCHECK_EQ(is_update,
            GenerateInstalledExtensionsSet()->Contains(extension->id()));
  DCHECK_EQ(is_update, !old_name.empty());
  for (auto& observer : observers_)
    observer.OnExtensionWillBeInstalled(browser_context_, extension, is_update,
                                        old_name);
}

void ExtensionRegistry::TriggerOnInstalled(const Extension* extension,
                                           bool is_update) {
  LOG(INFO) << "[EXTENSIONS] TriggerOnInstalled - Step 1";
  CHECK(extension);
  LOG(INFO) << "[EXTENSIONS] TriggerOnInstalled - Step 2";
  DCHECK(GenerateInstalledExtensionsSet()->Contains(extension->id()));
  LOG(INFO) << "[EXTENSIONS] TriggerOnInstalled - Step 3";
  for (auto& observer : observers_) {
    LOG(INFO) << "[EXTENSIONS] TriggerOnInstalled - Step 4";
    observer.OnExtensionInstalled(browser_context_, extension, is_update);
    LOG(INFO) << "[EXTENSIONS] TriggerOnInstalled - Step 5";
  }
  LOG(INFO) << "[EXTENSIONS] TriggerOnInstalled - Step 6";
}

void ExtensionRegistry::TriggerOnUninstalled(const Extension* extension,
                                             UninstallReason reason) {
  LOG(INFO) << "[EXTENSIONS] TriggerOnUninstalled - Step 1";
  CHECK(extension);
  LOG(INFO) << "[EXTENSIONS] TriggerOnUninstalled - Step 2";
  DCHECK(!GenerateInstalledExtensionsSet()->Contains(extension->id()));
  LOG(INFO) << "[EXTENSIONS] TriggerOnUninstalled - Step 3";
  for (auto& observer : observers_) {
    LOG(INFO) << "[EXTENSIONS] TriggerOnUninstalled - Step 4";
    observer.OnExtensionUninstalled(browser_context_, extension, reason);
    LOG(INFO) << "[EXTENSIONS] TriggerOnUninstalled - Step 5";
  }
  LOG(INFO) << "[EXTENSIONS] TriggerOnUninstalled - Step 6";
}

const Extension* ExtensionRegistry::GetExtensionById(const std::string& id,
                                                     int include_mask) const {
  std::string lowercase_id = base::ToLowerASCII(id);
  if (include_mask & ENABLED) {
    const Extension* extension = enabled_extensions_.GetByID(lowercase_id);
    if (extension)
      return extension;
  }
  if (include_mask & DISABLED) {
    const Extension* extension = disabled_extensions_.GetByID(lowercase_id);
    if (extension)
      return extension;
  }
  if (include_mask & TERMINATED) {
    const Extension* extension = terminated_extensions_.GetByID(lowercase_id);
    if (extension)
      return extension;
  }
  if (include_mask & BLACKLISTED) {
    const Extension* extension = blacklisted_extensions_.GetByID(lowercase_id);
    if (extension)
      return extension;
  }
  if (include_mask & BLOCKED) {
    const Extension* extension = blocked_extensions_.GetByID(lowercase_id);
    if (extension)
      return extension;
  }
  return NULL;
}

const Extension* ExtensionRegistry::GetInstalledExtension(
    const std::string& id) const {
  return GetExtensionById(id, ExtensionRegistry::EVERYTHING);
}

bool ExtensionRegistry::AddEnabled(
    const scoped_refptr<const Extension>& extension) {
  return enabled_extensions_.Insert(extension);
}

bool ExtensionRegistry::RemoveEnabled(const std::string& id) {
  // Only enabled extensions can be ready, so removing an enabled extension
  // should also remove from the ready set if possible.
  if (ready_extensions_.Contains(id))
    RemoveReady(id);
  return enabled_extensions_.Remove(id);
}

bool ExtensionRegistry::AddDisabled(
    const scoped_refptr<const Extension>& extension) {
  return disabled_extensions_.Insert(extension);
}

bool ExtensionRegistry::RemoveDisabled(const std::string& id) {
  return disabled_extensions_.Remove(id);
}

bool ExtensionRegistry::AddTerminated(
    const scoped_refptr<const Extension>& extension) {
  return terminated_extensions_.Insert(extension);
}

bool ExtensionRegistry::RemoveTerminated(const std::string& id) {
  return terminated_extensions_.Remove(id);
}

bool ExtensionRegistry::AddBlacklisted(
    const scoped_refptr<const Extension>& extension) {
  return blacklisted_extensions_.Insert(extension);
}

bool ExtensionRegistry::RemoveBlacklisted(const std::string& id) {
  return blacklisted_extensions_.Remove(id);
}

bool ExtensionRegistry::AddBlocked(
    const scoped_refptr<const Extension>& extension) {
  return blocked_extensions_.Insert(extension);
}

bool ExtensionRegistry::RemoveBlocked(const std::string& id) {
  return blocked_extensions_.Remove(id);
}

bool ExtensionRegistry::AddReady(
    const scoped_refptr<const Extension>& extension) {
  return ready_extensions_.Insert(extension);
}

bool ExtensionRegistry::RemoveReady(const std::string& id) {
  return ready_extensions_.Remove(id);
}

void ExtensionRegistry::ClearAll() {
  enabled_extensions_.Clear();
  disabled_extensions_.Clear();
  terminated_extensions_.Clear();
  blacklisted_extensions_.Clear();
  blocked_extensions_.Clear();
  ready_extensions_.Clear();
}

void ExtensionRegistry::Shutdown() {
  // Release references to all Extension objects in the sets.
  ClearAll();
  for (auto& observer : observers_)
    observer.OnShutdown(this);
}

}  // namespace extensions
