**Dependencies**
https://github.com/FireFlyForLife/Concordia/blob/master/src/Concordia/EventMgr.hpp

**Example:**
```cpp
gSimpleInput.init(1920.f, 1080.f,8,false);
gSimpleInput.listen(&eventmgr);
  
if (gSimpleInput.isKeyDown(KEY_F))
  std::cout << (1 / deltatime) << std::endl;


gSimpleInput.deinit();
```

event example:
```cpp
Concordia::EventMgr eventmgr;
evm->broadcast <si::BtnClicked>(ev.code, ev.time);

eventmgr.dispatch_event_queue();
```
