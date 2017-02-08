#ifndef __DUCTTEIP_HPP__
#define  __DUCTTEIP_HPP__

#include "context.hpp"
#include "basic.hpp"
#include "engine.hpp"
//#include "ml_manager.hpp"
#define DuctTeip_Start(a,b,c) dtEngine.start(a,b,c)
#define DuctTeip_Finish() dtEngine.finalize()

extern Config config;
#endif // __DUCTTEIP_HPP__

