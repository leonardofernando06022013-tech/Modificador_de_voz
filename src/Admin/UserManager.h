#pragma once
#include "UserAccount.h"
namespace vox {class UserManager{public:UserManager();juce::Array<UserAccount>users()const;bool save(const juce::Array<UserAccount>&)const;UserAccount currentUser();bool upsert(const UserAccount&);bool remove(const juce::String&);juce::File file()const{return usersFile;}private:juce::File usersFile;};}
