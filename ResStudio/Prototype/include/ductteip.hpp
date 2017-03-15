#ifndef __DUCTTEIP_HPP__
#define  __DUCTTEIP_HPP__

#include "context.hpp"
#include "basic.hpp"
#include "engine.hpp"
#define DuctTeip_Start(a,b) dtEngine.start(a,b)
#define DuctTeip_Finish() dtEngine.finalize()

extern Config config;
#endif // __DUCTTEIP_HPP__

