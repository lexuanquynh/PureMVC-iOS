//
//  LoginCommand.h
//  PureMVC
//
//  Created by Prank on 17/6/25.
//

#ifndef LOGIN_COMMAND_H
#define LOGIN_COMMAND_H

#include "PureMVC/Patterns/Command/SimpleCommand.hpp"
#include "PureMVC/Interfaces/INotification.hpp"
#include <string>

using namespace PureMVC;
using namespace PureMVC::Patterns;
using namespace PureMVC::Interfaces;

class LoginCommand : public SimpleCommand {
public:
    // Check the exact signature from ICommand interface
    // It might be const& instead of just &
    virtual void execute(INotification const& notification) override;
};

#endif // LOGIN_COMMAND_H
