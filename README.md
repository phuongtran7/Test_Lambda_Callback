This is a test for creating flightloop callback with lambda. Each topic will have seperate callback. 

Each of the lambda callback `void* inRefcon` that can be cast back into the pointer to the `Topic`, which then can invoke the `Update()`.

Also a draft of rewriting Topic in Ditto. Will need further test before integrate back into Ditto.