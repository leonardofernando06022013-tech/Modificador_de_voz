#pragma once
#include "UserAccount.h"
namespace vox {class PermissionManager{public:bool allowed(Role,Permission)const;juce::Array<Permission>permissions(Role)const;static juce::String name(Permission);};}
