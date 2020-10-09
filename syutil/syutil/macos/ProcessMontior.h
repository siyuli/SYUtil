
#pragma once

#ifdef __cplusplus
#include <memory>
#include <map>
#include <functional>
#endif



@interface ProcessMontior : NSObject

#ifdef __cplusplus

-(void) removeMontior:(NSTask*)task;

-(void) montiorTask:(NSTask*)task callback:(std::function<void()>) processDiedCallBack;

#endif

@property  NSMutableDictionary * callBackMap;
@end

