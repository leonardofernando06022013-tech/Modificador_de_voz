#pragma once
#include "UserAccount.h"
namespace vox {class BackupManager{public:juce::File directory()const;juce::File create(const juce::String&description={})const;juce::Array<juce::File>list()const;bool remove(const juce::File&)const;};}
