#pragma once

#include <map>
#include <memory>
#include "RaopContext.h"
#include <winsock.h>


typedef std::map < SOCKET, std::shared_ptr<CRaopContext> > SocketConnectionMap;
