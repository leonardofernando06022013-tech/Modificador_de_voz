#pragma once
#include "PermissionManager.h"
namespace vox {class AdminAccessController{public:AdminAccessController();bool begin(const UserAccount&,int expiryMinutes=30);bool active()const;bool can(Permission)const;void lock();const UserAccount&user()const{return sessionUser;}private:PermissionManager permissions;UserAccount sessionUser;juce::Time expires;bool authenticated=false;};}
